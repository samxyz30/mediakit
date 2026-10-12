#pragma once

#include <QObject>
#include <QProcess>
#include <QDateTime>
#include "common.h"

class MkJobFFmpeg : public QObject {
  Q_OBJECT

public:
  enum class Status {
    Queued, Cancelled,
    Running, Completed, Failed,
  };
  Q_ENUM(Status)

  struct Spec {
    QString name;
    QString infile;
    QString outfile;
    QString exec = MkCommon::findTool("ffmpeg");
    QStringList args;
    // assumed duration, used to calculate progress
    qint64 assumedLengthMs = 0;
  };

  MkJobFFmpeg(Spec spec, QObject* parent = nullptr) : QObject(parent),
                                                      m_spec(std::move(spec)) {
      m_proc = new QProcess(this);

      connect(m_proc, &QProcess::readyReadStandardOutput,
        this, &MkJobFFmpeg::readStdout);
      connect(m_proc, &QProcess::readyReadStandardError,
        this, &MkJobFFmpeg::readStderr);
      
      connect(m_proc, &QProcess::errorOccurred,
        this, [this](QProcess::ProcessError err) {
          if (err != QProcess::FailedToStart) return;
          doLog(QString("failed to start: %1").arg(m_proc->errorString()));
          doFinish(m_finishCancel ? Status::Cancelled : Status::Failed);
      });

      connect(m_proc, &QProcess::finished,
        this, &MkJobFFmpeg::processFinished);
  }

  const Spec& spec() const { return m_spec; }
  Status status() const { return m_stat; }
  float progress() const { return m_progress; }
  QStringList logs() const { return m_logs; }
  QDateTime createdAt() const { return m_createdAt; }


  void start() {
    if (m_stat != Status::Queued) return;
    setStatus(Status::Running);
    doLog("starting: " + m_spec.exec + " " + m_spec.args.join(' '));

    m_proc->setProgram(m_spec.exec);
    m_proc->setArguments(m_spec.args);
    m_proc->start();
  }

  void cancel() {
    if (m_stat == Status::Queued) {
      doFinish(Status::Cancelled);
    } else {
      m_finishCancel = true;
      m_proc->terminate();
    }
  }

signals:
  void changeStatus(Status status);
  void changeProgress(float percent);
  void addedLog(const QString& line);
  void finished();

private:
  QDateTime m_createdAt = QDateTime::currentDateTime();
  Spec m_spec;
  QProcess* m_proc = nullptr;
  
  Status m_stat = Status::Queued;
  float m_progress = 0.0f;
  
  QStringList m_logs;
  QString m_stdoutBuf;

  bool m_finishCancel = false;

  
  void setStatus(Status status) {
    if (m_stat == status) return;
    m_stat = status;
    emit changeStatus(m_stat);
  }

  void setProgress(float v) {
    v = qBound(0.0f, v, 100.0f);
    m_progress = v;
    emit changeProgress(m_progress);
  }

  void doLog(const QString& line) {
    m_logs.append(line);
    emit addedLog(line);
  }

  void doFinish(Status status) {
    setStatus(status);
    emit finished();
  }


private slots:
  void readStdout() {
    m_stdoutBuf += QString::fromUtf8(m_proc->readAllStandardOutput());
    const QStringList lines = m_stdoutBuf.split('\n');
    m_stdoutBuf = lines.isEmpty() ? QString() : lines.last();

    for (int i=0; i+1<lines.size(); ++i) {
      auto line = lines[i].trimmed();

      if (line.startsWith("out_time_us=")) {
        bool ok = false;
        const qint64 micro = line.mid(QString("out_time_us=").size()).toLongLong(&ok);
        if (ok && m_spec.assumedLengthMs > 0) {
          const qint64 ms = micro/1000;
          const float percent = ms * 100.0 / m_spec.assumedLengthMs;
          setProgress(qMin(percent, 99.0f));
        }
      }
    }
  }

  void readStderr() {
    const QString out = QString::fromUtf8(m_proc->readAllStandardError()).trimmed();
    if (!out.isEmpty()) doLog(out);
  }

  void processFinished(int code, QProcess::ExitStatus stat) {
    if (m_finishCancel) {
      doFinish(Status::Cancelled);
    } else if (stat == QProcess::NormalExit && code == 0) {
      setProgress(100.f);
      doFinish(Status::Completed);
    } else {
      doLog(QString("ffmpeg failed with code %1").arg(code));
      doFinish(Status::Failed);
    }
  }
};
