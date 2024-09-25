#ifndef POWERTOOLS_H
#define POWERTOOLS_H

#include "OpenRGBPluginInterface.h"
#include "ResourceManager.h"
#include "SleepSet.h"
#include "ProfileManager.h"
#include "ui_PowerTools.h"

#include <QObject>
#include <QString>
#include <QtPlugin>
#include <QWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QSettings>
#include <QDir>
#include <QMessageBox>
#include <QJsonDocument>
#include <QSettings>
#include <QCoreApplication>

namespace Ui {
class PowerTool;
}


class PowerTools : public QObject, public OpenRGBPluginInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID OpenRGBPluginInterface_IID)
    Q_INTERFACES(OpenRGBPluginInterface)

public:

    OpenRGBPluginInfo   GetPluginInfo() override;
    unsigned int        GetPluginAPIVersion() override;

    void                Load(bool dark_theme, ResourceManager* resource_manager_ptr) override;
    QWidget*            GetWidget() override;
    QMenu*              GetTrayMenu() override;
    void                Unload() override;
    void                SaveSettings(Ui::PowerTools& ui);
    void                LoadSettings(Ui::PowerTools& ui);

//    OpenRGBPluginInfo       PInfo;
//    OpenRGBPluginInfo       Initialize(bool, ResourceManager*)   override;
//    QWidget*                CreateGUI(QWidget *Parent)           override;
    static bool             DarkTheme;
    static ResourceManager* RMPointer;
    SleepSet sleepSet;

private:
    Ui::PowerTools *ui;
    ProfileManager* profileManager; // Add a pointer to ProfileManager

};

#endif // POWERTOOLS_H
