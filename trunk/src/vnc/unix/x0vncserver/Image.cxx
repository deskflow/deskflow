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
//
// Image.cxx
//


#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>
#include "Image.h"
#include <list>

class ImageCleanup {
public:
  std::list<Image*> images;

  ~ImageCleanup()
  {
    fprintf(stderr,"~ImageCleanup called\n");

    while (!images.empty()) {
      delete images.front();
    }
  }
};

ImageCleanup imageCleanup;

static bool caughtShmError = false;

static int ShmCreationXErrorHandler(Display *dpy, XErrorEvent *error)
{
  caughtShmError = true;
  return 0;
}

Image::Image(Display* d, int width, int height)
  : xim(0), dpy(d), shminfo(0), usingShm(false)
{
  if (createShmImage(width, height)) return;

  xim = XCreateImage(dpy, DefaultVisual(dpy, DefaultScreen(dpy)),
                     DefaultDepth(dpy,DefaultScreen(dpy)), ZPixmap,
                     0, 0, width, height, BitmapPad(dpy), 0);

  xim->data = (char*)malloc(xim->bytes_per_line * xim->height);
  if (!xim->data) {
    fprintf(stderr,"malloc failed\n");
    exit(1);
  }
}

Image::~Image()
{
  fprintf(stderr,"~Image called - usingShm %d\n",usingShm);
  if (usingShm) {
    usingShm = false;
    shmdt(shminfo->shmaddr);
    shmctl(shminfo->shmid, IPC_RMID, 0);
    imageCleanup.images.remove(this);
  }
  delete shminfo;
  // XDestroyImage will free xim->data if necessary
  if (xim) XDestroyImage(xim);
}

void Image::get(Window w)
{
  if (usingShm) {
    XShmGetImage(dpy, w, xim, 0, 0, AllPlanes);
  } else {
    XGetSubImage(dpy, w, 0, 0, xim->width, xim->height,
                 AllPlanes, ZPixmap, xim, 0, 0);
  }
}

bool Image::createShmImage(int width, int height)
{
  if (XShmQueryExtension(dpy)) {
    shminfo = new XShmSegmentInfo;

    xim = XShmCreateImage(dpy, DefaultVisual(dpy, DefaultScreen(dpy)),
                          DefaultDepth(dpy,DefaultScreen(dpy)), ZPixmap,
                          0, shminfo, width, height);

    if (xim) {
      shminfo->shmid = shmget(IPC_PRIVATE,
                              xim->bytes_per_line * xim->height,
                              IPC_CREAT|0777);

      if (shminfo->shmid != -1) {
        shminfo->shmaddr = xim->data = (char*)shmat(shminfo->shmid, 0, 0);

        if (shminfo->shmaddr != (char *)-1) {

          shminfo->readOnly = False;

          XErrorHandler oldHdlr = XSetErrorHandler(ShmCreationXErrorHandler);
          XShmAttach(dpy, shminfo);
          XSync(dpy, False);
          XSetErrorHandler(oldHdlr);

          if (!caughtShmError) {
            fprintf(stderr,"Using shared memory XImage\n");
            usingShm = true;
            imageCleanup.images.push_back(this);
            return true;
          }

          shmdt(shminfo->shmaddr);
        } else {
          fprintf(stderr,"shmat failed\n");
          perror("shmat");
        }

        shmctl(shminfo->shmid, IPC_RMID, 0);
      } else {
        fprintf(stderr,"shmget failed\n");
        perror("shmget");
      }

      XDestroyImage(xim);
      xim = 0;
    } else {
      fprintf(stderr,"XShmCreateImage failed\n");
    }
  }

  return false;
}
