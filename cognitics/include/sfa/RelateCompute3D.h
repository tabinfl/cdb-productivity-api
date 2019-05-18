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
/*! \brief Provides sfa::RelatePlane and sfa::RelateCompute3D.
\author Josh Anghel <janghel@cognitics.net>
\date 22 November 2010
*/
#pragma once
#include "RelateCompute.h"
#include "PlanarGraph.h"
#include "PlanarGraphOperation.h"

namespace sfa {

/*!    \class sfa::RelatePlane RelateCompute3D.h RelateCompute3D.h
\brief OverlayPlane

Provides an Overlay class with the ability to tranform 3D geometries to a projection only a 2D plane
*/
    class RelatePlane : public PlanarGraph
    {
    public:
        RelatePlane(Projection2D projection);
        virtual de9im computeIM(void);
    };

/*!    \class sfa::RelateCompute3D RelateCompute3D.h RelateCompute3D.h
\brief RelateCompute3D

Any given sfa::Geometry can be represented as a collection of 2D primatives. These 2D primatives can then
be collected into their respective planes. By treating each plane as a 2D environment and created an isomorphic
projection from 3D space onto that plane we can thus reduce any 3D geometry to a collection of 2D geometries.
By intersecting the planes first and transfering any portion of the geometries on each plane to the other, we
can then construct the resulting 3D overlay by itterating through each plane and constructing its overlay, 
projecting each result back into 3D coordinates, and then assembling them all into a GeometryCollection.

This class is inherited from the RelateOperation so it functions exactly like its RelateCompute 2D couterpart, 
but instead of returning a matrix representing the relation of two Geometries interaction in the xy plane, it
returns a matrix with their interaction in full 3D space. Because of this, one immediately noticable difference
will be that this 3D matrix will have a value of 3 in the EXTERIOR EXTERIOR componenent. This is because we 
are still working with finite/bound Geometries, but instead of 2D planar space, we are working in full 3D space.
*/
    class RelateCompute3D : public RelateOperation, public PlanarGraphOperation
    {
        const Geometry* A;
        const Geometry* B;
        bool initialized;

    public:
        RelateCompute3D(const Geometry* a, const Geometry* b);
        ~RelateCompute3D(void) {}

        virtual void addPoint(const Geometry* point, int arg);

        virtual PlanarGraph* createPlanarGraph(Projection2D proj);
        virtual de9im computeIM(void);
    };

}