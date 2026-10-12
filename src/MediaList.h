#pragma once
#include <QListWidget>

class MkMediaList : public QListWidget {
  Q_OBJECT
public:
  MkMediaList(QWidget* parent = nullptr);
  void addFiles(const QStringList& paths);

signals:
  void activatedMedia(const QString& path);

private:
  void itemSelected(QListWidgetItem* current);
protected:
  void dragEnterEvent(QDragEnterEvent* evt) override;
  void dragMoveEvent(QDragMoveEvent* evt) override;
  void dropEvent(QDropEvent* evt) override;
};
