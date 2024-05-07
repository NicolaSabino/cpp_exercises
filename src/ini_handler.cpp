/**
 * @file ini_handler.cpp
 * @author Nicola Sabino (nicolasabino94@gmail.com)
 * @brief ini handler shared library implementation
 * @version 0.1
 * @date 2024-05-07
 */
#include "ini_handler.h"
#include <fstream>
#include <iostream>
#include <map>
#include <tuple>

// Global variable to hold the key-value pairs from the INI file
std::map<std::string, std::map<std::string, std::string>> ini_data;
std::string ini_file_path;

/**
 * @brief Trim string
 * 
 * Utility function
 * Remove tabs spaces and new line at 
 * string beginning and end
 * 
 * @param targetString The target string to be trimmed.
 */
void trimString(std::string &targetString)
{
    targetString.erase(0, targetString.find_first_not_of(" \t"));
    targetString.erase(targetString.find_last_not_of(" \t") + 1);
    targetString.erase(targetString.find_last_not_of("\n") + 1);
}

/**
 * @brief Split header
 * 
 * Utility function
 * Split a ini header into section and key
 * 
 * The section is always the first sequence of char 
 * preceeding the first dot symbol.
 * The key is the resto of char sequence, including dots symbols.
 * 
 * @param key The key, a.k.a header, to be processed.
 * @return std::tuple<std::string, std::string> a touple containing section and key.
 */
std::tuple<std::string, std::string> splitHeader(const std::string &key)
{
    // Find the last occurrence of the delimiter '.'
    size_t pos = key.find_first_of('.');

    // If delimiter not found, return the original string,
    // an empty string and raise a warning
    if (pos == std::string::npos)
    {
        std::cout << "WRN: corrupted header " << key << std::endl;
        return std::make_pair(key, "");
    }

    // Split the string into two parts
    std::string first_part = key.substr(0, pos);
    std::string second_part = key.substr(pos + 1);

    return {first_part, second_part};
}

unsigned short load_resource(const std::string &path)
{
    std::string processedPath{path};
    trimString(processedPath);
    std::ifstream file(processedPath);
    if (!file.is_open())
    {
        std::cerr << "ERR: Unable to open file " << processedPath << std::endl;
        return 1;
    }
    else
    {
        ini_file_path = processedPath;
    }

    std::string current_section;
    std::string line;

    while (getline(file, line))
    {
        // skip comments
        if (line.empty() || line[0] == ';')
            continue;

        // Check if line is a section header
        if (line[0] == '[' && line.back() == ']')
        {
            current_section = line.substr(1, line.size() - 2);
            continue;
        }

        // If not header is key value section
        // Split line into key and value
        size_t pos = line.find('=');
        if (pos != std::string::npos)
        {
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);

            // Remove leading and trailing whitespace
            trimString(key);
            trimString(value);

            // Store key-value pair in nested unordered_map
            ini_data[current_section][key] = value;
        }
    }

    file.close();
    std::cout << "INF: file " << path << " successfully loaded" << std::endl;
    return 0;
}

unsigned short get_value(const std::string &key, std::string &value)
{
    std::string processedKey{key};
    trimString(processedKey);

    // Check if a resource file has been loaded
    if (ini_data.empty())
    {
        std::cerr << "ERR: No resource file has been loaded yet" << std::endl;
        return 4; // Resource file not loaded
    }

    std::string iniSection, iniKey;
    std::tie(iniSection, iniKey) = splitHeader(processedKey);

    // find section
    auto sectionPositionIt = ini_data.find(iniSection);
    if (sectionPositionIt == ini_data.end())
    {
        std::cerr << "ERR: Section '" << iniSection << "' not found in the resource file" << std::endl;
        return 3;
    }

    // find key
    const auto &keyValueMap = sectionPositionIt->second;
    auto keyPositionIt = keyValueMap.find(iniKey);
    if (keyPositionIt == keyValueMap.end())
    {
        std::cerr << "ERR: Key '" << key << "' not found in the resource file" << std::endl;
        return 3;
    }

    // Retrieve the value
    value = keyPositionIt->second;

    // Return success
    return 0;
}

unsigned short set_value(const std::string &key, const std::string &value)
{
    std::string processedKey{key};
    trimString(processedKey);

    // Check if a resource file has been loaded
    if (ini_data.empty())
    {
        std::cerr << "ERR: No resource file has been loaded yet" << std::endl;
        return 4; // Resource file not loaded
    }

    std::string iniSection, iniKey;
    std::tie(iniSection, iniKey) = splitHeader(processedKey);

    // update map
    ini_data[iniSection][iniKey] = value;

    return dump_values();
}

unsigned short delete_value(const std::string &key)
{
    std::string processedKey{key};
    trimString(processedKey);

    // Check if a resource file has been loaded
    if (ini_data.empty())
    {
        std::cerr << "ERR: No resource file has been loaded yet" << std::endl;
        return 4; // Resource file not loaded
    }

    std::string iniSection, iniKey;
    std::tie(iniSection, iniKey) = splitHeader(processedKey);

    // Find section
    auto section_it = ini_data.find(iniSection);
    if (section_it != ini_data.end())
    {
        // Find key within section
        auto key_it = section_it->second.find(iniKey);
        if (key_it != section_it->second.end())
        {
            // Delete key-value pair
            section_it->second.erase(key_it);
        }
        else
        {
            std::cerr << "ERR: Key '" << iniKey << "' not found in section '" << iniSection << "'" << std::endl;
            return 3; // Key not found
        }

        // if section is empty erese section
        if (section_it->second.empty())
        {
            ini_data.erase(section_it);
        }
    }
    else
    {
        std::cerr << "ERR: Section '" << iniSection << "' not found" << std::endl;
        return 3; // Section not found
    }

    return dump_values();
}

/**
 * @brief Dump values into file
 * 
 * @return 0 if success
 * @return 255 if not able to open the file
 */
unsigned short dump_values()
{
    // Update the INI file on the filesystem
    std::ofstream file(ini_file_path);
    if (!file.is_open())
    {
        std::cerr << "ERR: Unable to open file " << ini_file_path << " for writing" << std::endl;
        return 255;
    }

    // Iterate over each section
    for (const auto &sectionPair : ini_data)
    {

        const std::string &iniSection = sectionPair.first;
        const auto &sectionMap = sectionPair.second;

        // save section
        file << "[" << iniSection << "]" << std::endl;

        // Iterate over each key-value pair within the section
        for (const auto &keyValuePair : sectionMap)
        {

            const std::string &iniKey = keyValuePair.first;
            const std::string &value = keyValuePair.second;

            // save key, and value
            file << iniKey << " = " << value << std::endl;
        }
        // new line between each section
        file << std::endl;
    }

    file.close();
    return 0;
}
