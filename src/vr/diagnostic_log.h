#pragma once
#include <cstdarg>
#include <ctime>
static bool diagnosticEnabled=false,diagnosticTraceInitial=true;
// Fixed benchmark owns logging until explicitly released, even if the UI log toggle is OFF.
static bool diagnosticBenchmarkActive=false,diagnosticBenchmarkStopped=false;
static char diagnosticSettingPath[512]{};
static double diagnosticPollAt=-1;
static void pollDiagnosticSetting(){
 if(diagnosticBenchmarkActive){diagnosticEnabled=true;return;}
 if(diagnosticBenchmarkStopped){diagnosticEnabled=false;return;}
 timespec ts{};clock_gettime(CLOCK_MONOTONIC,&ts);double now=ts.tv_sec+ts.tv_nsec/1e9;
 if(diagnosticPollAt>=0&&now-diagnosticPollAt<1)return;diagnosticPollAt=now;
 FILE*f=fopen(diagnosticSettingPath,"rb");if(!f)return;int c=fgetc(f),tail=fgetc(f);fclose(f);if((c!='0'&&c!='1')||tail!=EOF)return;
 bool on=diagnosticBenchmarkActive||(!diagnosticBenchmarkStopped&&c=='1');if(on!=diagnosticEnabled){diagnosticEnabled=on;diagnosticTraceInitial=true;__android_log_print(ANDROID_LOG_INFO,"MythVR","Diagnostic logging enabled=%d",on);}
}
static void initDiagnosticLog(const char*path){diagnosticEnabled=false;diagnosticBenchmarkActive=diagnosticBenchmarkStopped=false;diagnosticTraceInitial=true;diagnosticPollAt=-1;snprintf(diagnosticSettingPath,sizeof(diagnosticSettingPath),"%s/diagnostic.enabled",path);pollDiagnosticSetting();}
static void diagnosticLog(const char*format,...){
 bool fatal=strncmp(format,"FATAL",5)==0;if(!diagnosticEnabled&&!fatal)return;
 va_list args;va_start(args,format);__android_log_vprint(fatal?ANDROID_LOG_ERROR:ANDROID_LOG_INFO,"MythVR",format,args);va_end(args);
}
