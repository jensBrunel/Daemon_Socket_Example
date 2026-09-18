/**
 * @file ConfigSerializer.h
 * @brief Utilities to serialize and deserialize configuration data.
 *
 * This header declares helpers for converting configuration structures
 * to and from a persistable format (files, streams). Implementation
 * details live in the corresponding source file.
 */

#ifndef CONFIG_SERIALIZER_H
#define CONFIG_SERIALIZER_H

#include <string>
#include <vector>
#include "ConfigParser.h"

class ConfigSerializer {
public:
    /**
     * @brief Serialize a ConfigParser object to a file.
     * @param config The ConfigParser object to serialize.
     **/

     ConfigSerializer();
     explicit ConfigSerializer(const ConfigParser &config);

private:
    const std::vector<std::string> serialize();
    
    const ConfigParser& m_config;
    
};


#endif // CONFIG_SERIALIZER_H

