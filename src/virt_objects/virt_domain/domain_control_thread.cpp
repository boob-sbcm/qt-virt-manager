#include "domain_control_thread.h"

DomControlThread::DomControlThread(QObject *parent) :
    QThread(parent)
{
    qRegisterMetaType<DomActions>("DomActions");
}

/* public slots */
bool DomControlThread::setCurrentWorkConnect(virConnectPtr conn)
{
    keep_alive = true;
    currWorkConnect = conn;
    //qDebug()<<"net_thread"<<currWorkConnect;
}
void DomControlThread::stop() { keep_alive = false; }
void DomControlThread::execAction(DomActions act, QStringList _args)
{
    action = DOM_EMPTY_ACTION;
    args.clear();
    if ( keep_alive && !isRunning() ) {
        action = act;
        args = _args;
        start();
    };
}

/* private slots */
void DomControlThread::run()
{
    QStringList result;
    switch (action) {
    case GET_ALL_DOMAIN :
        result.append(getAllDomainList());
        break;
    case CREATE_DOMAIN :
        result.append(createDomain());
        break;
    case DEFINE_DOMAIN :
        result.append(defineDomain());
        break;
    case START_DOMAIN :
        result.append(startDomain());
        break;
    case DESTROY_DOMAIN :
        result.append(destroyDomain());
        break;
    case UNDEFINE_DOMAIN :
        result.append(undefineDomain());
        break;
    case CHANGE_DOM_AUTOSTART :
        result.append(changeAutoStartDomain());
        break;
    case GET_DOM_XML_DESC :
        result.append(getDomainXMLDesc());
        break;
    default:
        break;
    };
    emit resultData(action, result);
}
QStringList DomControlThread::getAllDomainList()
{
    QStringList domainList;
    if ( currWorkConnect!=NULL && keep_alive ) {
        virDomainPtr *domain;
        unsigned int flags = VIR_CONNECT_LIST_DOMAINS_ACTIVE |
                             VIR_CONNECT_LIST_DOMAINS_INACTIVE;
        int ret = virConnectListAllDomains( currWorkConnect, &domain, flags);
        if ( ret<0 ) {
            sendConnErrors();
            free(domain);
            return domainList;
        };

        int i = 0;
        while ( domain[i] != NULL ) {
            QStringList currentAttr;
            QString autostartStr;
            int is_autostart = 0;
            if (virDomainGetAutostart(domain[i], &is_autostart) < 0) {
                autostartStr.append("no autostart");
            } else autostartStr.append( is_autostart ? "yes" : "no" );
            currentAttr<< QString( virDomainGetName(domain[i]) )
                       << QString( virDomainIsActive(domain[i]) ? "active" : "inactive" )
                       << autostartStr
                       << QString( virDomainIsPersistent(domain[i]) ? "yes" : "no" );
            domainList.append(currentAttr.join(" "));
            //qDebug()<<currentAttr;
            virDomainFree(domain[i]);
            i++;
        };
        free(domain);
    };
    return domainList;
}
QStringList DomControlThread::createDomain()
{
    QStringList result;
    QString path = args.first();
    QByteArray xmlData;
    QFile f;
    f.setFileName(path);
    if ( !f.open(QIODevice::ReadOnly) ) {
        emit errorMsg( QString("File \"%1\"\nnot opened.").arg(path) );
        return result;
    };
    xmlData = f.readAll();
    f.close();
    virDomainPtr domain = virDomainCreateXML(currWorkConnect, xmlData.data(), VIR_DOMAIN_START_AUTODESTROY);
    if ( domain==NULL ) {
        sendConnErrors();
        return result;
    };
    result.append(QString("'%1' Domain from\n\"%2\"\nis created.").arg(virDomainGetName(domain)).arg(path));
    virDomainFree(domain);
    return result;
}
QStringList DomControlThread::defineDomain()
{
    QStringList result;
    QString path = args.first();
    QByteArray xmlData;
    QFile f;
    f.setFileName(path);
    if ( !f.open(QIODevice::ReadOnly) ) {
        emit errorMsg( QString("File \"%1\"\nnot opened.").arg(path) );
        return result;
    };
    xmlData = f.readAll();
    f.close();
    virDomainPtr domain = virDomainDefineXML(currWorkConnect, xmlData.data());
    if ( domain==NULL ) {
        sendConnErrors();
        return result;
    };
    result.append(QString("'%1' Domain from\n\"%2\"\nis defined.").arg(virDomainGetName(domain)).arg(path));
    virDomainFree(domain);
    return result;
}
QStringList DomControlThread::startDomain()
{
    QStringList result;
    QString name = args.first();
    virDomainPtr *domain;
    unsigned int flags = VIR_CONNECT_LIST_DOMAINS_ACTIVE |
                         VIR_CONNECT_LIST_DOMAINS_INACTIVE;
    int ret = virConnectListAllDomains( currWorkConnect, &domain, flags);
    if ( ret<0 ) {
        sendConnErrors();
        free(domain);
        return result;
    };
    //qDebug()<<QString(virConnectGetURI(currWorkConnect));

    int i = 0;
    bool started = false;
    while ( domain[i] != NULL ) {
        QString currNetName = QString( virDomainGetName(domain[i]) );
        if ( !started && currNetName==name ) {
            started = (virDomainCreate(domain[i])+1) ? true : false;
            if (!started) sendGlobalErrors();
        };
        virDomainFree(domain[i]);
        i++;
    };
    free(domain);
    result.append(QString("'%1' Domain %2 Started.").arg(name).arg((started)?"":"don't"));
    return result;
}
QStringList DomControlThread::destroyDomain()
{
    QStringList result;
    QString name = args.first();
    virDomainPtr *domain;
    unsigned int flags = VIR_CONNECT_LIST_DOMAINS_ACTIVE |
                         VIR_CONNECT_LIST_DOMAINS_INACTIVE;
    int ret = virConnectListAllDomains( currWorkConnect, &domain, flags);
    if ( ret<0 ) {
        sendConnErrors();
        free(domain);
        return result;
    };
    //qDebug()<<QString(virConnectGetURI(currWorkConnect));

    int i = 0;
    bool deleted = false;
    while ( domain[i] != NULL ) {
        QString currDomName = QString( virDomainGetName(domain[i]) );
        if ( !deleted && currDomName==name ) {
            deleted = (virDomainDestroy(domain[i])+1) ? true : false;
            if (!deleted) sendGlobalErrors();
        };
        qDebug()<<QVariant(deleted).toString()<<currDomName<<name;
        virDomainFree(domain[i]);
        i++;
    };
    free(domain);
    result.append(QString("'%1' Domain %2 Destroyed.").arg(name).arg((deleted)?"":"don't"));
    return result;
}
QStringList DomControlThread::undefineDomain()
{
    QStringList result;
    QString name = args.first();
    virDomainPtr *domain;
    unsigned int flags = VIR_CONNECT_LIST_DOMAINS_ACTIVE |
                         VIR_CONNECT_LIST_DOMAINS_INACTIVE;
    int ret = virConnectListAllDomains( currWorkConnect, &domain, flags);
    if ( ret<0 ) {
        sendConnErrors();
        free(domain);
        return result;
    };
    //qDebug()<<QString(virConnectGetURI(currWorkConnect));

    int i = 0;
    bool deleted = false;
    while ( domain[i] != NULL ) {
        QString currDomName = QString( virDomainGetName(domain[i]) );
        if ( !deleted && currDomName==name ) {
            deleted = (virDomainUndefine(domain[i])+1) ? true : false;
            if (!deleted) sendGlobalErrors();
        };
        qDebug()<<QVariant(deleted).toString()<<currDomName<<name;
        virDomainFree(domain[i]);
        i++;
    };
    free(domain);
    result.append(QString("'%1' Domain %2 Undefined.").arg(name).arg((deleted)?"":"don't"));
    return result;
}
QStringList DomControlThread::changeAutoStartDomain()
{
    QStringList result;
    QString name = args.first();
    int autostart;
    if ( args.count()<2 || args.at(1).isEmpty() ) {
        result.append("Incorrect parameters.");
        return result;
    } else {
        bool converted;
        int res = args.at(1).toInt(&converted);
        if (converted) autostart = (res) ? 1 : 0;
        else {
            result.append("Incorrect parameters.");
            return result;
        };
    };
    virDomainPtr *domain;
    unsigned int flags = VIR_CONNECT_LIST_DOMAINS_ACTIVE |
                         VIR_CONNECT_LIST_DOMAINS_INACTIVE;
    int ret = virConnectListAllDomains( currWorkConnect, &domain, flags);
    if ( ret<0 ) {
        sendConnErrors();
        free(domain);
        return result;
    };
    //qDebug()<<QString(virConnectGetURI(currWorkConnect));

    int i = 0;
    bool set = false;
    while ( domain[i] != NULL ) {
        QString currNetName = QString( virDomainGetName(domain[i]) );
        if ( !set && currNetName==name ) {
            set = (virDomainSetAutostart(domain[i], autostart)+1) ? true : false;
            if (!set) sendGlobalErrors();
        };
        virDomainFree(domain[i]);
        i++;
    };
    free(domain);
    result.append(QString("'%1' Domain autostart %2 Set.").arg(name).arg((set)?"":"don't"));
    return result;
}
QStringList DomControlThread::getDomainXMLDesc()
{
    QStringList result;
    QString name = args.first();
    virDomainPtr *domain;
    unsigned int flags = VIR_CONNECT_LIST_DOMAINS_ACTIVE |
                         VIR_CONNECT_LIST_DOMAINS_INACTIVE;
    int ret = virConnectListAllDomains( currWorkConnect, &domain, flags);
    if ( ret<0 ) {
        sendConnErrors();
        free(domain);
        return result;
    };
    //qDebug()<<QString(virConnectGetURI(currWorkConnect));

    int i = 0;
    bool read = false;
    char *Returns = NULL;
    while ( domain[i] != NULL ) {
        QString currNetName = QString( virDomainGetName(domain[i]) );
        if ( !read && currNetName==name ) {
            Returns = (virDomainGetXMLDesc(domain[i], VIR_DOMAIN_XML_INACTIVE));
            if ( Returns==NULL ) sendGlobalErrors();
            else read = true;
        };
        virDomainFree(domain[i]);
        i++;
    };
    free(domain);
    QTemporaryFile f;
    f.setAutoRemove(false);
    read = f.open();
    if (read) f.write(Returns);
    result.append(f.fileName());
    f.close();
    free(Returns);
    result.append(QString("'%1' Domain %2 XML'ed").arg(name).arg((read)?"":"don't"));
    return result;
}

void DomControlThread::sendConnErrors()
{
    virtErrors = virConnGetLastError(currWorkConnect);
    if ( virtErrors!=NULL ) {
        emit errorMsg( QString("VirtError(%1) : %2").arg(virtErrors->code).arg(virtErrors->message) );
        virResetError(virtErrors);
    } else sendGlobalErrors();
}
void DomControlThread::sendGlobalErrors()
{
    virtErrors = virGetLastError();
    if ( virtErrors!=NULL )
        emit errorMsg( QString("VirtError(%1) : %2").arg(virtErrors->code).arg(virtErrors->message) );
    virResetLastError();
}