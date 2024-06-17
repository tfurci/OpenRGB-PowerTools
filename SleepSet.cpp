#include "SleepSet.h"
#include "PowerTools.h"
#include "ProfileManager.h"
#include "RGBController.h"
#include <QCoreApplication>

SleepSet::SleepSet(QObject *parent)
    : QObject(parent), running(false)
{
}

SleepSet::~SleepSet()
{
    stop();
}

void SleepSet::start()
{
    if (!running)
    {
        running = true;
        QCoreApplication::instance()->installNativeEventFilter(this); // Install native event filter
    }
}

void SleepSet::stop()
{
    if (running)
    {
        running = false;
        QCoreApplication::instance()->removeNativeEventFilter(this); // Remove native event filter
    }
}

QString readSettingFromQSettings(const QString& group, const QString& key)
{
    QString settingsFilePath = QDir::homePath() + "/AppData/Roaming/OpenRGB/powertools.ini";
    QSettings settings(settingsFilePath, QSettings::IniFormat);

    // Set the group context in QSettings
    settings.beginGroup(group);

    // Read the setting
    QString value = settings.value(key).toString();

    // End the group context
    settings.endGroup();

    return value;
}

bool SleepSet::nativeEventFilter(const QByteArray &eventType, void *message, long *result)
{
    Q_UNUSED(result);

    if (eventType == "windows_generic_MSG")
    {
        MSG* msg = static_cast<MSG*>(message);

        switch (msg->message)
        {
        case WM_POWERBROADCAST:
            switch (msg->wParam)
            {
            case PBT_APMSUSPEND:
                qDebug() << "[PowerTools] System is about to sleep (suspend).";
                handleSleepAction();
                break;
            case PBT_APMRESUMESUSPEND:
                qDebug() << "[PowerTools] System resumed from sleep (suspend).";
                handleWakeAction();
                break;
            }
            break;
        }
    }

    return false;  // Return false to allow normal event processing
}

void SleepSet::handleSleepAction()
{
    if (!PowerTools::RMPointer) {
        qDebug() << "ResourceManager pointer is null.";
        return;
    }

    QString action = readSettingFromQSettings("Sleep", "Action");
    printf("Sleep Action: %s\n", action.toUtf8().constData());

    if (action == "0")
    {
        turnOffLEDs();
    }
    else if (action == "1")
    {
        QString profileName = readSettingFromQSettings("Sleep", "Profile");
        loadProfile(profileName);
    }
    else
    {
        printf("[PowerTools] No valid action for sleep.");
    }
}

void SleepSet::handleWakeAction()
{
    if (!PowerTools::RMPointer) {
        qDebug() << "ResourceManager pointer is null.";
        return;
    }

    QString action = readSettingFromQSettings("ReturnFromSleep", "Action");

    if (action == "0")
    {
        turnOffLEDs();
    }
    else if (action == "1")
    {
        QString profileName = readSettingFromQSettings("ReturnFromSleep", "Profile");
        loadProfile(profileName);
    }
    else
    {
        qDebug() << "[PowerTools] No valid action for wake.";
    }
}

void SleepSet::turnOffLEDs()
{
    if (!PowerTools::RMPointer) {
        qDebug() << "ResourceManager pointer is null.";
        return;
    }

    for (RGBController* controller : PowerTools::RMPointer->GetRGBControllers())
    {
        controller->SetAllLEDs(ToRGBColor(0, 0, 0));  // Set LEDs to off
        controller->UpdateLEDs();  // Update LEDs
    }
}

void SleepSet::loadProfile(const QString& profileName)
{
    if (!PowerTools::RMPointer) {
        qDebug() << "ResourceManager pointer is null.";
        return;
    }

    ProfileManager* profileManager = PowerTools::RMPointer->GetProfileManager();
    if (profileManager)
    {
        profileManager->LoadProfile(profileName.toStdString());

        // Update LEDs for each RGB controller
        for (RGBController* controller : PowerTools::RMPointer->GetRGBControllers())
        {
            controller->UpdateLEDs();
            qDebug() << "[PowerTools] Updating LEDs.";
        }

        qDebug() << "[PowerTools] Loaded profile" << profileName << ".";
    }
}
