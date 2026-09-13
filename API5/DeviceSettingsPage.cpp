/*---------------------------------------------------------*\
| DeviceSettingsPage.cpp                                    |
|                                                           |
|   Individual device tab page for Matrix Text Plugin       |
|                                                           |
|   This file is part of the OpenRGB Matrix Text Plugin     |
|   project                                                 |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#include "DeviceSettingsPage.h"
#include "HardwareSensorManager.h"
#include <QColorDialog>
#include <QColor>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>

DeviceSettingsPage::DeviceSettingsPage(PixelScreenPlugin* plugin_ptr, const std::string& dev_name, QWidget *parent)
    : QWidget(parent), plugin(plugin_ptr), device_name(dev_name)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    QGroupBox *settingsGroupBox = new QGroupBox("Device Text && Animation Settings", this);
    QFormLayout *formLayout = new QFormLayout(settingsGroupBox);

    // 0. Enabled Checkbox
    enabledCheck = new QCheckBox("Enable Matrix Text for this device", settingsGroupBox);
    formLayout->addRow("Status:", enabledCheck);

    // 1. Display Mode
    displayModeCombo = new QComboBox(settingsGroupBox);
    displayModeCombo->addItem("Time / Clock");
    displayModeCombo->addItem("Custom Text");
    displayModeCombo->addItem("Pixel Art");
    displayModeCombo->addItem("Sensor Data");
    formLayout->addRow("Display Mode:", displayModeCombo);

    // 2. Font Size
    fontSizeCombo = new QComboBox(settingsGroupBox);
    fontSizeCombo->addItem("Small");
    fontSizeCombo->addItem("Medium");
    fontSizeCombo->addItem("Large");
    fontSizeCombo->addItem("Chinese");
    formLayout->addRow("Font Size:", fontSizeCombo);

    // 3. Custom Text (Multi-line QTextEdit)
    customTextEdit = new QTextEdit(settingsGroupBox);
    customTextEdit->setAcceptRichText(false);
    customTextEdit->setPlainText("OpenRGB");
    customTextEdit->setMaximumHeight(65);
    formLayout->addRow("Custom Text:", customTextEdit);

    // 4. Time Format (Multi-line QTextEdit)
    timeFormatEdit = new QTextEdit(settingsGroupBox);
    timeFormatEdit->setAcceptRichText(false);
    timeFormatEdit->setPlainText("hh:mm tt");
    timeFormatEdit->setMaximumHeight(65);
    formLayout->addRow("Time Format:", timeFormatEdit);

    // 5. Sensor Format (textarea)
    sensorFormatEdit = new QTextEdit(settingsGroupBox);
    sensorFormatEdit->setAcceptRichText(false);
    sensorFormatEdit->setPlaceholderText("e.g. CPU: [CPU\\Load\\CPU Total]  Temp: [CPU\\Temperatures\\Core Average]");
    sensorFormatEdit->setMaximumHeight(65);
    formLayout->addRow("Sensor Format:", sensorFormatEdit);

    lhmUrlEdit = new QLineEdit(settingsGroupBox);
    lhmUrlEdit->setPlaceholderText("http://127.0.0.1:8085/data.json");
    formLayout->addRow("LHM Server URL:", lhmUrlEdit);

    // 6. Sensor Picker Row
    sensorComboBox = new QComboBox(settingsGroupBox);
    sensorComboBox->setMinimumWidth(300);
    sensorComboBox->addItem("(refresh to load sensors)");
    sensorAddButton = new QPushButton("Add", settingsGroupBox);
    sensorRefreshButton = new QPushButton("Refresh", settingsGroupBox);
    sensorStatusLabel = new QLabel("", settingsGroupBox);
    sensorStatusLabel->setStyleSheet("color: gray; font-style: italic;");

    QHBoxLayout *sensorPickerLayout = new QHBoxLayout();
    sensorPickerLayout->addWidget(sensorComboBox, 1);
    sensorPickerLayout->addWidget(sensorAddButton);
    sensorPickerLayout->addWidget(sensorRefreshButton);
    formLayout->addRow("Sensor:", sensorPickerLayout);
    formLayout->addRow("", sensorStatusLabel);

    // 7. Sensor Update Interval Radio Buttons
    sensorInterval250Radio  = new QRadioButton("250ms",  settingsGroupBox);
    sensorInterval500Radio  = new QRadioButton("500ms",  settingsGroupBox);
    sensorInterval1000Radio = new QRadioButton("1s",     settingsGroupBox);
    sensorInterval2000Radio = new QRadioButton("2s",     settingsGroupBox);
    sensorInterval1000Radio->setChecked(true);

    // Explicit group prevents conflict with alignment radios
    sensorIntervalGroup = new QButtonGroup(this);
    sensorIntervalGroup->addButton(sensorInterval250Radio,  0);
    sensorIntervalGroup->addButton(sensorInterval500Radio,  1);
    sensorIntervalGroup->addButton(sensorInterval1000Radio, 2);
    sensorIntervalGroup->addButton(sensorInterval2000Radio, 3);

    QHBoxLayout *sensorIntervalLayout = new QHBoxLayout();
    sensorIntervalLayout->addWidget(sensorInterval250Radio);
    sensorIntervalLayout->addWidget(sensorInterval500Radio);
    sensorIntervalLayout->addWidget(sensorInterval1000Radio);
    sensorIntervalLayout->addWidget(sensorInterval2000Radio);
    sensorIntervalLayout->addStretch(1);
    formLayout->addRow("Update Interval:", sensorIntervalLayout);

    // Text Alignment Radio Buttons
    alignStartRadio = new QRadioButton("Start", settingsGroupBox);
    alignCenterRadio = new QRadioButton("Center", settingsGroupBox);
    alignEndRadio = new QRadioButton("End", settingsGroupBox);
    alignStartRadio->setChecked(true);

    // Explicit group prevents conflict with interval radios
    alignGroup = new QButtonGroup(this);
    alignGroup->addButton(alignStartRadio,  0);
    alignGroup->addButton(alignCenterRadio, 1);
    alignGroup->addButton(alignEndRadio,    2);

    QHBoxLayout *alignLayout = new QHBoxLayout();
    alignLayout->addWidget(alignStartRadio);
    alignLayout->addWidget(alignCenterRadio);
    alignLayout->addWidget(alignEndRadio);
    alignLayout->addStretch(1);
    formLayout->addRow("Text Alignment:", alignLayout);

    // Pixel Art 2D Matrix Data (Textarea)
    pixelArtEdit = new QTextEdit(settingsGroupBox);
    pixelArtEdit->setPlainText("[ [1, 0, 0, 1], [0, 1, 1, 0], [0, 1, 1, 0], [1, 0, 0, 1] ]");
    pixelArtEdit->setMaximumHeight(90);
    formLayout->addRow("Pixel Art Matrix (JSON):", pixelArtEdit);

    // Scroll Direction
    scrollDirCombo = new QComboBox(settingsGroupBox);
    scrollDirCombo->addItem("Off");
    scrollDirCombo->addItem("Left");
    scrollDirCombo->addItem("Right");
    scrollDirCombo->addItem("Ping-Pong");
    scrollDirCombo->addItem("Up");
    scrollDirCombo->addItem("Down");
    scrollDirCombo->addItem("Ping-Pong Up down");
    formLayout->addRow("Scroll Direction:", scrollDirCombo);

    // Scroll Speed
    scrollSpeedSlider = new QSlider(Qt::Horizontal, settingsGroupBox);
    scrollSpeedSlider->setRange(1, 100);
    scrollSpeedSlider->setValue(50);
    scrollSpeedValueLabel = new QLabel("50", settingsGroupBox);
    scrollSpeedValueLabel->setMinimumWidth(35);
    QHBoxLayout *speedLayout = new QHBoxLayout();
    speedLayout->addWidget(scrollSpeedSlider);
    speedLayout->addWidget(scrollSpeedValueLabel);
    formLayout->addRow("Scroll Speed:", speedLayout);

    // Text Color
    textColorButton = new QPushButton("Select Text Color", settingsGroupBox);
    formLayout->addRow("Text Color:", textColorButton);

    // FPS
    fpsSlider = new QSlider(Qt::Horizontal, settingsGroupBox);
    fpsSlider->setRange(1, 60);
    fpsSlider->setValue(20);
    fpsValueLabel = new QLabel("20", settingsGroupBox);
    fpsValueLabel->setMinimumWidth(35);
    QHBoxLayout *fpsLayout = new QHBoxLayout();
    fpsLayout->addWidget(fpsSlider);
    fpsLayout->addWidget(fpsValueLabel);
    formLayout->addRow("Frame Rate (FPS):", fpsLayout);

    // Invert Color
    invertColorCheck = new QCheckBox("Invert Background / Text Colors", settingsGroupBox);
    formLayout->addRow("Color FX:", invertColorCheck);

    // Padding
    paddingXSpin = new QSpinBox(settingsGroupBox);
    paddingXSpin->setRange(-100, 100);
    paddingYSpin = new QSpinBox(settingsGroupBox);
    paddingYSpin->setRange(-100, 100);

    QHBoxLayout *padLayout = new QHBoxLayout();
    padLayout->addWidget(new QLabel("X:"));
    padLayout->addWidget(paddingXSpin);
    padLayout->addWidget(new QLabel("Y:"));
    padLayout->addWidget(paddingYSpin);
    padLayout->addStretch(1);
    formLayout->addRow("Position Padding:", padLayout);

    mainLayout->addWidget(settingsGroupBox);
    mainLayout->addStretch(1);

    // Signal connections
    connect(enabledCheck, &QCheckBox::checkStateChanged, this, &DeviceSettingsPage::on_enabledCheck_stateChanged);
    connect(displayModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &DeviceSettingsPage::on_displayModeCombo_currentIndexChanged);
    connect(fontSizeCombo, &QComboBox::currentTextChanged, this, &DeviceSettingsPage::on_fontSizeCombo_currentTextChanged);
    connect(customTextEdit, &QTextEdit::textChanged, this, &DeviceSettingsPage::on_customTextEdit_textChanged);
    connect(timeFormatEdit, &QTextEdit::textChanged, this, &DeviceSettingsPage::on_timeFormatEdit_textChanged);
    connect(sensorFormatEdit, &QTextEdit::textChanged, this, &DeviceSettingsPage::on_sensorFormatEdit_textChanged);
    connect(lhmUrlEdit, &QLineEdit::textChanged, this, &DeviceSettingsPage::on_lhmUrlEdit_textChanged);
    connect(sensorRefreshButton, &QPushButton::clicked, this, &DeviceSettingsPage::on_sensorRefreshButton_clicked);
    connect(sensorAddButton, &QPushButton::clicked, this, &DeviceSettingsPage::on_sensorAddButton_clicked);
    connect(sensorInterval250Radio,  &QRadioButton::toggled, this, &DeviceSettingsPage::on_sensorIntervalRadio_toggled);
    connect(sensorInterval500Radio,  &QRadioButton::toggled, this, &DeviceSettingsPage::on_sensorIntervalRadio_toggled);
    connect(sensorInterval1000Radio, &QRadioButton::toggled, this, &DeviceSettingsPage::on_sensorIntervalRadio_toggled);
    connect(sensorInterval2000Radio, &QRadioButton::toggled, this, &DeviceSettingsPage::on_sensorIntervalRadio_toggled);
    connect(alignStartRadio, &QRadioButton::toggled, this, &DeviceSettingsPage::on_alignRadio_toggled);
    connect(alignCenterRadio, &QRadioButton::toggled, this, &DeviceSettingsPage::on_alignRadio_toggled);
    connect(alignEndRadio, &QRadioButton::toggled, this, &DeviceSettingsPage::on_alignRadio_toggled);
    connect(pixelArtEdit, &QTextEdit::textChanged, this, &DeviceSettingsPage::on_pixelArtEdit_textChanged);
    connect(scrollDirCombo, &QComboBox::currentTextChanged, this, &DeviceSettingsPage::on_scrollDirCombo_currentTextChanged);
    connect(scrollSpeedSlider, &QSlider::valueChanged, this, &DeviceSettingsPage::on_scrollSpeedSlider_valueChanged);
    connect(textColorButton, &QPushButton::clicked, this, &DeviceSettingsPage::on_textColorButton_clicked);
    connect(fpsSlider, &QSlider::valueChanged, this, &DeviceSettingsPage::on_fpsSlider_valueChanged);
    connect(invertColorCheck, &QCheckBox::checkStateChanged, this, &DeviceSettingsPage::on_invertColorCheck_stateChanged);
    connect(paddingXSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &DeviceSettingsPage::on_paddingXSpin_valueChanged);
    connect(paddingYSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &DeviceSettingsPage::on_paddingYSpin_valueChanged);

    // Connect to sensor manager signals
    if (HardwareSensorManager* sensor_manager = plugin->GetSensorManager())
    {
        connect(sensor_manager, &HardwareSensorManager::sensorDataUpdated, this, &DeviceSettingsPage::on_sensorDataUpdated);
        connect(sensor_manager, &HardwareSensorManager::sensorFetchError, this, [this](const QString& err){
            manual_refresh_requested = false;
            sensorStatusLabel->setText("Error: " + err);
            sensorStatusLabel->setStyleSheet("color: red; font-style: italic;");
        });
    }

    LoadSettingsToPage();
}

void DeviceSettingsPage::LoadSettingsToPage()
{
    loading_ui = true;

    const DeviceMatrixSettings dev_s = plugin->GetDeviceSettings(device_name);

    enabledCheck->setChecked(dev_s.enabled);

    // Map internal display_mode to combo index:
    // 0->0(Time), 1->1(Custom), 2->2(PixelArt), 3->2(PixelArt alt), 4->3(Sensor)
    int mode_idx = dev_s.display_mode;
    if (mode_idx == 3) mode_idx = 2;
    else if (mode_idx == 4) mode_idx = 3;
    else if (mode_idx > 4) mode_idx = 1;
    displayModeCombo->setCurrentIndex(mode_idx);

    bool is_sensor = (mode_idx == 3);
    bool is_art    = (mode_idx == 2);
    fontSizeCombo->setEnabled(!is_art);
    timeFormatEdit->setEnabled(mode_idx == 0);
    customTextEdit->setEnabled(mode_idx == 1);
    pixelArtEdit->setEnabled(is_art);
    UpdateSensorUI(is_sensor);

    fontSizeCombo->setCurrentText(QString::fromStdString(dev_s.font_size));
    customTextEdit->setPlainText(QString::fromStdString(dev_s.custom_text));
    timeFormatEdit->setPlainText(QString::fromStdString(dev_s.time_format));
    sensorFormatEdit->setPlainText(QString::fromStdString(dev_s.sensor_format));
    lhmUrlEdit->setText(QString::fromStdString(dev_s.lhm_url.empty() ? "http://127.0.0.1:8085/data.json" : dev_s.lhm_url));
    pixelArtEdit->setPlainText(QString::fromStdString(dev_s.pixel_art_json));

    // Sensor interval radios
    if (dev_s.sensor_update_interval <= 250)       sensorInterval250Radio->setChecked(true);
    else if (dev_s.sensor_update_interval <= 500)  sensorInterval500Radio->setChecked(true);
    else if (dev_s.sensor_update_interval <= 1000) sensorInterval1000Radio->setChecked(true);
    else                                           sensorInterval2000Radio->setChecked(true);

    if (dev_s.text_align == 0) alignStartRadio->setChecked(true);
    else if (dev_s.text_align == 1) alignCenterRadio->setChecked(true);
    else if (dev_s.text_align == 2) alignEndRadio->setChecked(true);

    scrollDirCombo->setCurrentText(QString::fromStdString(dev_s.scroll_direction));
    scrollSpeedSlider->setValue(dev_s.scroll_speed);
    scrollSpeedValueLabel->setText(QString::number(dev_s.scroll_speed));

    fpsSlider->setValue(dev_s.fps);
    fpsValueLabel->setText(QString::number(dev_s.fps));

    invertColorCheck->setChecked(dev_s.invert_color);
    paddingXSpin->setValue(dev_s.padding_x);
    paddingYSpin->setValue(dev_s.padding_y);

    UpdateColorButton(textColorButton, dev_s.text_r, dev_s.text_g, dev_s.text_b);

    on_sensorDataUpdated();

    loading_ui = false;
}

void DeviceSettingsPage::UpdateSensorUI(bool sensor_mode)
{
    sensorFormatEdit->setEnabled(sensor_mode);
    lhmUrlEdit->setEnabled(sensor_mode);
    sensorComboBox->setEnabled(sensor_mode);
    sensorAddButton->setEnabled(sensor_mode);
    sensorRefreshButton->setEnabled(sensor_mode);
    sensorStatusLabel->setEnabled(sensor_mode);
    sensorInterval250Radio->setEnabled(sensor_mode);
    sensorInterval500Radio->setEnabled(sensor_mode);
    sensorInterval1000Radio->setEnabled(sensor_mode);
    sensorInterval2000Radio->setEnabled(sensor_mode);
}

void DeviceSettingsPage::UpdateColorButton(QPushButton* button, unsigned char r, unsigned char g, unsigned char b)
{
    QString stylesheet = QString("background-color: rgb(%1, %2, %3); color: %4; font-weight: bold;")
        .arg(r).arg(g).arg(b)
        .arg((r*0.299 + g*0.587 + b*0.114) > 128 ? "black" : "white");
    button->setStyleSheet(stylesheet);
}

void DeviceSettingsPage::on_enabledCheck_stateChanged(int state)
{
    if (loading_ui) return;
    plugin->UpdateDeviceSettings(device_name, [state](DeviceMatrixSettings& settings)
    {
        settings.enabled = (state == Qt::Checked);
    });
}

void DeviceSettingsPage::on_displayModeCombo_currentIndexChanged(int index)
{
    if (loading_ui) return;
    // Combo: 0=Time, 1=Custom, 2=PixelArt, 3=Sensor
    // Internal: 0=Time, 1=Custom, 3=PixelArt, 4=Sensor
    plugin->UpdateDeviceSettings(device_name, [index](DeviceMatrixSettings& settings)
    {
        if (index == 2) settings.display_mode = 3;
        else if (index == 3) settings.display_mode = 4;
        else settings.display_mode = index;
    });

    bool is_sensor = (index == 3);
    bool is_art    = (index == 2);
    fontSizeCombo->setEnabled(!is_art);
    timeFormatEdit->setEnabled(index == 0);
    customTextEdit->setEnabled(index == 1);
    pixelArtEdit->setEnabled(is_art);
    UpdateSensorUI(is_sensor);

}

void DeviceSettingsPage::on_fontSizeCombo_currentTextChanged(const QString &text)
{
    if (loading_ui) return;
    const std::string font_size = text.toStdString();
    plugin->UpdateDeviceSettings(device_name, [font_size](DeviceMatrixSettings& settings)
    {
        settings.font_size = font_size;
    });
}

void DeviceSettingsPage::on_customTextEdit_textChanged()
{
    if (loading_ui) return;
    const std::string text = customTextEdit->toPlainText().toStdString();
    plugin->UpdateDeviceSettings(device_name, [text](DeviceMatrixSettings& settings)
    {
        settings.custom_text = text;
    });
}

void DeviceSettingsPage::on_timeFormatEdit_textChanged()
{
    if (loading_ui) return;
    const std::string format = timeFormatEdit->toPlainText().toStdString();
    plugin->UpdateDeviceSettings(device_name, [format](DeviceMatrixSettings& settings)
    {
        settings.time_format = format;
    });
}

void DeviceSettingsPage::on_sensorFormatEdit_textChanged()
{
    if (loading_ui) return;
    const std::string format = sensorFormatEdit->toPlainText().toStdString();
    plugin->UpdateDeviceSettings(device_name, [format](DeviceMatrixSettings& settings)
    {
        settings.sensor_format = format;
    });
}

void DeviceSettingsPage::on_lhmUrlEdit_textChanged()
{
    if (loading_ui) return;
    const std::string url = lhmUrlEdit->text().toStdString();
    plugin->UpdateDeviceSettings(device_name, [url](DeviceMatrixSettings& settings)
    {
        settings.lhm_url = url;
    });
}

void DeviceSettingsPage::on_sensorIntervalRadio_toggled()
{
    if (loading_ui) return;
    const QRadioButton* changed_button = qobject_cast<QRadioButton*>(sender());
    if (changed_button && !changed_button->isChecked()) return;

    int interval = 2000;
    if (sensorInterval250Radio->isChecked())       interval = 250;
    else if (sensorInterval500Radio->isChecked())  interval = 500;
    else if (sensorInterval1000Radio->isChecked()) interval = 1000;
    plugin->UpdateDeviceSettings(device_name, [interval](DeviceMatrixSettings& settings)
    {
        settings.sensor_update_interval = interval;
    });
    plugin->SetSensorUpdateInterval(interval);
}

void DeviceSettingsPage::on_sensorRefreshButton_clicked()
{
    manual_refresh_requested = true;
    sensorStatusLabel->setText("Refreshing...");
    sensorStatusLabel->setStyleSheet("color: gray; font-style: italic;");
    if (HardwareSensorManager* sensor_manager = plugin->GetSensorManager())
    {
        const DeviceMatrixSettings dev_s = plugin->GetDeviceSettings(device_name);
        sensor_manager->fetchSensors(dev_s.lhm_url);
    }
}

void DeviceSettingsPage::on_sensorAddButton_clicked()
{
    int idx = sensorComboBox->currentIndex();
    if (idx < 0 || sensorComboBox->currentText().startsWith("(")) return;

    std::string path = sensorComboBox->currentData().toString().toStdString();
    if (path.empty()) path = sensorComboBox->currentText().toStdString();

    QString current = sensorFormatEdit->toPlainText();
    if (!current.isEmpty() && !current.endsWith(' '))
        current += "  ";
    current += "[" + QString::fromStdString(path) + "]";
    sensorFormatEdit->setPlainText(current);
}

void DeviceSettingsPage::on_sensorDataUpdated()
{
    HardwareSensorManager* sensor_manager = plugin->GetSensorManager();
    if (!sensor_manager) return;

    auto sensors = sensor_manager->getSensorList();

    bool unpopulated = (sensorComboBox->count() <= 1 && (sensorComboBox->count() == 0 || sensorComboBox->itemText(0).startsWith("(")));
    if (!manual_refresh_requested && !unpopulated)
    {
        // Do not touch sensorComboBox during background automatic updates
        return;
    }

    manual_refresh_requested = false;

    QString current_text = sensorComboBox->currentText();

    sensorComboBox->blockSignals(true);
    sensorComboBox->clear();

    for (const auto& s : sensors)
    {
        if (s.path != "Sensor" && s.value != "Value") {
            QString label = QString::fromStdString(s.path);
            sensorComboBox->addItem(label, QString::fromStdString(s.path));
        }
    }

    if (sensorComboBox->count() == 0)
        sensorComboBox->addItem("(no sensors found)");

    // Restore selection if possible
    int restore = sensorComboBox->findText(current_text);
    if (restore >= 0) sensorComboBox->setCurrentIndex(restore);

    sensorComboBox->blockSignals(false);

    sensorStatusLabel->setText(QString("Last updated: %1 sensors").arg(sensors.size()));
    sensorStatusLabel->setStyleSheet("color: green; font-style: italic;");
}

void DeviceSettingsPage::on_alignRadio_toggled()
{
    if (loading_ui) return;
    const QRadioButton* changed_button = qobject_cast<QRadioButton*>(sender());
    if (changed_button && !changed_button->isChecked()) return;

    int alignment = 0;
    if (alignCenterRadio->isChecked()) alignment = 1;
    else if (alignEndRadio->isChecked()) alignment = 2;
    plugin->UpdateDeviceSettings(device_name, [alignment](DeviceMatrixSettings& settings)
    {
        settings.text_align = alignment;
    });
}

void DeviceSettingsPage::on_pixelArtEdit_textChanged()
{
    if (loading_ui) return;
    const std::string pixel_art = pixelArtEdit->toPlainText().toStdString();
    plugin->UpdateDeviceSettings(device_name, [pixel_art](DeviceMatrixSettings& settings)
    {
        settings.pixel_art_json = pixel_art;
    });
}

void DeviceSettingsPage::on_scrollDirCombo_currentTextChanged(const QString &text)
{
    if (loading_ui) return;
    const std::string direction = text.toStdString();
    plugin->UpdateDeviceSettings(device_name, [direction](DeviceMatrixSettings& settings)
    {
        settings.scroll_direction = direction;
    });
}

void DeviceSettingsPage::on_scrollSpeedSlider_valueChanged(int value)
{
    if (loading_ui) return;
    plugin->UpdateDeviceSettings(device_name, [value](DeviceMatrixSettings& settings)
    {
        settings.scroll_speed = value;
    });
    scrollSpeedValueLabel->setText(QString::number(value));
}

void DeviceSettingsPage::on_textColorButton_clicked()
{
    const DeviceMatrixSettings settings = plugin->GetDeviceSettings(device_name);
    const QColor current(settings.text_r, settings.text_g, settings.text_b);

    QColor selected = QColorDialog::getColor(current, this, "Select Text Color");
    if (selected.isValid())
    {
        plugin->UpdateDeviceSettings(device_name, [selected](DeviceMatrixSettings& settings)
        {
            settings.text_r = selected.red();
            settings.text_g = selected.green();
            settings.text_b = selected.blue();
        });
        UpdateColorButton(textColorButton, selected.red(), selected.green(), selected.blue());
    }
}

void DeviceSettingsPage::on_fpsSlider_valueChanged(int value)
{
    if (loading_ui) return;
    plugin->UpdateDeviceSettings(device_name, [value](DeviceMatrixSettings& settings)
    {
        settings.fps = value;
    });
    fpsValueLabel->setText(QString::number(value));
}

void DeviceSettingsPage::on_invertColorCheck_stateChanged(int state)
{
    if (loading_ui) return;
    plugin->UpdateDeviceSettings(device_name, [state](DeviceMatrixSettings& settings)
    {
        settings.invert_color = (state == Qt::Checked);
    });
}

void DeviceSettingsPage::on_paddingXSpin_valueChanged(int value)
{
    if (loading_ui) return;
    plugin->UpdateDeviceSettings(device_name, [value](DeviceMatrixSettings& settings)
    {
        settings.padding_x = value;
    });
}

void DeviceSettingsPage::on_paddingYSpin_valueChanged(int value)
{
    if (loading_ui) return;
    plugin->UpdateDeviceSettings(device_name, [value](DeviceMatrixSettings& settings)
    {
        settings.padding_y = value;
    });
}
