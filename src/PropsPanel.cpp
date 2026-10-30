#include "PropsPanel.h"
#include <QLabel>
#include <QVBoxLayout>
#include <QFileInfo>
#include <QTreeWidget>
#include <QComboBox>
#include <QSpinBox>
#include <QMessageBox>
#include <QFileDialog>
#include <QCheckBox>

#include <QProcess>
#include <QJsonDocument>
#include <QJsonValue>
#include <QJsonObject>
#include <QJsonArray>

#include "common.h"
#include "JobQueue.h"
#include "VideoPlayer.h"


MkPropsPanel::MkPropsPanel(QWidget* parent) : QWidget(parent) {
  auto layout = new QVBoxLayout(this);
  m_title = new QLabel("no media selected");
  m_title->setWordWrap(true);
  layout->addWidget(m_title);

  m_preset = new QComboBox;
  for (const auto& preset : MK_BUILTIN_PRESETS) m_preset->addItem(preset.name);
  layout->addWidget(m_preset);

  connect(m_preset, &QComboBox::currentIndexChanged, this, [this](int idx) {
    if (m_outputItem == nullptr || idx >= MK_BUILTIN_PRESETS.size()) return;
    m_output = MK_BUILTIN_PRESETS[idx].settings;
    m_outputItem->takeChildren();
    populateOutputWidget(m_outputItem);
  });


  m_tree = new QTreeWidget(this);
  m_tree->setColumnCount(2);
  m_tree->setHeaderLabels({"property", "value"});    
  m_tree->setAlternatingRowColors(true);
  layout->addWidget(m_tree, 1);
}

void MkPropsPanel::loadFile(const QString& path) {
  m_currentPath = QFileInfo(path).absoluteFilePath();
  m_tree->clear();

  QFileInfo info(m_currentPath);
  m_title->setText(
    QString("%1 (%2)").arg(info.fileName()).arg(MkCommon::fmtSize(info.size()))
  );

  auto file = new QTreeWidgetItem(m_tree, {"file", ""});
  new QTreeWidgetItem(file, {"name", info.fileName()});
  new QTreeWidgetItem(file, {"path", info.absoluteFilePath()});
  new QTreeWidgetItem(file, {"size", MkCommon::fmtSize(info.size())});
  file->setExpanded(true);

  auto stat = new QTreeWidgetItem(m_tree, {"stat", ""});
  new QTreeWidgetItem(stat, {"wait", "probing..."});
  stat->setExpanded(true);


  // its probing time
  auto process = new QProcess(this);
  connect(process, &QProcess::finished, this, [this, process, stat](int code, QProcess::ExitStatus status) {
    const auto sout = process->readAllStandardOutput();
    const auto serr = process->readAllStandardError();
    if (status != QProcess::NormalExit || code != 0) {
      stat->child(0)->setText(1, "ffprobe failed!");
      return;
    }

    QJsonParseError jsonErr;
    const auto jsonDoc = QJsonDocument::fromJson(sout, &jsonErr);
    if (jsonErr.error != QJsonParseError::NoError || !jsonDoc.isObject()) {
      stat->child(0)->setText(1, "ffprobe returned invalid json!");
      return;
    }

    delete stat;
    populateData(jsonDoc.object());
  });

  connect(process, &QProcess::errorOccurred, this, [this, process, stat](QProcess::ProcessError err) {
    if (err != QProcess::FailedToStart) return;
    stat->child(0)->setText(1, "could not run ffprobe: " + process->errorString());
  });

  const auto ffprobe = MkCommon::findTool("ffprobe");
  process->start(ffprobe, {
    "-v", "error",
    "-show_format", "-show_streams",
    "-of", "json",
    m_currentPath
  });
}


void MkPropsPanel::exportTranscode(MkJobQueue* queue, const MkVideoTrimRange trimRange) {
  if (m_currentPath.isEmpty()) {
    QMessageBox::warning(this, "export", "no file selected");
    return;
  }

  // TODO: maybe figure something else out for the path.
  QFileInfo info(m_currentPath);
  //const QString defaultName = info.completeBaseName() + "_encode." + info.completeSuffix();
  const QString defaultName = info.completeBaseName() + "_encode." + m_output.container;
  const QString outFile = QFileDialog::getSaveFileName(
    this,
    "export transcoded file", info.absolutePath() + "/" + defaultName,
    "All files (*)"
  );
  if (outFile.isEmpty()) return;


  const auto duration_sec = trimRange.end - trimRange.start;
  MkJobFFmpeg::Spec spec;
  spec.name = QFileInfo(outFile).fileName();
  spec.infile = m_currentPath;
  spec.outfile = outFile;
  spec.exec = MkCommon::findTool("ffmpeg");
  spec.assumedLengthMs = duration_sec * 1000.0;
  spec.args = {
    "-hide_banner", "-y",
    "-progress", "pipe:1",
    "-i", m_currentPath,
  };


  // trim range
  if (trimRange.start > 0.0) {
    spec.args.append({"-ss", QString::number(trimRange.start, 'f', 3)});
  }
  spec.args.append({"-t", QString::number(duration_sec, 'f', 3)});


  // encoder output options
  if (!m_output.video.enabled) {
    spec.args.push_back("-vn");
  } else {
    const auto& v = m_output.video;
    spec.args.append({"-c:v", v.codec});
    if (v.codec != "copy") {
      if (v.useCrfRate) {
        spec.args.append({"-crf", QString::number(v.crf)});
      } else {
        spec.args.append({"-b:v", QString("%1k").arg(v.bitrate)});
      }

      // TODO: -r is kind of problematic with audio streams. -vf fps= is generally a better choice.
      //      so try using that later?
      if (v.fps > 0.0) {
        spec.args.append({"-r", QString::number(v.fps, 'g', 8)});
      }
    }
  }

  if (!m_output.audio.enabled) {
    spec.args.push_back("-an");
  } else {
    const auto& a = m_output.audio;
    spec.args.append({"-c:a", a.codec});
    if (a.codec != "copy") {
      if (!a.codec.startsWith("pcm_")) {
        spec.args.append({"-b:a", QString("%1k").arg(a.bitrate)});
      }
      if (a.sampleRate > 0) {
        spec.args.append({"-ar", QString::number(a.sampleRate)});
      }
    }
  }

  spec.args.push_back(outFile);
  queue->enqueue(spec);
}




void MkPropsPanel::populateData(const QJsonObject& root) {
  auto valueof = [](const QJsonObject& a, const QString& k) -> QString {
    const QJsonValue val = a.value(k);
    if (val.isString()) return val.toString();
    if (val.isDouble()) return QString::number(val.toDouble(), 'g', 10);
    return {};
  };


  const QJsonObject format = root.value("format").toObject();
  
  // TODO: add more fields from the debug below
  qDebug() << "ffprobe result" << root;

  if (!format.isEmpty()) {
    auto sect = new QTreeWidgetItem(m_tree, {"format", ""});
    sect->setExpanded(true);

    auto currentDuration = valueof(format, "duration").toDouble() * 1000.0;
    new QTreeWidgetItem(sect, {"duration", MkCommon::fmtTime(currentDuration)});
    new QTreeWidgetItem(sect, {"container", valueof(format, "format_long_name")});
    new QTreeWidgetItem(sect, {"bitrate", QString("%1 kbps").arg(valueof(format, "bit_rate").toLongLong() / 1000)});
  }

  const QJsonArray streams = root.value("streams").toArray();
  if (!streams.isEmpty()) {
    auto sect = new QTreeWidgetItem(m_tree, {"streams", ""});
    sect->setExpanded(true);

    for (const auto& streamV : streams) {
      const auto stream = streamV.toObject();
      const int idx = stream.value("index").toInt(-1);
      const auto s_type = valueof(stream, "codec_type");
      const auto s_codec = valueof(stream, "codec_name");

      auto lbl = QString("#%1 %2 (%3)").arg(idx).arg(s_type).arg(s_codec);
      auto s_item = new QTreeWidgetItem(sect, {lbl, ""});

      const auto tryField = [&](const QString& key, const QString& prettyName) {
        const QString val = valueof(stream, key);
        if (!val.isEmpty()) new QTreeWidgetItem(s_item, {prettyName, val});
      };


      
      if (s_type == "video") {
        tryField("width", "width");
        tryField("height", "height");
        tryField("display_aspect_ratio", "aspect ratio");
        tryField("r_frame_rate", "framerate");
        tryField("avg_frame_rate", "averaged framerate");
        tryField("pix_fmt", "pixel format");
        tryField("color_space", "color space");
      }
      
      if (s_type == "audio") {
        tryField("channel_layout", "channel type");
        tryField("channels", "channels");
        tryField("sample_rate", "sample rate");
        tryField("sample_fmt", "sample format");
      }
      
      tryField("codec_long_name", "codec long name");
      tryField("duration", "duration");
      tryField("profile", "profile");
      tryField("bit_rate", "bitrate");
      tryField("nb_frames", "number of frames");
    }
  }


  m_outputItem = new QTreeWidgetItem(m_tree, {"output", ""});
  m_outputItem->setExpanded(true);
  populateOutputWidget(m_outputItem);
}


void MkPropsPanel::populateOutputWidget(QTreeWidgetItem* output) {
  auto addControl = [this](QTreeWidgetItem* parent, const QString& label, QWidget* control) {
    auto item = new QTreeWidgetItem(parent, {label, ""});
    m_tree->setItemWidget(item, 1, control);
    return item;
  };

  auto addCombo = [&]<typename T>(QTreeWidgetItem* parent, const QString& label,
                      const QList<QPair<QString, QVariant>>& opts, T& setting) {
    auto combo = new QComboBox(m_tree);
    for (const auto& [key, prop] : opts) combo->addItem(key, prop);
    combo->setCurrentIndex(combo->findData(QVariant::fromValue(setting)));

    auto* tgt = &setting;
    connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [tgt, combo](int idx) {
      *tgt = combo->itemData(idx).value<T>();
    });
    
    addControl(parent, label, combo);
    return combo;
  };


  auto addCheck = [&](QTreeWidgetItem* parent, const QString& label, bool& setting) {
    auto check = new QCheckBox(m_tree);
    check->setChecked(setting);
    auto* tgt = &setting;
    connect(check, &QCheckBox::toggled, this, [tgt](bool on) { *tgt = on; });
    
    addControl(parent, label, check);
    return check;
  };

  auto addSpin = [&](QTreeWidgetItem* parent, const QString& label, int min, int max, int& setting) {
    auto spin = new QSpinBox(m_tree);
    spin->setRange(min, max);
    spin->setValue(setting);
    auto* tgt = &setting;
    connect(spin, &QSpinBox::valueChanged, this, [tgt](int val) { *tgt = val; });
    
    addControl(parent, label, spin);
    return spin;
  };


  
  addCombo(output, "container", {
    {"MP4", "mp4"},
    {"MKV", "mkv"},
    {"WebM", "webm"},
    {"Ogg", "ogg"},
    {"Opus/Ogg", "opus"},
    {"MP3", "mp3"},
    {"FLAC", "flac"},
    {"WAV", "wav"},
  }, m_output.container);

  auto video = new QTreeWidgetItem(output, {"video", ""});
  video->setExpanded(true);
  addCheck(video, "enabled", m_output.video.enabled);

  addCombo(video, "codec", {
    {"copy source", "copy"},
    {"H.264", "libx264"},
    {"H.265", "libx265"},
    {"AV1 SVT", "libsvtav1"},
  }, m_output.video.codec);

  addCheck(video, "use CRF instead of bitrate", m_output.video.useCrfRate);

  addSpin(video, "CRF quality", 0, 68, m_output.video.crf);
  addSpin(video, "bitrate", 0, 100000, m_output.video.bitrate);

  // i know this is quite ugly, but whatever.
  addCombo(video, "fps", {
    {"copy source", -1.0},
    {"5",   5.0},
    {"10",  10.0},
    {"15",  15.0},
    {"20",  20.0},
    {"23.976", 23.976},
    {"24", 24.0},
    {"25", 25.0},
    {"30", 30.0},
    {"50", 50.0},
    {"60", 60.0},
    {"72", 72.0},
  }, m_output.video.fps);


  auto audio = new QTreeWidgetItem(output, {"audio", ""});
  audio->setExpanded(true);
  addCheck(audio, "enabled", m_output.audio.enabled);

  addCombo(audio, "codec", {
    {"copy source", "copy"},
    {"AAC", "aac"},
    {"Ogg Opus", "libopus"},
    {"Ogg Vorbis", "libvorbis"},
    {"MP3 LAME", "libmp3lame"},
    {"FLAC", "flac"},
    {"WAV 16-bit PCM", "pcm_s16le"},
  }, m_output.audio.codec);

  addSpin(audio, "bitrate", 32, 512, m_output.audio.bitrate);

  addCombo(audio, "sample rate", {
    {"copy source", -1},
    {"8000Hz", 8000},
    {"11025Hz", 11025},
    {"16000Hz", 16000},
    {"22050Hz", 22050},
    {"32000Hz", 32000},
    {"44100Hz", 44100},
    {"48000Hz", 48000},
    {"96000Hz", 96000},
    {"176400Hz", 176400},
  }, m_output.audio.sampleRate);
}
