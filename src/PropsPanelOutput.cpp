#include "PropsPanel.h"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QLineEdit>


static void addSpin(QFormLayout* parent, const QString& label, int min, int max, int& setting) {
  auto spin = new QSpinBox;
  spin->setRange(min, max);
  spin->setValue(setting);
  auto* tgt = &setting;
  QObject::connect(spin, &QSpinBox::valueChanged, spin, [tgt](int val) { *tgt = val; });
  parent->addRow(label, spin);
}

static void addCheck(QFormLayout* parent, const QString& label, bool& setting) {
  auto check = new QCheckBox;
  check->setChecked(setting);
  auto* tgt = &setting;
  QObject::connect(check, &QCheckBox::toggled, check, [tgt](bool on) { *tgt = on; });
  parent->addRow(label, check);
}

template<typename T>
static void addCombo(QFormLayout* parent, const QString& label,
                     const QList<QPair<QString, QVariant>>& opts, T& setting) {
  auto combo = new QComboBox;
  for (const auto& [key, prop] : opts) combo->addItem(key, prop);
  combo->setCurrentIndex(combo->findData(QVariant::fromValue(setting)));
  
  auto* tgt = &setting;
  QObject::connect(combo, &QComboBox::currentIndexChanged, combo, [tgt, combo](int idx) {
    *tgt = combo->itemData(idx).value<T>();
  });
  parent->addRow(label, combo);
}



// https://stackoverflow.com/questions/4857188/clearing-a-layout-in-qt
// wanted to try out the assignment in if thing here so i refactored it my way :)
static void clearLayout(QLayout* layout) {
  while (auto it = layout->takeAt(0)) {
    if (auto w = it->widget()) delete w;
    if (auto l = it->layout()) { clearLayout(l); delete l; }
    delete it;
  }
}

void MkPropsPanel::populateOutputWidget() {
  if (m_outputItem->layout())
    clearLayout(m_outputItem->layout());
  else
    new QVBoxLayout(m_outputItem);
  
  auto layout = (QVBoxLayout*)m_outputItem->layout();
  
  // ----- GENERAL
  auto gen_gbox = new QGroupBox("general");
  auto gen_l    = new QFormLayout(gen_gbox);
  addCombo(gen_l, "container", {
    {"MP4", "mp4"},
    {"MKV", "mkv"},
    {"WebM", "webm"},
    {"M4A", "m4a"},
    {"Ogg", "ogg"},
    {"Opus/Ogg", "opus"},
    {"MP3", "mp3"},
    {"FLAC", "flac"},
    {"WAV", "wav"},
  }, m_output.container);

  // need a addEdit() later. TODO
  auto ff_args = new QLineEdit;
  ff_args->setText(m_output.ffargs);
  connect(ff_args, &QLineEdit::textChanged, this, [this](const QString& text) { m_output.ffargs = text; });
  gen_l->addRow("advanced args", ff_args);

  layout->addWidget(gen_gbox);


  // ----- VIDEO
  auto vid_gbox = new QGroupBox("video");
  auto vid_l    = new QFormLayout(vid_gbox);
  addCheck(vid_l, "enabled", m_output.video.enabled);

  addCombo(vid_l, "codec", {
    {"copy source", "copy"},
    {"H.264", "libx264"},
    {"H.265", "libx265"},
    {"AV1 SVT", "libsvtav1"},
  }, m_output.video.codec);

  // TODO: maybe instead let's grey out the spinboxes?? do something more UX friendly
  addCheck(vid_l, "use CRF instead of bitrate", m_output.video.useCrfRate);

  addSpin(vid_l, "CRF quality", 0, 68, m_output.video.crf);
  addSpin(vid_l, "bitrate", 0, 100000, m_output.video.bitrate);

  // i know this is quite ugly, but whatever.
  addCombo(vid_l, "fps", {
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
  layout->addWidget(vid_gbox);


  // ----- AUDIO
  auto aud_gbox = new QGroupBox("audio");
  auto aud_l    = new QFormLayout(aud_gbox);
  addCheck(aud_l, "enabled", m_output.audio.enabled);

  addCombo(aud_l, "codec", {
    {"copy source", "copy"},
    {"AAC", "aac"},
    {"Ogg Opus", "libopus"},
    {"Ogg Vorbis", "libvorbis"},
    {"MP3 LAME", "libmp3lame"},
    {"FLAC", "flac"},
    {"WAV 16-bit PCM", "pcm_s16le"},
    {"WAV 32-bit float", "pcm_f32le"},
  }, m_output.audio.codec);

  addSpin(aud_l, "bitrate", 0, 512, m_output.audio.bitrate);

  addCombo(aud_l, "sample rate", {
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
  layout->addWidget(aud_gbox);

  layout->addStretch(1);
}
