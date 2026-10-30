#pragma once
#include <QDialog>

class QComboBox;

class SettingsWindow : public QDialog {
  Q_OBJECT
public:
  SettingsWindow(QWidget* parent = nullptr);
  static void reloadSettings();
  void saveSettings();

private:
  QComboBox* m_styleCombo = nullptr;
};
