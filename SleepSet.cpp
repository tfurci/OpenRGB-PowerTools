#include "SleepSet.h"
#include "PowerTools.h"
#include "ProfileManager.h"
#include "RGBController.h"
HPOWERNOTIFY SleepSet::m_globalRegistrationHandle = nullptr;

SleepSet::~SleepSet()
{
    start();
}

void SleepSet::start()
{
    registerpowrprof();
}

void SleepSet::stop()
{
    unregisterpowrprof();
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

void SleepSet::registerpowrprof()
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

void SleepSet::unregisterpowrprof()
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
