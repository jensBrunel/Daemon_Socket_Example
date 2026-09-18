#ifndef CONFIGPARSER_H
#define CONFIGPARSER_H

#include <string>
#include <unordered_map>
#include <vector>

#include "rapidjson/document.h"
#include <fstream>

class ConfigParser {
public:
    /**
     * @brief Construct an empty parser.
     */
    ConfigParser();

    /**
     * @brief Construct a parser for a specific .cfg file.
     * @param strPath Path to the configuration file.
     */
    explicit ConfigParser(const std::string &strPath);

    /**
     * @brief Get the stored config array as a const reference.
     * @return Reference to the config array.
     */
    const std::vector<std::string> &GetConfigArray() const;

private:
    void Load();
    void ParseValue(const rapidjson::Value &value, const std::string &strPrefix);
    static std::string ValueToString(const rapidjson::Value &value);
    static std::string Uppercase(const std::string &strText);

    std::unordered_map<std::string, std::string> m_mapValues;
    std::string m_strPath;
    std::ifstream m_stream;
    std::vector<std::string> m_configArray; // Store the Configuration array as strings
};

#endif // CONFIGPARSER_H
