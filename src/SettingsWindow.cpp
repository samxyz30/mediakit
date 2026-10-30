#include "SettingsWindow.h"
#include "common.h"
#include <QSettings>
#include <QStyleFactory>
#include <QVBoxLayout>
#include <QLabel>
#include <QComboBox>
#include <QApplication>

// TODO rethink this. i don't like the way i repeat the config a lot here and i wish it was easier

SettingsWindow::SettingsWindow(QWidget* parent) : QDialog(parent) {
  setMinimumSize(400, 300);
  auto layout = new QVBoxLayout(this);
  QSettings cfg;

  auto infostr = QString("Qt %1\nffmpeg: %2\nffprobe: %3")
    .arg(qVersion())
    .arg(MkCommon::findTool("ffmpeg"))
    .arg(MkCommon::findTool("ffprobe"));
  layout->addWidget(new QLabel(infostr, this));

  layout->addWidget(new QLabel("style:", this));
  m_styleCombo = new QComboBox(this);
  m_styleCombo->addItems(QStyleFactory::keys());
  m_styleCombo->setCurrentText(cfg.value("style").toString());
  connect(m_styleCombo, &QComboBox::currentTextChanged, this, &SettingsWindow::saveSettings);
  layout->addWidget(m_styleCombo);

  layout->addStretch(1);
}


void SettingsWindow::saveSettings() {
  QSettings cfg;
  cfg.setValue("style", m_styleCombo->currentText());
  reloadSettings();
}

void SettingsWindow::reloadSettings() {
  QSettings cfg;
  QString style = cfg.value("style", "Fusion").toString();
  qApp->setStyle(style);
}
