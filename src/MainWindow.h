#pragma once
#include <QMainWindow>

class MkVideoPlayer;
class QDockWidget;

class MkMainWindow : public QMainWindow {
public:
  MkMainWindow();
private:
  MkVideoPlayer* vidPlayer = nullptr;
  QDockWidget* makeDock(const QString& title, const QString& objName, QWidget* content);
};
