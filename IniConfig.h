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
    // Provide an optional path to a specific INI file. If empty, the loader
    // will search a set of candidate locations (including /etc/ssmm/).
    IniConfig(const std::string& ini_path = std::string());
    ~IniConfig();

    // Return the value for a key (case-insensitive). Returns empty string if missing
    std::string get(const std::string& key) const;

private:
    void load();
    static std::string uppercase(const std::string& s);

    std::unordered_map<std::string, std::string> m_mapValues; /**< map<string,string> */
    std::string m_sIniPath; /**< configured ini path (string) */
};

#endif // INI_CONFIG_H
