#pragma once
#include <QString>
#include <QStringList>

namespace MkCommon {

static QString fmtTime(qint64 ms) {
  const qint64 total = ms/1000;
  const qint64 hrs = total/3600;
  const qint64 mins = (total%3600)/60;
  const qint64 secs = total%60;

  return QString("%1:%2:%3")
    .arg(hrs, 2, 10, QChar('0'))
    .arg(mins, 2, 10, QChar('0'))
    .arg(secs, 2, 10, QChar('0'));
};

static QString fmtSize(qint64 bytes) {
  double size = bytes;
  const QStringList UNITS = {"B","KB","MB","GB","TB"};
  int unit = 0;
  while (size >= 1024.0 && unit < UNITS.size()-1) {
    size /= 1024.0;
    ++unit;
  }

  if (unit == 0) return QString("%1 B").arg(bytes);
  return QString("%1 %2").arg(size, 0, 'f', 2).arg(UNITS[unit]);
};

}
