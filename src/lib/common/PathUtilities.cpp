#include "PathUtilities.h"

// keep the default platform delimiter as the first in the list
#ifdef _WIN32
static const char *Delimiters = "\\/";
#else
static const char *Delimiters = "/";
#endif

static const char DefaultDelimiter = Delimiters[0];

std::string PathUtilities::basename(const std::string& path)
{
    return path.substr(path.find_last_of(Delimiters) + 1);
}

std::string PathUtilities::concat(const std::string& left, const std::string& right)
{
    // although npos is usually (-1) we can't count on that so handle it explicitly
    auto leftEnd = left.find_last_not_of(Delimiters);
    if (leftEnd == std::string::npos)
        leftEnd = 0;
    else
        ++leftEnd;
    auto rightStart = right.find_first_not_of(Delimiters, 0);
    if (rightStart == std::string::npos) {
        // l/r empty
        if (left.size() == 0 && right.size() == 0)
            return "";
        // r useless, l okay
        if (leftEnd > 0)
            return left.substr(0, leftEnd);
        // both useless but one has a delim
        return std::string(1, DefaultDelimiter);
    }
    if (leftEnd == 0) {
        // r okay and not prefixed with delims, l empty
        if (left.size() == 0 && rightStart == 0)
            return right.substr(rightStart);
        // r okay and prefixed with delims OR l full of delims
        return DefaultDelimiter + right.substr(rightStart);
    }
    // normal concatenation
    return left.substr(0, leftEnd) + DefaultDelimiter + right.substr(rightStart);
}
