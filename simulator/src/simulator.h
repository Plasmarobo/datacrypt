#ifndef __SIMULATOR_H__
#define __SIMULATOR_H__

#include <QtQuick/QQuickImageProvider>
#include <QtGui/QImage>
#include <mutex>

#include "defs.h"

class SimDisplay : public QQuickImageProvider
{
    Q_OBJECT
signals:
    void imageChanged();

private:
    QImage buffer[8];
    std::mutex mutex[8];

    static SimDisplay *instance;

public:
    SimDisplay();
    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize);
    void readDisplay(int index, buffer_t img, uint8_t width, uint8_t height);
    void writeDisplay(int index, const buffer_t img, uint8_t width, uint8_t height);
    static SimDisplay *getInstance();
};

#endif // __SIMULATOR_H__