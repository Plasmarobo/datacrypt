#ifndef __SIMULATOR_H__
#define __SIMULATOR_H__

#include <QtGui/QImage>
#include <QtQuick/QQuickPaintedItem>
#include <mutex>

#include "defs.h"

class DisplayView : public QQuickPaintedItem
{
    Q_OBJECT
    Q_PROPERTY(int height READ height WRITE setHeight NOTIFY heightChanged)
    Q_PROPERTY(int width READ width WRITE setWidth NOTIFY widthChanged)
    QML_ELEMENT

private:
    uint8_t framebuffer[128 * 64 / 8];
    std::mutex mutex;
    int _height;
    int _width;

public:
    explicit DisplayView(QQuickItem *parent = nullptr) : QQuickPaintedItem(parent)
    {
        _height = 64;
        _width = 128;
    }
    void paint(QPainter *painter) override;
    void readDisplay(buffer_t img, uint8_t width, uint8_t height);
    void writeDisplay(const buffer_t img, uint8_t width, uint8_t height);

    void heightChanged() {}
    int height() const { return _height; }
    void setHeight(int height) { _height = height; }

    void widthChanged() {}
    int width() const { return _width; }
    void setWidth(int width) { _width = width; }
signals:
    void flushBuffer();
};

class SimDisplays : public QObject
{
    Q_OBJECT
private:
    static const buffer_t power_on_pattern;

public:
    static void init()
    {
        displays[0] = nullptr;
        displays[1] = nullptr;
        displays[2] = nullptr;
        displays[3] = nullptr;
        displays[4] = nullptr;
        displays[5] = nullptr;
        displays[6] = nullptr;
        displays[7] = nullptr;
        textOutput = nullptr;
    }
    static DisplayView *getDisplay(int index)
    {
        if (index < 0 || index >= 8)
        {
            return nullptr;
        }
        return displays[index];
    }
    static void setDisplay(int index, DisplayView *display)
    {
        if (index < 0 || index >= 8)
        {
            return;
        }
        displays[index] = display;
        displays[index]->writeDisplay(power_on_pattern, 128, (index < 4) ? 64 : 32);
    }

    static void setTextOutput(QObject *textArea)
    {
        textOutput = textArea;
    }

    static void appendText(const char *text, int length)
    {
        // emit serialData(QString::fromUtf8(text, length));
        if (textOutput != nullptr)
        {
            //   QString currentText = textOutput->property("text").toString();
            //   currentText.append(QString::fromUtf8(text, length));
            //   textOutput->setProperty("text", currentText);
            QMetaObject::invokeMethod(textOutput, "append", Qt::QueuedConnection, Q_ARG(QString, QString::fromUtf8(text, length)));
        }
    }

private:
    static std::map<int, DisplayView *>
        displays;
    static QObject *textOutput;
};

#endif // __SIMULATOR_H__