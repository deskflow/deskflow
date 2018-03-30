#include "../DataDirectories.h"

#include <unistd.h>    // sysconf
#include <sys/types.h> // getpwuid(_r)
#include <pwd.h>       // getpwuid(_r)

#ifdef WINAPI_XWINDOWS
const std::string ProfileSubdir = "/.barrier";
#else // macos
const std::string ProfileSubdir = "/Library/Application Support/Barrier";
#endif

// static members
std::string DataDirectories::_personal;
std::string DataDirectories::_profile;
std::string DataDirectories::_global;

static std::string pw_dir(struct passwd* pwentp)
{
    if (pwentp != NULL && pwentp->pw_dir != NULL)
        return pwentp->pw_dir;
    return "";
}

#ifdef HAVE_GETPWUID_R

static std::string unix_home()
{
    long size = -1;
#if defined(_SC_GETPW_R_SIZE_MAX)
    size = sysconf(_SC_GETPW_R_SIZE_MAX);
#endif
    if (size == -1)
        size = BUFSIZ;

    struct passwd pwent;
    struct passwd* pwentp;
    std::string buffer(size, 0);
    getpwuid_r(getuid(), &pwent, &buffer[0], size, &pwentp);
    return pw_dir(pwentp);
}

#else // not HAVE_GETPWUID_R

static std::string unix_home()
{
    return pw_dir(getpwuid(getuid()));
}

#endif // HAVE_GETPWUID_R

const std::string& DataDirectories::personal()
{
    if (_personal.empty())
        _personal = unix_home();
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
        _profile = personal() + ProfileSubdir;
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
        // TODO: where on a unix system should public/global shared data go?
        // as of march 2018 global() is not used for unix
        _global = "/tmp";
    return _global;
}
const std::string& DataDirectories::global(const std::string& path)
{
    _global = path;
    return _global;
}

