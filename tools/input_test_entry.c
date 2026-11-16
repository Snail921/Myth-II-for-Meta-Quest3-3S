// Test-only DLL entry: exposes the actual hook dispatcher for isolated CPU tests.
#include "../src/input/game_hook.c"
__declspec(dllexport) U MythTestExecute(const struct Command*c){return execute(c);}
