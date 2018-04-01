#pragma once

#include <string>

class DataDirectories
{
public:
    static const std::string& personal();
    static const std::string& personal(const std::string& path);

    static const std::string& profile();
    static const std::string& profile(const std::string& path);

    static const std::string& global();
    static const std::string& global(const std::string& path);

    static const std::string& systemconfig();
    static const std::string& systemconfig(const std::string& path);

private:
    // static class
    DataDirectories() {}

    static std::string _personal;
    static std::string _profile;
    static std::string _global;
    static std::string _systemconfig;
};
