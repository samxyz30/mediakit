#include "SettingsWindow.h"
#include "common.h"
#include <QSettings>
#include <QStyleFactory>
#include <QVBoxLayout>
#include <QLabel>
#include <QComboBox>
#include <QApplication>

#include <version.h>  // from version.h.in

MkSettingsWindow::MkSettingsWindow(QWidget* parent) : QDialog(parent) {
  setMinimumSize(400, 300);
  auto layout = new QVBoxLayout(this);
  QSettings cfg;

  auto infostr = QString("version: %1\nQt %2\n\nffmpeg: %3\nffprobe: %4")
    .arg(VERSION_STRING, qVersion())
    .arg(MkCommon::findTool("ffmpeg"), MkCommon::findTool("ffprobe"));
  layout->addWidget(new QLabel(infostr, this));

  layout->addWidget(new QLabel("style:", this));
  m_styleCombo = new QComboBox(this);
  m_styleCombo->addItems(QStyleFactory::keys());
  m_styleCombo->setCurrentText(cfg.value("style").toString());
  connect(m_styleCombo, &QComboBox::currentTextChanged, this, &MkSettingsWindow::saveSettings);
  layout->addWidget(m_styleCombo);

  layout->addStretch(1);
}


void MkSettingsWindow::saveSettings() {
  QSettings cfg;
  cfg.setValue("style", m_styleCombo->currentText());
  reloadSettings();
}

void MkSettingsWindow::reloadSettings() {
  QSettings cfg;
  QString style = cfg.value("style", "Fusion").toString();
  qApp->setStyle(style);
}
