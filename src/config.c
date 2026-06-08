#include <string.h>

#include "../include/config.h"

CompilerConfig config = {
    .jumpBeforeFuncstart   = CONFIG_JUMP_BEFORE_FUNCSTART_DEFAULT,
    .jumpAfterReturn       = CONFIG_JUMP_AFTER_RETURN_DEFAULT,
    .shortCircuitBackpatch = CONFIG_SHORT_CIRCUIT_BACKPATCH_DEFAULT,
};

bool config_TryParseFlag(const char* arg){
    if(strcmp(arg, "--funcstart-jump") == 0)            { config.jumpBeforeFuncstart   = true;  return true; }
    if(strcmp(arg, "--no-funcstart-jump") == 0)         { config.jumpBeforeFuncstart   = false; return true; }
    if(strcmp(arg, "--return-jump") == 0)               { config.jumpAfterReturn       = true;  return true; }
    if(strcmp(arg, "--no-return-jump") == 0)            { config.jumpAfterReturn       = false; return true; }
    if(strcmp(arg, "--short-circuit-backpatch") == 0)   { config.shortCircuitBackpatch = true;  return true; }
    if(strcmp(arg, "--no-short-circuit-backpatch") == 0){ config.shortCircuitBackpatch = false; return true; }
    return false;
}
