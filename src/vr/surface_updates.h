#pragma once
#include <cstdint>
#include <cstring>
#include <map>
#include <vector>
#include <algorithm>
namespace surface {
constexpr size_t PersistentPixels=65536,PersistentRow=131076;
struct Updates {
 unsigned tileLimit=42;uint64_t epoch=0;std::map<unsigned,std::vector<uint8_t>> pending;
 // Validate a complete update tail before modifying any queued state.
 bool stage(const uint8_t*p,size_t size){
  if(size<12)return false;uint64_t next;uint32_t count;memcpy(&next,p,8);memcpy(&count,p+8,4);
  if(count>4||size!=12+count*PersistentRow||(!next&&count))return false;
  std::vector<unsigned> seen;
  for(unsigned i=0;i<count;i++){uint32_t tile;memcpy(&tile,p+12+i*PersistentRow,4);if(tile>=tileLimit||std::find(seen.begin(),seen.end(),tile)!=seen.end())return false;seen.push_back(tile);}
  if(!next)return true;
  if(epoch!=next){pending.clear();epoch=next;}
  for(unsigned i=0;i<count;i++){auto r=p+12+i*PersistentRow;uint32_t tile;memcpy(&tile,r,4);pending[tile]=std::vector<uint8_t>(r+4,r+PersistentRow);}
  return true;
 }
};
}
