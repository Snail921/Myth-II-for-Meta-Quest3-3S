#pragma once
// Original object +0x38 links a body to its type-5 visual overlay. Resolve
// per snapshot: pool slots can be reused, and converted collections may be absent.
// The overlay receives light with the body's owner, never an unrelated FX owner.
static void resolveSpriteOverlays(std::vector<Object>&items,const unsigned char*raw,unsigned count){
 std::vector<int> bySlot(count,-1);
 for(size_t i=0;i<items.size();i++){
  items[i].inheritedLightingOwner=0;
  unsigned slot=items[i].objectKey>>16;if(slot<count)bySlot[slot]=int(i);
 }
 for(size_t i=0;i<items.size();i++){
  const auto&body=items[i];if(body.objectType==5||!physicalSprite(body))continue;
  unsigned slot=body.objectKey>>16;if(slot>=count)continue;
  int child=i16(raw+slot*80+56);
  if(child<0||unsigned(child)>=count||bySlot[child]<0)continue;
  auto&overlay=items[bySlot[child]];
  if(overlay.objectType==5&&overlay.col==body.col)overlay.inheritedLightingOwner=unsigned(i)+1;
 }
}
