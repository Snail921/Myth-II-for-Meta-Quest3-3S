static void drawSelectionBox(){
 if(!selectionGesture.box)return;
 auto a=selectionGesture.start,b=selectionGesture.end;
 auto vertex=[&](ScreenPoint p){auto v=selectionDragView.point(p,selectionDragDepth);return Vertex{v.x,v.y,v.z,0,0};};
 Vertex p=vertex(a),q=vertex({b.x,a.y}),r=vertex({a.x,b.y}),s=vertex(b);
 Vertex fill[]={p,q,r,q,s,r};Vertex lines[]={p,q,q,s,s,r,r,p};
 glDisable(GL_DEPTH_TEST);glDepthMask(GL_FALSE);glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);attributes(vbo);glBindTexture(GL_TEXTURE_2D,whiteTexture);
 glUniform4f(glGetUniformLocation(program,"tint"),1,1,0,.08f);glBufferData(GL_ARRAY_BUFFER,sizeof(fill),fill,GL_STREAM_DRAW);glDrawArrays(GL_TRIANGLES,0,6);
 glUniform4f(glGetUniformLocation(program,"tint"),1,1,0,.85f);glBufferData(GL_ARRAY_BUFFER,sizeof(lines),lines,GL_STREAM_DRAW);glDrawArrays(GL_LINES,0,8);
 glDisable(GL_BLEND);glDepthMask(GL_TRUE);glEnable(GL_DEPTH_TEST);glUniform4f(glGetUniformLocation(program,"tint"),1,1,1,1);
}
