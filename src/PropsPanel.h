#pragma once
#include <QWidget>
#include <QList>
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


// presets
struct MkOutputPreset {
  QString name;
  MkOutputSettings settings;
};

const QList<MkOutputPreset> MK_BUILTIN_PRESETS = {
  {"Copy streams", MkOutputSettings{
    "mp4",
    MkOutputVideoSettings{true, "copy", true, 1, 1, -1.0},
    MkOutputAudioSettings{true, "copy", 128, -1},
  }},
  {"H.264 Small File", MkOutputSettings{
    "mp4",
    MkOutputVideoSettings{true, "libx264", true, 26, 2500, -1.0},
    MkOutputAudioSettings{true, "aac", 128, 48000},
  }},
};



class QTreeWidget;
class QTreeWidgetItem;
class QJsonObject;
class QComboBox;

class MkPropsPanel : public QWidget {
  Q_OBJECT
public:
  MkPropsPanel(QWidget* parent = nullptr);
  void loadFile(const QString& path);
  void exportTranscode(MkJobQueue* queue, const MkVideoTrimRange trimRange);

private:
  QString m_currentPath;
  MkOutputSettings m_output;
  QLabel* m_title = nullptr;
  QTreeWidget* m_tree = nullptr;
  QComboBox* m_preset = nullptr;
  QTreeWidgetItem* m_outputItem = nullptr;

  void populateData(const QJsonObject& root);
  void populateOutputWidget(QTreeWidgetItem* output);
};
