#pragma once
// Deterministic local poses; world placement comes from the level's initial camera.
// Runtime projection poses remain tracked so compositor reprojection is truthful.
struct BenchmarkPose {
 bool enabled=false;
 void load(const char*root){
  enabled=false;char path[512];snprintf(path,sizeof(path),"%s/benchmark-pose.enabled",root);
  if(FILE*f=fopen(path,"rb")){enabled=fgetc(f)=='1';fclose(f);}
 }
 void views(XrView*v)const{
  if(!enabled)return;
  for(int i=0;i<2;i++){v[i].pose={};v[i].pose.orientation.w=1;v[i].pose.position.x=i?0.032f:-0.032f;}
 }
 XrPosef hand(bool left)const{
  XrPosef p{};p.orientation.w=1;p.position={left?-.25f:.25f,-.35f,-.4f};return p;
 }
};
static BenchmarkPose benchmarkPose;
