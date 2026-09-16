#include "ConfigParser.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <iostream>

#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include "rapidjson/istreamwrapper.h"
#include "quickdigest5.h"


namespace {
std::string JsonValueToString(const rapidjson::Value &value) {
    if (value.IsString()) {
        return value.GetString();
    }
    if (value.IsBool()) {
        return value.GetBool() ? "true" : "false";
    }
    if (value.IsInt()) {
        return std::to_string(value.GetInt());
    }
    if (value.IsUint()) {
        return std::to_string(value.GetUint());
    }
    if (value.IsInt64()) {
        return std::to_string(value.GetInt64());
    }
    if (value.IsUint64()) {
        return std::to_string(value.GetUint64());
    }
    if (value.IsDouble()) {
        return std::to_string(value.GetDouble());
    }
    if (value.IsNull()) {
        return "";
    }
    if (value.IsArray()) {
        std::ostringstream stream;
        for (rapidjson::SizeType i = 0; i < value.Size(); ++i) {
            if (i > 0) {
                stream << ",";
            }
            stream << JsonValueToString(value[i]);
        }
        return stream.str();
    }
    return "";
}
}

std::string ConfigParser::Uppercase(const std::string &strText) {
    std::string strResult = strText;
    std::transform(strResult.begin(), strResult.end(), strResult.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::toupper(ch)); });
    return strResult;
}

ConfigParser::ConfigParser() : m_strPath() {
}

ConfigParser::ConfigParser(const std::string &strPath) : m_strPath(strPath) {
    Load();
}

bool ConfigParser::Open(const std::string &strPath) {
    m_strPath = strPath;
    m_mapValues.clear();
    std::cout << "Opening config file: " << m_strPath << std::endl;
    Load();
    return !m_strPath.empty();
}

bool ConfigParser::IsOpen() const {
    return !m_strPath.empty();
}

const std::string &ConfigParser::GetPath() const {
    return m_strPath;
}

void ConfigParser::ParseValue(const rapidjson::Value &value, const std::string &strPrefix) {
    if (value.IsObject()) {
        for (auto it = value.MemberBegin(); it != value.MemberEnd(); ++it) {
            const std::string strKey = strPrefix.empty() ? it->name.GetString() : strPrefix + "." + it->name.GetString();
            ParseValue(it->value, strKey);
        }
        return;
    }

    if (strPrefix.empty()) {
        return;
    }

    m_mapValues[Uppercase(strPrefix)] = JsonValueToString(value);
}

void ConfigParser::Load() {
    std::stringstream inputBuffer;
    std::string inputLine;
    std::cout << "Loading config file: " << m_strPath << std::endl;
    if (m_stream.is_open()) {
        m_stream.close();
        std::cout << "Closed previous config file stream." << std::endl;
    }
    m_stream.clear();
    m_stream.open(m_strPath);
    if (!m_stream.is_open()) {
        std::cerr << "Failed to open config file: " << m_strPath << std::endl;
        return;
    }

    while (std::getline(m_stream, inputLine))
    {
        inputBuffer << inputLine << "\n";
    }
    //std::cout << "Read config file content:\n" << inputBuffer.str() << std::endl;
    rapidjson::Document document;
    document.Parse(inputBuffer.str().c_str());
    if (document.HasParseError()) {
        std::cerr << "ConfigParser: JSON parse error: "
                  << rapidjson::GetParseErrorFunc(document.GetParseError())
                  << " at offset " << document.GetErrorOffset() << std::endl;
        return;
    }

    if (!document.IsObject()) {
        std::cerr << "ConfigParser: JSON root is not an object in '" << m_strPath << "'" << std::endl;
        return;
    }

    rapidjson::Value::ConstMemberIterator configIter = document.FindMember("Configuration");
    if (configIter != document.MemberEnd() && configIter->value.IsArray()) {
        for (const auto& configParameter : configIter->value.GetArray()) {
            if (configParameter.IsObject()) {
                //const auto cmdIt = configParameter.FindMember("command");
                //std::cout << "Command: " << cmdIt->value.GetString() << std::endl;
            }
        }
    }
    const rapidjson::Value& configArray = document["Configuration"];
    rapidjson::StringBuffer configBuffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(configBuffer);
    configArray.Accept(writer);
    std::string configAsString = configBuffer.GetString();

    auto hash = QuickDigest5::toHash(configAsString); // Compute the checksum of the JSON config array string
    //std::cout << "Checksum of Configuration array: " << hash << std::endl;
    const rapidjson::Value& checksumValue = document["CHECKSUM"];
    if (checksumValue.IsString()) {
        std::string checksumInFile = checksumValue.GetString();
        //std::cout << "Checksum in file: " << checksumInFile << std::endl;
        if (hash != checksumInFile) {
            std::cerr << "Checksum mismatch! The Configuration array may have been tampered with." << std::endl;
        } else {
            //std::cout << "Checksum matches. The Configuration array is valid." << std::endl;
        }
    } else {
        std::cerr << "CHECKSUM field is missing or not a string in the config file." << std::endl;
    }

    m_mapValues.clear();
    ParseValue(document, "");
}

std::string ConfigParser::GetValue(const std::string &strKey) const {
    const auto it = m_mapValues.find(Uppercase(strKey));
    if (it == m_mapValues.end()) {
        return "";
    }
    return it->second;
}

std::string ConfigParser::GetValue(const std::string &arrayKey, const std::string &strKey) const {
    const auto it = m_mapValues.find(Uppercase(arrayKey + "." + strKey));
    if (it == m_mapValues.end()) {
        return "";
    }
    return it->second;
}

bool ConfigParser::HasKey(const std::string &strKey) const {
    return m_mapValues.find(Uppercase(strKey)) != m_mapValues.end();
}
