/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2004 Chris Schoeneman
 *
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "platform/MSWindowsClipboardBitmapConverter.h"

#include "base/Log.h"

//
// MSWindowsClipboardBitmapConverter
//

MSWindowsClipboardBitmapConverter::MSWindowsClipboardBitmapConverter () {
    // do nothing
}

MSWindowsClipboardBitmapConverter::~MSWindowsClipboardBitmapConverter () {
    // do nothing
}

IClipboard::EFormat
MSWindowsClipboardBitmapConverter::getFormat () const {
    return IClipboard::kBitmap;
}

UINT
MSWindowsClipboardBitmapConverter::getWin32Format () const {
    return CF_DIB;
}

HANDLE
MSWindowsClipboardBitmapConverter::fromIClipboard (const String& data) const {
    // copy to memory handle
    HGLOBAL gData = GlobalAlloc (GMEM_MOVEABLE | GMEM_DDESHARE, data.size ());
    if (gData != NULL) {
        // get a pointer to the allocated memory
        char* dst = (char*) GlobalLock (gData);
        if (dst != NULL) {
            memcpy (dst, data.data (), data.size ());
            GlobalUnlock (gData);
        } else {
            GlobalFree (gData);
            gData = NULL;
        }
    }

    return gData;
}

String
MSWindowsClipboardBitmapConverter::toIClipboard (HANDLE data) const {
    // get datator
    LPVOID src = GlobalLock (data);
    if (src == NULL) {
        return String ();
    }
    UInt32 srcSize = (UInt32) GlobalSize (data);

    // check image type
    const BITMAPINFO* bitmap = static_cast<const BITMAPINFO*> (src);
    LOG ((CLOG_INFO "bitmap: %dx%d %d",
          bitmap->bmiHeader.biWidth,
          bitmap->bmiHeader.biHeight,
          (int) bitmap->bmiHeader.biBitCount));
    if (bitmap->bmiHeader.biPlanes == 1 &&
        (bitmap->bmiHeader.biBitCount == 24 ||
         bitmap->bmiHeader.biBitCount == 32) &&
        bitmap->bmiHeader.biCompression == BI_RGB) {
        // already in canonical form
        String image (static_cast<char const*> (src), srcSize);
        GlobalUnlock (data);
        return image;
    }

    // create a destination DIB section
    LOG ((CLOG_INFO "convert image from: depth=%d comp=%d",
          bitmap->bmiHeader.biBitCount,
          bitmap->bmiHeader.biCompression));
    void* raw;
    BITMAPINFOHEADER info;
    LONG w               = bitmap->bmiHeader.biWidth;
    LONG h               = bitmap->bmiHeader.biHeight;
    info.biSize          = sizeof (BITMAPINFOHEADER);
    info.biWidth         = w;
    info.biHeight        = h;
    info.biPlanes        = 1;
    info.biBitCount      = 32;
    info.biCompression   = BI_RGB;
    info.biSizeImage     = 0;
    info.biXPelsPerMeter = 1000;
    info.biYPelsPerMeter = 1000;
    info.biClrUsed       = 0;
    info.biClrImportant  = 0;
    HDC dc               = GetDC (NULL);
    HBITMAP dst          = CreateDIBSection (
        dc, (BITMAPINFO*) &info, DIB_RGB_COLORS, &raw, NULL, 0);

    // find the start of the pixel data
    const char* srcBits = (const char*) bitmap + bitmap->bmiHeader.biSize;
    if (bitmap->bmiHeader.biBitCount >= 16) {
        if (bitmap->bmiHeader.biCompression == BI_BITFIELDS &&
            (bitmap->bmiHeader.biBitCount == 16 ||
             bitmap->bmiHeader.biBitCount == 32)) {
            srcBits += 3 * sizeof (DWORD);
        }
    } else if (bitmap->bmiHeader.biClrUsed != 0) {
        srcBits += bitmap->bmiHeader.biClrUsed * sizeof (RGBQUAD);
    } else {
        // http://msdn.microsoft.com/en-us/library/ke55d167(VS.80).aspx
        srcBits += (1i64 << bitmap->bmiHeader.biBitCount) * sizeof (RGBQUAD);
    }

    // copy source image to destination image
    HDC dstDC         = CreateCompatibleDC (dc);
    HGDIOBJ oldBitmap = SelectObject (dstDC, dst);
    SetDIBitsToDevice (
        dstDC, 0, 0, w, h, 0, 0, 0, h, srcBits, bitmap, DIB_RGB_COLORS);
    SelectObject (dstDC, oldBitmap);
    DeleteDC (dstDC);
    GdiFlush ();

    // extract data
    String image ((const char*) &info, info.biSize);
    image.append ((const char*) raw, 4 * w * h);

    // clean up GDI
    DeleteObject (dst);
    ReleaseDC (NULL, dc);

    // release handle
    GlobalUnlock (data);

    return image;
}
