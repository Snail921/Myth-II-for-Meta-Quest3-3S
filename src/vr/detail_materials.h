#pragma once
#include <array>
namespace detail_materials {
struct Triangle {
 std::array<float,3> ids{};
 std::array<std::array<float,3>,3> weights{};
 unsigned samples=0;
};
// Combine identical material IDs before interpolation. ID zero contributes
// no detail; do not renormalize the remaining weights across a zero-ID edge.
inline Triangle compact(const std::array<unsigned,3>& source){
 Triangle result;
 for(unsigned vertex=0;vertex<3;vertex++){
  if(!source[vertex])continue;
  unsigned slot=0;
  while(slot<result.samples&&result.ids[slot]!=float(source[vertex]))++slot;
  if(slot==result.samples)result.ids[result.samples++]=float(source[vertex]);
  result.weights[vertex][slot]=1.f;
 }
 return result;
}
}
