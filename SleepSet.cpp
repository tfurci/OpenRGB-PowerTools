#include "SleepSet.h"
#include "PowerTools.h"
#include "ProfileManager.h"
#include "RGBController.h"
HPOWERNOTIFY SleepSet::m_globalRegistrationHandle = nullptr;

SleepSet::~SleepSet()
{
    start();
}

ULONG SleepSet::PowerCheck(PVOID Context, ULONG Type, PVOID Setting)
{
    SleepSet* self = reinterpret_cast<SleepSet*>(Context);
    QString debugMessage = QString("[PowerTools][SleepSet] PChanged to type %1").arg(Type);
    qDebug() << debugMessage;

    switch (Type)
    {
    case PBT_APMSUSPEND:
        qDebug() << "[PowerTools][SleepSet] System is entering sleep (APMSUSPEND)";
        self->handleSleepAction();
        break;
    case PBT_APMRESUMEAUTOMATIC:
        qDebug() << "[PowerTools][SleepSet] System has resumed from sleep (APMRESUMEAUTOMATIC)";
        self->handleWakeAction();
        break;
    default:
        qDebug() << "[PowerTools][SleepSet] Unhandled power event" << Type;
        break;
    }

    return 0;
}

void SleepSet::start()
{
    if (!m_globalRegistrationHandle) // Check if the registration handle is null
    {
        qDebug() << "[PowerTools][SleepSet] Starting PowerTools";

        HMODULE powrprof = LoadLibrary(TEXT("powrprof.dll"));
        if (powrprof != NULL)
        {
            DWORD (_stdcall *PowerRegisterSuspendResumeNotification)(DWORD, HANDLE, PHPOWERNOTIFY);
            PowerRegisterSuspendResumeNotification = (DWORD(_stdcall *)(DWORD, HANDLE, PHPOWERNOTIFY))GetProcAddress(powrprof, "PowerRegisterSuspendResumeNotification");
            if (PowerRegisterSuspendResumeNotification)
            {
                static DEVICE_NOTIFY_SUBSCRIBE_PARAMETERS powerParams = { PowerCheck, this };
                DWORD result = PowerRegisterSuspendResumeNotification(DEVICE_NOTIFY_CALLBACK, &powerParams, &m_registrationHandle);
                qDebug() << "[PowerTools][SleepSet] PowerRegisterSuspendResumeNotification result:" << result;

                if (m_registrationHandle)
                {
                    qDebug() << "[PowerTools][SleepSet] Successfully registered power setting notification. Handle:" << m_registrationHandle;
                    m_globalRegistrationHandle = m_registrationHandle;

                }
                else
                {
                    qDebug() << "[PowerTools][SleepSet] Failed to get a valid registration handle.";
                }
            }
            else
            {
                qDebug() << "[PowerTools][SleepSet] Failed to register power setting notification.";
            }
            FreeLibrary(powrprof);
        }
        else
        {
            qDebug() << "[PowerTools][SleepSet] Failed to load powrprof.dll.";
        }
    }
    else
    {
        qDebug() << "[PowerTools][SleepSet] Already registered power setting notification.";
    }
}

void SleepSet::stop()
{
    qDebug() << "[PowerTools][SleepSet] Handle:" << m_globalRegistrationHandle;
    if (m_globalRegistrationHandle) // Check if the registration handle is valid
    {
        qDebug() << "[PowerTools][SleepSet] Unregistering with handle:" << m_globalRegistrationHandle;

        HMODULE powrprof = LoadLibrary(TEXT("powrprof.dll"));
        if (powrprof != NULL)
        {
            DWORD (_stdcall *PowerUnregisterSuspendResumeNotification)(HPOWERNOTIFY);
            PowerUnregisterSuspendResumeNotification = (DWORD(_stdcall *)(HPOWERNOTIFY))GetProcAddress(powrprof, "PowerUnregisterSuspendResumeNotification");
            if (PowerUnregisterSuspendResumeNotification)
            {
                DWORD result = PowerUnregisterSuspendResumeNotification(m_globalRegistrationHandle); // Pass the handle directly
                qDebug() << "[PowerTools][SleepSet] PowerUnregisterSuspendResumeNotification result:" << result;

                if (result == 0)
                {
                    m_globalRegistrationHandle = nullptr; // Reset the handle to null
                    qDebug() << "[PowerTools][SleepSet] Successfully unregistered power setting notification.";
                }
                else
                {
                    qDebug() << "[PowerTools][SleepSet] Failed to unregister power setting notification. Error:" << result;
                }
            }
            else
            {
                qDebug() << "[PowerTools][SleepSet] Failed to find PowerUnregisterSuspendResumeNotification.";
            }
            FreeLibrary(powrprof);
        }
        else
        {
            qDebug() << "[PowerTools][SleepSet] Failed to load powrprof.dll for unregistration.";
        }
    }
    else
    {
        qDebug() << "[PowerTools][SleepSet] No power setting notification to unregister.";
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

void SleepSet::handleSleepAction()
{
    if (!PowerTools::RMPointer) {
        qDebug() << "[PowerTools][SleepSet] ResourceManager pointer is null.";
        return;
    }

    QString enabled = readSettingFromQSettings("Sleep", "Enabled");
    if (enabled == "false")
    {
        return;
    }

    QString action = readSettingFromQSettings("Sleep", "Action");
    if (action == "0")
    {
        qDebug() << "[PowerTools][SleepSet] Action: Turn off LEDs";
        turnOffLEDs();
    }
    else if (action == "1")
    {
        QString profileName = readSettingFromQSettings("Sleep", "Profile");
        qDebug() << "[PowerTools][SleepSet] Action: Load profile" << profileName;
        loadProfile(profileName);
    }
    else
    {
        qDebug() << "[PowerTools][SleepSet] No valid action for sleep.";
    }
}

void SleepSet::handleWakeAction()
{
    if (!PowerTools::RMPointer) {
        qDebug() << "[PowerTools][SleepSet] ResourceManager pointer is null.";
        return;
    }

    QString enabled = readSettingFromQSettings("ReturnFromSleep", "Enabled");
    if (enabled == "false")
    {
        return;
    }

    QString action = readSettingFromQSettings("ReturnFromSleep", "Action");
    if (action == "0")
    {
        qDebug() << "[PowerTools][SleepSet] Action: Turn off LEDs";
        turnOffLEDs();
    }
    else if (action == "1")
    {
        QString profileName = readSettingFromQSettings("ReturnFromSleep", "Profile");
        qDebug() << "[PowerTools][SleepSet] Action: Load profile" << profileName;
        loadProfile(profileName);
    }
    else
    {
        qDebug() << "[PowerTools][SleepSet] No valid action for wake.";
    }
}

void SleepSet::turnOffLEDs()
{
    if (!PowerTools::RMPointer) {
        qDebug() << "[PowerTools][SleepSet] ResourceManager pointer is null.";
        return;
    }

    for (RGBController* controller : PowerTools::RMPointer->GetRGBControllers())
    {
        controller->SetAllLEDs(ToRGBColor(0, 0, 0));  // Set LEDs to off
        controller->UpdateLEDs();  // Update LEDs

        std::this_thread::sleep_for(std::chrono::milliseconds(250));
        controller->UpdateLEDs();
    }
}

void SleepSet::loadProfile(const QString& profileName)
{
    if (!PowerTools::RMPointer) {
        qDebug() << "[PowerTools][SleepSet] ResourceManager pointer is null.";
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

            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            controller->UpdateLEDs();
            std::this_thread::sleep_for(std::chrono::milliseconds(250));
            controller->UpdateLEDs();
        }

        qDebug() << "[PowerTools][SleepSet] Loaded profile" << profileName << ".";
    }
}
