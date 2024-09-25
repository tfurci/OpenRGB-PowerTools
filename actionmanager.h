#ifndef ACTIONMANAGER_H
#define ACTIONMANAGER_H


#include "ui_PowerTools.h"
#include "SleepSet.h"
#include "PowerTools.h"

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


class ActionManager
{
public:
    void                SaveSettings(Ui::PowerTools& ui);
    void                LoadSettings(Ui::PowerTools& ui);
};

#endif // ACTIONMANAGER_H
