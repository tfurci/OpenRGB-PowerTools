#include "SleepSet.h"
#include "PowerTools.h"
#include "ProfileManager.h"
#include "RGBController.h"

SleepSet::SleepSet(QObject *parent)
    {
    m_registrationHandle = 0;
}

SleepSet::~SleepSet()
{
    running = false;
    qDebug() << "[SleepSet] Start called";
    start();
}

HWND SleepSet::s_notifyHwnd = nullptr;

ULONG SleepSet::PowerCheck(PVOID Context, ULONG Type, PVOID Setting)
{
    SleepSet* self = reinterpret_cast<SleepSet*>(Context);
    QString debugMessage = QString("[PowerTools] PChanged to type %1").arg(Type);
    qDebug() << debugMessage;

    switch (Type)
    {
    case PBT_APMSUSPEND:
        qDebug() << "[PowerTools] System is entering sleep (APMSUSPEND)";
        self->handleSleepAction();
        break;
    case PBT_APMRESUMEAUTOMATIC:
        qDebug() << "[PowerTools] System has resumed from sleep (APMRESUMEAUTOMATIC)";
        self->handleWakeAction();
        break;
    default:
        qDebug() << "[PowerTools] Unhandled power event" << Type;
        break;
    }

    return 0;
}

void SleepSet::start()
{
    if (!running)
    {
        running = true;
        qDebug() << "[SleepSet] Starting event filter";

        HMODULE powrprof = LoadLibrary(TEXT("powrprof.dll"));
        if (powrprof != NULL)
        {
            DWORD (_stdcall *PowerRegisterSuspendResumeNotification)(DWORD, HANDLE, PHPOWERNOTIFY);
            PowerRegisterSuspendResumeNotification = (DWORD(_stdcall *)(DWORD, HANDLE, PHPOWERNOTIFY))GetProcAddress(powrprof, "PowerRegisterSuspendResumeNotification");
            if (PowerRegisterSuspendResumeNotification)
            {
                static DEVICE_NOTIFY_SUBSCRIBE_PARAMETERS powerParams = { PowerCheck, this };
                s_notifyHwnd = GetConsoleWindow(); // Using the console window for demonstration purposes
                PowerRegisterSuspendResumeNotification(DEVICE_NOTIFY_CALLBACK, &powerParams, &m_registrationHandle);
                qDebug() << "[SleepSet] Successfully registered power setting notification.";
            }
            else
            {
                qDebug() << "[SleepSet] Failed to register power setting notification.";
            }
            FreeLibrary(powrprof);
        }
        else
        {
            qDebug() << "[SleepSet] Failed to load powrprof.dll.";
        }
    }
}

void SleepSet::stop()
{
    if (running)
    {
        running = false;

        if (m_registrationHandle)
        {
            HMODULE powrprof = LoadLibrary(TEXT("powrprof.dll"));
            if (powrprof != NULL)
            {
                DWORD (_stdcall *PowerUnregisterSuspendResumeNotification)(PHPOWERNOTIFY);
                PowerUnregisterSuspendResumeNotification = (DWORD(_stdcall *)(PHPOWERNOTIFY))GetProcAddress(powrprof, "PowerUnregisterSuspendResumeNotification");
                if (PowerUnregisterSuspendResumeNotification)
                {
                    PowerUnregisterSuspendResumeNotification(&m_registrationHandle);
                    qDebug() << "[SleepSet] Successfully unregistered power setting notification.";
                }
                FreeLibrary(powrprof);
            }
        }
    }
}

bool SleepSet::nativeEventFilter(const QByteArray &eventType, void *message, long *result)
{
    Q_UNUSED(eventType);
    Q_UNUSED(message);
    Q_UNUSED(result);

    // This method is now redundant as we're using RegisterPowerSettingNotification
    return false;
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

void SleepSet::handleSleepAction()
{
    if (!PowerTools::RMPointer) {
        qDebug() << "[PowerTools] ResourceManager pointer is null.";
        return;
    }

    QString action = readSettingFromQSettings("Sleep", "Action");
    if (action == "0")
    {
        qDebug() << "[PowerTools] Action: Turn off LEDs";
        turnOffLEDs();
    }
    else if (action == "1")
    {
        QString profileName = readSettingFromQSettings("Sleep", "Profile");
        qDebug() << "[PowerTools] Action: Load profile" << profileName;
        loadProfile(profileName);
    }
    else
    {
        qDebug() << "[PowerTools] No valid action for sleep.";
    }
}

void SleepSet::handleWakeAction()
{
    if (!PowerTools::RMPointer) {
        qDebug() << "[PowerTools] ResourceManager pointer is null.";
        return;
    }

    QString action = readSettingFromQSettings("ReturnFromSleep", "Action");

    if (action == "0")
    {
        qDebug() << "[PowerTools] Action: Turn off LEDs";
        turnOffLEDs();
    }
    else if (action == "1")
    {
        QString profileName = readSettingFromQSettings("ReturnFromSleep", "Profile");
        qDebug() << "[PowerTools] Action: Load profile" << profileName;
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
        qDebug() << "[PowerTools] ResourceManager pointer is null.";
        return;
    }

    for (RGBController* controller : PowerTools::RMPointer->GetRGBControllers())
    {
        controller->SetAllLEDs(ToRGBColor(0, 0, 0));  // Set LEDs to off
        controller->UpdateLEDs();  // Update LEDs
        controller->UpdateLEDs();  // Update LEDs
    }
}

void SleepSet::loadProfile(const QString& profileName)
{
    if (!PowerTools::RMPointer) {
        qDebug() << "[PowerTools] ResourceManager pointer is null.";
        return;
    }

    ProfileManager* profileManager = PowerTools::RMPointer->GetProfileManager();
    if (profileManager)
    {
        profileManager->LoadProfile(profileName.toStdString());

        // Update LEDs for each RGB controller
        for (RGBController* controller : PowerTools::RMPointer->GetRGBControllers())
        {
            controller->UpdateLEDs();  // Update LEDs
            QThread::msleep(10);  // Delay for 10 milliseconds
            controller->UpdateLEDs();  // Update LEDs
        }

        qDebug() << "[PowerTools] Loaded profile" << profileName << ".";
    }
}
