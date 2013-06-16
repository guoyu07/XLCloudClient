/*
 *  CloudClient - A Qt cloud client for lixian.vip.xunlei.com
 *  Copyright (C) 2012 by Aaron Lewis <the.warl0ck.1989@gmail.com>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "thunderpanel.h"
#include "ui_thunderpanel.h"

#define OFFSET_DOWNLOAD 1
#define OFFSET_SOURCE 2
#define OFFSET_TASKID 3
#define OFFSET_TYPE 4

ThunderPanel::ThunderPanel(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ThunderPanel),
    my_quickViewMode(true),
    my_filterModel(new QSortFilterProxyModel),
    my_model(new QStandardItemModel),
    my_contextMenu(new QMenu(this))
{
    ui->setupUi(this);
    ui->filterPanel->hide();

    QAction *action = new QAction (QIcon(":/resources/images/movie.png"),
                                   tr("Preview"), this);
    connect (action, SIGNAL(triggered()), SLOT(slotPreviewThisTask()));
    my_contextMenu->addAction(action);

    action = new QAction (tr("Download"), this);
    connect (action, SIGNAL(triggered()), SLOT(slotDownloadThisTask()));
    my_contextMenu->addAction(action);
    my_contextMenu->addSeparator();

    action = new QAction (QIcon(":/resources/images/user-trash.png"),
                          tr("Remove selected tasks"), this);
    connect (action, SIGNAL(triggered()), SLOT(slotRemoveTheseTasks()));
    my_contextMenu->addAction(action);
    my_contextMenu->addSeparator();

    action = new QAction (tr("Copy download address"), this);
    connect (action, SIGNAL(triggered()), SLOT(slotCopyDownloadAddress()));
    my_contextMenu->addSeparator();
    my_contextMenu->addAction(action);

    action = new QAction (tr("Copy source address"), this);
    connect (action, SIGNAL(triggered()), SLOT(slotCopySourceAddress()));
    my_contextMenu->addAction(action);

    action = new QAction (tr("Copy task name"), this);
    connect (action, SIGNAL(triggered()), SLOT(slotCopyTaskName()));
    my_contextMenu->addAction(action);

    action = new QAction (tr("Copy as Aria2c"), this);
    connect (action, SIGNAL(triggered()), SLOT(slotCopyAria2cScript()));
    my_contextMenu->addAction(action);

    my_contextMenu->addSeparator();
    action = new QAction (tr("Collapse all"), this);
    connect (action, SIGNAL(triggered()), ui->treeView, SLOT(collapseAll()));
    my_contextMenu->addAction(action);

    action = new QAction (tr("Expand all"), this);
    connect (action, SIGNAL(triggered()), ui->treeView, SLOT(expandAll()));
    my_contextMenu->addAction(action);

    action = new QAction (tr("Resize Columns"), this);
    connect (action, SIGNAL(triggered()), this, SLOT(slotResizeAllColumnsOfTreeView()));
    my_contextMenu->addAction(action);

    connect (ui->treeView, SIGNAL(customContextMenuRequested(QPoint)),
             SLOT(slotShowContextMenu(QPoint)));

    my_model->setHorizontalHeaderLabels(QStringList()
                                        << tr("Size")
                                        << tr("Name"));
    my_filterModel->setSourceModel(my_model);

    ui->treeView->setModel(my_model);
    ui->treeView->resizeColumnToContents(0);
    connect (ui->treeView, SIGNAL(expanded(QModelIndex)), SLOT(slotResizeFirstColumnOfTreeView()));
}

ThunderPanel::~ThunderPanel()
{
    delete ui;
}

void ThunderPanel::slotResizeAllColumnsOfTreeView()
{
    for (int i = 0; i < my_filterModel->columnCount(); ++i)
        ui->treeView->resizeColumnToContents(i);
}

void ThunderPanel::slotResizeFirstColumnOfTreeView()
{
    ui->treeView->resizeColumnToContents(0);
}

void ThunderPanel::resizeColumnToContents(int column)
{
    ui->treeView->resizeColumnToContents(column);
}

void ThunderPanel::keyEvent(QKeyEvent *e)
{
    switch (e->modifiers())
    {
    case Qt::CTRL:
        switch (e->key())
        {
        case Qt::Key_F:
            if (ui->filterPanel->isVisible())
            {
                ui->filter->selectAll();
            }
            else
            {
                ui->filterPanel->show();
                ui->filter->setFocus();
            }
            break;
        }
    default:
        switch (e->key())
        {
        case Qt::Key_Escape:
            ui->filterPanel->hide();
            break;
        }
        break;
    }
}

void ThunderPanel::slotShowContextMenu (const QPoint &pt)
{
    Q_UNUSED(pt)
    my_contextMenu->exec(QCursor::pos());
}

void ThunderPanel::setQuickViewMode(bool ok)
{
    my_quickViewMode = ok;
}

void ThunderPanel::slotDownloadThisTask()
{
    const QString & url = getUserDataByOffset(OFFSET_DOWNLOAD);
    if (url.isEmpty()) return;
    emit doThisLink(getFirstSelectedTask(), Download, false);
}

void ThunderPanel::slotCopyAria2cScript()
{
    /// No bad case seen yet, escaping single quote seems necessary
    QString name = getUserDataByOffset(0);
    QString link = getUserDataByOffset(OFFSET_DOWNLOAD);
    if (name.isEmpty() || link.isEmpty()) return;

    QApplication::clipboard()->setText(QString("aria2c --load-cookies '%1' '%2' -o '%3'")
                                       .arg(my_cookiePath)
                                       .arg(link.replace("'", "\\'"))
                                       .arg(name.replace("'", "\\'"))
                                       );
}

void ThunderPanel::slotCopyTaskName()
{
    const QString & name = getUserDataByOffset(0);
    if (name.isEmpty()) return;

    QApplication::clipboard()->setText(name);
}

void ThunderPanel::slotRemoveTheseTasks()
{
    emit doIndirectRequest(RemoveTasks);
}

void ThunderPanel::slotPreviewThisTask()
{
    const QString & url = getUserDataByOffset(OFFSET_DOWNLOAD);
    if (url.isEmpty()) return;
    emit doThisLink(getFirstSelectedTask(), Preview, false);
}

Thunder::RemoteTask ThunderPanel::getFirstSelectedTask ()
{
    Thunder::RemoteTask task;
    QModelIndex idx, idx2, currentIdx = ui->treeView->currentIndex();

    //    qDebug() << currentIdx.row();

    // top level
    if (! currentIdx.parent().isValid())
    {
        int row = ui->treeView->currentIndex().row();
        idx = my_model->index(row, 0);
        idx2 = my_model->index(row, 1);
    }
    // child item
    else
    {
        idx = currentIdx.parent().child(currentIdx.row(), 0);
        idx2 = currentIdx.parent().child(currentIdx.row(), 1);
    }

    if (idx.isValid())
    {
        task.url = my_model->data(idx, Qt::UserRole + OFFSET_DOWNLOAD).toString();
        task.name = my_model->data(idx2).toString();
    }

    //    qDebug() << task.name << task.url;

    return task;
}

QString ThunderPanel::getUserDataByOffset (unsigned long long offset, int row)
{
    const QModelIndex & currentIndex = ui->treeView->currentIndex();
    int role = (offset == 0) ? (Qt::DisplayRole + 0) : (Qt::UserRole + offset);

    // top level
    if (! currentIndex.parent().isValid())
    {
        if (row == -1) row = currentIndex.row();
        const QModelIndex & idx = my_model->index(row, (offset == 0) ? 1 : 0);
        if (idx.isValid())
            return my_model->data(idx, role).toString();
    }

    else
    {
        const QModelIndex & idx = currentIndex.parent().child(
                    currentIndex.row(), (offset == 0) ? 1 : 0);
        if (idx.isValid())
            return my_model->data(idx, role).toString();
    }

    return QString ();
}

void ThunderPanel::removeSelectedTasks (const QList<int> &ids)
{
    foreach (const int & row, ids)
    {
        my_filterModel->removeRow(row);
    }
}

QPair<QStringList, QList<int> > ThunderPanel::getSelectedTaskIDsAndRows()
{
    QPair<QStringList, QList<int> > result;

    foreach (const QModelIndex & idx,
             ui->treeView->selectionModel()->selectedIndexes())
        if (! idx.parent().isValid()
                && idx.column() == 0)
        {
            result.first.append(getUserDataByOffset(OFFSET_TASKID, idx.row()));
            result.second.push_back(idx.row());
        }

    return result;
}

void ThunderPanel::slotCopyDownloadAddress()
{
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(getUserDataByOffset(OFFSET_DOWNLOAD));
}

void ThunderPanel::slotCopySourceAddress()
{
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(getUserDataByOffset(OFFSET_SOURCE));
}

void ThunderPanel::setBTSubTask(const Thunder::BitorrentTask &task)
{
    if (! my_BTSubTaskMapping.contains(task.taskid))
    {
        qDebug() << "Mismatch: " << task.taskid;
        return;
    }

    foreach (const Thunder::BTSubTask & subtask, task.subtasks)
    {
        QList<QStandardItem*> items = QList<QStandardItem*>()
                << new QStandardItem (subtask.size)
                << new QStandardItem (subtask.name);

        items.first()->setIcon(Util::getFileAttr(subtask.name, false).icon);

        items.first()->setData(subtask.link,   Qt::UserRole + OFFSET_DOWNLOAD);
        //        items.first()->setData(subtask.id,     Qt::UserRole + OFFSET_TASKID);

        for (int i = 0; i < items.size(); ++i)
            items.at(i)->setTextAlignment(Qt::AlignCenter);

        if (subtask.link.isEmpty())
            items.at(1)->setBackground(QBrush (QColor("#9CC6EE")));

        my_BTSubTaskMapping.value(task.taskid)->appendRow(items);
    }
}

void ThunderPanel::setCloudTasks(const QList<Thunder::Task> &tasks)
{
    my_model->setRowCount(0);
    my_BTSubTaskMapping.clear();

    foreach (const Thunder::Task & task, tasks)
    {
        QList<QStandardItem*> items = QList<QStandardItem*>()
                << new QStandardItem (Util::toReadableSize(task.size))
                << new QStandardItem (task.name);

        my_model->appendRow(items);

        items.first()->setIcon(Util::getFileAttr(task.name,
                                                 task.type == Thunder::BT).icon);

        items.first()->setData(task.link,   Qt::UserRole + OFFSET_DOWNLOAD);
        items.first()->setData(task.id,     Qt::UserRole + OFFSET_TASKID);
        items.first()->setData(task.source, Qt::UserRole + OFFSET_SOURCE);
        items.first()->setData(task.type,   Qt::UserRole + OFFSET_TYPE);

        for (int i = 0; i < items.size(); ++i)
        {
            if (task.link.isEmpty() && i == 1 && task.type != Thunder::BT)
            {
                items.at(i)->setBackground(QBrush (QColor("#9CC6EE")));
            }
            items.at(i)->setTextAlignment(Qt::AlignCenter);
        }

        if (task.type == Thunder::BT)
        {
            my_BTSubTaskMapping.insert(task.id, items.first());
        }
    }

    ui->treeView->resizeColumnToContents(0);
}

void ThunderPanel::on_treeView_doubleClicked(const QModelIndex &index)
{
    Q_UNUSED(index);
    if (! my_quickViewMode) return;

    const Thunder::RemoteTask & task = getFirstSelectedTask ();
    switch (Util::shouldAutoOpen(task.name))
    {
    case Thunder::Never:
        break;
    case Thunder::Video:
        emit doThisLink(task, Preview, false);
        break;
    case Thunder::Document:
        emit doThisLink(task, Download, true);
        break;
    case Thunder::Image:
        emit doThisLink(task, Image, true);
        break;
    }

}

void ThunderPanel::on_filter_textChanged(const QString &arg1)
{
    my_filterModel->setFilterRegExp(arg1);
}
