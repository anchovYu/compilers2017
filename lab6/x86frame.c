#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"
#include "symbol.h"
#include "temp.h"
#include "table.h"
#include "tree.h"
#include "frame.h"
#include "helper.h"

const int F_wordSize = 4;

// varibales
struct F_access_ {
	enum {inFrame, inReg} kind;
	union {
		int offset;       //inFrame
		Temp_temp reg;    //inReg
	} u;
};

F_accessList F_AccessList(F_access head, F_accessList tail) {
    F_accessList p = (F_accessList)checked_malloc(sizeof *p);
    p->head = head;
    p->tail = tail;
    return p;
}

static F_access InFrame(int offset) {
    F_access p = (F_access)checked_malloc(sizeof *p);
    p->kind = inFrame;
    p->u.offset = offset;
    return p;
}
static F_access InReg(Temp_temp reg) {
    F_access p = (F_access)checked_malloc(sizeof *p);
    p->kind = inReg;
    p->u.reg = reg;
    return p;
}

struct F_frame_ {
    Temp_label name;
    F_accessList formals;
    F_accessList locals;
    int localCnt;  // number of local vars on stack
};
static F_accessList reverseAccessList(F_accessList accList) {
    F_accessList res = NULL;
    while (accList && accList->head) {
        res = F_AccessList(accList->head, res);
        accList = accList->tail;
    }
    return res;
}

// alloc a new frame, with all formals on stack.
F_frame F_newFrame(Temp_label name, U_boolList formals) {
    F_frame p = (F_frame)checked_malloc(sizeof *p);
    p->name = name;
    p->locals = NULL;
    p->localCnt = 0;

    F_accessList accList = NULL;
    // formals starts from %ebp + 8 (including static link)
    int offset = 8;
    U_boolList tmp = formals;
    while (tmp != NULL) {
        accList = F_AccessList(InFrame(offset), accList);
        offset += 4;
        tmp = tmp->tail;
    }
    // reverse the formals to promise the head is static link
    p->formals = reverseAccessList(accList);
    return p;
// TODO: note that formals are in reverse order. Done.
}

Temp_label F_name(F_frame f) {
    return f->name;
}

F_accessList F_formals(F_frame f) {
    return f->formals;
}

F_access F_allocLocal(F_frame f, bool escape) {
    F_access acc;
    if (escape) {
        f->localCnt ++;
        acc = InFrame((-4) * (f->localCnt));
        printf("(F_allocLocal) alloc local in frame, offset %d.\n", (-4) * (f->localCnt));
    }
    else {
        Temp_temp r = Temp_newtemp();
        acc = InReg(r);
        printf("F_allocLocal: alloc local in reg %d.\n", Temp_int(r));
    }
    f->locals = F_AccessList(acc, f->locals);
    return acc;
// TODO: note that locals are in reverse order.
}

// alloc an escape local var in process of register allocation
int F_allocSpill(F_frame f) {
    F_access acc = F_allocLocal(f, TRUE);
    return acc->u.offset;
}


F_frag F_StringFrag(Temp_label label, string str) {
    F_frag p = (F_frag)checked_malloc(sizeof *p);
    p->kind = F_stringFrag;
    p->u.stringg.label = label;
    p->u.stringg.str = str;
    return p;
}

F_frag F_ProcFrag(T_stm body, F_frame frame) {
    F_frag p = (F_frag)checked_malloc(sizeof *p);
    p->kind = F_procFrag;
    p->u.proc.body = body;
    p->u.proc.frame = frame;
    return p;
}

F_fragList F_FragList(F_frag head, F_fragList tail) {
    F_fragList p = (F_fragList)checked_malloc(sizeof *p);
    p->head = head;
    p->tail = tail;
    return p;
}


/* caller eax edx ecx
   callee ebx esi edi */
static Temp_temp fp = NULL;
static Temp_temp sp = NULL;
static Temp_temp eax = NULL;
static Temp_temp ecx = NULL;
static Temp_temp edx = NULL;
static Temp_temp ebx = NULL;
static Temp_temp esi = NULL;
static Temp_temp edi = NULL;

Temp_temp F_FP(void) {
    if (!fp)
        fp = Temp_newtemp();
    return fp;
}

Temp_temp F_SP(void) {
    if (!sp)
        sp = Temp_newtemp();
    return sp;
}

Temp_temp F_RV(void) {
    if (!eax)
        eax = Temp_newtemp();
    return eax;
}

Temp_temp F_EAX(void) {
    if (!eax)
        eax = Temp_newtemp();
    return eax;
}

Temp_temp F_ECX(void) {
    if (!ecx)
        ecx = Temp_newtemp();
    return ecx;
}

Temp_temp F_EDX(void) {
    if (!edx)
        edx = Temp_newtemp();
    return edx;
}

Temp_temp F_EBX(void) {
    if (!ebx)
        ebx = Temp_newtemp();
    return ebx;
}

Temp_temp F_ESI(void) {
    if (!esi)
        esi = Temp_newtemp();
    return esi;
}

Temp_temp F_EDI(void) {
    if (!edi)
        edi = Temp_newtemp();
    return edi;
}

static Temp_tempList callersaves = NULL;
static Temp_tempList calleesaves = NULL;

/* caller eax edx ecx
   callee ebx esi edi */
Temp_tempList F_CallerSaves() {
   if (!callersaves)
       callersaves = Temp_TempList(F_EAX(), Temp_TempList(F_EDX(), Temp_TempList(F_ECX(), NULL)));
   return callersaves;
}

Temp_tempList F_CalleeSaves() {
    if (!calleesaves)
        calleesaves = Temp_TempList(F_EBX(), Temp_TempList(F_ESI(), Temp_TempList(F_EDI(), NULL)));
    return calleesaves;
}

static Temp_map initmap = NULL;
Temp_map F_initialTempMap() {
    if (!initmap) {
        initmap = Temp_empty();
        Temp_enter(initmap, F_EAX(), "%eax");
        Temp_enter(initmap, F_ECX(), "%ecx");
        Temp_enter(initmap, F_EDX(), "%edx");
        Temp_enter(initmap, F_EBX(), "%ebx");
        Temp_enter(initmap, F_ESI(), "%esi");
        Temp_enter(initmap, F_EDI(), "%edi");
        Temp_enter(initmap, F_FP(),  "%ebp");
        Temp_enter(initmap, F_SP(),  "%esp");
    }
    return initmap;
}


// turn a F_access @acc declared in @framePtr to a T_exp
T_exp F_Exp(F_access acc, T_exp framePtr) {
    // in frame
    if (acc->kind == inFrame) {
         return T_Mem(T_Binop(T_plus, framePtr, T_Const(get_access_offset(acc))));
    } // in reg
    return T_Temp(get_access_reg(acc));
}

T_exp F_externalCall(string s, T_expList args) {
    return T_Call(T_Name(Temp_namedlabel(s)), args);
}


static Temp_tempList L(Temp_temp temp, Temp_tempList l) {
    return Temp_TempList(temp, l);
}


// move calleesaves to or from registers,
// before register allocation, after codegen
// calleesave: ebx esi edi
AS_instrList F_procEntryExit1(AS_instrList body) {
    AS_instrList pro = NULL;
    AS_instrList epi = NULL;
    AS_instrList res;

    Temp_temp ebx_tmp = Temp_newtemp();
    Temp_temp esi_tmp = Temp_newtemp();
    Temp_temp edi_tmp = Temp_newtemp();

    pro = AS_InstrList(AS_Move("movl `s0, `d0", L(ebx_tmp, NULL), L(F_EBX(), NULL)), pro);
    pro = AS_InstrList(AS_Move("movl `s0, `d0", L(esi_tmp, NULL), L(F_ESI(), NULL)), pro);
    pro = AS_InstrList(AS_Move("movl `s0, `d0", L(edi_tmp, NULL), L(F_EDI(), NULL)), pro);

    epi = AS_InstrList(AS_Move("movl `s0, `d0", L(F_EBX(), NULL), L(ebx_tmp, NULL)), epi);
    epi = AS_InstrList(AS_Move("movl `s0, `d0", L(F_ESI(), NULL), L(esi_tmp, NULL)), epi);
    epi = AS_InstrList(AS_Move("movl `s0, `d0", L(F_EDI(), NULL), L(edi_tmp, NULL)), epi);

    res = AS_splice(pro, body);
    res = AS_splice(res, epi);
    return res;
}

// TODO: actually, sp is not needed.
// sp + calleesaves(ebx, esi, edi)
static Temp_tempList returnsink = NULL;

// add live to end,
// before register allocation, after codegen
AS_instrList F_procEntryExit2(AS_instrList body) {
    // if (!returnsink)
    //     returnsink = Temp_TempList(F_SP(),
    //                     Temp_TempList(F_EBX(),
    //                         Temp_TempList(F_ESI(),
    //                             Temp_TempList(F_EDI(), NULL))));
    return AS_splice(body,
                     AS_InstrList(AS_Oper("", NULL, F_CalleeSaves(), NULL), NULL));
}

// entry and exit for func,
// after register allocation
AS_instrList F_procEntryExit3(F_frame f, AS_instrList body) {
    AS_instrList pro = NULL;
    AS_instrList epi = NULL;
    AS_instrList res;

    char* inst = checked_malloc(80);
    sprintf(inst, "subl $%d, `d0", 4 * (f->localCnt));
    pro = AS_InstrList(AS_Oper(inst, L(F_SP(), NULL), NULL, NULL), pro);    // 3
    pro = AS_InstrList(AS_Move("movl `s0, `d0", L(F_FP(), NULL), L(F_SP(), NULL)), pro);// 2
    pro = AS_InstrList(AS_Oper("pushl `s0", NULL, L(F_FP(), NULL), NULL), pro);   // 1

    // TODO: remove all ebp and esp liveness.
    epi = AS_InstrList(AS_Oper("ret", NULL, NULL, NULL), epi);  // 2
    epi = AS_InstrList(AS_Oper("leave", NULL, NULL, NULL), epi); // 1

    res = AS_splice(pro, body);
    res = AS_splice(res, epi);
    return res;
}
