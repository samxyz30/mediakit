#include <QListWidget>
#include <QLabel>
#include <QDockWidget>
#include <QToolBar>
#include <QFileDialog>
#include <QStatusBar>
#include <QMessageBox>
#include "common.h"
#include "MainWindow.h"
#include "VideoPlayer.h"

#include "JobQueue.h"
#include "MediaList.h"
#include "PropsPanel.h"
#include "SettingsWindow.h"


MkMainWindow::MkMainWindow() {
  setWindowTitle("mediakit");
  resize(1280, 720);

  setDockNestingEnabled(true);
  setTabPosition(Qt::AllDockWidgetAreas, QTabWidget::North);

  auto queue = new MkJobQueue;
  auto mediaList = new MkMediaList;
  auto props = new MkPropsPanel;

  vidPlayer = new MkVideoPlayer(this);
  setCentralWidget(vidPlayer);
  
  addDockWidget(Qt::BottomDockWidgetArea, makeDock("queue", "queueDock", queue));
  addDockWidget(Qt::LeftDockWidgetArea, makeDock("media list", "mediaListDock", mediaList));
  addDockWidget(Qt::RightDockWidgetArea, makeDock("properties", "propsDock", props));
  


  auto fileTb = addToolBar("file");
  fileTb->setObjectName("fileTb");
  
  auto openAct = fileTb->addAction("open media");
  connect(openAct, &QAction::triggered, this, [this, mediaList]() {
    const auto paths = QFileDialog::getOpenFileNames(
      this,
      "open video", QString(),
      "video files (*.mp4 *.mkv *.mov *.avi *.webm);;all files (*)"
    );

    if (!paths.isEmpty()) mediaList->addFiles(paths);
  });

  connect(mediaList, &MkMediaList::activatedMedia, this, [this, props](const QString& path) {
    vidPlayer->loadFile(path);
    props->loadFile(path);
    statusBar()->showMessage("loaded: " + path);
  });


  auto trimAct = fileTb->addAction("trim");
  connect(trimAct, &QAction::triggered, this, [this, queue]() {
    vidPlayer->exportTrim(queue);
  });

  auto transcodeAct = fileTb->addAction("transcode");
  connect(transcodeAct, &QAction::triggered, this, [&]() {
    const auto range = vidPlayer->getTrimRange();
    props->exportTranscode(queue, range);
  });


  auto settingsAct = fileTb->addAction("settings");
  connect(settingsAct, &QAction::triggered, this, [this]() {
    SettingsWindow cfg(this);
    cfg.exec();
  });


  statusBar()->showMessage("ready");
}


QDockWidget* MkMainWindow::makeDock(const QString& title, const QString& objName, QWidget* content) {
  auto dock = new QDockWidget(title, this);
  dock->setObjectName(objName);
  dock->setWidget(content);
  dock->setAllowedAreas(Qt::AllDockWidgetAreas);
  dock->setFeatures(dock->features() & ~QDockWidget::DockWidgetClosable);
  return dock;
}
