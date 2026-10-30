#include "MediaList.h"
#include <QListWidget>
#include <QMenu>
#include <QMimeData>
#include <QUrl>
#include <QFileInfo>
#include <QDrag>
#include <QDragEnterEvent>

MkMediaList::MkMediaList(QWidget* parent) : QListWidget(parent) {
  setAcceptDrops(true);
  setDropIndicatorShown(true);
  setDragDropMode(QAbstractItemView::DropOnly);

  setSelectionMode(QAbstractItemView::SingleSelection);
  setContextMenuPolicy(Qt::CustomContextMenu);

  
  connect(this, &QListWidget::itemDoubleClicked,
    this, &MkMediaList::itemSelected);
  
  connect(this, &QWidget::customContextMenuRequested, this, [this](const QPoint& pos) {
    auto item = itemAt(pos);
    if (!item) return;
    QMenu menu(this);
    auto removeAct = menu.addAction("remove item");
    if (menu.exec(viewport()->mapToGlobal(pos)) == removeAct) {
      delete takeItem(row(item));
    }
  });
}

void MkMediaList::addFiles(const QStringList& paths) {
  for (auto& path : paths) {
    const QString full = QFileInfo(path).absoluteFilePath();
    bool exists = false;
    for (int i=0; i<count(); ++i) {
      if (item(i)->data(Qt::UserRole).toString() == full) {
        exists = true;
        break;
      }
    }

    if (exists) continue;
    auto entry = new QListWidgetItem(QFileInfo(full).fileName(), this);
    entry->setData(Qt::UserRole, full);
    entry->setToolTip(full);
  }

  // pick the first item automatically
  // this is commented out for now, i just don't really like this behavior.
  // TODO. i still have to decide if this is good or not
  //if (count() > 0) {
  //  setCurrentRow(0);
  //  itemSelected(currentItem());
  //}
}


void MkMediaList::itemSelected(QListWidgetItem* current) {
  const QString path = current->data(Qt::UserRole).toString();
  if (!path.isEmpty()) emit activatedMedia(path);
}

void MkMediaList::dragEnterEvent(QDragEnterEvent* evt) {
  if (evt->mimeData()->hasUrls()) evt->acceptProposedAction();
  else evt->ignore();
}
void MkMediaList::dragMoveEvent(QDragMoveEvent* evt) {
  if (evt->mimeData()->hasUrls()) evt->acceptProposedAction();
  else evt->ignore();
}

void MkMediaList::dropEvent(QDropEvent* evt) {
  QStringList paths;
  for (const QUrl& url : evt->mimeData()->urls()) {
    if (url.isLocalFile()) paths.append(url.toLocalFile());
  }
  addFiles(paths);
  evt->acceptProposedAction();
}
