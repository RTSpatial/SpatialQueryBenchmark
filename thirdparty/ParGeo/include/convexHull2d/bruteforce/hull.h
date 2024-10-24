#pragma once

#include "parlay/sequence.h"

namespace pargeo {
  namespace hull2d {
    namespace bruteforce {

      template<class pointT>
      parlay::sequence<size_t>
      compute(parlay::slice<pointT*, pointT*> P);

    }
  }
}
