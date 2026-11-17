#include "../src/vr/group_cycle.h"
#include <cassert>
#include <cstdio>
int main(){
 std::array<unsigned,10> groups{};groups[1]=2;groups[4]=3;groups[8]=1;GroupCycle c;
 assert(c.next(groups,1,false)==1);assert(c.next(groups,-1,false)==8);
 auto r=c.request(4,true);c.acknowledge(r,true);assert(c.next(groups,1,true)==8);assert(c.next(groups,-1,true)==1);
 c.interrupt();assert(c.next(groups,1,true)==4);assert(c.next(groups,-1,false)==4);
 r=c.request(4,true);c.acknowledge(r,true);assert(c.next(groups,1,true)==8);assert(c.next(groups,-1,true)==1);
 assert(c.next(groups,1,false)==4); // all selection gone
 r=c.request(8,false);c.acknowledge(r,true);assert(c.next(groups,1,true)==8); // dial selection recalls first
 r=c.request(1,true);c.interrupt();c.acknowledge(r,true);assert(!c.active&&c.next(groups,1,true)==1); // delayed ACK cannot undo manual selection
 r=c.request(4,true);c.acknowledge(r,false);assert(c.last==1&&!c.active);
 groups[1]=0;assert(c.next(groups,1,false)==4);assert(c.next(groups,-1,false)==8);
 groups.fill(0);assert(c.next(groups,1,false)==-1);
 groups[3]=1;r=c.request(3,true);c.acknowledge(r,true);assert(c.next(groups,-1,true)==3);
 puts("PASS group recall, bidirectional cycle, manual/empty selection, dial, stale ACK, failure, removed and empty groups");
}
