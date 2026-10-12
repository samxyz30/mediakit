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

class MkVideoPlayer : public QWidget {
public:
  MkVideoPlayer(QWidget* parent = nullptr, MkJobQueue* = nullptr);
  void loadFile(const QString& path);
  void exportTrim();
private:
  MkJobQueue* m_queue;

  QString m_currentPath = "";
  QMediaPlayer* m_player = nullptr;
  QAudioOutput* m_audioOut = nullptr;
  QVideoWidget* m_videoOut = nullptr;

  QPushButton* m_btnPlay = nullptr;
  TrimSlider* m_sldSeek = nullptr;
  QLabel* m_lblTime = nullptr;

  void lblTimeUpdate();
};
