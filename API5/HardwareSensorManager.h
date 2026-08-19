/*---------------------------------------------------------*\
| HardwareSensorManager.h                                   |
|                                                           |
|   Hardware Sensor Manager for OpenRGB Pixel Screen Plugin |
|   Windows: LibreHardwareMonitor HTTP REST API via curl    |
|   Linux:   /sys/class/hwmon + /proc                       |
|                                                           |
|   Uses QProcess+curl (no Qt Network module required)      |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#pragma once

#include <QObject>
#include <QProcess>
#include <QTimer>
#include <QMutex>
#include <QMutexLocker>
#include <QCoreApplication>
#include <QPointer>
#include <map>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

#ifndef _WIN32
#   include <QDir>
#   include <QFile>
#   include <QTextStream>
#endif

// ──────────────────────────────────────────────────────────────────────────────
// SensorEntry: a single sensor with a human-readable path and its last value
// ──────────────────────────────────────────────────────────────────────────────
struct SensorEntry
{
    std::string path;   // e.g. "CPU\Load\CPU Total"
    std::string value;  // e.g. "42.3 %"
};

// ──────────────────────────────────────────────────────────────────────────────
// HardwareSensorManager
// ──────────────────────────────────────────────────────────────────────────────
class HardwareSensorManager : public QObject
{
    Q_OBJECT

public:
    explicit HardwareSensorManager(QObject* parent = nullptr)
        : QObject(parent)
    {
        if (qApp)
        {
            connect(qApp, &QCoreApplication::aboutToQuit, this, &HardwareSensorManager::onAboutToQuit);
        }
    }

    ~HardwareSensorManager()
    {
        onAboutToQuit();
    }

    // Immediately stop operations and terminate active child process on shutdown/unload
    void onAboutToQuit()
    {
        is_shutting_down = true;
        if (current_proc)
        {
            current_proc->disconnect();
            if (current_proc->state() != QProcess::NotRunning)
            {
                current_proc->kill();
                current_proc->waitForFinished(300);
            }
            current_proc->deleteLater();
            current_proc = nullptr;
        }
    }

    void setLhmUrl(const std::string& url)
    {
        if (!url.empty())
        {
            lhm_url = url;
        }
    }

    std::string getLhmUrl() const
    {
        return lhm_url;
    }

    // Kick off an async sensor fetch. Emits sensorDataUpdated() on success,
    // sensorFetchError() on failure.
    void fetchSensors(const std::string& url = "")
    {
        if (is_shutting_down || (qApp && QCoreApplication::closingDown()))
            return;

#ifdef _WIN32
        fetchLHMViaCurl(url);
#else
        fetchLinux();
        emit sensorDataUpdated();
#endif
    }

    // Thread-safe: returns a snapshot of all known sensors
    std::vector<SensorEntry> getSensorList() const
    {
        QMutexLocker lock(&mutex);
        return sensor_list;
    }

    // Thread-safe: look up a sensor value by path (\ or / separators both work)
    std::string getValue(const std::string& path) const
    {
        QMutexLocker lock(&mutex);
        std::string norm = normalizePath(path);

        // 1. Direct exact match
        auto it = sensor_map.find(norm);
        if (it != sensor_map.end()) return it->second;

        // 2. Suffix match (e.g. "Load\CPU Total" or "11th Gen...\Load\CPU Total")
        std::string suffix = "\\" + norm;
        for (const auto& pair : sensor_map)
        {
            if (pair.first.length() >= suffix.length() &&
                pair.first.compare(pair.first.length() - suffix.length(), suffix.length(), suffix) == 0)
            {
                return pair.second;
            }
        }
        return "";
    }

    // Resolves a format string: replaces every [path] token with its sensor value
    std::string resolveFormat(const std::string& format) const
    {
        std::string result;
        result.reserve(format.size());
        size_t i = 0;
        while (i < format.size())
        {
            if (format[i] == '[')
            {
                size_t end = format.find(']', i + 1);
                if (end != std::string::npos)
                {
                    std::string token = format.substr(i + 1, end - i - 1);
                    std::string val   = getValue(token);
                    result += val.empty() ? ("?" + token + "?") : val;
                    i = end + 1;
                    continue;
                }
            }
            result += format[i++];
        }
        return result;
    }

signals:
    void sensorDataUpdated();
    void sensorFetchError(const QString& message);

private:
    // ── Helpers ───────────────────────────────────────────────────────────────
    static std::string normalizePath(const std::string& p)
    {
        std::string n = p;
        for (char& c : n)
            if (c == '/') c = '\\';
        return n;
    }

    void storeSensor(const std::string& path, const std::string& value)
    {
        // NOTE: caller must hold mutex
        std::string norm = normalizePath(path);
        sensor_map[norm] = value;
        for (auto& e : sensor_list)
        {
            if (normalizePath(e.path) == norm) { e.value = value; return; }
        }
        sensor_list.push_back({path, value});
    }

    std::string lhm_url = "http://127.0.0.1:8085/data.json";

#ifdef _WIN32
    // ── Windows: LibreHardwareMonitor via curl (built-in on Win10+) ──────────
    void fetchLHMViaCurl(const std::string& url = "")
    {
        if (is_shutting_down || (qApp && QCoreApplication::closingDown()))
            return;

        // Prevent spawning multiple concurrent curl processes if previous fetch is still running
        if (current_proc && current_proc->state() != QProcess::NotRunning)
            return;

        current_proc = new QProcess(this);

        connect(current_proc.data(), QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this, [this](int exit_code, QProcess::ExitStatus)
        {
            if (!current_proc) return;
            QProcess* proc = current_proc.data();
            current_proc = nullptr;
            proc->deleteLater();

            if (is_shutting_down || (qApp && QCoreApplication::closingDown()))
                return;

            if (exit_code != 0)
            {
                QString err = QString::fromLocal8Bit(proc->readAllStandardError());
                if (err.isEmpty()) err = "curl exited with code " + QString::number(exit_code);
                emit sensorFetchError(err.trimmed());
                return;
            }
            QByteArray data = proc->readAllStandardOutput();
            parseLHMJson(data);
        });

        connect(current_proc.data(), &QProcess::errorOccurred, this, [this](QProcess::ProcessError err)
        {
            if (is_shutting_down || (qApp && QCoreApplication::closingDown()))
                return;

            if (err == QProcess::FailedToStart)
            {
                if (current_proc)
                {
                    current_proc->deleteLater();
                    current_proc = nullptr;
                }
                emit sensorFetchError("curl not found — install curl or add it to PATH");
            }
        });

        std::string target_url = url.empty() ? lhm_url : url;
        if (target_url.empty()) target_url = "http://127.0.0.1:8085/data.json";

        // curl flags: silent, fail on HTTP error, 2-second timeouts
        current_proc->start("curl", QStringList()
                    << "-s" << "--fail" << "--connect-timeout" << "2" << "--max-time" << "2"
                    << QString::fromStdString(target_url));
    }

    void parseLHMJson(const QByteArray& data)
    {
        try
        {
            nlohmann::json root = nlohmann::json::parse(
                data.constData(), data.constData() + data.size());
            {
                QMutexLocker lock(&mutex);
                sensor_map.clear();
                sensor_list.clear();
                walkLHMNode(root, "", 0);
            }
            emit sensorDataUpdated();
        }
        catch (...)
        {
            if (!is_shutting_down && (!qApp || !QCoreApplication::closingDown()))
                emit sensorFetchError("Failed to parse LibreHardwareMonitor JSON");
        }
    }

    // Recursively walk the LHM data.json tree
    void walkLHMNode(const nlohmann::json& node, const std::string& parent_path, int depth = 0)
    {
        if (!node.is_object()) return;

        std::string name;
        if (node.contains("Text") && node["Text"].is_string())
            name = node["Text"].get<std::string>();

        std::string path = parent_path;

        // Skip depth 0 ("Sensor" root) and depth 1 (computer hostname) so paths start directly with hardware device
        if (depth >= 2)
        {
            path = parent_path.empty() ? name : (parent_path + "\\" + name);
        }

        if (node.contains("Value") && node["Value"].is_string())
        {
            std::string val = node["Value"].get<std::string>();
            if (!val.empty() && val != "-")
            {
                std::string store_path = path.empty() ? name : path;
                storeSensor(store_path, val);
            }
        }

        if (node.contains("Children") && node["Children"].is_array())
        {
            for (const auto& child : node["Children"])
                walkLHMNode(child, path, depth + 1);
        }
    }

#else
    // ── Linux: /sys/class/hwmon + /proc ──────────────────────────────────────
    void fetchLinux()
    {
        QMutexLocker lock(&mutex);
        sensor_map.clear();
        sensor_list.clear();
        lock.unlock();   // individual storeSensor calls re-acquire

        fetchLinuxHwmon();
        fetchLinuxCpuLoad();
        fetchLinuxMemory();
    }

    void fetchLinuxHwmon()
    {
        QDir hwmon_dir("/sys/class/hwmon");
        QStringList hwmons = hwmon_dir.entryList(
            QStringList() << "hwmon*", QDir::Dirs | QDir::NoDotAndDotDot);

        for (const QString& hw : hwmons)
        {
            QString base = "/sys/class/hwmon/" + hw + "/";
            QString chip_name = readFile(base + "name").trimmed();
            if (chip_name.isEmpty()) chip_name = hw;

            for (int i = 1; i <= 20; i++)
            {
                QString input_file = base + QString("temp%1_input").arg(i);
                if (!QFile::exists(input_file)) break;
                QString label_file = base + QString("temp%1_label").arg(i);
                QString label = QFile::exists(label_file)
                    ? readFile(label_file).trimmed()
                    : QString("temp%1").arg(i);
                bool ok; double milli = readFile(input_file).trimmed().toDouble(&ok);
                if (ok)
                {
                    std::string path = chip_name.toStdString() + "\\Temperatures\\" + label.toStdString();
                    QMutexLocker lock(&mutex);
                    storeSensor(path, (QString::number(milli / 1000.0, 'f', 1) + " °C").toStdString());
                }
            }

            for (int i = 1; i <= 10; i++)
            {
                QString input_file = base + QString("fan%1_input").arg(i);
                if (!QFile::exists(input_file)) break;
                QString label_file = base + QString("fan%1_label").arg(i);
                QString label = QFile::exists(label_file)
                    ? readFile(label_file).trimmed()
                    : QString("fan%1").arg(i);
                bool ok; int rpm = readFile(input_file).trimmed().toInt(&ok);
                if (ok)
                {
                    std::string path = chip_name.toStdString() + "\\Fans\\" + label.toStdString();
                    QMutexLocker lock(&mutex);
                    storeSensor(path, (QString::number(rpm) + " RPM").toStdString());
                }
            }
        }
    }

    void fetchLinuxCpuLoad()
    {
        static unsigned long long prev_idle = 0, prev_total = 0;
        QString stat = readFile("/proc/stat");
        for (const QString& line : stat.split('\n'))
        {
            if (!line.startsWith("cpu ")) continue;
            QStringList p = line.split(' ', Qt::SkipEmptyParts);
            if (p.size() < 5) break;
            unsigned long long user=p[1].toULongLong(), nice=p[2].toULongLong(),
                sys=p[3].toULongLong(), idle=p[4].toULongLong(),
                io=(p.size()>5)?p[5].toULongLong():0;
            unsigned long long total=user+nice+sys+idle+io;
            double load=(prev_total>0 && total>prev_total)
                ? 100.0*(1.0-(double)(idle-prev_idle)/(double)(total-prev_total))
                : 0.0;
            prev_idle=idle; prev_total=total;
            QMutexLocker lock(&mutex);
            storeSensor("CPU\\Load\\CPU Total",
                        (QString::number(load,'f',1)+" %").toStdString());
            break;
        }
    }

    void fetchLinuxMemory()
    {
        QString meminfo = readFile("/proc/meminfo");
        unsigned long long total=0, avail=0;
        for (const QString& line : meminfo.split('\n'))
        {
            QStringList p=line.split(' ',Qt::SkipEmptyParts);
            if (line.startsWith("MemTotal:") && p.size()>=2)    total=p[1].toULongLong();
            if (line.startsWith("MemAvailable:") && p.size()>=2) avail=p[1].toULongLong();
        }
        if (total>0)
        {
            double used_gb=(total-avail)/(1024.0*1024.0);
            double tot_gb=total/(1024.0*1024.0);
            double pct=100.0*(total-avail)/total;
            QMutexLocker lock(&mutex);
            storeSensor("RAM\\Load\\Memory",      (QString::number(pct,'f',1)+" %").toStdString());
            storeSensor("RAM\\Data\\Used Memory", (QString::number(used_gb,'f',2)+" GB").toStdString());
            storeSensor("RAM\\Data\\Total Memory",(QString::number(tot_gb,'f',2)+" GB").toStdString());
        }
    }

    static QString readFile(const QString& path)
    {
        QFile f(path);
        if (!f.open(QIODevice::ReadOnly|QIODevice::Text)) return "";
        return QTextStream(&f).readAll();
    }
#endif

    bool                               is_shutting_down = false;
    QPointer<QProcess>                 current_proc;
    mutable QMutex                     mutex;
    std::map<std::string, std::string> sensor_map;
    std::vector<SensorEntry>           sensor_list;
};
