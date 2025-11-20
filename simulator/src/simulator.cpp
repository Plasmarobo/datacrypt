#include "hal.h"
#include "defs.h"

#include <thread>
#include <mutex>
#include <QtQuick/QQuickView>
#include <QtGui/QImage>
#include <QtQuick/QQuickImageProvider>
#include <QtWidgets/QApplication>
#include <QtCore/QTimer>

#include "simulator.h"

static std::thread *host_thread;

#define QBUFFER(i) QImage(128, i < 4 ? 64 : 32, QImage::Format_Mono)

SimDisplay *SimDisplay::instance = NULL;

SimDisplay::SimDisplay() : QQuickImageProvider(QQuickImageProvider::Image)
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

QImage SimDisplay::requestImage(QString const &id, QSize *size, QSize const &requestedSize)
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

void SimDisplay::readDisplay(int index, buffer_t img, uint8_t width, uint8_t height)
{
    if (index < 0 || index >= 8)
    {
        return;
    }
    std::lock_guard<std::mutex> guard(this->mutex[index]);
    memcpy(img, this->buffer[index].bits(), width * height);
}

void SimDisplay::writeDisplay(int index, const buffer_t img, uint8_t width, uint8_t height)
{
    if (index < 0 || index >= 8)
    {
        return;
    }
    std::lock_guard<std::mutex> guard(this->mutex[index]);
    this->buffer[index] = QImage(img, width, height, QImage::Format_Mono);
    emit imageChanged();
}

SimDisplay *SimDisplay::getInstance()
{
    return instance;
}

void hal_worker()
{
    int argc = 0;
    char *argv[1] = {NULL};
    QApplication app(argc, argv);

    // Using QQuickView
    QQuickView view;
    QQmlEngine *engine = view.engine();
    engine->addImageProvider(QLatin1String("SimDisplay"), SimDisplay::getInstance());
    engine->addImportPath("qrc:/ui");
    view.setSource(QUrl::fromLocalFile("ui/App.qml"));

    QObject *object = reinterpret_cast<QObject *>(view.rootObject());
    object->findChild<QObject *>("dc0")->setProperty("index", 0);
    object->findChild<QObject *>("dc0")->setProperty("btn_label", "A");
    object->findChild<QObject *>("dc1")->setProperty("index", 1);
    object->findChild<QObject *>("dc1")->setProperty("btn_label", "B");
    object->findChild<QObject *>("dc2")->setProperty("index", 2);
    object->findChild<QObject *>("dc2")->setProperty("btn_label", "C");
    object->findChild<QObject *>("dc3")->setProperty("index", 3);
    object->findChild<QObject *>("dc3")->setProperty("btn_label", "D");
    view.show();

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