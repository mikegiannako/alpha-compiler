#include "../include/avm_trace.h"
#include "../include/avm_structs.h"
#include "../include/avm_dispatcher.h"

#include <stdio.h>

unsigned char vmTraceInstr = 0;
unsigned char vmTraceStack = 0;

// ------------------------------- instruction printing -------------------------------

static const char* argTypeName(VMArgType_enum t){
    switch(t){
        case LABEL_IARGTYPE:    return "label";
        case GLOBAL_IARGTYPE:   return "global";
        case FORMAL_IARGTYPE:   return "formal";
        case LOCAL_IARGTYPE:    return "local";
        case NUMBER_IARGTYPE:   return "number";
        case STRING_IARGTYPE:   return "string";
        case BOOL_IARGTYPE:     return "bool";
        case NIL_IARGTYPE:      return "nil";
        case USERFUNC_IARGTYPE: return "userfunc";
        case LIBFUNC_IARGTYPE:  return "libfunc";
        case RETVAL_IARGTYPE:   return "retval";
        default:                return "";
    }
}

static void printArg(FILE* out, VMArg arg){
    char buf[40];
    if(arg.type == NULL_IARGTYPE){
        buf[0] = '\0';
    } else {
        snprintf(buf, sizeof(buf), "(%s),%u", argTypeName(arg.type), arg.val);
    }
    fprintf(out, "%-18s", buf);
}

void VMInstruction_Print(FILE* out, VMInstruction* instr, unsigned pc){
    fprintf(out, "[%3u] L%-3u %-13s ", pc, instr->srcLine, VMOpcodeStrings[instr->opcode]);
    printArg(out, instr->result);
    printArg(out, instr->arg1);
    printArg(out, instr->arg2);
    fprintf(out, "\n");
}

// ------------------------------- stack dumping -------------------------------

// Brief, cycle-safe rendering of a cell (tables are shown as a handle).
static void cellBrief(VMMemCell_ptr c, char* buf, size_t n){
    switch(c->type){
        case NUMBER_MEMCELL:
            snprintf(buf, n, "%g", c->data.number); break;
        case STRING_MEMCELL:
            snprintf(buf, n, "\"%s\"", c->data.string ? c->data.string : ""); break;
        case BOOL_MEMCELL:
            snprintf(buf, n, "%s", c->data.boolean ? "true" : "false"); break;
        case TABLE_MEMCELL:
            snprintf(buf, n, "table@%p (total=%u)", (void*)c->data.table,
                     c->data.table ? c->data.table->total : 0); break;
        case USERFUNC_MEMCELL:
            if(c->data.userfunc < vm_program.user_functions_count)
                snprintf(buf, n, "userfunc#%u '%s'", c->data.userfunc,
                         vm_program.user_functions[c->data.userfunc].name);
            else
                snprintf(buf, n, "userfunc#%u", c->data.userfunc);
            break;
        case LIBFUNC_MEMCELL:
            snprintf(buf, n, "libfunc '%s'", c->data.libfunc ? c->data.libfunc : ""); break;
        case NIL_MEMCELL:
            snprintf(buf, n, "nil"); break;
        case UNDEFINED_MEMCELL:
            snprintf(buf, n, "undefined"); break;
        default:
            snprintf(buf, n, "?"); break;
    }
}

// Read a stack cell as an unsigned env value (numbers only); 0 otherwise.
static unsigned envAt(unsigned idx){
    if(idx >= AVM_STACK_SIZE || ExecutionStack[idx]->type != NUMBER_MEMCELL) return 0;
    return (unsigned)ExecutionStack[idx]->data.number;
}

void VMStack_Dump(FILE* out){
    char buf[160];

    fprintf(out, "---- stack @PC=%u  BP=%u  SP=%u  globals=%u ----\n",
            VM_PC, VM_BP, VM_SP, globalCount);

    // Globals live at the top of the stack.
    for(unsigned off = 0; off < globalCount; off++){
        unsigned idx = AVM_STACK_SIZE - 1 - off;
        cellBrief(ExecutionStack[idx], buf, sizeof(buf));
        fprintf(out, "  [%4u] global %-3u = %s\n", idx, off, buf);
    }

    // Call frames, innermost first, walked via the saved BP.
    unsigned globalFrameBP = AVM_STACK_SIZE - globalCount - 1;
    unsigned bp = VM_BP;
    unsigned localsFloor = VM_SP + 1;   // innermost frame's locals stop at SP
    int depth = 0;

    while(bp != globalFrameBP && depth < 128){
        unsigned numActuals = envAt(bp + VM_NUMACTUALS_OFFSET);
        unsigned savedBP    = envAt(bp + VM_SAVEDBP_OFFSET);
        unsigned savedSP    = envAt(bp + VM_SAVEDSP_OFFSET);
        unsigned savedPC    = envAt(bp + VM_SAVEDPC_OFFSET);

        fprintf(out, "  frame#%d @BP=%u  numActuals=%u  (savedPC=%u savedSP=%u savedBP=%u)\n",
                depth, bp, numActuals, savedPC, savedSP, savedBP);

        // Formals / actuals: bp+5 .. bp+5+numActuals-1
        for(unsigned i = 0; i < numActuals; i++){
            unsigned idx = bp + VM_STACK_ENV_SIZE + 1 + i;
            cellBrief(ExecutionStack[idx], buf, sizeof(buf));
            fprintf(out, "    [%4u] formal %-3u = %s\n", idx, i, buf);
        }

        // Locals: bp (local 0) down to localsFloor.
        if(bp >= localsFloor){
            for(unsigned idx = bp; ; idx--){
                cellBrief(ExecutionStack[idx], buf, sizeof(buf));
                fprintf(out, "    [%4u] local  %-3u = %s\n", idx, bp - idx, buf);
                if(idx == localsFloor) break;
            }
        }

        // The caller's locals stop just above this frame's topmost cell.
        localsFloor = bp + VM_STACK_ENV_SIZE + 1 + numActuals;
        bp = savedBP;
        depth++;
    }
    fprintf(out, "----------------------------------------------\n");
}

void VMTrace_Step(VMInstruction* instr, unsigned pc){
    if(vmTraceInstr) VMInstruction_Print(stderr, instr, pc);
    if(vmTraceStack) VMStack_Dump(stderr);
}
