// Diagnostic overlay in native OpenXR reference space. No worldPose,
// game camera translation/yaw, minimap pitch or keyboard pose is applied.
static void drawTrackingGizmo(int eyeIndex,const Mat&worldMVP){
 if(!chat.open||eyeIndex<0||eyeIndex>1)return;
 const auto&eye=chatTrackingEyes[eyeIndex];
 Mat trackingMVP=mul(projection(eye.fov),view(eye.pose));
 glUniformMatrix4fv(glGetUniformLocation(program,"mvp"),1,GL_FALSE,trackingMVP.data());
 glDisable(GL_DEPTH_TEST);glDepthMask(GL_FALSE);glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);attributes(vbo);
 glUniform1f(glGetUniformLocation(program,"alphaCutoff"),0);
 auto segment=[&](Vec3 a,Vec3 b,float r,float g,float blue){
  Vertex v[]={{a.x,a.y,a.z,0,0},{b.x,b.y,b.z,0,0}};
  glBindTexture(GL_TEXTURE_2D,whiteTexture);glUniform4f(glGetUniformLocation(program,"tint"),r,g,blue,1);
  glBufferData(GL_ARRAY_BUFFER,sizeof(v),v,GL_STREAM_DRAW);glDrawArrays(GL_LINES,0,2);
 };
 pointer_style::Lines width;
 for(int hand=0;hand<2;hand++){
  if(chatGripValid[hand]){
   auto&p=chatTrackingGrip[hand];Vec3 o{p.position.x,p.position.y,p.position.z};
   segment(o,add(o,rotated(p.orientation,{.12f,0,0})),1,0,0);
   segment(o,add(o,rotated(p.orientation,{0,.12f,0})),0,1,0);
   segment(o,add(o,rotated(p.orientation,{0,0,.12f})),0,.35f,1);
   multiplayerText(hand?"L":"R",add(o,Vec3{0,.15f,0}),rotated(eye.pose.orientation,{1,0,0}),rotated(eye.pose.orientation,{0,1,0}),.001f,.05f);
  }
  if(chatTrackingValid[hand]){
   auto&p=chatTrackingAim[hand];Vec3 o{p.position.x,p.position.y,p.position.z};
   auto d=rotated(p.orientation,{0,0,-1});
   // Dashed native aim stays visible even when it misses the keyboard.
   for(int i=0;i<40;i++)segment(add(o,times(d,i*.05f)),add(o,times(d,i*.05f+.025f)),hand?0.f:1.f,hand?1.f:0.f,1);
  }
 }
 glUniformMatrix4fv(glGetUniformLocation(program,"mvp"),1,GL_FALSE,worldMVP.data());
 glDepthMask(GL_TRUE);glEnable(GL_DEPTH_TEST);glDisable(GL_BLEND);glUniform1f(glGetUniformLocation(program,"alphaCutoff"),.5f);glUniform4f(glGetUniformLocation(program,"tint"),1,1,1,1);
}
