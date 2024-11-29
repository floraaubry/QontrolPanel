#include "SettingsPage.h"
#include "ui_SettingsPage.h"
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonParseError>
#include <shortcutmanager.h>

using namespace ShortcutManager;

SettingsPage::SettingsPage(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::SettingsPage)
    , settings("Odizinne", "QuickSoundSwitcher")
{
    ui->setupUi(this);
    this->setFixedSize(this->size());
    loadSettings();

    connect(ui->mergeSimilarCheckBox, &QCheckBox::checkStateChanged, this, &SettingsPage::saveSettings);
}

SettingsPage::~SettingsPage()
{
    emit closed();
    delete ui;
}

void SettingsPage::loadSettings()
{
    ui->mergeSimilarCheckBox->setChecked(settings.value("mergeSimilarApps", true).toBool());
}

void SettingsPage::saveSettings()
{
    settings.setValue("mergeSimilarApps", ui->mergeSimilarCheckBox->isChecked());

    emit settingsChanged();
}
