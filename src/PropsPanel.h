#pragma once
#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QFileInfo>
#include <QTreeWidget>
#include <QComboBox>
#include <QSpinBox>
#include <QMessageBox>
#include <QFileDialog>

#include <QProcess>
#include <QJsonDocument>
#include <QJsonValue>
#include <QJsonObject>

#include "common.h"
#include "JobQueue.h"


struct MkOutputSettings {
  QString container = "mp4";
  QString codecVideo = "libx264";
  bool useCrf = true;
  int crf = 23;
  int bitrate = 0;
  bool noVideo = false;
  bool noAudio = false;
  QString codecAudio = "aac";
  int bitrateAudio = 128;
};

class MkPropsPanel : public QWidget {
  Q_OBJECT
public:
  MkPropsPanel(QWidget* parent = nullptr) : QWidget(parent) {
    auto layout = new QVBoxLayout(this);
    m_title = new QLabel("no media selected");
    m_title->setWordWrap(true);
    layout->addWidget(m_title);

    m_tree = new QTreeWidget(this);
    m_tree->setColumnCount(2);
    m_tree->setHeaderLabels({"property", "value"});    
    layout->addWidget(m_tree, 1);
  }

  void loadFile(const QString& path) {
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


  void exportTranscode(MkJobQueue* queue) {
    if (m_currentPath.isEmpty()) {
      QMessageBox::warning(this, "export", "no file selected");
      return;
    }

    // do NOT use m_currentPath, figure something nicer out
    const QString outFile = QFileDialog::getSaveFileName(
      this,
      "export transcode", m_currentPath,
      "All files (*)"
    );
    if (outFile.isEmpty()) return;

    QMessageBox::critical(this, "TODO", "TODO");
    return;

    // TODO: construct command from m_output
    MkJobFFmpeg::Spec spec;
    spec.name = QFileInfo(outFile).fileName();
    queue->enqueue(spec);
  }


private:
  QString m_currentPath;
  QLabel* m_title;
  QTreeWidget* m_tree;
  MkOutputSettings m_output;


  void populateData(const QJsonObject& root) {
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

      auto timestr = MkCommon::fmtTime(valueof(format, "duration").toDouble() * 1000.0);
      new QTreeWidgetItem(sect, {"duration", timestr});
      new QTreeWidgetItem(sect, {"container", valueof(format, "format_long_name")});
      new QTreeWidgetItem(sect, {"bitrate", QString("%1 kbps").arg(valueof(format, "bit_rate").toLongLong() / 1000)});
    }


    // TODO: clean up. i know this is bad.
    auto output = new QTreeWidgetItem(m_tree, {"output", ""});
    output->setExpanded(true);
    auto containerItem = new QTreeWidgetItem(output, {"container", ""});
    auto container = new QComboBox(m_tree);
    container->addItem("MP4", "mp4");
    container->addItem("MKV", "mkv");
    container->addItem("WebM", "webm");
    container->addItem("Ogg", "ogg");
    container->addItem("WAV (audio)", "wav");
    m_tree->setItemWidget(containerItem, 1, container);
    connect(container, &QComboBox::currentIndexChanged, this, [this, container](int idx) {
      m_output.container = container->itemData(idx).toString();
    });

    auto videoItem = new QTreeWidgetItem(output, {"video codec", ""});
    auto videoCodec = new QComboBox(m_tree);
    videoCodec->addItem("copy", "copy");
    videoCodec->addItem("H.264", "libx264");
    videoCodec->addItem("H.265", "libx265");
    videoCodec->addItem("AV1", "libsvtav1");
    m_tree->setItemWidget(videoItem, 1, videoCodec);
    connect(videoCodec, &QComboBox::currentIndexChanged, this, [this, videoCodec](int idx) {
      m_output.codecVideo = videoCodec->itemData(idx).toString();
    });

    auto qualityItem = new QTreeWidgetItem(output, {"video quality (CRF)", ""});
    auto qualityCrf = new QSpinBox(m_tree);
    qualityCrf->setRange(0, 64);
    qualityCrf->setValue(23);
    m_tree->setItemWidget(qualityItem, 1, qualityCrf);
    connect(qualityCrf, &QSpinBox::valueChanged, this, [this](int val) {
      m_output.crf = val;
    });

    // TODO: bitrate. probably check useCrf flag part of m_output?

    // TODO: noVideo, noAudio flags

    ///
    auto audioItem = new QTreeWidgetItem(output, {"audio codec", ""});
    auto audioCodec = new QComboBox(m_tree);
    audioCodec->addItem("copy", "copy");
    audioCodec->addItem("aac", "aac");
    audioCodec->addItem("opus", "libopus");
    audioCodec->addItem("vorbis", "libvorbis");
    audioCodec->addItem("mp3", "libmp3lame");
    audioCodec->addItem("flac", "flac");
    audioCodec->addItem("pcm 16-bit", "pcm_s16le");
    m_tree->setItemWidget(audioItem, 1, audioCodec);
    connect(audioCodec, &QComboBox::currentIndexChanged, this, [this, audioCodec](int idx) {
      m_output.codecAudio = audioCodec->itemData(idx).toString();
    });

    auto aqualityItem = new QTreeWidgetItem(output, {"audio quality (kbps)", ""});
    auto aqualityRate = new QSpinBox(m_tree);
    aqualityRate->setRange(32, 512);
    aqualityRate->setValue(128);
    m_tree->setItemWidget(aqualityItem, 1, aqualityRate);
    connect(aqualityRate, &QSpinBox::valueChanged, this, [this](int val) {
      m_output.bitrateAudio = val;
    });

  }
};
