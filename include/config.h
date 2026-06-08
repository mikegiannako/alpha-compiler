#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <stdbool.h>

/*
 * Compiler feature toggles.
 *
 * These select between behaviours that are all valid for Phase 3 but that
 * students implemented differently. They exist so this reference compiler can
 * reproduce a given student's quad output for grading.
 *
 * The flags are runtime values (a single global `config`) so that a future
 * Emscripten build can flip them from JS checkboxes without recompiling. Their
 * defaults are #define'd below, so the build can still pin a default with -D...
 * and the CLI can override per-run with the flags parsed by config_ParseArgs().
 */

// Emit a `jump` before every `funcstart` (patched to skip the function body
// during sequential execution). Phase-4 nicety some students already did.
#ifndef CONFIG_JUMP_BEFORE_FUNCSTART_DEFAULT
#define CONFIG_JUMP_BEFORE_FUNCSTART_DEFAULT true
#endif

// Emit a `jump` after every `ret` (patched to the function's `funcend`).
#ifndef CONFIG_JUMP_AFTER_RETURN_DEFAULT
#define CONFIG_JUMP_AFTER_RETURN_DEFAULT true
#endif

// Short-circuit a condition used in if/while/for by backpatching its
// true/false lists straight into the branch, instead of materialising the
// boolean into a temp and re-testing it with an if_eq.
#ifndef CONFIG_SHORT_CIRCUIT_BACKPATCH_DEFAULT
#define CONFIG_SHORT_CIRCUIT_BACKPATCH_DEFAULT false
#endif

typedef struct CompilerConfig {
    bool jumpBeforeFuncstart;
    bool jumpAfterReturn;
    bool shortCircuitBackpatch;
} CompilerConfig;

extern CompilerConfig config;

// Returns true and applies the flag if `arg` is a recognised toggle.
bool config_TryParseFlag(const char* arg);

#endif
