/*************************************************************************
Copyright (c) 2019 Cognitics, Inc.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
****************************************************************************/
/*! \brief Provides sfa::MultiSurface.
\author Aaron Brinton <abrinton@cognitics.net>
\date 30 October 2009
\sa OpenGIS Implementation Specification for Geographic Information - Simple Feature Access (OGC 06-103r3 Version 1.2.0)
*/
#pragma once

#include "GeometryCollection.h"
#include "Point.h"

namespace sfa
{
    class MultiSurface;
    typedef std::shared_ptr<MultiSurface> MultiSurfaceSP;

/*! \class sfa::MultiSurface MultiSurface.h MultiSurface.h
\brief MultiSurface

\sa OpenGIS Implementation Specification for Geographic Information - Simple Feature Access (OGC 06-103r3 Version 1.2.0) 6.1.13
*/
    class MultiSurface : public GeometryCollection
    {
    public:
        virtual ~MultiSurface(void);
        MultiSurface(void);
        MultiSurface(const MultiSurface& surface);
        MultiSurface(const MultiSurface* surface);

        virtual int getDimension(void) const;

        // TODO: virtual bool isClosed(void);
        // TODO: virtual double getLength(void);

        virtual double getArea(void) const = 0;
        virtual Point* getCentroid(void) const = 0;
        virtual Point* getPointOnSurface(void) const = 0;

    };

}
