#ifndef _IPV4_H
#define _IPV4_H

#include "_ipvx.h"

class _IPv4 : public _IPvX
{
    Q_OBJECT
public:
    explicit _IPv4(
            QWidget *parent = NULL,
            bool    *hasDHCP = NULL);

private:

public slots:
    void             setStaticRouteMode(bool);
};

#endif // _IPV4_H
