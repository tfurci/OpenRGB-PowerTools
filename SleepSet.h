#ifndef SLEEPSET_H
#define SLEEPSET_H

#include <QObject>
#include <QAbstractNativeEventFilter>
#include <QDebug>
#include <windows.h>  // Include Windows-specific headers

class SleepSet : public QObject, public QAbstractNativeEventFilter
{
    Q_OBJECT

public:
    SleepSet(QObject *parent = nullptr);
    virtual ~SleepSet();

    void start();  // Start the event filtering and LED update loop
    void stop();   // Stop the event filtering and LED update loop

protected:
    bool nativeEventFilter(const QByteArray &eventType, void *message, long *result) override;

private:
    bool running;
    void handleWakeAction();  // Function to turn off LEDs
    void handleSleepAction();
    void turnOffLEDs();
    void loadProfile(const QString& profileName);

signals:
    void logMessage(const QString &message);  // Signal to emit log messages
};

#endif // SLEEPSET_H
