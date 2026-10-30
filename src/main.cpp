#include <QApplication>
#include "MainWindow.h"
#include "SettingsWindow.h"

int main(int argc, char** argv) {
  QApplication app(argc, argv);
  app.setOrganizationName("mediakit");
  app.setApplicationName("mediakit");
  
  SettingsWindow::reloadSettings();
  
  MkMainWindow wnd;
  wnd.show();

  return app.exec();
}
