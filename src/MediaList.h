#pragma once
#include <QListWidget>
#include <QMenu>
#include <QMimeData>
#include <QUrl>
#include <QFileInfo>
#include <QDrag>

class MkMediaList : public QListWidget {
  Q_OBJECT
public:
  MkMediaList(QWidget* parent = nullptr) : QListWidget(parent) {
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
  

  void addFiles(const QStringList& paths) {
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
    if (count() > 0) {
      setCurrentRow(0);
      itemSelected(currentItem());
    }
  }

signals:
  void activatedMedia(const QString& path);


private:
  void itemSelected(QListWidgetItem* current) {
    const QString path = current->data(Qt::UserRole).toString();
    if (!path.isEmpty()) emit activatedMedia(path);
  }

protected:
  void dragEnterEvent(QDragEnterEvent* evt) override {
    if (evt->mimeData()->hasUrls()) evt->acceptProposedAction();
    else evt->ignore();
  }
  void dragMoveEvent(QDragMoveEvent* evt) override {
    if (evt->mimeData()->hasUrls()) evt->acceptProposedAction();
    else evt->ignore();
  }

  void dropEvent(QDropEvent* evt) override {
    QStringList paths;
    for (const QUrl& url : evt->mimeData()->urls()) {
      if (url.isLocalFile()) paths.append(url.toLocalFile());
    }
    addFiles(paths);
    evt->acceptProposedAction();
  }
};
