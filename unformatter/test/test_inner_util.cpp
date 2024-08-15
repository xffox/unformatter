#include "unformatter/inner/util.hpp"

using namespace unformatter::inner::util;

namespace
{
static_assert(areNondecreasingIndices<1, 3, 5>());
static_assert(areNondecreasingIndices<3, 7, 7, 8>());
static_assert(!areNondecreasingIndices<3, 2, 5>());
}
