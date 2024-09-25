#ifndef SLEEPSET_H
#define SLEEPSET_H

#include <QObject>
#include <QAbstractNativeEventFilter>
#include <QDebug>
#include <QThread>
#include <windows.h>
#include <powrprof.h>
#include <QTimer>

// Link with PowrProf.lib
#pragma comment(lib, "PowrProf.lib")
#pragma comment(lib, "User32.lib")

class SleepSet : public QObject
{
    Q_OBJECT

public:
    virtual ~SleepSet();

    void start();  // Start the event filtering and LED update loop
    void stop();   // Stop the event filtering and LED update loop

    static HPOWERNOTIFY m_globalRegistrationHandle; // Static member variable

private:
    HPOWERNOTIFY m_registrationHandle = nullptr;  // Initialize to nullptr
    static ULONG PowerCheck(PVOID Context, ULONG Type, PVOID Setting);
    void handleWakeAction();  // Function to turn off LEDs
    void handleSleepAction();
    void turnOffLEDs();
    void loadProfile(const QString& profileName);

signals:
    void logMessage(const QString &message);  // Signal to emit log messages
};

#endif // SLEEPSET_H
