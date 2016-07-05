#include "create_volume.h"

pooHelperThread::pooHelperThread(
        QObject *parent,
        virConnectPtr *connPtrPtr,
        QString _name) :
    _VirtThread(parent, connPtrPtr), name(_name)
{

}
void pooHelperThread::run()
{
    if ( nullptr==ptr_ConnPtr || nullptr==*ptr_ConnPtr ) {
        emit ptrIsNull();
        return;
    };
    if ( virConnectRef(*ptr_ConnPtr)<0 ) {
        sendConnErrors();
        return;
    };
    // something data reading
    pool = virStoragePoolLookupByName(
            *ptr_ConnPtr, name.toUtf8().data());
    QDomDocument doc;
    doc.setContent(
                QString(
                    virStoragePoolGetXMLDesc(
                        pool,
                        VIR_STORAGE_XML_INACTIVE))
                );
    type = doc.firstChildElement("pool").attribute("type");
    if ( virConnectClose(*ptr_ConnPtr)<0 )
        sendConnErrors();
}

/*
 * http://libvirt.org/formatstorage.html
 */

CreateVolume::CreateVolume(
        QWidget         *parent,
        virConnectPtr   *connPtrPtr,
        QString          _poolName) :
    _CreateStorage(parent)
{
    setUrl("http://libvirt.org/formatstorage.html");
    settingName.append("CreateStorageVolume");
    settings.beginGroup(settingName);
    restoreGeometry(
                settings.value("Geometry").toByteArray());
    showAtClose->setChecked(
                settings.value("ShowAtClose").toBool() );
    settings.endGroup();
    suff->setVisible(true);
    allocLabel = new QComboBox(this);
    allocLabel->addItem("Allocation (KiB):", "KiB");
    allocLabel->addItem("Allocation (MiB):", "MiB");
    allocLabel->addItem("Allocation (GiB):", "GiB");
    allocLabel->addItem("Allocation (TiB):", "TiB");
    allocLabel->addItem("Allocation (EiB):", "EiB");
    capLabel = new QComboBox(this);
    capLabel->addItem("Capacity (KiB):", "KiB");
    capLabel->addItem("Capacity (MiB):", "MiB");
    capLabel->addItem("Capacity (GiB):", "GiB");
    capLabel->addItem("Capacity (TiB):", "TiB");
    capLabel->addItem("Capacity (EiB):", "EiB");
    allocation = new QSpinBox(this);
    allocation->setRange(0, 1024);
    capacity = new QSpinBox(this);
    capacity->setRange(0, 1024);
    sizeLayout = new QGridLayout();
    sizeLayout->addWidget(allocLabel, 0, 0);
    sizeLayout->addWidget(allocation, 0, 1);
    sizeLayout->addWidget(capLabel, 1, 0);
    sizeLayout->addWidget(capacity, 1, 1);
    sizeWdg = new QWidget(this);
    sizeWdg->setLayout(sizeLayout);

    helperThread = new pooHelperThread(
                this, connPtrPtr, _poolName);
    connect(helperThread, SIGNAL(finished()),
            this, SLOT(initData()));
    connect(helperThread, SIGNAL(errorMsg(QString&,uint)),
            this, SIGNAL(errorMsg(QString&)));
    helperThread->start();
}

/* public slots */
QString CreateVolume::getXMLDescFileName() const
{
    QDomDocument doc;
    QDomText _text;
    QDomElement _volume, _name, _target,
            _source, _allocation, _capacity,
            _format, _permissions, _owner,
            _group, _mode, _label, _encrypt;
    _volume = doc.createElement("volume");
    _name = doc.createElement("name");
    _text = doc.createTextNode(
                QString("%1.img").arg(stName->text()));
    _name.appendChild(_text);
    _volume.appendChild(_name);
    _allocation = doc.createElement("allocation");
    _text = doc.createTextNode(allocation->text());
    _allocation.setAttribute(
                "unit",
                allocLabel->itemData(
                    allocLabel->currentIndex(),
                    Qt::UserRole).toString());
    _allocation.appendChild(_text);
    _volume.appendChild(_allocation);
    _capacity = doc.createElement("capacity");
    _text = doc.createTextNode(capacity->text());
    _capacity.setAttribute(
                "unit",
                capLabel->itemData(
                    capLabel->currentIndex(),
                    Qt::UserRole).toString());
    _capacity.appendChild(_text);
    _volume.appendChild(_capacity);
    _target = doc.createElement("target");
    if ( target->usePerm->isChecked() ||
         target->format->currentText()!="default" ) {
        if ( target->format->currentText()!="default" ) {
            _format = doc.createElement("format");
            _format.setAttribute(
                        "type",
                        target->format->currentText().toLower());
            _target.appendChild(_format);
        };
        if ( target->usePerm->isChecked() ) {
            _permissions = doc.createElement("permissions");
            _owner = doc.createElement("owner");
            _text = doc.createTextNode(target->owner->text());
            _owner.appendChild(_text);
            _permissions.appendChild(_owner);
            _group = doc.createElement("group");
            _text = doc.createTextNode(target->group->text());
            _group.appendChild(_text);
            _permissions.appendChild(_group);
            _mode = doc.createElement("mode");
            _text = doc.createTextNode(target->mode->text());
            _mode.appendChild(_text);
            _permissions.appendChild(_mode);
            _label = doc.createElement("label");
            _text = doc.createTextNode(target->label->text());
            _label.appendChild(_text);
            _permissions.appendChild(_label);
            _target.appendChild(_permissions);
        };
    };
    if ( target->encrypt->isUsed() ) {
        _encrypt = doc.createElement("encryption");
        _target.appendChild(_encrypt);
        _encrypt.setAttribute(
                    "format",
                    target->encrypt->getFormat());
    };
    if ( !_target.isNull() ) _volume.appendChild(_target);
    doc.appendChild(_volume);
    //qDebug()<<doc.toString();

    bool read = xml->open();
    if (read) xml->write(doc.toByteArray(4).data());
    xml->close();
    return xml->fileName();
}

/* private slots */
void CreateVolume::initData()
{
    type->addItem(helperThread->type);

    //source = new _Storage_Source(this);
    target = new _Storage_Target(this, helperThread->type);
    target->formatWdg->setVisible(true);
    target->encrypt->setVisible(true);

    infoStuffLayout = new QVBoxLayout();
    //infoStuffLayout->addWidget(source);
    infoStuffLayout->addWidget(target);
    infoStuffLayout->addStretch(-1);
    infoStuff = new QWidget(this);
    infoStuff->setLayout(infoStuffLayout);
    info->addWidget(infoStuff);

    commonLayout->insertWidget(
                commonLayout->count()-1, sizeWdg);
    commonLayout->insertWidget(
                commonLayout->count()-1, infoWidget, -1);
    connect(allocLabel, SIGNAL(currentIndexChanged(int)),
            capLabel, SLOT(setCurrentIndex(int)));
    connect(capLabel, SIGNAL(currentIndexChanged(int)),
            allocLabel, SLOT(setCurrentIndex(int)));
    allocLabel->setCurrentIndex(1);
    //capLabel->setCurrentIndex(1);
}