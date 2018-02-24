#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

class WindowsHookResource
{
public:
    explicit WindowsHookResource();
    ~WindowsHookResource();

    bool set(int idHook, HOOKPROC lpfn, HINSTANCE hmod, DWORD dwThreadId);
    bool unset();

    bool is_set() const;
    operator HHOOK() const;

private:
    HHOOK _hook;
};