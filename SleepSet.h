#ifndef SLEEPSET_H
#define SLEEPSET_H

#include <QObject>
#include <QAbstractNativeEventFilter>
#include <QDebug>
#include <QThread>
#include <windows.h>
#include <powrprof.h>

// Link with PowrProf.lib
#pragma comment(lib, "PowrProf.lib")
#pragma comment(lib, "User32.lib")

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
    HPOWERNOTIFY hPowerNotify;
    HWND hWnd;
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void handleWakeAction();  // Function to turn off LEDs
    void handleSleepAction();
    void turnOffLEDs();
    void loadProfile(const QString& profileName);

signals:
    void logMessage(const QString &message);  // Signal to emit log messages
};

#endif // SLEEPSET_H
