/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * 
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 * USA.
 */

// -=- CleanDesktop.cxx

#include <windows.h>
#include <wininet.h>
#include <shlobj.h>
#include <rfb_win32/CleanDesktop.h>
#include <rfb_win32/CurrentUser.h>
#include <rfb_win32/Registry.h>
#include <rfb_win32/OSVersion.h>
#include <rfb/LogWriter.h>
#include <rdr/Exception.h>
#include <set>

#ifdef SPI_GETUIEFFECTS
#define RFB_HAVE_SPI_UIEFFECTS
#else
#pragma message("  NOTE: Not building Get/Set UI Effects support.")
#endif

#pragma warning(disable: 4018)

using namespace rfb;
using namespace rfb::win32;

static LogWriter vlog("CleanDesktop");


struct ActiveDesktop {
  ActiveDesktop() : handle(0) {
    // - Contact Active Desktop
    HRESULT result = CoCreateInstance(CLSID_ActiveDesktop, NULL, CLSCTX_INPROC_SERVER,
                                      IID_IActiveDesktop, (PVOID*)&handle);
    if (result != S_OK)
      throw rdr::SystemException("failed to contact Active Desktop", result);
  }
  ~ActiveDesktop() {
    if (handle)
      handle->Release();
  }

  // enableItem
  //   enables or disables the Nth Active Desktop item
  bool enableItem(int i, bool enable_) {
    COMPONENT item;
    memset(&item, 0, sizeof(item));
    item.dwSize = sizeof(item);

    HRESULT hr = handle->GetDesktopItem(i, &item, 0);
    if (hr != S_OK) {
      vlog.error("unable to GetDesktopItem %d: %ld", i, hr);
      return false;
    }
    item.fChecked = enable_;
    vlog.debug("%sbling %d: \"%s\"", enable_ ? "ena" : "disa", i, (const char*)CStr(item.wszFriendlyName));

    hr = handle->ModifyDesktopItem(&item, COMP_ELEM_CHECKED);
    return hr == S_OK;
  }
  
  // enable
  //   Attempts to enable/disable Active Desktop, returns true if the setting changed,
  //   false otherwise.
  //   If Active Desktop *can* be enabled/disabled then that is done.
  //   If Active Desktop is always on (XP/2K3) then instead the individual items are
  //   disabled, and true is returned to indicate that they need to be restored later.
  bool enable(bool enable_) {
    bool modifyComponents = false;

    vlog.debug("ActiveDesktop::enable");

    // - Firstly, try to disable Active Desktop entirely
    HRESULT hr;
    COMPONENTSOPT adOptions;
    memset(&adOptions, 0, sizeof(adOptions));
    adOptions.dwSize = sizeof(adOptions);

    // Attempt to actually disable/enable AD
    hr = handle->GetDesktopItemOptions(&adOptions, 0);
    if (hr == S_OK) {
      // If Active Desktop is already in the desired state then return false (no change)
      // NB: If AD is enabled AND restoreItems is set then we regard it as disabled...
      if (((adOptions.fActiveDesktop==0) && restoreItems.empty()) == (enable_==false))
        return false;
      adOptions.fActiveDesktop = enable_;
      hr = handle->SetDesktopItemOptions(&adOptions, 0);
    }
    // Apply the change, then test whether it actually took effect
    if (hr == S_OK)
      hr = handle->ApplyChanges(AD_APPLY_REFRESH);
    if (hr == S_OK)
      hr = handle->GetDesktopItemOptions(&adOptions, 0);
    if (hr == S_OK)
      modifyComponents = (adOptions.fActiveDesktop==0) != (enable_==false);
    if (hr != S_OK) {
      vlog.error("failed to get/set Active Desktop options: %ld", hr);
      return false;
    }

    if (enable_) {
      // - We are re-enabling Active Desktop.  If there are components in restoreItems
      //   then restore them!
      std::set<int>::const_iterator i;
      for (i=restoreItems.begin(); i!=restoreItems.end(); i++) {
        enableItem(*i, true);
      }
      restoreItems.clear();
    } else if (modifyComponents) {
      // - Disable all currently enabled items, and add the disabled ones to restoreItems
      int itemCount = 0;
      hr = handle->GetDesktopItemCount(&itemCount, 0);
      if (hr != S_OK) {
        vlog.error("failed to get desktop item count: %ld", hr);
        return false;
      }
      for (unsigned int i=0; i<itemCount; i++) {
        if (enableItem(i, false))
          restoreItems.insert(i);
      }
    }

    // - Apply whatever changes we have made, but DON'T save them!
    hr = handle->ApplyChanges(AD_APPLY_REFRESH);
    return hr == S_OK;
  }
  IActiveDesktop* handle;
  std::set<int> restoreItems;
};


DWORD SysParamsInfo(UINT action, UINT param, PVOID ptr, UINT ini) {
  DWORD r = ERROR_SUCCESS;
  if (!SystemParametersInfo(action, param, ptr, ini)) {
    r = GetLastError();
    vlog.info("SPI error: %d", r);
  }
  return r;
}


CleanDesktop::CleanDesktop() : restoreActiveDesktop(false), restoreWallpaper(false),
                               restorePattern(false), restoreEffects(false) {
  CoInitialize(0);
}

CleanDesktop::~CleanDesktop() {
  enableEffects();
  enablePattern();
  enableWallpaper();
  CoUninitialize();
}

void CleanDesktop::disableWallpaper() {
  try {
    ImpersonateCurrentUser icu;

    vlog.debug("disable desktop wallpaper/Active Desktop");

    // -=- First attempt to remove the wallpaper using Active Desktop
    try {
      ActiveDesktop ad;
      if (ad.enable(false))
        restoreActiveDesktop = true;
    } catch (rdr::Exception& e) {
      vlog.error(e.str());
    }

    // -=- Switch of normal wallpaper and notify apps
    SysParamsInfo(SPI_SETDESKWALLPAPER, 0, "", SPIF_SENDCHANGE);
    restoreWallpaper = true;

  } catch (rdr::Exception& e) {
    vlog.info(e.str());
  }
}

void CleanDesktop::enableWallpaper() {
  try {
    ImpersonateCurrentUser icu;

    if (restoreActiveDesktop) {
      vlog.debug("restore Active Desktop");

      // -=- First attempt to re-enable Active Desktop
      try {
        ActiveDesktop ad;
        ad.enable(true);
        restoreActiveDesktop = false;
      } catch (rdr::Exception& e) {
        vlog.error(e.str());
      }
    }

    if (restoreWallpaper) {
      vlog.debug("restore desktop wallpaper");

      // -=- Then restore the standard wallpaper if required
	    SysParamsInfo(SPI_SETDESKWALLPAPER, 0, NULL, SPIF_SENDCHANGE);
      restoreWallpaper = false;
    }

  } catch (rdr::Exception& e) {
    vlog.info(e.str());
  }
}


void CleanDesktop::disablePattern() {
  try {
    ImpersonateCurrentUser icu;

    vlog.debug("disable desktop pattern");
    SysParamsInfo(SPI_SETDESKPATTERN, 0, "", SPIF_SENDCHANGE);
    restorePattern = true;

  } catch (rdr::Exception& e) {
    vlog.info(e.str());
  }
}

void CleanDesktop::enablePattern() {
  try {
    if (restorePattern) {
      ImpersonateCurrentUser icu;

      vlog.debug("restoring pattern...");

      TCharArray pattern;
      if (osVersion.isPlatformWindows) {
        RegKey cfgKey;
        cfgKey.openKey(HKEY_CURRENT_USER, _T("Control Panel\\Desktop"));
        pattern.buf = cfgKey.getString(_T("Pattern"));
      }
      SysParamsInfo(SPI_SETDESKPATTERN, 0, pattern.buf, SPIF_SENDCHANGE);
      restorePattern = false;
    }

  } catch (rdr::Exception& e) {
    vlog.info(e.str());
  }
}


void CleanDesktop::disableEffects() {
  try {
    ImpersonateCurrentUser icu;

    vlog.debug("disable desktop effects");

    SysParamsInfo(SPI_SETFONTSMOOTHING, FALSE, 0, SPIF_SENDCHANGE);
#ifdef RFB_HAVE_SPI_UIEFFECTS
    if (SysParamsInfo(SPI_GETUIEFFECTS, 0, &uiEffects, 0) == ERROR_CALL_NOT_IMPLEMENTED) {
      SysParamsInfo(SPI_GETCOMBOBOXANIMATION, 0, &comboBoxAnim, 0);
      SysParamsInfo(SPI_GETGRADIENTCAPTIONS, 0, &gradientCaptions, 0);
      SysParamsInfo(SPI_GETHOTTRACKING, 0, &hotTracking, 0);
      SysParamsInfo(SPI_GETLISTBOXSMOOTHSCROLLING, 0, &listBoxSmoothScroll, 0);
      SysParamsInfo(SPI_GETMENUANIMATION, 0, &menuAnim, 0);
      SysParamsInfo(SPI_SETCOMBOBOXANIMATION, 0, FALSE, SPIF_SENDCHANGE);
      SysParamsInfo(SPI_SETGRADIENTCAPTIONS, 0, FALSE, SPIF_SENDCHANGE);
      SysParamsInfo(SPI_SETHOTTRACKING, 0, FALSE, SPIF_SENDCHANGE);
      SysParamsInfo(SPI_SETLISTBOXSMOOTHSCROLLING, 0, FALSE, SPIF_SENDCHANGE);
      SysParamsInfo(SPI_SETMENUANIMATION, 0, FALSE, SPIF_SENDCHANGE);
    } else {
      SysParamsInfo(SPI_SETUIEFFECTS, 0, FALSE, SPIF_SENDCHANGE);

      // We *always* restore UI effects overall, since there is no Windows GUI to do it
      uiEffects = TRUE;
    }
#else
    vlog.debug("  not supported");
#endif
    restoreEffects = true;

  } catch (rdr::Exception& e) {
    vlog.info(e.str());
  }
}

void CleanDesktop::enableEffects() {
  try {
    if (restoreEffects) {
      ImpersonateCurrentUser icu;

      vlog.debug("restore desktop effects");

      RegKey desktopCfg;
      desktopCfg.openKey(HKEY_CURRENT_USER, _T("Control Panel\\Desktop"));
      SysParamsInfo(SPI_SETFONTSMOOTHING, desktopCfg.getInt(_T("FontSmoothing"), 0) != 0, 0, SPIF_SENDCHANGE);
#ifdef RFB_HAVE_SPI_UIEFFECTS
      if (SysParamsInfo(SPI_SETUIEFFECTS, 0, (void*)uiEffects, SPIF_SENDCHANGE) == ERROR_CALL_NOT_IMPLEMENTED) {
        SysParamsInfo(SPI_SETCOMBOBOXANIMATION, 0, (void*)comboBoxAnim, SPIF_SENDCHANGE);
        SysParamsInfo(SPI_SETGRADIENTCAPTIONS, 0, (void*)gradientCaptions, SPIF_SENDCHANGE);
        SysParamsInfo(SPI_SETHOTTRACKING, 0, (void*)hotTracking, SPIF_SENDCHANGE);
        SysParamsInfo(SPI_SETLISTBOXSMOOTHSCROLLING, 0, (void*)listBoxSmoothScroll, SPIF_SENDCHANGE);
        SysParamsInfo(SPI_SETMENUANIMATION, 0, (void*)menuAnim, SPIF_SENDCHANGE);
      }
      restoreEffects = false;
#else
      vlog.info("  not supported");
#endif
    }

  } catch (rdr::Exception& e) {
    vlog.info(e.str());
  }
}
