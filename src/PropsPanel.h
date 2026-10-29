#pragma once
#include <QWidget>
#include "JobQueue.h"
#include "VideoPlayer.h"

// HERE: for doubles, -1.0 means same as source
//       for ints, -1 means same as source.
struct MkOutputVideoSettings {
  bool enabled = true;
  QString codec = "copy";
  bool useCrfRate = true;
  int crf = 23;
  int bitrate = 5000;
  double fps = -1.0;
  // TODO: resolution and scaling
};
struct MkOutputAudioSettings {
  bool enabled = true;
  QString codec = "copy";
  int bitrate = 128;
  int sampleRate = -1;
  // TODO: gain, mixdown
};

struct MkOutputSettings {
  QString container = "mp4";
  MkOutputVideoSettings video;
  MkOutputAudioSettings audio;
};


class QTreeWidget;
class QTreeWidgetItem;
class QJsonObject;

class MkPropsPanel : public QWidget {
  Q_OBJECT
public:
  MkPropsPanel(QWidget* parent = nullptr);
  void loadFile(const QString& path);
  void exportTranscode(MkJobQueue* queue, const MkVideoTrimRange trimRange);

private:
  QString m_currentPath;
  QLabel* m_title;
  QTreeWidget* m_tree;
  MkOutputSettings m_output;

  void populateData(const QJsonObject& root);
  void populateOutputWidget(QTreeWidgetItem* output);
};
