// This code is part of the Pargeo Library
// Copyright (c) 2020 Yiqiu Wang and the Pargeo Team
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights (to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#pragma once

#include <vector>
#include "convexHull3d/vertex.h"
#include "parlay/sequence.h"
#include "pargeo/algebra.h"

namespace pargeo {
  namespace hull3d {
    namespace serialQuickHull {
      template<class pointT>
      class linkedFacet;
    }
  }
}

template<class pointT>
class pargeo::hull3d::serialQuickHull::linkedFacet {
public:
  using vertexT = pargeo::hull3d::vertex<linkedFacet, pointT>;
  using floatT = typename vertexT::floatT;
  using seqT = std::vector<vertexT>;

  vertexT a, b, c;

  linkedFacet *abFacet;

  linkedFacet *bcFacet;

  linkedFacet *caFacet;

  vertexT area;

  // seqT *seeList;
  seqT seeList;

  //size_t numPts() { return seeList->size(); }
  size_t numPts() { return seeList.size(); }

  //vertexT& pts(size_t i) { return seeList->at(i); }
  vertexT& pts(size_t i) { return seeList.at(i); }

  //void clear() { seeList->clear(); }
  void clear() { seeList.clear(); }

  //void push_back(vertexT v) { seeList->push_back(v); }
  void push_back(vertexT v) { seeList.push_back(v); }

  vertexT furthest() {
    auto apex = vertexT();
    typename vertexT::floatT m = pointT::eps;
    for (size_t i=0; i<numPts(); ++i) {
      //auto m2 = (a-pts(i)).dot(crossProduct3d(b-a, c-a));
      auto m2 = (a-pts(i)).dot(area);
      if (m2 > m) {
	m = m2;
	apex = pts(i);
      }
    }
    return apex;
  }

  vertexT furthestSample(size_t s) {
    auto apex = vertexT();
    typename vertexT::floatT m = pointT::eps;
    for (size_t i=0; i<std::min(numPts(), s); ++i) {
      size_t ii = parlay::hash64(i) % numPts();
      auto m2 = (a-pts(ii)).dot(area);
      if (m2 > m) {
	m = m2;
	apex = pts(ii);
      }
    }
    return apex;
  }

  linkedFacet(vertexT _a, vertexT _b, vertexT _c): a(_a), b(_b), c(_c) {
    if (pargeo::determinant3by3(a, b, c) > pointT::eps)
      std::swap(b, c);

    //seeList = new seqT();
    seeList = seqT();

    area = crossProduct3d(b-a, c-a);
    //area = crossProduct3d(b, c); //todo, minus is problematic
  }

  ~linkedFacet() {
    //delete seeList;
  }

};

/* static std::ostream& operator<<(std::ostream& os, const linkedFacet& v) { */
/* #ifdef HULL_SERIAL_VERBOSE */
/*   os << "(" << v.a.i << "," << v.b.i << "," << v.c.i << ")"; */
/* #else */
/*   os << "(" << v.a << "," << v.b << "," << v.c << ")"; */
/* #endif */
/*   return os; */
/* } */

// class pointOrigin {
//   using vertexT = pargeo::hullInternal::vertex;
//   using facetT = linkedFacet<vertexT>;

//   static constexpr typename pargeo::fpoint<3>::floatT numericKnob = 1e-5;

//   vertexT o;

// public:

//   void setOrigin(vertexT _o) { o = _o; }

//   inline vertexT get() {return o;};

//   pointOrigin() {}

//   template<class pt>
//   pointOrigin(pt _o) {
//     o = vertexT(_o.coords());
//   }

//   inline bool visible(facetT* f, vertexT p) {
//     return (f->a - p).dot(f->area) > numericKnob;
//   }

//   inline bool keep(facetT* f, vertexT p) {
//     if ((f->a - p).dot(f->area) > numericKnob)
//       return f->a != p && f->b != p && f->c != p;
//     else
//       return false;
//   }

// };
