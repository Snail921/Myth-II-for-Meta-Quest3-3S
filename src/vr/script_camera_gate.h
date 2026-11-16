#pragma once
// Wall-clock grace period begins on the observed end, not the start of the camera move.
struct ScriptCameraGate {
 bool wasActive=false;double releaseAt=0;
 bool update(bool active,bool event,double now){
  if(active||wasActive||event)releaseAt=now+.5;
  wasActive=active;return active||now<releaseAt;
 }
};
