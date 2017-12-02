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

/*Lab5: Your implementation here.*/
F_accessList F_AccessList(F_access head, F_accessList tail) {
    F_accessList p = (F_accessList)checked_malloc(sizeof *p);
    p->head = head;
    p->tail = tail;
    return p;
}

//varibales
struct F_access_ {
	enum {inFrame, inReg} kind;
	union {
		int offset; //inFrame
		Temp_temp reg; //inReg
	} u;
};
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

    int argSize;    // number of auguments
    int localSize;  // number of local vars
};

F_frame F_newFrame(Temp_label name, U_boolList formals) {
    F_frame p = (F_frame)checked_malloc(sizeof *p);
    p->name = name;
    p->formals = F_AccessList(NULL, NULL);
    F_accessList accList = p->formals;
    while (formals && formals->head) {
        if (formals->head == TRUE) {
            accList->head = InFrame(0);
            accList->tail = F_AccessList(NULL, NULL);
        } else {
            Temp_temp r = Temp_newtemp();
            accList->head = InReg(r);
            accList->tail = F_AccessList(NULL, NULL);
        }
        accList = accList->tail;
        formals = formals->tail;   //TODO
    }
    return p;
}

Temp_label F_name(F_frame f) {
    return f->name;
}
F_accessList F_formals(F_frame f) {
    return f->formals;
}

F_access F_allocLocal(F_frame f, bool escape) {
    F_access acc;
    if (escape)
        acc = InFrame(0);//acc = InFrame();
    else {
        Temp_temp r = Temp_newtemp();
        acc = InReg(r);
    }
    f->locals = F_AccessList(acc, f->locals); // TODO and the order
    return acc;
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

Temp_tempList F_SpecialRegs() {
    return NULL; // TODO
}

Temp_tempList F_ArgRegs() {
    return NULL; // TODO
}

Temp_tempList F_CalleeSaves() {
    return NULL; // TODO
}

Temp_tempList F_CallerSaves() {
    return NULL; // TODO
}

Temp_temp F_FP(void) {
    return Temp_newtemp();  // TODO: ??
}

Temp_temp F_SP(void) {
    return Temp_newtemp();  // TODO: ??
}

Temp_temp F_EAX(void) {
    return Temp_newtemp(); //TODO
}

Temp_temp F_EDX(void) {
    return Temp_newtemp(); //TODO
}

Temp_temp F_RV(void) {
    return Temp_newtemp();
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

T_stm F_procEntryExit1(F_frame frame, T_stm stm) {
    return stm; //TODO
}
