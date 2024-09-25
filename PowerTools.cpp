#include "PowerTools.h"
#include <QHBoxLayout>

bool PowerTools::DarkTheme = false;
ResourceManager* PowerTools::RMPointer = nullptr;

OpenRGBPluginInfo PowerTools::GetPluginInfo()
{
    printf("[PowerTools] Loading plugin info.\n");

    OpenRGBPluginInfo info;
    info.Name         = "Power Tools";
    info.Description  = "Add ability to enable/disable, set custom profiles on sleep/wake windows event!";
    info.Version  = VERSION_STRING;
    info.Commit  = GIT_COMMIT_ID;
    info.URL  = "https://gitlab.com/tfurci/openrgb-powertools";
    info.Icon.load(":/PowerTools.png");

    info.Location     =  OPENRGB_PLUGIN_LOCATION_TOP;
    info.Label        =  "Power Tools";
    info.TabIconString        =  "Power Tools";
    info.TabIcon.load(":/PowerTools.png");

    return info;
}

unsigned int PowerTools::GetPluginAPIVersion()
{
    printf("[PowerTools] Loading plugin API version.\n");

    return OPENRGB_PLUGIN_API_VERSION;
}

void PowerTools::Load(bool dark_theme, ResourceManager* resource_manager_ptr)
{
    printf("[PowerTools] Loading plugin.\n");

    RMPointer                = resource_manager_ptr;
    DarkTheme                = dark_theme;
}


QWidget* PowerTools::GetWidget()
{
    printf("[PowerTools] Creating widget.\n");

    // Load UI from file
    QFile file(":/PowerTools.ui");

    QWidget* widget = new QWidget();
    Ui::PowerTools ui;
    ui.setupUi(widget);

    // Load profiles from ProfileManager and populate combo boxes
    std::vector<std::string> profiles = PowerTools::RMPointer->GetProfileManager()->profile_list;

    QStringList profileNames;
    for (const auto& profile : profiles) {
        profileNames.append(QString::fromStdString(profile));
    }

    QList<QComboBox*> comboBoxes = {
        ui.comboBoxSleepProfile,
        ui.comboBoxReturnFromSleepProfile,
        ui.comboBoxLockProfile,
        ui.comboBoxReturnFromLockProfile,
        ui.comboBoxShutdownProfile,
        ui.comboBoxMonitorShutdownProfile,
        ui.comboBoxMonitorComebackProfile
    };

    for (auto comboBox : comboBoxes) {
        comboBox->addItems(profileNames);
    }

    // Load saved settings
    LoadSettings(ui);

    // Connect save button
    connect(ui.saveButton, &QPushButton::clicked, [ui, this]() mutable {
        SaveSettings(ui);
        QMessageBox::information(ui.saveButton, "Settings Saved", "Your settings have been saved.");
    });

    return widget;
}

QMenu* PowerTools::GetTrayMenu()
{
    return 0;
}

void PowerTools::Unload()
{
    printf("[PowerTools] Time to call some cleaning stuff.\n");
    sleepSet.stop();
}

void PowerTools::LoadSettings(Ui::PowerTools& ui)
{
    QString settingsFilePath = QDir::homePath() + "/AppData/Roaming/OpenRGB/powertools.ini";
    QSettings settings(settingsFilePath, QSettings::IniFormat);

    // Check if the settings file exists
    if (!QFile(settingsFilePath).exists()) {
        qDebug() << "[PowerTools] Settings file does not exist. Creating with default values.";
        SaveSettings(ui); // Create the settings file with default values
        return;
    }

    // Load settings from QSettings
    settings.beginGroup("Sleep");
    ui.checkBoxSleep->setChecked(settings.value("Enabled", false).toBool());
    ui.comboBoxSleepAction->setCurrentIndex(settings.value("Action", 0).toInt());
    ui.comboBoxSleepProfile->setCurrentText(settings.value("Profile", "OFF").toString());
    settings.endGroup();

    settings.beginGroup("ReturnFromSleep");
    ui.checkBoxReturnFromSleep->setChecked(settings.value("Enabled", false).toBool());
    ui.comboBoxReturnFromSleepAction->setCurrentIndex(settings.value("Action", 0).toInt());
    ui.comboBoxReturnFromSleepProfile->setCurrentText(settings.value("Profile", "OFF").toString());
    settings.endGroup();

    settings.beginGroup("Lock");
    ui.checkBoxLock->setChecked(settings.value("Enabled", false).toBool());
    ui.comboBoxLockAction->setCurrentIndex(settings.value("Action", 0).toInt());
    ui.comboBoxLockProfile->setCurrentText(settings.value("Profile", "OFF").toString());
    settings.endGroup();

    settings.beginGroup("ReturnFromLock");
    ui.checkBoxReturnFromLock->setChecked(settings.value("Enabled", false).toBool());
    ui.comboBoxReturnFromLockAction->setCurrentIndex(settings.value("Action", 0).toInt());
    ui.comboBoxReturnFromLockProfile->setCurrentText(settings.value("Profile", "OFF").toString());
    settings.endGroup();

    settings.beginGroup("Shutdown");
    ui.checkBoxShutdown->setChecked(settings.value("Enabled", false).toBool());
    ui.comboBoxShutdownAction->setCurrentIndex(settings.value("Action", 0).toInt());
    ui.comboBoxShutdownProfile->setCurrentText(settings.value("Profile", "OFF").toString());
    settings.endGroup();

    settings.beginGroup("MonitorShutdown");
    ui.checkBoxMonitorShutdown->setChecked(settings.value("Enabled", false).toBool());
    ui.comboBoxMonitorShutdownAction->setCurrentIndex(settings.value("Action", 0).toInt());
    ui.comboBoxMonitorShutdownProfile->setCurrentText(settings.value("Profile", "OFF").toString());
    settings.endGroup();

    settings.beginGroup("MonitorComeback");
    ui.checkBoxMonitorComeback->setChecked(settings.value("Enabled", false).toBool());
    ui.comboBoxMonitorComebackAction->setCurrentIndex(settings.value("Action", 0).toInt());
    ui.comboBoxMonitorComebackProfile->setCurrentText(settings.value("Profile", "OFF").toString());
    settings.endGroup();

    qDebug() << "[PowerTools] Settings loaded from:" << settingsFilePath;
}

void PowerTools::SaveSettings(Ui::PowerTools& ui)
{
    QString settingsFilePath = QDir::homePath() + "/AppData/Roaming/OpenRGB/powertools.ini";
    QSettings settings(settingsFilePath, QSettings::IniFormat);

    settings.beginGroup("Sleep");
    settings.setValue("Enabled", ui.checkBoxSleep->isChecked());
    settings.setValue("Action", ui.comboBoxSleepAction->currentIndex());
    settings.setValue("Profile", ui.comboBoxSleepProfile->currentText());
    settings.endGroup();

    settings.beginGroup("ReturnFromSleep");
    settings.setValue("Enabled", ui.checkBoxReturnFromSleep->isChecked());
    settings.setValue("Action", ui.comboBoxReturnFromSleepAction->currentIndex());
    settings.setValue("Profile", ui.comboBoxReturnFromSleepProfile->currentText());
    settings.endGroup();

    settings.beginGroup("Lock");
    settings.setValue("Enabled", ui.checkBoxLock->isChecked());
    settings.setValue("Action", ui.comboBoxLockAction->currentIndex());
    settings.setValue("Profile", ui.comboBoxLockProfile->currentText());
    settings.endGroup();

    settings.beginGroup("ReturnFromLock");
    settings.setValue("Enabled", ui.checkBoxReturnFromLock->isChecked());
    settings.setValue("Action", ui.comboBoxReturnFromLockAction->currentIndex());
    settings.setValue("Profile", ui.comboBoxReturnFromLockProfile->currentText());
    settings.endGroup();

    settings.beginGroup("Shutdown");
    settings.setValue("Enabled", ui.checkBoxShutdown->isChecked());
    settings.setValue("Action", ui.comboBoxShutdownAction->currentIndex());
    settings.setValue("Profile", ui.comboBoxShutdownProfile->currentText());
    settings.endGroup();

    settings.beginGroup("MonitorShutdown");
    settings.setValue("Enabled", ui.checkBoxMonitorShutdown->isChecked());
    settings.setValue("Action", ui.comboBoxMonitorShutdownAction->currentIndex());
    settings.setValue("Profile", ui.comboBoxMonitorShutdownProfile->currentText());
    settings.endGroup();

    settings.beginGroup("MonitorComeback");
    settings.setValue("Enabled", ui.checkBoxMonitorComeback->isChecked());
    settings.setValue("Action", ui.comboBoxMonitorComebackAction->currentIndex());
    settings.setValue("Profile", ui.comboBoxMonitorComebackProfile->currentText());
    settings.endGroup();

    settings.sync(); // Ensure all data is written to file
    qDebug() << "[PowerTools] Settings saved to:" << settingsFilePath;
}
