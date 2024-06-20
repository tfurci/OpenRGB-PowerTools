#include "SleepSet.h"
#include "PowerTools.h"
#include "ProfileManager.h"
#include "RGBController.h"

SleepSet::SleepSet(QObject *parent)
    : QObject(parent), running(false), hPowerNotify(nullptr), hWnd(nullptr)
{
    qDebug() << "[SleepSet] Constructor called";
    start();
}

SleepSet::~SleepSet()
{
    qDebug() << "[SleepSet] Destructor called";
    //stop();
}

LRESULT CALLBACK SleepSet::WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == WM_POWERBROADCAST)
    {
        if (wParam == PBT_POWERSETTINGCHANGE)
        {
            POWERBROADCAST_SETTING* pbs = reinterpret_cast<POWERBROADCAST_SETTING*>(lParam);
            SleepSet* self = reinterpret_cast<SleepSet*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

            if (pbs->PowerSetting == GUID_CONSOLE_DISPLAY_STATE)
            {
                DWORD value = *reinterpret_cast<DWORD*>(pbs->Data);
                if (value == 0)
                {
                    // Display off (sleep)
                    qDebug() << "[PowerSettingCallback] Display off (sleep)";
                    self->handleSleepAction();
                }
                else if (value == 1)
                {
                    // Display on (resume from sleep)
                    qDebug() << "[PowerSettingCallback] Display on (resume from sleep)";
                    self->handleWakeAction();
                }
            }
        }
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void SleepSet::start()
{
    if (!running)
    {
        running = true;
        qDebug() << "[SleepSet] Starting event filter";

        // Register window class
        WNDCLASS wc = {0};
        wc.lpfnWndProc = WndProc;
        wc.hInstance = GetModuleHandle(nullptr);
        wc.lpszClassName = TEXT("SleepSetWindowClass");

        if (!RegisterClass(&wc))
        {
            qDebug() << "Failed to register window class. Error:" << GetLastError();
            return;
        }

        // Create hidden message-only window
        hWnd = CreateWindowEx(
            0,
            wc.lpszClassName,
            TEXT("SleepSetWindow"),
            0,
            0, 0, 0, 0,
            HWND_MESSAGE,
            nullptr,
            wc.hInstance,
            nullptr
            );

        if (!hWnd)
        {
            qDebug() << "Failed to create message-only window. Error:" << GetLastError();
            return;
        }

        // Store pointer to this instance
        SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

        // Register for power setting notifications
        hPowerNotify = RegisterPowerSettingNotification(
            hWnd,
            &GUID_CONSOLE_DISPLAY_STATE,
            DEVICE_NOTIFY_WINDOW_HANDLE
            );

        if (!hPowerNotify)
        {
            DWORD dwError = GetLastError();
            qDebug() << "Failed to register power setting notification. Error:" << dwError;
        }
        else
        {
            qDebug() << "Successfully registered power setting notification.";
        }
    }
}

void SleepSet::stop()
{
    if (running)
    {
        running = false;
        qDebug() << "[SleepSet] Stopping event filter";
        if (hPowerNotify)
        {
            UnregisterPowerSettingNotification(hPowerNotify);
            hPowerNotify = nullptr;
            qDebug() << "Successfully unregistered power setting notification.";
        }

        if (hWnd)
        {
            DestroyWindow(hWnd);
            hWnd = nullptr;
            qDebug() << "Successfully destroyed message-only window.";
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
