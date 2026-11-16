#include <cstdio>
#include <cstring>
#include <cassert>
#include <android/log.h>
#include "../src/vr/diagnostic_log.h"
int main(){
 const char*root="/data/local/tmp/myth-diagnostic-test";mkdir(root,0700);
 char path[512];snprintf(path,sizeof(path),"%s/diagnostic.enabled",root);remove(path);
 initDiagnosticLog(root);assert(!diagnosticEnabled);
 auto set=[&](const char*s){FILE*f=fopen(path,"wb");assert(f);fputs(s,f);fclose(f);diagnosticPollAt=-1;pollDiagnosticSetting();};
 set("1");assert(diagnosticEnabled&&diagnosticTraceInitial);diagnosticTraceInitial=false;
 set("bad");assert(diagnosticEnabled&&!diagnosticTraceInitial);
 set("0");assert(!diagnosticEnabled);set("1");assert(diagnosticEnabled&&diagnosticTraceInitial);
 initDiagnosticLog(root);assert(diagnosticEnabled);set("0");initDiagnosticLog(root);assert(!diagnosticEnabled);
 remove(path);puts("PASS diagnostic toggle: default OFF, enable, disable, malformed preservation, restart persistence");
}
