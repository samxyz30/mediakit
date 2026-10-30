#include <QApplication>
#include "MainWindow.h"
#include "SettingsWindow.h"

int main(int argc, char** argv) {
  // https://doc.qt.io/qt-6/advanced-ffmpeg-configuration.html#configure-hardware-acceleration-in-backends
  // this disables hardware decoding.
  // the hardware decoders are super inconsistent across devices and sometimes don't work for certain encodings (e.g. AV1)
  // this is so hacky but until i find a way which doesn't involve recompiling qt itself, it'll do
  qputenv("QT_FFMPEG_DECODING_HW_DEVICE_TYPES", ",");


  QApplication app(argc, argv);
  app.setOrganizationName("mediakit");
  app.setApplicationName("mediakit");

  SettingsWindow::reloadSettings();

  MkMainWindow wnd;
  wnd.show();

  return app.exec();
}
