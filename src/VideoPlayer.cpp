#include "VideoPlayer.h"

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
#include <QPainter>
#include <QShortcut>
#include <QMouseEvent>

#include "JobQueue.h"
#include "common.h"
using MkCommon::fmtTime;


// TODO: zoom in/out for long ranges
class TrimSlider : public QWidget {
  Q_OBJECT
public:
  TrimSlider(QWidget* parent = nullptr) : QWidget(parent) {
    setMinimumHeight(48);
  }

  void setDuration(qint64 ms) {
    m_duration = qMax(0, ms);
    m_start = qBound(0, m_start, m_duration);
    m_end = qBound(m_start, m_end, m_duration);
    update();
  }
  
  void setPosition(qint64 ms) {
    m_position = qBound(0, ms, m_duration);
    update();
  }

  void setRange(qint64 start, qint64 end) {
    m_start = qBound(0, start, m_duration);
    m_end = qBound(m_start, end, m_duration);
    emit rangeChanged(m_start, m_end);
    update();
  }

  qint64 start() const { return m_start; }
  qint64 end() const { return m_end; }

signals:
  void sliderMoved(qint64 pos);
  void rangeChanged(qint64 start, qint64 end);

private:
  qint64 m_duration = 0;
  qint64 m_position = 0;
  qint64 m_start = 0;
  qint64 m_end = 0;
  enum class Drag { None, Start, End };
  Drag m_drag = Drag::None;

  void paintEvent(QPaintEvent*) override {
    // https://doc.qt.io/qt-6/qtwidgets-painting-basicdrawing-example.html
    QPainter p(this);

    const double left = 12;
    const double right = width()-12;
    const double trackY = height()/2.0;
    const double trackW = right-left;
    const double trackH = 12;

    p.setPen(Qt::NoPen);
    p.setBrush(QColor("#454545"));
    p.drawRect(QRectF(left, trackY - trackH / 2, trackW, trackH));
    
    if (m_duration <= 0) return;
    auto xft = [&](qint64 ms) {
      if (m_duration <= 0) return left;
      return left+ trackW*ms / m_duration;
    };
    const double xStart = xft(m_start);
    const double xEnd = xft(m_end);
    const double xPlay = xft(m_position);

    p.setBrush(QColor("#0d77da"));
    p.drawRect(QRectF(xStart, trackY - trackH/2, qMax<double>(0, xEnd-xStart), trackH));

    
    auto drawHandle = [&](double x, const QColor& color) {
      p.setBrush(color);
      p.drawRect(QRectF(x-4, trackY-15, 8, 30));
    };

    drawHandle(xStart, QColor("#fff"));
    drawHandle(xEnd, QColor("#fff"));

    p.setPen(QPen(Qt::white, 2));
    p.drawLine(QPointF(xPlay, trackY-19), QPointF(xPlay, trackY+19));
  }

  //
  void mousePressEvent(QMouseEvent* evt) override {
    if (m_duration <= 0) return;
    const double x = evt->position().x();
    const double left = 12;
    const double right = width()-12;
    const double trackW = right-left;

    const double xStart = left+trackW * m_start/m_duration;
    const double xEnd =   left+trackW * m_end/m_duration;

    if (qAbs(x-xStart) <= 10) {
      m_drag = Drag::Start;
    } else if (qAbs(x-xEnd) <= 10) {
      m_drag = Drag::End;
    } else {
      m_drag = Drag::None;
      const qint64 ms = tfx(x);
      emit sliderMoved(ms);
    }
  }

  void mouseMoveEvent(QMouseEvent* evt) override {
    if (m_drag != Drag::None) {
      double x = evt->position().x();
      const qint64 ms = tfx(x);
      if (m_drag == Drag::Start) m_start = qBound(0, ms, m_end);
      else if (m_drag == Drag::End) m_end = qBound(m_start, ms, m_duration);
      else return;
      
      update();
      emit rangeChanged(m_start, m_end);
    }
  }

  void mouseReleaseEvent(QMouseEvent* evt) override {
    m_drag = Drag::None;
    evt->accept();
  }

  qint64 tfx(double x) const {
    const double left = 12;
    const double right =width()-12;
    const double frac = qBound<double>(0, (x-left)/(right-left), 1);
    return frac * m_duration;
  }
};



MkVideoPlayer::MkVideoPlayer(QWidget* parent, MkJobQueue* q) : QWidget(parent) {
  m_queue = q;
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
  controls->addWidget(m_sldSeek, 1);
  controls->addWidget(m_lblTime);


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
    (m_player->playbackState() == QMediaPlayer::PlayingState) ? m_player->pause() : m_player->play();
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
    //inTrimStart->setMaximum(dur / 1000.0);
    //inTrimStart->setEnabled(dur > 0);
    //inTrimStart->setValue(0.0);
    //inTrimEnd->setMaximum(dur / 1000.0);
    //inTrimEnd->setEnabled(dur > 0);
    //inTrimEnd->setValue(dur / 1000.0);
  });

  // sync seek with playback
  connect(m_player, &QMediaPlayer::positionChanged, this, [this](qint64 pos) {
    /*if (!sldSeek->isSliderDown())*/ m_sldSeek->setPosition(pos);
    lblTimeUpdate();
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



void MkVideoPlayer::exportTrim() {
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
    "-hide_banner", "-n",
    "-ss", QString::number(start, 'f', 3),
    "-i", m_currentPath,
    "-t", QString::number(duration, 'f', 3),
    "-map", "0:v?", "-map", "0:a?",
    "-c", "copy",
    "-progress", "pipe:1",
    "-nostats",
    outFile
  };

  m_queue->enqueue(spec);
}


#include "VideoPlayer.moc"