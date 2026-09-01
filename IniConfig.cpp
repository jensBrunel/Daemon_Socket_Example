/**
 * @file IniConfig.cpp
 * @brief Implementation of IniConfig.
 */
#include "IniConfig.h"
#include <fstream>
#include <algorithm>
#include <cctype>

static std::string trim(const std::string &s) {
    auto start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return std::string();
    auto end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

IniConfig::IniConfig()
{
    load();
}

IniConfig::~IniConfig() = default;

std::string IniConfig::uppercase(const std::string& s)
{
    std::string out = s;
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c){ return std::toupper(c); });
    return out;
}

void IniConfig::load()
{
    const char* candidates[] = {
        "daemon_socket.ini",
        "config/daemon_socket.ini",
        "/etc/daemon_socket.ini",
        "/etc/daemon_socket/daemon_socket.ini",
        nullptr
    };

    for (const char** p = candidates; *p; ++p) {
        std::ifstream ifs(*p);
        if (!ifs.good()) continue;

        std::string line;
        while (std::getline(ifs, line)) {
            auto pos = line.find_first_of("#;");
            if (pos != std::string::npos) line = line.substr(0, pos);
            line = trim(line);
            if (line.empty()) continue;

            auto eq = line.find('=');
            if (eq == std::string::npos) continue;
            std::string key = trim(line.substr(0, eq));
            std::string val = trim(line.substr(eq + 1));
            if (val.size() >= 2 && ((val.front() == '"' && val.back() == '"') || (val.front() == '\'' && val.back() == '\''))) {
                val = val.substr(1, val.size() - 2);
            }

            values_[uppercase(key)] = val;
        }
        // stop after first readable candidate
        break;
    }
}

std::string IniConfig::get(const std::string& key, const std::string& default_value) const
{
    auto it = values_.find(uppercase(key));
    if (it != values_.end()) return it->second;
    return default_value;
}

std::string IniConfig::getSocketPath() const
{
    const std::string def = "/tmp/daemon_socket_example.sock";
    return get("SOCKET_PATH", def);
}
