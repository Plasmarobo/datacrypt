#include "hal.h"
#include "defs.h"

#include <thread>
#include <mutex>
#include <QtQuick/QQuickView>
#include <QtGui/QImage>
#include <QtQuick/QQuickImageProvider>
#include <QtWidgets/QApplication>

#include "simulator.h"

static std::thread *host_thread;

#define QBUFFER(i) QImage(128, i < 4 ? 64 : 32, QImage::Format_Mono)

SimulatorImageProvider *SimulatorImageProvider::instance = NULL;

SimulatorImageProvider::SimulatorImageProvider() : QQuickImageProvider(QQuickImageProvider::Image)
{
    if (instance)
    {
        return;
    }
    instance = this;
    for (int i = 0; i < 8; i++)
    {
        buffer[i] = QBUFFER(i);
        buffer[i].fill(QColor(0, 0, 0));
    }
}

QImage SimulatorImageProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
    UNUSED(requestedSize);
    QImage image;
    QString big_prefix = "image://SimulatorImageProvider/display_big_";
    QString small_prefix = "image://SimulatorImageProvider/display_small_";
    int index = -1;

    if (id.startsWith(big_prefix))
    {
        index = id.mid(big_prefix.length()).toInt();
    }
    else if (id.startsWith(small_prefix))
    {
        index = id.mid(small_prefix.length()).toInt();
    }

    if (index >= 0 && index < 8)
    {
        std::lock_guard<std::mutex> guard(this->mutex[index]);
        image = this->buffer[index];
    }
    else
    {
        return QBUFFER(0);
    }

    if (size)
    {
        *size = image.size();
    }

    return image;
}

void SimulatorImageProvider::readDisplay(int index, buffer_t img, uint8_t width, uint8_t height)
{
    if (index < 0 || index >= 8)
    {
        return;
    }
    std::lock_guard<std::mutex> guard(this->mutex[index]);
    memcpy(img, this->buffer[index].bits(), width * height);
}

void SimulatorImageProvider::writeDisplay(int index, const buffer_t img, uint8_t width, uint8_t height)
{
    if (index < 0 || index >= 8)
    {
        return;
    }
    std::lock_guard<std::mutex> guard(this->mutex[index]);
    this->buffer[index] = QImage(img, width, height, QImage::Format_Mono);
    emit imageChanged();
}

SimulatorImageProvider *SimulatorImageProvider::getInstance()
{
    return instance;
}

void hal_worker()
{
    int argc = 0;
    char *argv[1] = {NULL};
    QApplication app(argc, argv);
    qmlRegisterType<SimulatorImageProvider>("SimulatorImageProvider", 1, 0, "SimulatorImageProvider");

    // Using QQuickView
    QQuickView view;
    QQmlEngine *engine = view.engine();
    engine->addImageProvider(QLatin1String("SimulatorImageProvider"), SimulatorImageProvider::getInstance());
    view.setSource(QUrl::fromLocalFile("ui/SimulatorUI/SimulatorUIContent/App.qml"));
    view.show();
    // QQuickItem *object = view.rootObject();

    app.exec();
}

// Early hardware setup
void hal_hw_init()
{
    host_thread = new std::thread(hal_worker);
}

// Late hardware setup (post-scheduler)
void hal_task_init()
{
}

static timespan_t _us = 0;
void tick(timespan_t us) { _us += us; }
timespan_t microseconds() { return _us; }
timespan_t milliseconds() { return _us / 1000; }
void enter_critical() {}
void exit_critical() {}