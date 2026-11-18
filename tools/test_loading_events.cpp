#include <ctime>
#include <stdexcept>
#include <cassert>
#include <cstdio>
struct android_app{int activityState=0,destroyRequested=0;};
struct android_poll_source{void(*process)(android_app*,android_poll_source*);};
static constexpr int APP_CMD_RESUME=11;
static int commands[8],count=0,at=0;
static void process(android_app*a,android_poll_source*){int c=commands[at++];if(c==-1)a->destroyRequested=1;else a->activityState=c;}
static android_poll_source source{process};
static int ALooper_pollOnce(int,void*,int*,void**out){if(at==count)return -1;*out=&source;return 1;}
#include "../src/vr/loading_events.h"
int main(){android_app a;commands[0]=10;commands[1]=APP_CMD_RESUME;count=2;awaitLoadingResume(&a);assert(a.activityState==APP_CMD_RESUME&&at==2);commands[0]=12;at=0;count=1;pumpLoadingEvents(true);assert(a.activityState==12);commands[0]=-1;at=0;bool caught=false;try{pumpLoadingEvents(true);}catch(const std::runtime_error&){caught=true;}assert(caught);a={};at=0;caught=false;try{awaitLoadingResume(&a);}catch(const std::runtime_error&){caught=true;}assert(caught);loadingApp=nullptr;pumpLoadingEvents(true);puts("PASS lifecycle resume before load; pause processed; destruction cancels before/during load; disabled no-op");}
