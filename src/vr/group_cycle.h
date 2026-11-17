#pragma once
#include <array>
struct GroupCycle {
 int last=-1;bool active=false;unsigned revision=0;
 struct Request {int group;bool cycling;unsigned revision;};
 void interrupt(){active=false;++revision;}
 Request request(int group,bool cycling){interrupt();return {group,cycling,revision};}
 void acknowledge(const Request&r,bool success){if(success)last=r.group;if(r.revision==revision)active=success&&r.cycling;}
 int next(const std::array<unsigned,10>&groups,int direction,bool selected)const{
  if((!active||!selected)&&last>=0&&last<10&&groups[last])return last;
  for(int n=1;n<=10;n++){int i=last<0?(direction>0?n-1:10-n):(last+direction*n+20)%10;if(groups[i])return i;}return -1;
 }
};
