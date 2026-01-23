#include "hal.h"
#include "defs.h"
#include "images.h"
#include "gpio.h"

#include <thread>
#include <mutex>
#include <iostream>
#include <map>
#include <condition_variable>
#include <chrono>

#include <QtQuick/QQuickView>
#include <QtGui/QImage>
#include <QtQuick/QQuickImageProvider>
#include <QtQml/QQmlApplicationEngine>
#include <QtWidgets/QApplication>
#include <QtCore/QTimer>
#include <QtQuick/QQuickPaintedItem>
#include <QtGui/QPainter>

#include "simulator.h"

SimulatorContext *SimulatorContext::_context = nullptr;

static std::thread *host_thread;
static std::mutex hal_mutex;
static std::atomic<bool> gui_ready = false;
static std::condition_variable hal_cond;

void DisplayView::paint(QPainter *painter)
{
    std::lock_guard<std::mutex> guard(this->mutex);
    QSize size = QSize(_width, _height);
    QImage image(_width, _height, QImage::Format_RGB888);
    for (int y = 0; y < _height; ++y)
    {
        for (int x = 0; x < _width; ++x)
        {
            uint16_t idx = (x + (y * _width)) / 8;
            uint8_t fragment = framebuffer[idx];
            uint8_t bit = 7 - (x % 8);

            auto foreground = Qt::white;
            auto background = Qt::black;

            if (fragment & (0x01 << bit))
            {
                image.setPixelColor(x, y, _inverted ? background : foreground);
            }
            else
            {
                image.setPixelColor(x, y, _inverted ? foreground : background);
            }
        }
    }

    painter->drawImage(0, 0, image);
}

void DisplayView::readDisplay(buffer_t img, uint8_t width, uint8_t height)
{
    std::lock_guard<std::mutex> guard(this->mutex);
    memcpy(img, this->framebuffer, (width * height) / 8);
}

void DisplayView::writeDisplay(const buffer_t img, uint8_t width, uint8_t height)
{
    memcpy(this->framebuffer, img, (width * height) / 8);
    QMetaObject::invokeMethod(this, "update", Qt::QueuedConnection);
    // emit flushBuffer();
}

const buffer_t SimulatorContext::power_on_pattern = (const buffer_t)&img_millibyte_alt_cropped[0];

void hal_worker()
{
    int argc = 0;
    char *argv[1] = {NULL};
    std::unique_lock<std::mutex> lock(hal_mutex);
    QGuiApplication app(argc, argv);
    SimulatorContext context;
    context.init();
    qmlRegisterType<DisplayView>("com.millibyte.displayview", 1, 0, "DisplayView");
    //  Using QQuickView
    QQuickView view;

    view.setSource(QUrl::fromLocalFile("ui/App.qml"));

    auto connectUI = [&context](QObject *dcX, int idx, QString btn_label, gpio_t *btn_gpio = nullptr, gpio_t *sw_gpio = nullptr)
    {
        if (dcX == nullptr)
        {
            return;
        }
        dcX->setProperty("index", idx);
        dcX->setProperty("btn_label", btn_label);
        DisplayView *ibX = dcX->findChild<DisplayView *>("image_b");
        context.setDisplay(idx, ibX);
        QObject::connect(ibX, &DisplayView::flushBuffer, [ibX]()
                         { QMetaObject::invokeMethod(ibX, "update", Qt::QueuedConnection); });
        DisplayView *isX = dcX->findChild<DisplayView *>("image_s");
        context.setDisplay(idx + 4, isX);
        QObject::connect(isX, &DisplayView::flushBuffer, [isX]()
                         { QMetaObject::invokeMethod(isX, "update", Qt::QueuedConnection); });
        auto btn_input = context.gpioInput(btn_gpio);
        QObject::connect(dcX, SIGNAL(pressed(bool)), btn_input, SLOT(setGPIO(bool)));
        auto sw_input = context.gpioInput(sw_gpio);
        QObject::connect(dcX, SIGNAL(switched(bool)), sw_input, SLOT(setGPIO(bool)));
    };

    QObject *object = reinterpret_cast<QObject *>(view.rootObject());
    QObject *dc0 = object->findChild<QObject *>("dc0");
    connectUI(dc0, 0, "A", &LEFT_SW, &LOCK0_TGL);
    QObject *dc1 = object->findChild<QObject *>("dc1");
    connectUI(dc1, 1, "B", &ACCEPT_SW, &LOCK1_TGL);
    QObject *dc2 = object->findChild<QObject *>("dc2");
    connectUI(dc2, 2, "C", &CANCEL_SW, &LOCK2_TGL);
    QObject *dc3 = object->findChild<QObject *>("dc3");
    connectUI(dc3, 3, "D", &RIGHT_SW, &LOCK3_TGL);
    QObject *txt = object->findChild<QObject *>("textOutput");
    context.setTextOutput(txt);
    view.show();
    gui_ready = true;
    hal_cond.notify_one();
    lock.unlock();
    app.exec();
}

static std::chrono::time_point<std::chrono::high_resolution_clock> boot_time;

// Early hardware setup
void hal_hw_init()
{
    boot_time = std::chrono::high_resolution_clock::now();
    std::unique_lock<std::mutex> lock(hal_mutex);
    host_thread = new std::thread(hal_worker);
    hal_cond.wait(lock, []
                  { return gui_ready.load(); });
    lock.unlock();
}

// Late hardware setup (post-scheduler)
void hal_task_init()
{
}

void hal_tick()
{
    std::this_thread::sleep_for(std::chrono::microseconds(1));
}

timespan_t microseconds() { return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - boot_time).count(); }
timespan_t milliseconds() { return microseconds() / 1000; }
void enter_critical() {}
void exit_critical() {}
