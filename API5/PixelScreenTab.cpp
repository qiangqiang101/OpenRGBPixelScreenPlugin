/*---------------------------------------------------------*\
| OpenRGBMatrixTextTab.cpp                                  |
|                                                           |
|   OpenRGB Matrix Text Plugin Tab                          |
|                                                           |
|   This file is part of the OpenRGB Matrix Text Plugin     |
|   project                                                 |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#include "PixelScreenTab.h"
#include "ui_PixelScreenTab.h"
#include "DeviceSettingsPage.h"

PixelScreenTab::PixelScreenTab(PixelScreenPlugin* plugin_ptr, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PixelScreenTab),
    plugin(plugin_ptr)
{
    ui->setupUi(this);

    UpdateDeviceList();
}

PixelScreenTab::~PixelScreenTab()
{
    delete ui;
}

void PixelScreenTab::UpdateDeviceList()
{
    const auto names = plugin->GetMatrixZoneNames();
    bool unchanged = ui->deviceTabWidget->count() == static_cast<int>(names.size());
    for (int i = 0; unchanged && i < ui->deviceTabWidget->count(); ++i)
        unchanged = ui->deviceTabWidget->tabText(i) == QString::fromStdString(names[i]);
    if (unchanged) return;

    // QTabWidget::clear() does not delete pages or their signal connections.
    while (ui->deviceTabWidget->count())
    {
        QWidget* page = ui->deviceTabWidget->widget(0);
        ui->deviceTabWidget->removeTab(0);
        delete page;
    }

    for (const std::string& display_name : names)
    {
        DeviceSettingsPage *page = new DeviceSettingsPage(plugin, display_name, this);
        ui->deviceTabWidget->addTab(page, QString::fromStdString(display_name));
    }
}
