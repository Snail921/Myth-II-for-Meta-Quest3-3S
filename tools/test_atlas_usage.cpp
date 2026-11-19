#include "../src/vr/atlas_usage.h"
#include <cassert>
#include <iterator>
static std::string contents(const std::string&p){std::ifstream f(p);return {std::istreambuf_iterator<char>(f),{}};}
int main(){const char*root="/data/local/tmp";std::string p=std::string(root)+"/atlas-high-water.tsv";std::remove(p.c_str());
 atlas_usage::begin(root,"test-level-a","Name\twith\ncontrols");assert(atlas_usage::update({64,4,3,1,100,30,130,0}));auto a=contents(p);
 assert(!atlas_usage::update({32,2,1,1,90,20,100,0}));assert(contents(p)==a);
 atlas_usage::begin(root,"test-level-a","Name");assert(!atlas_usage::update({64,4,3,1,100,30,130,0}));assert(atlas_usage::update({32,2,1,1,90,20,100,5}));assert(atlas_usage::records["test-level-a"].peak[0]==64);assert(atlas_usage::records["test-level-a"].peak[7]==5);
 atlas_usage::begin(root,"test-level-b","B");assert(atlas_usage::update({16,1,1,0,20,0,20,0}));assert(atlas_usage::records.size()==2);
 atlas_usage::begin(root,"test-level-a","A");assert(atlas_usage::records.size()==2);assert(atlas_usage::records["test-level-a"].peak[7]==5);
 assert(!atlas_usage::update({}));
 atlas_usage::path="/nonexistent-atlas-test/record.tsv";auto before=atlas_usage::records["test-level-a"].peak;assert(!atlas_usage::update({1000,10,8,2,1000,300,1300,8}));assert(atlas_usage::records["test-level-a"].peak==before);
 std::remove(p.c_str());puts("PASS persistence, independent maxima, unchanged no-write, multiple levels, sanitization, write failure");}
