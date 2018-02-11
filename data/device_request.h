#ifndef DEVICE_REQUEST_H
#define DEVICE_REQUEST_H
#include "type.h"
#include <QDataStream>
class device_request
{
public:
     Device device;      // 设备类型
     int width;          // 在指定宽度中返回视频帧(0为默认)
     int height;         // 在指定高度中返回视频帧(0为默认)
     int frame_rate_num; // 在指定帧速率中返回视频帧(0为默认)
     int frame_rate_den; // For example 1/25, 1/30, ...
     bool request_mjpeg; // Request MJPEG format from device

     device_request();

     // 是否为有效的请求设备
     bool is_device() const
     {
         return device !=DEVICE_NO;
     }

     // 序列化
     void save(QDataStream &os) const;
     void load(QDataStream &is);
};

#endif // DEVICE_REQUEST_H
