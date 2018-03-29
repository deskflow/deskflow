#pragma once

#include <string>

class DataDirectories
{
public:
    static const std::string& personal();
    static const std::string& personal(const std::string& path);

    static const std::string& profile();
    static const std::string& profile(const std::string& path);

private:
    // static class
    DataDirectories() {}

    static std::string _personal;
    static std::string _profile;
};