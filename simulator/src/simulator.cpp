#include "hal.h"
#include "defs.h"
#include "images.h"

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

static std::thread *host_thread;
static std::mutex hal_mutex;
static std::atomic<bool> gui_ready = false;
static std::condition_variable hal_cond;
std::map<int, DisplayView *> SimDisplays::displays;
QObject *SimDisplays::textOutput = nullptr;

void DisplayView::paint(QPainter *painter)
{
    std::lock_guard<std::mutex> guard(this->mutex);
    QSize size = QSize(_width, _height);
    QImage image(_width, _height, QImage::Format_RGB888);
    for (int y = 0; y < _height; ++y)
    {
        for (int x = 0; x < _width; ++x)
        {
            uint8_t fragment = framebuffer[(x + (y * _width)) / 8];

            if (fragment & (0x01 << (7 - (x % 8))))
            {
                image.setPixelColor(x, y, Qt::white);
            }
            else
            {
                image.setPixelColor(x, y, Qt::black);
            }
        }
    }

    painter->drawImage(0, 0, image);
    /*
    QBitmap bitmap = QBitmap::fromData(size, reinterpret_cast<const uchar *>(this->framebuffer));
    painter->drawPixmap(0, 0, _width, _height, bitmap);
    */
    }

    void DisplayView::readDisplay(buffer_t img, uint8_t width, uint8_t height)
    {
        std::lock_guard<std::mutex> guard(this->mutex);
        memcpy(img, this->framebuffer, (width * height) / 8);
    }

    void DisplayView::writeDisplay(const buffer_t img, uint8_t width, uint8_t height)
    {
        memcpy(this->framebuffer, img, (width * height) / 8);
        // QMetaObject::invokeMethod(this, "update", Qt::QueuedConnection);
        // emit flushBuffer();
    }

    const buffer_t SimDisplays::power_on_pattern = (const buffer_t)&img_millibyte_alt_cropped[0];

    void hal_worker()
    {
        int argc = 0;
        char *argv[1] = {NULL};
        std::unique_lock<std::mutex> lock(hal_mutex);
        QGuiApplication app(argc, argv);
        qmlRegisterType<DisplayView>("com.millibyte.displayview", 1, 0, "DisplayView");
        //  Using QQuickView
        QQuickView view;

        view.setSource(QUrl::fromLocalFile("ui/App.qml"));

        QObject *object = reinterpret_cast<QObject *>(view.rootObject());
        QObject *dc0 = object->findChild<QObject *>("dc0");
        if (dc0 != nullptr)
        {
            dc0->setProperty("index", 0);
            dc0->setProperty("btn_label", "A");
            DisplayView *ib0 = dc0->findChild<DisplayView *>("image_b0");
            SimDisplays::setDisplay(0, ib0);
            QObject::connect(ib0, &DisplayView::flushBuffer, [ib0]()
                             { ib0->update(); });
            DisplayView *is0 = dc0->findChild<DisplayView *>("image_s0");
            SimDisplays::setDisplay(1, is0);
            QObject::connect(is0, &DisplayView::flushBuffer, [is0]()
                             { is0->update(); });
        }
        QObject *dc1 = object->findChild<QObject *>("dc1");
        if (dc1 != nullptr)
        {
            dc1->setProperty("index", 1);
            dc1->setProperty("btn_label", "B");
            DisplayView *ib1 = dc1->findChild<DisplayView *>("image_b1");
            SimDisplays::setDisplay(2, ib1);
            QObject::connect(ib1, &DisplayView::flushBuffer, [ib1]()
                             { ib1->update(); });
            DisplayView *is1 = dc1->findChild<DisplayView *>("image_s1");
            SimDisplays::setDisplay(3, is1);
            QObject::connect(is1, &DisplayView::flushBuffer, [is1]()
                             { is1->update(); });
        }
        QObject *dc2 = object->findChild<QObject *>("dc2");
        if (dc2 != nullptr)
        {
            dc2->setProperty("index", 2);
            dc2->setProperty("btn_label", "C");
            DisplayView *ib2 = dc2->findChild<DisplayView *>("image_b2");
            SimDisplays::setDisplay(4, ib2);
            QObject::connect(ib2, &DisplayView::flushBuffer, [ib2]()
                             { ib2->update(); });
            DisplayView *is2 = dc2->findChild<DisplayView *>("image_s2");
            SimDisplays::setDisplay(5, is2);
            QObject::connect(is2, &DisplayView::flushBuffer, [is2]()
                             { is2->update(); });
        }
        QObject *dc3 = object->findChild<QObject *>("dc3");
        if (dc3 != nullptr)
        {
            dc3->setProperty("index", 3);
            dc3->setProperty("btn_label", "D");
            DisplayView *ib3 = dc3->findChild<DisplayView *>("image_b3");
            SimDisplays::setDisplay(6, ib3);
            QObject::connect(ib3, &DisplayView::flushBuffer, [ib3]()
                             { ib3->update(); });
            DisplayView *is3 = dc3->findChild<DisplayView *>("image_s3");
            SimDisplays::setDisplay(7, is3);
            QObject::connect(is3, &DisplayView::flushBuffer, [is3]()
                             { is3->update(); });
        }
        QObject *txt = object->findChild<QObject *>("textOutput");
        SimDisplays::setTextOutput(txt);
        view.show();
        gui_ready = true;
        hal_cond.notify_one();
        lock.unlock();
        app.exec();
    }

// Early hardware setup
void hal_hw_init()
{
    SimDisplays::init();
    host_thread = new std::thread(hal_worker);
}

// Late hardware setup (post-scheduler)
void hal_task_init()
{
    std::unique_lock<std::mutex> lock(hal_mutex);
    hal_cond.wait(lock, []
                  { return gui_ready.load(); });
    lock.unlock();
}

timespan_t microseconds() { return std::chrono::high_resolution_clock::now().time_since_epoch().count(); }
timespan_t milliseconds() { return microseconds() / 1000; }
void enter_critical() {}
void exit_critical() {}