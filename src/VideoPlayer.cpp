#include "VideoPlayer.h"
#include "VideoPlayerTrimmer.h"
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QVideoWidget>
#include <QPushButton>
#include <QSlider>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QDoubleSpinBox>

#include <QMessageBox>
#include <QProcess>
#include <QFileInfo>
#include <QFileDialog>
#include <QShortcut>

#include "common.h"
#include "JobQueue.h"
using MkCommon::fmtTime;



MkVideoPlayer::MkVideoPlayer(QWidget* parent) : QWidget(parent) {
  m_player = new QMediaPlayer(this);
  m_audioOut = new QAudioOutput(this);
  m_videoOut = new QVideoWidget(this);
  m_player->setAudioOutput(m_audioOut);
  m_player->setVideoOutput(m_videoOut);

  m_btnPlay = new QPushButton("play");
  m_sldSeek = new TrimSlider;
  m_lblTime = new QLabel("00:00 / 00:00");

  auto controls = new QHBoxLayout;
  controls->addWidget(m_btnPlay);
  controls->addWidget(m_lblTime);
  controls->addWidget(m_sldSeek, 1);


  // TODO: loop toggle to loop video somewhere here?
  
  auto lblTrim = new QLabel("");
  auto btnTrimStart = new QPushButton("set start");
  auto btnTrimEnd = new QPushButton("set end");

  auto trimControls = new QHBoxLayout;
  trimControls->addWidget(lblTrim, 1);
  trimControls->addWidget(btnTrimStart);
  trimControls->addWidget(btnTrimEnd);


  auto layout = new QVBoxLayout(this);
  layout->setContentsMargins(0,0,0,0);
  layout->addWidget(m_videoOut, 1);
  layout->addLayout(controls);
  layout->addLayout(trimControls);


  connect(m_btnPlay, &QPushButton::clicked, this, [this]() {
    if (m_player->playbackState() == QMediaPlayer::PlayingState) {
      m_player->pause();
      return;
    }
    
    // restart if outside trim range behavior
    const auto pos = m_player->position();
    if (pos < m_sldSeek->start() || pos >= m_sldSeek->end()) {
      m_player->setPosition(m_sldSeek->start());
    }

    m_player->play();
  });

  connect(m_player, &QMediaPlayer::playbackStateChanged, this, [this](QMediaPlayer::PlaybackState s) {
    m_btnPlay->setText(s == QMediaPlayer::PlayingState ? "pause" : "play");
  });


  // set slider range
  connect(m_player, &QMediaPlayer::durationChanged, this, [this](qint64 dur) {
    m_sldSeek->setDuration(dur);
    m_sldSeek->setEnabled(dur > 0);
    lblTimeUpdate();

    m_sldSeek->setRange(0, dur);
  });

  // sync seek with playback
  connect(m_player, &QMediaPlayer::positionChanged, this, [this](qint64 pos) {
    m_sldSeek->setPosition(pos);
    lblTimeUpdate();

    // if we go past the end, pause
    if (pos >= m_sldSeek->end()) {
      m_player->pause();
    }
  });

  // seek in player
  connect(m_sldSeek, &TrimSlider::sliderMoved, this, [this](qint64 pos) {
    m_player->setPosition(pos);
  });

  

  // trimmer
  connect(m_sldSeek, &TrimSlider::rangeChanged, this, [this, lblTrim](qint64 start, qint64 end) {
    lblTrim->setText(QString("trim %1 - %2, length %3")
                      .arg(fmtTime(start)).arg(fmtTime(end))
                      .arg(fmtTime(end-start))
    );
  });
  

  connect(btnTrimStart, &QPushButton::clicked, this, [this]() {
    m_sldSeek->setRange(m_player->position(), m_sldSeek->end());
  });
  connect(btnTrimEnd, &QPushButton::clicked, this, [this]() {
    m_sldSeek->setRange(m_sldSeek->start(), m_player->position());
  });


  // shortcuts
  connect(new QShortcut(QKeySequence(Qt::Key_I), this), &QShortcut::activated, btnTrimStart, &QPushButton::click);
  connect(new QShortcut(QKeySequence(Qt::Key_O), this), &QShortcut::activated, btnTrimEnd, &QPushButton::click);
  connect(new QShortcut(QKeySequence(Qt::Key_Space), this), &QShortcut::activated, m_btnPlay, &QPushButton::click);
}


void MkVideoPlayer::loadFile(const QString& path) {
  if (path.isEmpty()) return;
  m_currentPath = path;
  m_player->setSource(QUrl::fromLocalFile(path));
  m_player->play();
}

void MkVideoPlayer::lblTimeUpdate() {
  m_lblTime->setText(fmtTime(m_player->position()) + " / " + fmtTime(m_player->duration()));
}



MkVideoTrimRange MkVideoPlayer::getTrimRange() {
  const double start = m_sldSeek->start() / 1000.0;
  const double end   = m_sldSeek->end() / 1000.0;
  return { start, end };
}

void MkVideoPlayer::exportTrim(MkJobQueue* queue) {
  if (m_currentPath.isEmpty()) {
    QMessageBox::warning(this, "export", "no file selected");
    return;
  }

  const double start = m_sldSeek->start() / 1000.0;
  const double end =   m_sldSeek->end() / 1000.0;

  if (start < 0 || end <= start || end > m_player->duration()) {
    QMessageBox::warning(this, "export", "invalid trim range");
    return;
  }

  // TODO: unhardcode .mp4 and video assumptions
  const QString defaultName = QFileInfo(m_currentPath).completeBaseName() + "_trimmed.mp4";
  const QString outFile = QFileDialog::getSaveFileName(
    this,
    "export video", QFileInfo(m_currentPath).absolutePath() + "/" + defaultName,
    "MP4 (*.mp4);;Matroska (*.mkv);;All files (*)"
  );

  if (outFile.isEmpty()) return;

  const double duration = end - start;
  MkJobFFmpeg::Spec spec;
  spec.name = QFileInfo(outFile).fileName();
  spec.infile = m_currentPath;
  spec.outfile = outFile;
  spec.exec = MkCommon::findTool("ffmpeg");
  spec.assumedLengthMs = duration;
  spec.args = {
    "-hide_banner", "-y",
    "-progress", "pipe:1",
    "-nostats",
    "-i", m_currentPath,
    "-ss", QString::number(start, 'f', 3),
    "-t", QString::number(duration, 'f', 3),
    "-map", "0:v?", "-map", "0:a?",
    "-c", "copy",
    outFile
  };

  queue->enqueue(spec);
}
