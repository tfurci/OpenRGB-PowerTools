#include "SleepSet.h"
#include "PowerTools.h"
#include "ProfileManager.h"
#include "RGBController.h"

#include <windows.h>
#include <Wtsapi32.h>
#pragma comment(lib, "Wtsapi32.lib")


HPOWERNOTIFY SleepSet::m_globalRegistrationHandle = nullptr;
HWND SleepSet::m_hiddenWindow = nullptr; // Definition of the static hidden window handle


SleepSet::~SleepSet()
{
    start();
}

void SleepSet::start()
{
    registerpowrprof();
    createHiddenWindow();
}

void SleepSet::stop()
{
    unregisterpowrprof();
    unregisterWindowClass();
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

void SleepSet::createHiddenWindow()
{

    m_hiddenWindow = FindWindow(TEXT("PowerToolsWCN"), NULL);
    if (m_hiddenWindow)
    {
        // If the window is found, we assume it already exists
        qDebug() << "[PowerTools][SleepSet] Hidden window already exists.";

        // You can also re-register for session notifications if needed
        WTSRegisterSessionNotification(m_hiddenWindow, NOTIFY_FOR_ALL_SESSIONS);
        return; // Exit if the window already exists
    }

    // Register the window class
    WNDCLASS wc = {};
    wc.lpfnWndProc = SleepSet::WndProc; // Set the window procedure
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = TEXT("PowerToolsWCN");

    RegisterClass(&wc);

    // Create the hidden window
    m_hiddenWindow = CreateWindowEx(
        WS_EX_TOOLWINDOW, // Style: Tool window
        wc.lpszClassName,
        TEXT("PowerToolsWCN"), // Window name
        0, // No visible style
        0, 0, 0, 0, // No size or position
        NULL, // No parent
        NULL, // No menu
        wc.hInstance,
        this // Pass `this` as the window user data
        );

    if (m_hiddenWindow)
    {
        // Register for session notifications
        WTSRegisterSessionNotification(m_hiddenWindow, NOTIFY_FOR_ALL_SESSIONS);
        qDebug() << "[PowerTools][SleepSet] Hidden window created and registered for session notifications.";
    }
    else
    {
        qDebug() << "[PowerTools][SleepSet] Failed to create hidden window.";
    }
}

LRESULT CALLBACK SleepSet::WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    SleepSet* self = reinterpret_cast<SleepSet*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

    switch (uMsg)
    {
    case WM_WTSSESSION_CHANGE:
        if (wParam == WTS_SESSION_LOCK)
        {
            QString state = "locked";
            qDebug() << "[PowerTools][SleepSet] Workstation is" << state;
            self->handleLockAction(); // Separate lock action
        }
        else if (wParam == WTS_SESSION_UNLOCK)
        {
            QString state = "unlocked";
            qDebug() << "[PowerTools][SleepSet] Workstation is" << state;
            self->handleUnlockAction(); // Separate unlock action
        }
        break;
    case WM_DESTROY:
        WTSUnRegisterSessionNotification(hwnd); // Unregister session notifications
        break;
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

void SleepSet::unregisterWindowClass()
{
    if (m_hiddenWindow) // Check if the hidden window exists
    {
        qDebug() << "[PowerTools][SleepSet] Unregistering session notifications.";

        // Unregister WTS session notifications
        WTSUnRegisterSessionNotification(m_hiddenWindow);

        // Destroy the window
        DestroyWindow(m_hiddenWindow);
        m_hiddenWindow = nullptr; // Reset the handle to null
        qDebug() << "[PowerTools][SleepSet] Hidden window destroyed and notifications unregistered.";
    }
    else
    {
        qDebug() << "[PowerTools][SleepSet] No hidden window to unregister.";
    }
}
