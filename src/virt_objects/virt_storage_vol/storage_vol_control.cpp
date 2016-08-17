#include "storage_vol_control.h"

VirtStorageVolControl::VirtStorageVolControl(QWidget *parent) :
    VirtEntityControl(parent)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setObjectName("VirtStorageVolControl");
    setWindowTitle("StorageVol Control");
    setWindowIcon(QIcon::fromTheme("virtual-engineering"));
    storageVolModel = new StorageVolModel();
    entityList->setModel(storageVolModel);
    //connect(entityList, SIGNAL(doubleClicked(const QModelIndex&)),
    //        this, SLOT(entityDoubleClicked(const QModelIndex&)));
    connect(entityList, SIGNAL(customContextMenuRequested(const QPoint&)),
            this, SLOT(entityClicked(const QPoint&)));
    setUsageInSoftTouched(false);
    settings.beginGroup("VirtStorageVolControl");
    entityList->setColumnWidth(0, settings.value("column0", 132).toInt());
    entityList->setColumnWidth(1, settings.value("column1", 32).toInt());
    entityList->setColumnWidth(2, settings.value("column2", 32).toInt());
    entityList->setColumnWidth(3, settings.value("column3", 32).toInt());
    int area_int = settings.value("ToolBarArea", 4).toInt();
    restoreGeometry(settings.value("Geometry").toByteArray());
    settings.endGroup();
    toolBar = new StorageVolToolBar(this);
    addToolBar(toolBar->get_ToolBarArea(area_int), toolBar);
    connect(toolBar, SIGNAL(fileForMethod(const Act_Param&)),
            this, SLOT(newVirtEntityFromXML(const Act_Param&)));
    connect(toolBar, SIGNAL(execMethod(const Act_Param&)),
            this, SLOT(execAction(const Act_Param&)));
}
VirtStorageVolControl::~VirtStorageVolControl()
{
    settings.beginGroup("VirtStorageVolControl");
    settings.setValue("column0", entityList->columnWidth(0));
    settings.setValue("column1", entityList->columnWidth(1));
    settings.setValue("column2", entityList->columnWidth(2));
    settings.setValue("column3", entityList->columnWidth(3));
    settings.setValue("ToolBarArea", toolBarArea(toolBar));
    settings.setValue("Geometry", saveGeometry());
    settings.endGroup();
    settings.sync();

    stopProcessing();
}
QString VirtStorageVolControl::getCurrentVolumeName() const
{
    QModelIndex index = entityList->currentIndex();
    if ( !index.isValid() ) return QString();
    return storageVolModel->DataList.at(index.row())->getName();
}
QString VirtStorageVolControl::getCurrentVolumePath() const
{
    QModelIndex index = entityList->currentIndex();
    if ( !index.isValid() ) return QString();
    return storageVolModel->DataList.at(index.row())->getPath();
}

/* public slots */
void VirtStorageVolControl::stopProcessing()
{
    setEnabled(false);
    toolBar->stopProcessing();

    // clear StorageVol list
    while ( storageVolModel->DataList.count() ) {
        storageVolModel->removeRow(0);
    };
    storageVolModel->setHeaderData(0, Qt::Horizontal, QString("Name"), Qt::EditRole);
}
bool VirtStorageVolControl::setCurrentStoragePool(
        virConnectPtr *connPtrPtr, const QString &connName, const QString &poolName)
{
    stopProcessing();
    ptr_ConnPtr = connPtrPtr;
    setEnabled(true);
    currConnName = connName;
    currPoolName = poolName;
    storageVolModel->setHeaderData(
                0,
                Qt::Horizontal,
                QString("Volume in [ %1 ]").arg(poolName),
                Qt::EditRole);
    toolBar->enableAutoReload();
    // for initiation content
    reloadState();
    return true;
}
void VirtStorageVolControl::resultReceiver(Result data)
{
    if ( data.name!=objectName() ) return;
    //qDebug()<<data.msg<<"result";
    if ( data.action == GET_ALL_ENTITY_STATE ) {
        entityList->setEnabled(true);
        int chain  = storageVolModel->columnCount();
        int chains = data.msg.count()/chain;
        if ( chains > storageVolModel->DataList.count() ) {
            int _diff = chains - storageVolModel->DataList.count();
            for ( int i = 0; i<_diff; i++ ) {
                storageVolModel->insertRow(1);
                //qDebug()<<i<<"insert";
            };
        };
        if ( storageVolModel->DataList.count() > chains ) {
            int _diff = storageVolModel->DataList.count() - chains;
            for ( int i = 0; i<_diff; i++ ) {
                storageVolModel->removeRow(0);
                //qDebug()<<i<<"remove";
            };
        };
        for (int i = 0; i<chains; i++) {
            for (int j = 0; j<chain; j++) {
                storageVolModel->setData(storageVolModel->index(i,j), data.msg.at(i*chain+j), Qt::EditRole);
            };
        };
    } else if ( data.action == CREATE_ENTITY ) {
        if ( !data.msg.isEmpty() ) {
            QString msg = data.msg.join(" ");
            msgRepeater(msg);
            reloadState();
        };
    } else if ( data.action == DELETE_ENTITY ) {
        if ( !data.msg.isEmpty() ) {
            QString msg = data.msg.join(" ");
            msgRepeater(msg);
            reloadState();
        };
    } else if ( data.action == DOWNLOAD_ENTITY ) {
        if ( !data.msg.isEmpty() ) {
            QString msg = data.msg.join(" ");
            msgRepeater(msg);
        };
    } else if ( data.action == UPLOAD_ENTITY ) {
        if ( !data.msg.isEmpty() ) {
            QString msg = data.msg.join(" ");
            msgRepeater(msg);
        };
    } else if ( data.action == RESIZE_ENTITY ) {
        if ( !data.msg.isEmpty() ) {
            QString msg = data.msg.join(" ");
            msgRepeater(msg);
            reloadState();
        };
    } else if ( data.action == WIPE_ENTITY ) {
        if ( !data.msg.isEmpty() ) {
            QString msg = data.msg.join(" ");
            msgRepeater(msg);
        };
    } else if ( data.action == GET_XML_DESCRIPTION ) {
        QString xml = data.fileName;
        data.msg.append(QString("to <a href='%1'>%1</a>").arg(xml));
        QString msg = data.msg.join(" ");
        msgRepeater(msg);
        if ( data.result )
            QDesktopServices::openUrl(QUrl(xml));
    };
}
void VirtStorageVolControl::closeEvent(QCloseEvent *ev)
{
    if ( ev->type()==QEvent::Close ) {
        QString key = objectName();
        QString msg = QString("'<b>%1</b>' pool overview closed.")
                .arg(currPoolName);
        emit msgRepeater(msg);
        emit finished(key);
    };
}

/* private slots */
void VirtStorageVolControl::reloadState()
{
    entityList->setEnabled(false);
    entityList->clearSelection();
    TASK task;
    task.type       = VIRT_STORAGE_VOLUME;
    task.srcConnPtr = ptr_ConnPtr;
    task.srcConName = currConnName;
    task.action     = GET_ALL_ENTITY_STATE;
    task.method     = reloadEntity;
    task.args.object= currPoolName;
    emit addNewTask(task);
}
void VirtStorageVolControl::changeDockVisibility()
{
    toolBar->setEnabled( !toolBar->isEnabled() );
    entityList->setEnabled( !entityList->isEnabled() );
}
void VirtStorageVolControl::entityClicked(const QPoint &p)
{
    //qDebug()<<"custom Menu request";
    QModelIndex idx = entityList->indexAt(p);
    QStringList params;
    if ( idx.isValid() && storageVolModel->DataList.count()>idx.row() ) {
        //qDebug()<<storageVolModel->DataList.at(idx.row())->getName();
        params<<storageVolModel->DataList.at(idx.row())->getName();
        //params<<storageVolModel->DataList.at(idx.row())->getPath();
        //params<<storageVolModel->DataList.at(idx.row())->getType();
        //params<<storageVolModel->DataList.at(idx.row())->getCurrSize();
        //params<<storageVolModel->DataList.at(idx.row())->getLogicSize();
    } else {
        entityList->clearSelection();
    };
    bool state = toolBar->getAutoReloadState();
    StorageVolControlMenu *storageVolControlMenu =
            new StorageVolControlMenu(this, params, state);
    connect(storageVolControlMenu, SIGNAL(execMethod(const Act_Param&)),
            this, SLOT(execAction(const Act_Param&)));
    storageVolControlMenu->move(QCursor::pos());
    storageVolControlMenu->exec();
    disconnect(storageVolControlMenu, SIGNAL(execMethod(const Act_Param&)),
               this, SLOT(execAction(const Act_Param&)));
    storageVolControlMenu->deleteLater();
}
void VirtStorageVolControl::entityDoubleClicked(const QModelIndex &index)
{
    if ( index.isValid() ) {
        qDebug()<<storageVolModel->DataList.at(index.row())->getName();
    }
}
void VirtStorageVolControl::execAction(const Act_Param &param)
{
    QModelIndex idx = entityList->currentIndex();
    if ( idx.isValid() && storageVolModel->DataList.count()>idx.row() ) {
        QString storageVolName =
                storageVolModel->DataList.at(idx.row())->getName();
        TASK task;
        task.type = VIRT_STORAGE_VOLUME;
        task.srcConnPtr = ptr_ConnPtr;
        task.srcConName = currConnName;
        task.object     = storageVolName;
        task.args.object= currPoolName;
        task.method     = param.method;
        if        ( param.method==reloadEntity ) {
            reloadState();
        } else if ( param.method==deleteEntity ) {
            task.action     = DELETE_ENTITY;
            emit addNewTask(task);
        } else if ( param.method==downloadVirtStorageVol ) {
            QString path =
                    QFileDialog::getSaveFileName(
                        this, "Save to", "~");
            if ( !path.isEmpty() ) {
                task.action     = DOWNLOAD_ENTITY;
                task.args.path  = path;
                //task.args.size  = storageVolModel->DataList
                //        .at(idx.row())->getCurrSize().toULongLong();
                emit addNewTask(task);
            } else return;
        } else if ( param.method==resizeVirtStorageVol ) {
            ResizeDialog *resizeDialog = new ResizeDialog(
                        this,
                        ptr_ConnPtr,
                        currPoolName,
                        storageVolName);
            int res = resizeDialog->exec();
            unsigned long long size = resizeDialog->getNewSize();
            resizeDialog->deleteLater();
            if ( res ) {
                task.args.size = size;
            } else {
                return;
            };
            task.action     = RESIZE_ENTITY;
            emit addNewTask(task);
        } else if ( param.method==uploadVirtStorageVol ) {
            QString path =
                    QFileDialog::getOpenFileName(
                        this, "Read from", "~");
            if ( !path.isEmpty() ) {
                task.action     = UPLOAD_ENTITY;
                task.args.path  = path;
                emit addNewTask(task);
            } else return;
        } else if ( param.method==wipeVirtStorageVol ) {
            task.action     = WIPE_ENTITY;
            task.args.sign  = param.path.toUInt();
            emit addNewTask(task);
        } else if ( param.method==getEntityXMLDesc ) {
            task.action     = GET_XML_DESCRIPTION;
            emit addNewTask(task);
        };
    } else if ( param.method==reloadEntity ) {
        reloadState();
    };
}
void VirtStorageVolControl::newVirtEntityFromXML(const Act_Param &args)
{
    TASK task;
    task.type = VIRT_STORAGE_VOLUME;
    Methods method;
    if ( args.act==CREATE_ENTITY ) {
        method = createEntity;
    } else {
        method = defineEntity;
    };
    task.srcConnPtr = ptr_ConnPtr;
    task.srcConName = currConnName;
    task.method     = method;
    task.action     = args.act;
    task.args.path  = args.path;
    if ( args.context==DO_AsIs ) {
        emit addNewTask(task);
    } else if ( args.context==DO_Edit ) {
        emit volumeToEditor(task);
    } else {
        QString path;
        bool show = false;
        // show SRC Creator widget
        // get path for method
        CreateVolume *createVolumeDialog =
                new CreateVolume(this, ptr_ConnPtr, currPoolName);
        connect(createVolumeDialog, SIGNAL(errorMsg(const QString)),
                this, SLOT(msgRepeater(const QString&)));
        int result = createVolumeDialog->exec();
        if ( result==QDialog::Accepted ) {
            path = createVolumeDialog->getXMLDescFileName();
            show = createVolumeDialog->showXMLDescription();
        };
        createVolumeDialog->deleteLater();
        if ( result==QDialog::Rejected ) return;
        task.args.path = path;
        if ( show ) QDesktopServices::openUrl(QUrl(path));
        emit addNewTask(task);
    };
}
