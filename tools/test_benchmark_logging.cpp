#include <cstdio>
#include <cstring>
#include <cassert>
#include <cstdarg>
#include <cstdlib>
#include <string>
#include <unistd.h>
#define ANDROID_LOG_INFO 4
#define ANDROID_LOG_ERROR 6
static int messages=0;
static int __android_log_print(int,const char*,const char*,...){++messages;return 0;}
static int __android_log_vprint(int,const char*,const char*,va_list){++messages;return 0;}
#include "../src/vr/diagnostic_log.h"
int main(){
 char dir[]="/data/local/tmp/myth-logging-test-XXXXXX";assert(mkdtemp(dir));std::string path=std::string(dir)+"/diagnostic.enabled";
 FILE*f=fopen(path.c_str(),"wb");fputc('0',f);fclose(f);initDiagnosticLog(dir);assert(!diagnosticEnabled);
 diagnosticBenchmarkActive=true;pollDiagnosticSetting();assert(diagnosticEnabled);diagnosticLog("Benchmark BEGIN");int start=messages;assert(start>0);
 diagnosticLog("Benchmark END");assert(messages==start+1);diagnosticBenchmarkActive=false;diagnosticBenchmarkStopped=true;pollDiagnosticSetting();assert(!diagnosticEnabled);
 f=fopen(path.c_str(),"wb");fputc('1',f);fclose(f);diagnosticPollAt=-1;pollDiagnosticSetting();assert(!diagnosticEnabled);diagnosticLog("should not log");assert(messages==start+1);
 initDiagnosticLog(dir);assert(diagnosticEnabled&&!diagnosticBenchmarkStopped);diagnosticLog("normal logging restored on next entry");assert(messages>start+1);
 remove(path.c_str());rmdir(dir);puts("PASS: fixed mode forces logging with UI OFF; release suppresses logging even with UI ON; next entry resets override");
}
