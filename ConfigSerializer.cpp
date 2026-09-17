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

const std::vector<std::string> ConfigSerializer::serialize()
{
    auto configArray = m_config.GetConfigArray();
    for (const auto &entry : configArray)
    {
        std::cout << "Config entry: " << entry << std::endl;
    }
    return configArray;
}
