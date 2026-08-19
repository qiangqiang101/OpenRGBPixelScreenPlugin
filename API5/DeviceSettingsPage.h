/*---------------------------------------------------------*\
| DeviceSettingsPage.h                                      |
|                                                           |
|   Individual device tab page for Matrix Text Plugin       |
|                                                           |
|   This file is part of the OpenRGB Matrix Text Plugin     |
|   project                                                 |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#pragma once

#include <QWidget>
#include <QCheckBox>
#include <QComboBox>
#include <QLineEdit>
#include <QTextEdit>
#include <QSlider>
#include <QSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QRadioButton>
#include <QButtonGroup>
#include "PixelScreenPlugin.h"

class DeviceSettingsPage : public QWidget
{
    Q_OBJECT

public:
    DeviceSettingsPage(PixelScreenPlugin* plugin_ptr, const std::string& dev_name, QWidget *parent = nullptr);
    ~DeviceSettingsPage() = default;

    void LoadSettingsToPage();

private slots:
    void on_enabledCheck_stateChanged(int state);
    void on_displayModeCombo_currentIndexChanged(int index);
    void on_fontSizeCombo_currentTextChanged(const QString &text);
    void on_customTextEdit_textChanged();
    void on_timeFormatEdit_textChanged();
    void on_sensorFormatEdit_textChanged();
    void on_lhmUrlEdit_textChanged();
    void on_sensorIntervalRadio_toggled();
    void on_sensorRefreshButton_clicked();
    void on_sensorAddButton_clicked();
    void on_alignRadio_toggled();
    void on_pixelArtEdit_textChanged();
    void on_scrollDirCombo_currentTextChanged(const QString &text);
    void on_scrollSpeedSlider_valueChanged(int value);
    void on_textColorButton_clicked();
    void on_fpsSlider_valueChanged(int value);
    void on_invertColorCheck_stateChanged(int state);
    void on_paddingXSpin_valueChanged(int value);
    void on_paddingYSpin_valueChanged(int value);
    void on_sensorDataUpdated();

private:
    PixelScreenPlugin *plugin;
    std::string device_name;
    bool loading_ui = false;
    bool manual_refresh_requested = false;

    QCheckBox   *enabledCheck;
    QComboBox   *displayModeCombo;
    QComboBox   *fontSizeCombo;
    QTextEdit   *customTextEdit;
    QTextEdit   *timeFormatEdit;
    // Sensor UI
    QTextEdit   *sensorFormatEdit;
    QLineEdit   *lhmUrlEdit;
    QComboBox   *sensorComboBox;
    QPushButton *sensorAddButton;
    QPushButton *sensorRefreshButton;
    QRadioButton *sensorInterval250Radio;
    QRadioButton *sensorInterval500Radio;
    QRadioButton *sensorInterval1000Radio;
    QRadioButton *sensorInterval2000Radio;
    QLabel      *sensorStatusLabel;
    // Alignment
    QRadioButton *alignStartRadio;
    QRadioButton *alignCenterRadio;
    QRadioButton *alignEndRadio;
    QTextEdit   *pixelArtEdit;
    QComboBox   *scrollDirCombo;
    QSlider     *scrollSpeedSlider;
    QLabel      *scrollSpeedValueLabel;
    QPushButton *textColorButton;
    QSlider     *fpsSlider;
    QLabel      *fpsValueLabel;
    QCheckBox   *invertColorCheck;
    QSpinBox    *paddingXSpin;
    QSpinBox    *paddingYSpin;

    void UpdateColorButton(QPushButton* button, unsigned char r, unsigned char g, unsigned char b);
    void UpdateSensorUI(bool sensor_mode);

    QButtonGroup *alignGroup;
    QButtonGroup *sensorIntervalGroup;
};
