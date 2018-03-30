#include "../DataDirectories.h"

#include <Shlobj.h>

// static member
std::string DataDirectories::_personal;
std::string DataDirectories::_profile;
std::string DataDirectories::_global;

std::string unicode_to_mb(const WCHAR* utfStr)
{
    int utfLength = lstrlenW(utfStr);
    int mbLength = WideCharToMultiByte(CP_UTF8, 0, utfStr, utfLength, NULL, 0, NULL, NULL);
    std::string mbStr(mbLength, 0);
    WideCharToMultiByte(CP_UTF8, 0, utfStr, utfLength, &mbStr[0], mbLength, NULL, NULL);
    return mbStr;
}

std::string known_folder_path(const KNOWNFOLDERID& id)
{
    std::string path;
    WCHAR* buffer;
    HRESULT result = SHGetKnownFolderPath(id, 0, NULL, &buffer);
    if (result == S_OK) {
        path = unicode_to_mb(buffer);
        CoTaskMemFree(buffer);
    }
    return path;
}

const std::string& DataDirectories::personal()
{
    if (_personal.empty())
        _personal = known_folder_path(FOLDERID_Documents);
    return _personal;
}
const std::string& DataDirectories::personal(const std::string& path)
{
    _personal = path;
    return _personal;
}

const std::string& DataDirectories::profile()
{
    if (_profile.empty())
        _profile = known_folder_path(FOLDERID_LocalAppData) + "\\Barrier";
    return _profile;
}
const std::string& DataDirectories::profile(const std::string& path)
{
    _profile = path;
    return _profile;
}

const std::string& DataDirectories::global()
{
    if (_global.empty())
        _global = known_folder_path(FOLDERID_ProgramData) + "\\Barrier";
    return _global;
}
const std::string& DataDirectories::global(const std::string& path)
{
    _global = path;
    return _global;
}
