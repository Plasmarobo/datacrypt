#ifndef __SIMULATOR_H__
#define __SIMULATOR_H__

#include <QtQuick/QQuickImageProvider>
#include <QtGui/QImage>
#include <mutex>

#include "defs.h"

class SimulatorImageProvider : public QQuickImageProvider
{
private:
    QImage buffer[8];
    std::mutex mutex[8];

    static SimulatorImageProvider *instance;

public:
    SimulatorImageProvider();
    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize);
    void readDisplay(int index, buffer_t img, uint8_t width, uint8_t height);
    void writeDisplay(int index, const buffer_t img, uint8_t width, uint8_t height);
    static SimulatorImageProvider *getInstance();
};

#endif // __SIMULATOR_H__