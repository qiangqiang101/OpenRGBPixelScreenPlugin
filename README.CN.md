# OpenRGB像素屏插件
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

[English README](README.md)

<img width="870" height="621" alt="image" src="https://github.com/user-attachments/assets/1f94cb0d-d0f7-4d63-8864-2c253d62358f" />
<img width="870" height="621" alt="image" src="https://github.com/user-attachments/assets/b7f89322-ac52-4860-b4c5-aa84c43c4ebc" />

在 OpenRGB 像素屏上渲染自定义滚动文字、时钟、硬件传感器和像素艺术。

# 🛠️ 下载
## OpenRGB 1.0（插件 API 版本 5）
- [Windows 64](https://github.com/qiangqiang101/OpenRGBPixelScreenPlugin/releases)
- [Linux amd64](https://github.com/qiangqiang101/OpenRGBPixelScreenPlugin/releases)
- [Linux arm64](https://github.com/qiangqiang101/OpenRGBPixelScreenPlugin/releases)
- [Linux armhf](https://github.com/qiangqiang101/OpenRGBPixelScreenPlugin/releases)
- [Linux i386](https://github.com/qiangqiang101/OpenRGBPixelScreenPlugin/releases)

## OpenRGB 1.0 RC - 1.0 RC3（插件 API 版本 4）
- [Windows 86](https://github.com/qiangqiang101/OpenRGBPixelScreenPlugin/releases)
- [Windows 64](https://github.com/qiangqiang101/OpenRGBPixelScreenPlugin/releases)
- [Linux amd64](https://github.com/qiangqiang101/OpenRGBPixelScreenPlugin/releases)
- [Linux arm64](https://github.com/qiangqiang101/OpenRGBPixelScreenPlugin/releases)
- [Linux armhf](https://github.com/qiangqiang101/OpenRGBPixelScreenPlugin/releases)
- [Linux i386](https://github.com/qiangqiang101/OpenRGBPixelScreenPlugin/releases)

你可以在这里获取旧版本发布包：[GitHub Releases](https://github.com/qiangqiang101/OpenRGBPixelScreenPlugin/releases)。

# ⚙️ 设置
## 🕔 时间 / 时钟格式
配置支持标准的日期/时间格式标记，例如 `hh:mm:ss aa`。

<table>
  <tr>
    <td><b>格式</b></td>
    <td><b>说明</b></td>
    <td><b>示例</b></td>
  </tr>
  <tr>
    <td>d</td>
    <td>日期</td>
    <td>1</td>
  </tr>
  <tr>
    <td>dd</td>
    <td>补零日期</td>
    <td>01</td>
  </tr>
  <tr>
    <td>ddd</td>
    <td>星期缩写（Sun 到 Sat）</td>
    <td>Tue</td>
  </tr>
  <tr>
    <td>dddd</td>
    <td>完整星期名称（Sunday 到 Saturday）</td>
    <td>Tuesday</td>
  </tr>
  <tr>
    <td>M</td>
    <td>月份</td>
    <td>3</td>
  </tr>
  <tr>
    <td>MM</td>
    <td>补零月份</td>
    <td>03</td>
  </tr>
  <tr>
    <td>MMM</td>
    <td>月份缩写（Jan 到 Dec）</td>
    <td>Mar</td>
  </tr>
  <tr>
    <td>MMMM</td>
    <td>完整月份名称（January 到 December）</td>
    <td>March</td>
  </tr>
  <tr>
    <td>yy</td>
    <td>两位数年份</td>
    <td>24</td>
  </tr>
  <tr>
    <td>yyyy</td>
    <td>四位数完整年份</td>
    <td>2024</td>
  </tr>
  <tr>
    <td>h</td>
    <td>12小时制</td>
    <td>8</td>
  </tr>
  <tr>
    <td>hh</td>
    <td>12小时制（补零）</td>
    <td>08</td>
  </tr>
  <tr>
    <td>H</td>
    <td>24小时制</td>
    <td>20</td>
  </tr>
  <tr>
    <td>HH</td>
    <td>24小时制（补零）</td>
    <td>20</td>
  </tr>
  <tr>
    <td>m</td>
    <td>分钟</td>
    <td>5</td>
  </tr>
  <tr>
    <td>mm</td>
    <td>分钟（补零）</td>
    <td>05</td>
  </tr>
  <tr>
    <td>s</td>
    <td>秒</td>
    <td>3</td>
  </tr>
  <tr>
    <td>ss</td>
    <td>秒（补零）</td>
    <td>03</td>
  </tr>
  <tr>
    <td>t 或 a</td>
    <td>A/P 标识</td>
    <td>P</td>
  </tr>
  <tr>
    <td>tt 或 aa</td>
    <td>AM/PM 标识</td>
    <td>PM</td>
  </tr>
</table>

## 🍄 像素艺术 (Pixel Art)
像素艺术是用包含 1 和 0 的二维数组来表示的，1 代表发光，0 代表熄灭。每个包含 1 和 0 的子数组对应矩阵上的每一行。

例如：在我的 16x16 矩阵上的恐龙图案
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

另一个示例：在我的 32x8 矩阵上的 On Air 图案
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
我制作了一个简单的 HTML 页面用来在线绘制像素艺术：https://pixelart.nolliergb.com/

### ⏲ 硬件传感器数据
### 需要配合 [Libre Hardware Monitor](https://github.com/LibreHardwareMonitor/LibreHardwareMonitor) 运行，并开启其远程 Web 服务器（Remote Web Server）。

在传感器列表中选择你的传感器并添加至传感器格式中，你还可以结合普通文本和传感器，例如：
```
CPU: [11th Gen Intel Core i5-11400\Load\CPU Total]  [11th Gen Intel Core i5-11400\Temperatures\Core Average]
GPU: [NVIDIA GeForce GTX 1660 SUPER\Load\GPU Core]  [NVIDIA GeForce GTX 1660 SUPER\Temperatures\GPU Core]
```

# 🔨 自行编译

这个仓库包含两个插件版本，每个版本都对应不同的 OpenRGB 插件 API：

- `API5/` — 适用于 OpenRGB 0.9+ / Pipeline / Next（插件 API 5）
- `API4/` — 适用于 OpenRGB 1.0 RC / 更早版本（插件 API 4）

请根据你打算运行的 OpenRGB 版本使用对应的目录。

## 先决条件

在编译之前，请安装：

- Git
- 带有 `qmake` 的 Qt 开发环境
- 支持 C++17 的编译器
- 本仓库中各 API 目录下自带的 OpenRGB 源代码（`API4/OpenRGB` 或 `API5/OpenRGB`）

推荐的 Qt 版本：

- OpenRGB 0.9+ / API 5：Qt 6.8.x
- OpenRGB 1.0 RC / API 4：Qt 5.15.x

### Windows

- 安装带有 MSVC 工具链和 `qmake` 的 Qt
- 安装 Visual Studio C++ 构建工具或兼容的编译器
- 打开 Qt 环境命令行（例如 Qt 6 x64 Native Tools Command Prompt 或 Qt Creator）

### Linux

安装常见的 Qt 构建依赖，例如：

- Debian/Ubuntu：
  `sudo apt install git build-essential qtcreator qtbase5-dev qtchooser qt5-qmake qtbase5-dev-tools libusb-1.0-0-dev libhidapi-dev pkgconf libmbedtls-dev qttools5-dev-tools`

- Fedora：
  `sudo dnf install automake gcc-c++ git hidapi-devel libusbx-devel mbedtls-devel pkgconf qt5-qtbase-devel qt5-linguist`

如果是基于 Qt 6 的构建，请改用对应的 Qt 6 开发包，而不是 Qt 5 包。

## 使用 Qt Creator 构建（推荐）

1. 使用子模块克隆仓库：
   `git clone --recurse-submodules https://github.com/qiangqiang101/OpenRGBPixelScreenPlugin.git`
2. 在 Qt Creator 中打开对应的项目文件：
   - `API5/PixelScreenPlugin.pro`
   - 或 `API4/PixelScreenPlugin.pro`
3. 选择目标所需的 Qt 套件和编译器。
4. 运行 `qmake`，然后编译项目。
5. 插件输出会生成在该套件对应的构建输出目录中。

## 使用命令行构建

### API 5 构建示例

```bash
cd API5
qmake PixelScreenPlugin.pro
make -j$(nproc)
```

### API 4 构建示例

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

或者：

```bat
cd API4
qmake PixelScreenPlugin.pro
nmake
```

## 安装 / 使用插件

编译完成后，将生成的插件库复制到你的 OpenRGB 插件目录中，或者使用 OpenRGB 中的插件管理器加载它。

在 Linux 上，如果使用适当的 `PREFIX` 值并执行 `make install`，该项目会安装到标准的 OpenRGB 插件路径中：

```bash
make install PREFIX=/usr
```

在 Windows 上，通常需要将插件加载到生成的构建输出目录中，或者复制到已安装 OpenRGB 使用的插件目录。

## 说明

- 项目文件会在编译时自动生成版本信息和 API 版本等元数据。
- 插件源码使用各自 API 目录中内置的 OpenRGB SDK（`OpenRGB/` 文件夹），因此应针对你想要运行的 OpenRGB 版本编译相应的 API 目录。
- 如果你不确定应该使用哪个 API，较新的 OpenRGB 构建请优先选择 `API5`，较旧的 RC 版本则使用 `API4`。
