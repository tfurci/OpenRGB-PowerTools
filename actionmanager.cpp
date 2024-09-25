#include "actionmanager.h"

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
