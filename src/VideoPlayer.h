#pragma once
#include <QWidget>

class QMediaPlayer;
class QAudioOutput;
class QVideoWidget;
class QPushButton;
class QSlider;
class QLabel;
class QDoubleSpinBox;
class QProcess;

class MkJobQueue;
class TrimSlider;


struct MkVideoTrimRange {
  double start;
  double end;
};

class MkVideoPlayer : public QWidget {
  Q_OBJECT
public:
  MkVideoPlayer(QWidget* parent = nullptr);
  void loadFile(const QString& path);
  MkVideoTrimRange getTrimRange();
  void exportTrim(MkJobQueue*);

private:
  QString m_currentPath = "";
  QMediaPlayer* m_player = nullptr;
  QAudioOutput* m_audioOut = nullptr;
  QVideoWidget* m_videoOut = nullptr;

  QPushButton* m_btnPlay = nullptr;
  TrimSlider* m_sldSeek = nullptr;
  QLabel* m_lblTime = nullptr;

  void lblTimeUpdate();
};
