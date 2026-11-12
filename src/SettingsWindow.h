#pragma once
#include <QDialog>

class QComboBox;

class MkSettingsWindow : public QDialog {
  Q_OBJECT
public:
  MkSettingsWindow(QWidget* parent = nullptr);
  static void reloadSettings();
  void saveSettings();

private:
  QComboBox* m_styleCombo = nullptr;
};
