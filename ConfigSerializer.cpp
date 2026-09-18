/**
 * @file ConfigSerializer.cpp
 * @brief Implementation stubs for configuration serialization helpers.
 */
#include <iostream>
#include "ConfigSerializer.h"

// TODO: Implement serialization and deserialization helpers declared
// in ConfigSerializer.h. This file intentionally provides a neutral
// starting point for adding concrete functionality.

/// @brief
/// @param config
ConfigSerializer::ConfigSerializer(const ConfigParser &config) : m_config(config)
{
    serialize();
}

bool ConfigSerializer::wasSerialized() const
{
    return m_serializeSuccess;
}

const std::vector<std::string> ConfigSerializer::serialize()
{
    auto configArray = m_config.GetConfigArray();
    for (const auto &entry : configArray)
    {
        std::cout << "Config entry: " << entry << std::endl;
    }
    m_serializeSuccess = !configArray.empty();
    return configArray;
}

bool ConfigSerializer::writeConfigToTextFile(const std::string &filePath) const
{
    std::ofstream out(filePath, std::ios::out | std::ios::trunc);

    if (!out.is_open())
    {
        return false;
    }

    for (const auto &entry : m_config.GetConfigArray())
    {
        out << entry << '\n';
    }

    out.close();
    return out.good();
}