#include <QApplication>
#include "MainWindow.h"


int main(int argc, char** argv) {
  QApplication app(argc, argv);
  app.setApplicationName("mediakit");
  app.setStyle("Fusion");
  
  MkMainWindow wnd;
  wnd.show();

  return app.exec();
}
