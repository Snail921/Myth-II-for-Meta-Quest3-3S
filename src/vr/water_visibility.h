#pragma once
#include <cstdint>
// Per eye, three asynchronous queries. Never request an unavailable result.
struct WaterVisibility {
 struct Slot{GLuint query=0;bool pending=false;uint64_t frame=0;};
 struct Eye{Slot slots[3];bool known=false,visible=true;uint64_t resultFrame=0;};
 Eye eyes[2];bool render=true;
 unsigned issued=0,completed=0,busy=0,skipped=0;
 void poll(uint64_t frame){
  for(auto&e:eyes)for(auto&s:e.slots)if(s.pending){GLuint available=0;glGetQueryObjectuiv(s.query,GL_QUERY_RESULT_AVAILABLE,&available);if(!available)continue;
   GLuint result=1;glGetQueryObjectuiv(s.query,GL_QUERY_RESULT,&result);s.pending=false;++completed;
   if(!e.known||s.frame>=e.resultFrame){e.known=true;e.visible=result!=0;e.resultFrame=s.frame;}}
  render=false;for(auto&e:eyes)if(!e.known||e.visible||frame-e.resultFrame>30)render=true;
 }
 bool begin(unsigned eye,uint64_t frame){auto&e=eyes[eye];for(auto&s:e.slots)if(!s.pending){if(!s.query)glGenQueries(1,&s.query);if(!s.query)return false;s.pending=true;s.frame=frame;glBeginQuery(GL_ANY_SAMPLES_PASSED_CONSERVATIVE,s.query);++issued;return true;}++busy;return false;}
 void end(){glEndQuery(GL_ANY_SAMPLES_PASSED_CONSERVATIVE);}
 void release(){for(auto&e:eyes)for(auto&s:e.slots)if(s.query)glDeleteQueries(1,&s.query);*this=WaterVisibility{};}
};
