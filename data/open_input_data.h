#ifndef OPEN_INPUT_DATA_H
#define OPEN_INPUT_DATA_H

#include "type.h"
#include "device_request.h"

#include <QStringList>
#include <QString>

class open_input_data
{
public:
    device_request dev_request;    // 输入设备的设置要求
    QStringList urls;              // 输入媒体对象
public:
    open_input_data();

    void save(QDataStream &os) const;
    void load(QDataStream &is);
};

#endif // OPEN_INPUT_DATA_H
