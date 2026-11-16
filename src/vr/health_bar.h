#pragma once
// Build 434 health-bar renderer 0x4bfa81: signed current/max at object +0x42/+0x44.
static float unitHealth(const unsigned char* object){
 int maximum=i16(object+0x44);if(maximum<=0)return -1;
 return std::clamp(float(i16(object+0x42))/maximum,0.f,1.f);
}
static bool showHealthBar(unsigned unit,float health,bool selected,bool hovered){return unit&&health>=0&&(selected||hovered);}
template<class Point> static void appendHealthBar(std::vector<Vertex>& out,float left,float bottom,float top,float width,float health,Point point){
 float ratio=std::clamp(health,0.f,1.f),split=bottom+(top-bottom)*ratio,u=(ratio*255.f+.5f)/256.f;
 auto strip=[&](float b,float t,float v){if(t<=b)return;auto a=point(left,t),c=point(left,b),d=point(left+width,b),e=point(left+width,t);for(auto* p:{&a,&c,&d,&e}){p->u=u;p->v=v;}out.insert(out.end(),{a,e,c,e,d,c});};
 strip(bottom,split,.25f);strip(split,top,.75f);
}
