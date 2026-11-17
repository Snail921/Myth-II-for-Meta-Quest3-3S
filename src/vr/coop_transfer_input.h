// Modal picker in tracking space; the original engine remains authoritative.
static std::vector<CoopRecipient> coopChoices;
static Vec3 coopCenter{},coopRight{},coopUp{},coopNormal{};
static unsigned coopFrozenSelection=0;static int coopHover=-1;
static float coopRowHeight(){return .065f;}
static float coopTop(){return (coopChoices.size()*coopRowHeight()-.006f)*.5f;}
static bool coopCurrent(const CoopRecipient&p){for(auto&r:coopRecipients)if(r.slot==p.slot&&r.identity==p.identity)return true;return false;}
static void sendCoopTransfer(const CoopRecipient&p,unsigned selection){
 if(selection!=coopSelectionHash||!coopCurrent(p)){commandFailed=true;return;}
 command(23,p.slot,p.identity,0,0,(int)selection);
}
static void openCoopTransfer(const XrPosef&head){
 bool selected=false;for(auto&o:objects)if(o.selected){if(!o.controllable){commandFailed=true;return;}selected=true;}
 if(!selected||coopRecipients.empty()){commandFailed=true;return;}
 if(coopRecipients.size()==1){sendCoopTransfer(coopRecipients[0],coopSelectionHash);return;}
 coopChoices=coopRecipients;coopFrozenSelection=coopSelectionHash;coopHover=-1;
 coopRight=rotated(head.orientation,{1,0,0});coopUp=rotated(head.orientation,{0,1,0});coopNormal=rotated(head.orientation,{0,0,1});
 coopCenter=add(Vec3{head.position.x,head.position.y,head.position.z},times(coopNormal,-1.2f));coopMenuOpen=true;
}
static void pointCoopTransfer(Vec3 origin,Vec3 direction){
 coopHover=-1;float den=dot(direction,coopNormal);if(fabsf(den)<.00001f)return;
 float t=dot(sub(coopCenter,origin),coopNormal)/den;if(t<=0)return;auto p=add(origin,times(direction,t));auto d=sub(p,coopCenter);
 float x=dot(d,coopRight),y=dot(d,coopUp);int row=(int)floorf((coopTop()-y)/coopRowHeight());
 if(fabsf(x)>.29f||row<0||row>=(int)coopChoices.size())return;
 float inRow=coopTop()-y-row*coopRowHeight();if(inRow>.059f)return;
 coopHover=row;pointerEnd=add(camera,turn(p,cameraYaw));
}
