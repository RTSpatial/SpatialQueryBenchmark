#pragma once

#include "parlay/sequence.h"

namespace pargeo {

  template<typename pt>
  parlay::sequence<pt> zorderSort2d_2(const parlay::sequence<pt>& P);

  template<typename pt>
  parlay::sequence<pt> zorderSort3d_2(const parlay::sequence<pt>& P);

} // End namespace

#include "mortonSortImpl.h"
