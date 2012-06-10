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
// ColourCube - structure to represent a colour cube.  The colour cube consists
// of its dimensions (nRed x nGreen x nBlue) and a table mapping an (r,g,b)
// triple to a pixel value.
//
// A colour cube is used in two cases.  The first is internally in a viewer
// when it cannot use a trueColour format, nor can it have exclusive access to
// a writable colour map.  This is most notably the case for an X viewer
// wishing to use a PseudoColor X server's default colormap.
//
// The second use is on the server side when a client has asked for a colour
// map and the server is trueColour.  Instead of setting an uneven trueColour
// format like bgr233, it can set the client's colour map up with a 6x6x6
// colour cube.  For this use the colour cube table has a null mapping, which
// makes it easy to perform the reverse lookup operation from pixel value to
// r,g,b values.

#ifndef __RFB_COLOURCUBE_H__
#define __RFB_COLOURCUBE_H__

#include <rfb/Pixel.h>
#include <rfb/ColourMap.h>

namespace rfb {

  class ColourCube : public ColourMap {
  public:
    ColourCube(int nr, int ng, int nb, Pixel* table_=0)
      : nRed(nr), nGreen(ng), nBlue(nb), table(table_), deleteTable(false)
    {
      if (!table) {
        table = new Pixel[size()];
        deleteTable = true;
        // set a null mapping by default
        for (int i = 0; i < size(); i++)
          table[i] = i;
      }
    }

    ColourCube() : deleteTable(false) {}

    virtual ~ColourCube() {
      if (deleteTable) delete [] table;
    }

    void set(int r, int g, int b, Pixel p) {
      table[(r * nGreen + g) * nBlue + b] = p;
    }

    Pixel lookup(int r, int g, int b) const {
      return table[(r * nGreen + g) * nBlue + b];
    }

    int size()      const { return nRed*nGreen*nBlue; }
    int redMult()   const { return nGreen*nBlue; }
    int greenMult() const { return nBlue; }
    int blueMult()  const { return 1; }

    // ColourMap lookup() method.  Note that this only works when the table has
    // the default null mapping.
    virtual void lookup(int i, int* r, int* g, int* b) {
      if (i >= size()) return;
      *b = i % nBlue;
      i /= nBlue;
      *g = i % nGreen;
      *r = i / nGreen;
      *r = (*r * 65535 + (nRed-1)   / 2) / (nRed-1);
      *g = (*g * 65535 + (nGreen-1) / 2) / (nGreen-1);
      *b = (*b * 65535 + (nBlue-1)  / 2) / (nBlue-1);
    }

    int nRed;
    int nGreen;
    int nBlue;
    Pixel* table;
    bool deleteTable;
  };
}
#endif
