#include "MSWindowsHookResource.h"

WindowsHookResource::WindowsHookResource() :
    _hook(NULL)
{
}

WindowsHookResource::~WindowsHookResource()
{
    unset();
}

bool WindowsHookResource::set(int idHook, HOOKPROC lpfn, HINSTANCE hmod, DWORD dwThreadId)
{
    if (is_set())
        return false;
    _hook = SetWindowsHookEx(idHook, lpfn, hmod, dwThreadId);
    return is_set();
}

bool WindowsHookResource::unset()
{
    if (is_set()) {
        if (UnhookWindowsHookEx(_hook) == 0) {
            return false;
        }
        _hook = NULL;
    }
    return true;
}

bool WindowsHookResource::is_set() const { return _hook != NULL; }
WindowsHookResource::operator HHOOK() const { return _hook; }