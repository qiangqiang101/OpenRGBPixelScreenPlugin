# OpenRGB Pixel Screen Plugin
![Github All Releases](https://img.shields.io/github/downloads/qiangqiang101/OpenRGBPixelScreenPlugin/total.svg)
![GitHub release (latest by date)](https://img.shields.io/github/v/release/qiangqiang101/OpenRGBPixelScreenPlugin)
![GitHub](https://img.shields.io/github/license/qiangqiang101/OpenRGBPixelScreenPlugin)
![GitHub branch checks state](https://img.shields.io/github/checks-status/qiangqiang101/OpenRGBPixelScreenPlugin/master)
![GitHub issues](https://img.shields.io/github/issues/qiangqiang101/OpenRGBPixelScreenPlugin)
![GitHub forks](https://img.shields.io/github/forks/qiangqiang101/OpenRGBPixelScreenPlugin?style=social)
![GitHub Repo stars](https://img.shields.io/github/stars/qiangqiang101/OpenRGBPixelScreenPlugin?style=social)
![YouTube Channel Subscribers](https://img.shields.io/youtube/channel/subscribers/UCAZlasvEy1euunP1M7nwj5Q?style=social)
[![Donate via PayPal](https://img.shields.io/badge/Donate-Paypal-brightgreen)](https://paypal.me/imnotmental)
[![Follow on Patreon](https://img.shields.io/badge/Donate-Patreon-orange)](https://www.patreon.com/imnotmental)

[中文 README](README.CN.md)

<img width="870" height="621" alt="image" src="https://github.com/user-attachments/assets/1f94cb0d-d0f7-4d63-8864-2c253d62358f" />
<img width="870" height="621" alt="image" src="https://github.com/user-attachments/assets/b7f89322-ac52-4860-b4c5-aa84c43c4ebc" />

Render custom scrolling text, clock, hardware sensors and pixel art on OpenRGB matrices.

# 🛠️ Download
## OpenRGB 1.0 (Plugin API Version 5)
- [Windows 64](https://github.com/qiangqiang101/OpenRGBPixelScreenPlugin/releases)
- [Linux amd64](https://github.com/qiangqiang101/OpenRGBPixelScreenPlugin/releases)
- [Linux arm64](https://github.com/qiangqiang101/OpenRGBPixelScreenPlugin/releases)
- [Linux armhf](https://github.com/qiangqiang101/OpenRGBPixelScreenPlugin/releases)
- [Linux i386](https://github.com/qiangqiang101/OpenRGBPixelScreenPlugin/releases)

## OpenRGB 1.0 RC - 1.0 RC3 (Plugin API Version 4)
- [Windows 86](https://github.com/qiangqiang101/OpenRGBPixelScreenPlugin/releases)
- [Windows 64](https://github.com/qiangqiang101/OpenRGBPixelScreenPlugin/releases)
- [Linux amd64](https://github.com/qiangqiang101/OpenRGBPixelScreenPlugin/releases)
- [Linux arm64](https://github.com/qiangqiang101/OpenRGBPixelScreenPlugin/releases)
- [Linux armhf](https://github.com/qiangqiang101/OpenRGBPixelScreenPlugin/releases)
- [Linux i386](https://github.com/qiangqiang101/OpenRGBPixelScreenPlugin/releases)

You can get older releases [here](https://github.com/qiangqiang101/OpenRGBPixelScreenPlugin/releases).

# ⚙️ Settings
## 🕔 Time / Clock formats
The configuration accepts standard date/time format tokens like hh:mm:ss aa.

<table>
  <tr>
    <td><b>Format</b></td>
    <td><b>Description</b></td>
    <td><b>Example</b></td>
  </tr>
  <tr>
    <td>d</td>
    <td>Date</td>
    <td>1</td>
  </tr>
  <tr>
    <td>dd</td>
    <td>Date with leading zero</td>
    <td>01</td>
  </tr>
  <tr>
    <td>ddd</td>
    <td>Short day name abbreviation (Sun to Sat)</td>
    <td>Tue</td>
  </tr>
  <tr>
    <td>dddd</td>
    <td>Complete weekday name (Sunday to Saturday)</td>
    <td>Tuesday</td>
  </tr>
  <tr>
    <td>M</td>
    <td>Month</td>
    <td>3</td>
  </tr>
  <tr>
    <td>MM</td>
    <td>Month with leading zero</td>
    <td>03</td>
  </tr>
  <tr>
    <td>MMM</td>
    <td>Short month name abbreviation (Jan to Dec)</td>
    <td>Mar</td>
  </tr>
  <tr>
    <td>MMMM</td>
    <td>Full month name (January to December)</td>
    <td>March</td>
  </tr>
  <tr>
    <td>yy</td>
    <td>2-digits year</td>
    <td>24</td>
  </tr>
  <tr>
    <td>yyyy</td>
    <td>full year</td>
    <td>2024</td>
  </tr>
  <tr>
    <td>h</td>
    <td>12-hour</td>
    <td>8</td>
  </tr>
  <tr>
    <td>hh</td>
    <td>12-hour with leading zero</td>
    <td>08</td>
  </tr>
  <tr>
    <td>H</td>
    <td>24-hour</td>
    <td>20</td>
  </tr>
  <tr>
    <td>HH</td>
    <td>24-hour with leading zero</td>
    <td>20</td>
  </tr>
  <tr>
    <td>m</td>
    <td>Minute</td>
    <td>5</td>
  </tr>
  <tr>
    <td>mm</td>
    <td>Minute with leading zero</td>
    <td>05</td>
  </tr>
  <tr>
    <td>s</td>
    <td>Second</td>
    <td>3</td>
  </tr>
  <tr>
    <td>ss</td>
    <td>Second with leading zero</td>
    <td>03</td>
  </tr>
  <tr>
    <td>t or a</td>
    <td>A/P</td>
    <td>P</td>
  </tr>
  <tr>
    <td>tt or aa</td>
    <td>AM/PM</td>
    <td>PM</td>
  </tr>
</table>

## 🍄 Pixel Art
Pixel Art is an Array of ones and zeroes in an Array, One represents light on and Zero represents light off. Each group of Ones and Zeroes is equals to each row on your Matrix.

For example: Dino on my 16x16 Matrix
```
[
[0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0], 
[0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 1, 1, 1], 
[0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1], 
[0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1], 
[0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0], 
[0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0], 
[1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0], 
[1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0], 
[1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0], 
[0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0], 
[0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 0], 
[0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0], 
[0, 0, 0, 0, 1, 1, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0], 
[0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0], 
[0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0], 
[0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]
]
```

Another example: On Air on my 32x8 Matrix
```
[
[0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0], 
[0, 1, 1, 1, 1, 1, 0, 0, 1, 0, 0, 0, 0, 1, 0.5, 0, 0, 1, 1, 1, 1, 0, 0, 1, 0.5, 0, 1, 1, 1, 1, 1, 1], 
[1, 0.5, 0.5, 0.5, 0.5, 0.5, 1, 0, 1, 1, 0, 0, 0, 1, 0.5, 0, 1, 0.5, 0.5, 0.5, 0.5, 1, 0, 1, 0.5, 0, 1, 0.5, 0.5, 0.5, 0.5, 1], 
[1, 0.5, 0, 0, 0, 0, 1, 0, 1, 0.5, 1, 0, 0, 1, 0.5, 0, 1, 0.5, 0, 0, 0, 1, 0, 1, 0.5, 0, 1, 0.5, 0, 0, 0, 1], 
[1, 0.5, 0, 0, 0, 0, 1, 0, 1, 0.5, 0.5, 1, 0, 1, 0.5, 0, 1, 0.5, 0, 0, 0, 1, 0, 1, 0.5, 0, 1, 1, 1, 1, 1, 1], 
[1, 0.5, 0, 0, 0, 0, 1, 0, 1, 0.5, 0, 0.5, 1, 1, 0.5, 0, 1, 1, 1, 1, 1, 1, 0, 1, 0.5, 0, 1, 0.5, 0.5, 0.5, 1, 0], 
[0, 1, 1, 1, 1, 1, 0, 0, 1, 0.5, 0, 0, 0.5, 1, 0.5, 0, 1, 0.5, 0, 0, 0, 1, 0, 1, 0.5, 0, 1, 0.5, 0, 0, 0, 1], 
[0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]
]
```

<img width="1437" height="904" alt="image" src="https://github.com/user-attachments/assets/a3984138-a460-42b2-88eb-52d8066fb057" />
I made a simple html page to create pixel art: https://pixelart.nolliergb.com/

### ⏲ Sensor Data
### Requires [Libre Hardware Monitor](https://github.com/LibreHardwareMonitor/LibreHardwareMonitor) and with Remote Web Server running.

Add your sensors from the Sensor list into Sensor Format, you can combine with normal text and Sensors for example:
```
CPU: [11th Gen Intel Core i5-11400\Load\CPU Total]  [11th Gen Intel Core i5-11400\Temperatures\Core Average]
GPU: [NVIDIA GeForce GTX 1660 SUPER\Load\GPU Core]  [NVIDIA GeForce GTX 1660 SUPER\Temperatures\GPU Core]
```

# 🔨 Build yourself
This repository contains two plugin variants, each built against a different OpenRGB plugin API:

- `API5/` — for OpenRGB 0.9+ / Pipeline / Next (Plugin API 5)
- `API4/` — for OpenRGB 1.0 RC / older builds (Plugin API 4)

Use the matching folder for the OpenRGB version you plan to run.

## Prerequisites

Before building, install:

- Git
- A Qt development environment with `qmake`
- A C++17-capable compiler
- The OpenRGB source code included in this repo under each API folder (`API4/OpenRGB` or `API5/OpenRGB`)

Recommended Qt versions:

- OpenRGB 0.9+ / API 5: Qt 6.8.x
- OpenRGB 1.0 RC / API 4: Qt 5.15.x

### Windows

- Install Qt with the MSVC toolchain and `qmake`
- Install the Visual Studio C++ build tools or a compatible compiler
- Open a Qt-enabled shell (for example, Qt 6 x64 Native Tools Command Prompt or Qt Creator)

### Linux

Install the usual Qt build dependencies, for example:

- Debian/Ubuntu:
  `sudo apt install git build-essential qtcreator qtbase5-dev qtchooser qt5-qmake qtbase5-dev-tools libusb-1.0-0-dev libhidapi-dev pkgconf libmbedtls-dev qttools5-dev-tools`

- Fedora:
  `sudo dnf install automake gcc-c++ git hidapi-devel libusbx-devel mbedtls-devel pkgconf qt5-qtbase-devel qt5-linguist`

For Qt 6-based builds, use the matching Qt 6 development packages instead of the Qt 5 ones.

## Build with Qt Creator (recommended)

1. Clone the repo with submodules:
   `git clone --recurse-submodules https://github.com/qiangqiang101/OpenRGBPixelScreenPlugin.git`
2. Open the matching project file in Qt Creator:
   - `API5/PixelScreenPlugin.pro`
   - or `API4/PixelScreenPlugin.pro`
3. Select the correct Qt kit and compiler for your target.
4. Run `qmake` and then build the project.
5. The plugin output will be generated in the build output directory for that kit.

## Build from the command line

### API 5 build example

```bash
cd API5
qmake PixelScreenPlugin.pro
make -j$(nproc)
```

### API 4 build example

```bash
cd API4
qmake PixelScreenPlugin.pro
make -j$(nproc)
```

### Windows (MSVC)

```bat
cd API5
qmake PixelScreenPlugin.pro
nmake
```

or:

```bat
cd API4
qmake PixelScreenPlugin.pro
nmake
```

## Install / use the plugin

After building, copy the generated plugin library into your OpenRGB plugins directory or use the plugin manager in OpenRGB to load it.

For Linux, the project is set up for installation into a standard OpenRGB plugin path when using `make install` with an appropriate `PREFIX` value:

```bash
make install PREFIX=/usr
```

On Windows, the plugin is typically loaded from the build output folder or copied into the OpenRGB plugin directory used by your installed OpenRGB build.

## Notes

- The project files automatically generate metadata such as version info and API version at build time.
- The plugin source uses the bundled OpenRGB SDK in the `OpenRGB/` folder inside each API directory, so you should build the matching API folder for the OpenRGB version you want to target.
- If you are unsure which API to use, choose `API5` for newer OpenRGB builds and `API4` for older/RC builds.
