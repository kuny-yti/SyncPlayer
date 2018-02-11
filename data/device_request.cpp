#include "device_request.h"

device_request::device_request() :
    device(DEVICE_NO),
    width(0),
    height(0),
    frame_rate_num(0),
    frame_rate_den(0),
    request_mjpeg(false)
{
}

void device_request::save(QDataStream &os) const
{
    os << static_cast<int>(device);
    os <<width;
    os <<height;
    os <<frame_rate_num;
    os <<frame_rate_den;
    os <<request_mjpeg;
}

void device_request::load(QDataStream &is)
{
    int x;
    is >>  x;
    device = static_cast<Device>(x);
    is >> width;
    is >> height;
    is >> frame_rate_num;
    is >> frame_rate_den;
    is >> request_mjpeg;
}
