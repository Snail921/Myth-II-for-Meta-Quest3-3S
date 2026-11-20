// Verified live multiplayer mode is 1 (Quest co-op log, 2026-11-20).
// Build 471b only. Called on the guest UI thread, never from the capture worker.
extern void map_pen_mode185(I enabled);
extern void map_pen_flush185(I end);
// Private batches never populate the stock mouse pending buffer.
static I map_pen_player=-1,map_pen_active=0,map_pen_count=0,map_pen_samples=0,map_pen_has_anchor=0;
static U map_pen_mesh=0;
static B map_pen_points[12],map_pen_anchor[2];
static void map_pen_reduce(I first,I last,B*keep){
 I ax=map_pen_points[first*2],ay=map_pen_points[first*2+1];
 I dx=(I)map_pen_points[last*2]-ax,dy=(I)map_pen_points[last*2+1]-ay,len=dx*dx+dy*dy,best=-1;float error=.25f;
 for(I i=first+1;i<last;i++){
  I x=(I)map_pen_points[i*2]-ax,y=(I)map_pen_points[i*2+1]-ay,dot=x*dx+y*dy;float d;
  if(!len||dot<=0)d=(float)(x*x+y*y);
  else if(dot>=len){x-=dx;y-=dy;d=(float)(x*x+y*y);}
  else{I cross=x*dy-y*dx;d=(float)cross*cross/len;}
  if(d>error){error=d;best=i;}
 }
 if(best>=0){keep[best]=1;map_pen_reduce(first,best,keep);map_pen_reduce(best,last,keep);}
}
static void map_pen_batch(I end){
 B points[14],keep[6]={0};I n=0;
 if(map_pen_count){
  keep[0]=keep[map_pen_count-1]=1;map_pen_reduce(0,map_pen_count-1,keep);
  for(I i=map_pen_has_anchor?1:0;i<map_pen_count;i++)if(keep[i]){points[n*2]=map_pen_points[i*2];points[n*2+1]=map_pen_points[i*2+1];n++;}
  map_pen_anchor[0]=map_pen_points[(map_pen_count-1)*2];map_pen_anchor[1]=map_pen_points[(map_pen_count-1)*2+1];
 }
 if(end&&map_pen_active){points[n*2]=points[n*2+1]=0;n++;}
 if(n){US flags=0;((void(*)(I,I,B*))0x49ef8e)(map_pen_player,n,points);((void(*)(US*,I,B*))0x47b9b5)(&flags,n,points);}
 map_pen_samples=0;map_pen_count=end?0:1;map_pen_has_anchor=!end;
 if(!end){map_pen_points[0]=map_pen_anchor[0];map_pen_points[1]=map_pen_anchor[1];}
 if(end)map_pen_active=0;
}
static U map_pen_command(const struct Command*c){
 if(c->id>4||c->flags||c->reserved)return 16;
 if(c->id!=2&&(c->x||c->y))return 16;
 if(c->id==2&&(c->x<1||c->x>255||c->y<0||c->y>255))return 16;
 I player=*(I*)0x5332dc,team=*(I*)0x5332e0;
 if(*(short*)0x6bee46!=1||player<0||player>=*(US*)0x533296||team<0||team>=32)return 20;
 U mesh=*(U*)0x529bd4;
 if(map_pen_mesh!=mesh||map_pen_player!=player){map_pen_active=map_pen_count=map_pen_samples=map_pen_has_anchor=0;map_pen_mesh=mesh;map_pen_player=player;}
 if(c->id<2){map_pen_batch(1);if(*(US*)0x66b9a0)map_pen_flush185(1);map_pen_mode185(c->id);return 1;}
 if(c->id==4){map_pen_active=map_pen_count=map_pen_samples=map_pen_has_anchor=0;((void(*)(I,I))0x49eed6)(player,1);return 1;}
 if(c->id==3){map_pen_batch(1);return 1;}
 if(!*(U*)0x66b990)return 19;
 if(!map_pen_active){if(*(US*)0x66b9a0)map_pen_flush185(1);map_pen_active=1;map_pen_count=map_pen_samples=map_pen_has_anchor=0;}
 I n=map_pen_count;
 if(!n||map_pen_points[(n-1)*2]!=c->x||map_pen_points[(n-1)*2+1]!=c->y){map_pen_points[n*2]=(B)c->x;map_pen_points[n*2+1]=(B)c->y;map_pen_count++;}
 if(++map_pen_samples>=4)map_pen_batch(0);
 return 1;
}
