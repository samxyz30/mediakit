#include "JobQueue.h"
#include "JobFFmpeg.h"
#include <QWidget>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QLabel>
#include <QLayout>
#include <QSplitter>
#include <QScrollBar>

static QString statusToText(MkJobFFmpeg::Status status) {
  using Status = MkJobFFmpeg::Status;
  
  switch (status) {
  case Status::Queued:    return "Queued";
  case Status::Running:   return "Running";
  case Status::Completed: return "Completed";
  case Status::Failed:    return "Failed";
  case Status::Cancelled: return "Cancelled";
  }
  return "N/A";
}


MkJobQueue::MkJobQueue(QWidget* parent) : QWidget(parent) {
  m_joblist = new QListWidget(this);
  m_details = new QPlainTextEdit(this);
  m_details->setReadOnly(true);

  m_progress = new QProgressBar(this);
  m_status = new QLabel("pick a job", this);

  auto btnCancel = new QPushButton("cancel", this);
  auto btnClear = new QPushButton("clear all", this);
  
  auto btns = new QHBoxLayout;
  btns->addWidget(btnCancel);
  btns->addWidget(btnClear);

  auto details = new QVBoxLayout;
  details->addWidget(m_status);
  details->addWidget(m_progress);
  details->addLayout(btns);
  details->addWidget(m_details);

  auto rw = new QWidget(this);
  rw->setLayout(details);


  auto splitter = new QSplitter(this);
  splitter->addWidget(m_joblist);
  splitter->addWidget(rw);
  splitter->setStretchFactor(0,1);

  auto layout = new QVBoxLayout(this);
  layout->addWidget(splitter);

  connect(m_joblist, &QListWidget::currentRowChanged, this, &MkJobQueue::showSelected);
  connect(btnCancel, &QPushButton::clicked, this, &MkJobQueue::cancelJob);
  connect(btnClear, &QPushButton::clicked, this, &MkJobQueue::clearJobs);
}

MkJobFFmpeg* MkJobQueue::enqueue(const MkJobFFmpeg::Spec& spec) {
  auto job = new MkJobFFmpeg(spec, this);
  m_jobs.append(job);
  auto item = new QListWidgetItem(spec.name, m_joblist);
  m_items.append(item);

  connect(job, &MkJobFFmpeg::changeStatus, this, [this](MkJobFFmpeg::Status) {
    refreshJobs();
  });
  connect(job, &MkJobFFmpeg::changeProgress, this, [this](float) {
    refreshJobs();
  });

  connect(job, &MkJobFFmpeg::addedLog, this, [this](const QString&) {
    showSelected(m_joblist->currentRow());
  });
  
  connect(job, &MkJobFFmpeg::finished, this, [this]() {
    scheduler();
    refreshJobs();
  });
  
  m_joblist->setCurrentItem(item);
  scheduler();
  refreshJobs();
  return job;
}


void MkJobQueue::scheduler() {
  if (m_running && m_running->status() == MkJobFFmpeg::Status::Running) return;
  m_running = nullptr;
  for (auto job : m_jobs) {
    if (job->status() == MkJobFFmpeg::Status::Queued) {
      m_running = job;
      job->start();
      return;
    }
  }
}

void MkJobQueue::cancelJob() {
  if (m_selected) m_selected->cancel();
}

void MkJobQueue::clearJobs() {
  for (int i=m_jobs.size()-1; i>=0; --i) {
    auto job = m_jobs[i];
    if (job == m_selected) m_selected = nullptr;
    delete m_joblist->takeItem(i);
    m_jobs.removeAt(i);
    m_items.removeAt(i);
    job->deleteLater();
  }

  showSelected(m_joblist->currentRow());
  scheduler();
}


void MkJobQueue::refreshJobs() {
  for (int i=0; i<m_jobs.size(); ++i) {
    auto job = m_jobs[i];
    m_items[i]->setText(QString("%1 - %2%").arg(job->spec().name).arg(job->progress()));
    m_items[i]->setToolTip(statusToText(job->status()));
  }

  showSelected(m_joblist->currentRow());
}

void MkJobQueue::showSelected(int row) {
  if (row < 0 || row >= m_jobs.size()) {
    m_selected = nullptr;
    m_status->setText("pick a job");
    m_progress->setValue(0);
    m_details->clear();
    return;
  }

  m_selected = m_jobs[row];
  m_status->setText(statusToText(m_selected->status()));
  m_progress->setValue(m_selected->progress());
  m_details->setPlainText(m_selected->logs().join('\n'));
  m_details->verticalScrollBar()->setValue(m_details->verticalScrollBar()->maximum());
}

