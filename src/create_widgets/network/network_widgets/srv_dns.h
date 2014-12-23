#ifndef SRV_DNS_H
#define SRV_DNS_H

#include "_list_widget.h"

class SRV_DNS : public _List_Widget
{
    Q_OBJECT
public:
    explicit SRV_DNS(
            QWidget *parent = NULL,
            QString  tag = "SRVs");

private:
    QLabel          *serviceL, *domainL, *targetL,
                    *portL, *priorityL, *weightL,
                    *protocolL;
    QLineEdit       *service, *domain, *target;
    QSpinBox        *port, *priority, *weight;
    QComboBox       *protocol;
    QGridLayout     *setsLayout;
    QWidget         *sets;

public slots:
    QDomDocument     getDataDocument() const;
    void             addItem();
};

#endif // SRV_DNS_H
