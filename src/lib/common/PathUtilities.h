#pragma once

#include <string>

class PathUtilities
{
public:
    static std::string basename(const std::string& path);
    static std::string concat(const std::string& left, const std::string& right);

private:
    // static class
    PathUtilities() {}
};
