#pragma once
#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QFileInfo>
#include <QTreeWidget>

#include <QProcess>
#include <QJsonDocument>
#include <QJsonValue>
#include <QJsonObject>
#include "common.h"


class MkPropsPanel : public QWidget {
  Q_OBJECT
public:
  MkPropsPanel(QWidget* parent = nullptr) : QWidget(parent) {
    auto layout = new QVBoxLayout(this);
    m_title = new QLabel("no media selected");
    m_title->setWordWrap(true);
    layout->addWidget(m_title);

    m_tree = new QTreeWidget(this);
    m_tree->setColumnCount(2);
    m_tree->setHeaderLabels({"property", "value"});    
    layout->addWidget(m_tree, 1);
  }

  void loadFile(const QString& path) {
    m_currentPath = QFileInfo(path).absoluteFilePath();
    m_tree->clear();

    QFileInfo info(m_currentPath);
    m_title->setText(
      QString("%1 (%2)").arg(info.fileName()).arg(MkCommon::fmtSize(info.size()))
    );

    auto file = new QTreeWidgetItem(m_tree, {"file", ""});
    new QTreeWidgetItem(file, {"name", info.fileName()});
    new QTreeWidgetItem(file, {"path", info.absoluteFilePath()});
    new QTreeWidgetItem(file, {"size", MkCommon::fmtSize(info.size())});
    file->setExpanded(true);

    auto stat = new QTreeWidgetItem(m_tree, {"stat", ""});
    new QTreeWidgetItem(stat, {"wait", "probing..."});
    stat->setExpanded(true);


    // its probing time
    auto process = new QProcess(this);
    connect(process, &QProcess::finished, this, [this, process, stat](int code, QProcess::ExitStatus status) {
      const auto sout = process->readAllStandardOutput();
      const auto serr = process->readAllStandardError();
      if (status != QProcess::NormalExit || code != 0) {
        stat->child(0)->setText(1, "ffprobe failed!");
        return;
      }

      QJsonParseError jsonErr;
      const auto jsonDoc = QJsonDocument::fromJson(sout, &jsonErr);
      if (jsonErr.error != QJsonParseError::NoError || !jsonDoc.isObject()) {
        stat->child(0)->setText(1, "ffprobe returned invalid json!");
        return;
      }

      delete stat;
      populateData(jsonDoc.object());
    });

    connect(process, &QProcess::errorOccurred, this, [this, process, stat](QProcess::ProcessError err) {
      if (err != QProcess::FailedToStart) return;
      stat->child(0)->setText(1, "could not run ffprobe: " + process->errorString());
    });

    // TODO: unhardcode ffprobe and ffmpeg! define some mechanism in MkCommon
    process->start("ffprobe", {
      "-v", "error",
      "-show_format", "-show_streams",
      "-of", "json",
      m_currentPath
    });
  }


private:
  QString m_currentPath;
  QLabel* m_title;
  QTreeWidget* m_tree;


  void populateData(const QJsonObject& root) {
    auto valueof = [](const QJsonObject& a, const QString& k) -> QString {
      const QJsonValue val = a.value(k);
      if (val.isString()) return val.toString();
      if (val.isDouble()) return QString::number(val.toDouble(), 'g', 10);
      return {};
    };
    

    const QJsonObject format = root.value("format").toObject();
    
    // TODO: add more fields from the debug below
    qDebug() << "ffprobe result" << root;

    if (!format.isEmpty()) {
      auto sect = new QTreeWidgetItem(m_tree, {"format", ""});
      auto timestr = MkCommon::fmtTime(valueof(format, "duration").toDouble() * 1000.0);
      new QTreeWidgetItem(sect, {"duration", timestr});

      new QTreeWidgetItem(sect, {"container", valueof(format, "format_long_name")});
      new QTreeWidgetItem(sect, {"bitrate", QString("%1 kbps").arg(valueof(format, "bit_rate").toLongLong() / 1000)});
    }
  }
};
