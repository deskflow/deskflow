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
#ifndef __RFB_TRUECOLOURMAP_H__
#define __RFB_TRUECOLOURMAP_H__

#include <rfb/ColourMap.h>

namespace rfb {

  class TrueColourMap : public ColourMap {
  public:
    TrueColourMap(const PixelFormat& pf_) : pf(pf_) {}

    virtual void lookup(int i, int* r, int* g, int* b)
    {
      *r = (((i >> pf.redShift  ) & pf.redMax)
            * 65535 + pf.redMax/2) / pf.redMax;
      *g = (((i >> pf.greenShift) & pf.greenMax)
            * 65535 + pf.greenMax/2) / pf.greenMax;
      *b = (((i >> pf.blueShift) & pf.blueMax)
            * 65535 + pf.blueMax/2) / pf.blueMax;
    }
  private:
    PixelFormat pf;
  };
}
#endif
