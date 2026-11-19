#pragma once
#include <array>
#include <map>
#include <string>
#include <fstream>
#include <sstream>
#include <chrono>
#include <cstdio>
#include <algorithm>
namespace atlas_usage {
using Values=std::array<unsigned long long,8>;
// Independent high-water marks, not one simultaneous snapshot.
struct Record{std::string name;Values peak{};unsigned long long updated=0;};
static std::map<std::string,Record> records;
static std::string path,key,name;
static double lastCheck=0;
static std::string clean(std::string s){for(auto&c:s)if(c=='\t'||c=='\r'||c=='\n')c=' ';return s;}
static void begin(const char*root,const std::string&id,const std::string&title){
 path=std::string(root)+"/atlas-high-water.tsv";key=clean(id);name=clean(title);lastCheck=0;records.clear();
 std::ifstream in(path);std::string line;std::getline(in,line);
 while(std::getline(in,line)){std::istringstream row(line);std::string id;Record r;if(!std::getline(row,id,'\t')||!std::getline(row,r.name,'\t'))continue;bool ok=true;for(auto&v:r.peak)if(!(row>>v)){ok=false;break;}if(ok&&row>>r.updated)records[id]=r;}
}
static bool update(const Values&v){
 if(path.empty()||key.empty())return false;
 bool nonzero=false;for(auto n:v)nonzero|=n!=0;if(!nonzero)return false;
 auto it=records.find(key);Record next;if(it!=records.end())next=it->second;bool changed=false;
 for(size_t i=0;i<v.size();i++)if(v[i]>next.peak[i]){next.peak[i]=v[i];changed=true;}
 if(!changed)return false;
 next.name=name;next.updated=(unsigned long long)std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
 auto updated=records;updated[key]=next;
 std::string tmp=path+".tmp";std::ofstream out(tmp,std::ios::trunc);
 out<<"level_id\tlevel_name\tatlas_bytes_peak\tpages_peak\tcolor_pages_peak\tnormal_pages_peak\tsources_peak\tnormal_images_peak\tcopies_peak\tfallback_failures_peak\tupdated_unix_seconds\n";
 for(const auto&entry:updated){out<<entry.first<<'\t'<<entry.second.name;for(auto n:entry.second.peak)out<<'\t'<<n;out<<'\t'<<entry.second.updated<<'\n';}
 out.flush();bool ok=out.good();out.close();ok=ok&&!out.fail();
 if(!ok||std::rename(tmp.c_str(),path.c_str())!=0){std::remove(tmp.c_str());return false;}
 records.swap(updated);return true;
}
}
