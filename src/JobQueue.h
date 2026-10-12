#pragma once
#include <QWidget>
#include "JobFFmpeg.h"

class QListWidget;
class QListWidgetItem;
class QLabel;
class QProgressBar;
class QPlainTextEdit;


class MkJobQueue : public QWidget {
  Q_OBJECT
public:
  MkJobQueue(QWidget* parent = nullptr);
  MkJobFFmpeg* enqueue(const MkJobFFmpeg::Spec& spec);

private:
  QList<MkJobFFmpeg*> m_jobs;
  QList<QListWidgetItem*> m_items;
  QListWidget* m_joblist = nullptr;

  QLabel* m_status = nullptr;
  QProgressBar* m_progress = nullptr;
  QPlainTextEdit* m_details = nullptr;

  MkJobFFmpeg* m_running = nullptr;
  MkJobFFmpeg* m_selected = nullptr;

  void scheduler();
  void cancelJob();
  void clearJobs();
  void refreshJobs();
  void showSelected(int row);
};
