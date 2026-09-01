/**
 * @file IniConfig.h
 * @brief Simple INI configuration loader.
 */
#ifndef INI_CONFIG_H
#define INI_CONFIG_H

#include <string>
#include <unordered_map>

class IniConfig {
public:
    IniConfig();
    ~IniConfig();

    // Return the value for a key (case-insensitive), or default_value if missing
    std::string get(const std::string& key, const std::string& default_value = "") const;

    // Convenience: get SOCKET_PATH with fallback
    std::string getSocketPath() const;

private:
    void load();
    static std::string uppercase(const std::string& s);

    std::unordered_map<std::string, std::string> values_;
};

#endif // INI_CONFIG_H
