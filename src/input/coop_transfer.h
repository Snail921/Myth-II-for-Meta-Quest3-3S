// Build 471b only; dispatch through the original DETACH command builder.
#ifdef MYTH_COOP_471
extern void detach_selected185(I target);
static U transfer_selected(const struct Command*c){
 if(c->id>=32||c->x||c->y||!controlled_selection())return 18;
 U count=*(US*)0x533296;B*players=*(B**)0x53329c;I local=*(I*)0x5332dc,team=*(I*)0x5332e0;
 if(count>32||c->id>=count||local<0||(U)local>=count||team<0||!players||c->id==(U)local)return 18;
 B*p=players+c->id*104;
 if(*(U*)p!=c->flags||(signed char)p[10]!=team||p[9]!=0||(p[8]&0x44))return 18;
 U hash=2166136261u;B*actors=*(B**)0x532188;US n=*(US*)0x67ddb0;
 for(U i=0;i<n;i++)hash=(hash^*(US*)(actors+((US*)0x67ddb4)[i]*220))*16777619u;
 if(hash!=c->reserved)return 19;
 routeCount=routeFacing=0;staging=0;cancel_mode();detach_selected185(c->id);return 1;
}
#else
static U transfer_selected(const struct Command*c){(void)c;return 17;}
#endif
