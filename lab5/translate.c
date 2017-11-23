#include <stdio.h>
#include "util.h"
#include "table.h"
#include "symbol.h"
#include "absyn.h"
#include "temp.h"
#include "tree.h"
#include "printtree.h"
#include "frame.h"
#include "translate.h"

//LAB5: you can modify anything you want.

F_fragList fragList;

struct Tr_access_ {
	Tr_level level;
	F_access access;
};
struct Tr_level_ {
	F_frame frame;
	Tr_level parent;
};
Tr_access Tr_Access(Tr_level level, F_access access) {
    Tr_access p = (Tr_access)checked_malloc(sizeof *p);
    p->level = level;
    p->access = access;
    return p;
}
Tr_accessList Tr_AccessList(Tr_access head, Tr_accessList tail) {
    Tr_accessList p = (Tr_accessList)checked_malloc(sizeof *p);
    p->head = head;
    p->tail = tail;
    return p;
}

Tr_expList Tr_ExpList(Tr_exp head, Tr_expList tail) {
    Tr_expList p = (Tr_expList)checked_malloc(sizeof *p);
    p->head = head;
    p->tail = tail;
    return p;
}

typedef struct patchList_ *patchList;
struct patchList_ {
	Temp_label *head;
	patchList tail;
};
static patchList PatchList(Temp_label *head, patchList tail) {
	patchList list;

	list = (patchList)checked_malloc(sizeof(struct patchList_));
	list->head = head;
	list->tail = tail;
	return list;
}

void doPatch(patchList tList, Temp_label label) {
	for(; tList; tList = tList->tail)
		*(tList->head) = label;
}

patchList joinPatch(patchList first, patchList second) {
	if(!first) return second;
	for(; first->tail; first = first->tail);
	first->tail = second;
	return first;
}

struct Cx {
	patchList trues;
	patchList falses;
	T_stm stm;
};
struct Tr_exp_ {
	enum {Tr_ex, Tr_nx, Tr_cx} kind;
	union {T_exp ex; T_stm nx; struct Cx cx; } u;
};

static Tr_exp Tr_Ex(T_exp ex) {
    Tr_exp p = (Tr_exp)checked_malloc(sizeof *p);
    p->kind = Tr_ex;
    p->u.ex = ex;
    return p;
}

static Tr_exp Tr_Nx(T_stm nx) {
    Tr_exp p = (Tr_exp)checked_malloc(sizeof *p);
    p->kind = Tr_nx;
    p->u.nx = nx;
    return p;
}

static Tr_exp Tr_Cx(patchList trues, patchList falses, T_stm stm) {
    Tr_exp p = (Tr_exp)checked_malloc(sizeof *p);
    p->kind = Tr_cx;
    p->u.cx.trues = trues;
    p->u.cx.falses = falses;
    p->u.cx.stm = stm;
    return p;
}

static T_exp unEx(Tr_exp e) {
    switch (e->kind) {
        case Tr_ex:
            return e->u.ex;
        case Tr_cx: {
            Temp_temp r = Temp_newtemp();
            Temp_label t = Temp_newlabel(), f = Temp_newlabel();
            doPatch(e->u.cx.trues, t);
            doPatch(e->u.cx.falses, f);
            return T_Eseq(T_Move(T_Temp(r), T_Const(1)),
                    T_Eseq(e->u.cx.stm,
                        T_Eseq(T_Label(f),
                            T_Eseq(T_Move(T_Temp(r), T_Const(0)),
                                T_Eseq(T_Label(t),
                                    T_Temp(r))))));
        }
        case Tr_nx:
            return T_Eseq(e->u.nx, T_Const(0));
    }
    assert(0); /* can’t get here */
}

static T_stm unNx(Tr_exp e) {
    switch (e->kind) {
        case Tr_ex:
            // discard the result
            return T_Exp(e->u.ex);
        case Tr_nx:
            return e->u.nx;
        case Tr_cx: {
            Temp_label done = Temp_newlabel();
            doPatch(e->u.cx.trues, done);
            doPatch(e->u.cx.falses, done);
            return T_Seq(e->u.cx.stm, T_Label(done));

        }
    }
    assert(0); /* can’t get here */
}

static struct Cx unCx(Tr_exp e) {
    switch (e->kind) {
        case Tr_ex: {
            struct Cx cx;
            cx.stm = T_Cjump(T_ne, e->u.ex, T_Const(0), NULL, NULL);
            cx.trues = PatchList(&(cx.stm->u.CJUMP.true), NULL);
            cx.falses = PatchList(&(cx.stm->u.CJUMP.false), NULL);
            return cx;
        }
        case Tr_nx: {
            assert(0); /* can’t get here */
        }
        case Tr_cx:
            return e->u.cx;
    }
    assert(0); /* can’t get here */
}



// TODO
Tr_level outermost;
Tr_level Tr_outermost(void) {
    if (outermost)
        return outermost;
    else {
        outermost = Tr_newLevel(NULL, Temp_newlabel(), NULL);
        return outermost;
    }
}
Tr_level Tr_newLevel(Tr_level parent, Temp_label name, U_boolList formals) {
    Tr_level p = (Tr_level)checked_malloc(sizeof *p);
    p->frame = F_newFrame(name, formals);
    p->parent = parent;
    return p;
}
Tr_accessList makeTrAccList(Tr_level level, F_accessList f_accList) {
    // note the order.
    Tr_accessList res = NULL;
    F_accessList accList = f_accList;
    if (accList && accList->head) {
        Tr_access tr_access = Tr_Access(level, accList->head);
        res = Tr_AccessList(tr_access, NULL);
        accList = accList->tail;
    }
    Tr_accessList temp = res;
    while (accList && accList->head) {
        Tr_access tr_access = Tr_Access(level, accList->head);
        temp->tail = Tr_AccessList(tr_access, NULL);
        temp = temp->tail;
        accList = accList->tail;
    }
    return res;
}

Tr_accessList Tr_formals(Tr_level level) {
    F_accessList f_acclist = F_formals(level->frame);
    return makeTrAccList(level, f_acclist);

}
Tr_access Tr_allocLocal(Tr_level level, bool escape) {
    F_access f_acc = F_allocLocal(level->frame, escape);
    return Tr_Access(level, f_acc);
}



static T_exp makeStaticLink(Tr_level cur, Tr_level need) {
    Tr_level temp = cur;
    T_exp framePtr = T_Temp(F_FP());
    if (need == Tr_outermost()) printf("emheng.\n");
    while (temp != need) {
        // the first param is static link
        if (temp == Tr_outermost()) printf("finally..\n");
        framePtr = F_Exp(F_formals(temp->frame)->head, framePtr);
        temp = temp->parent;
    }
    return framePtr;
}

Tr_exp Tr_nop(){
    return Tr_Ex(T_Const(0));
}

Tr_exp Tr_Nil() {
    return Tr_Ex(T_Const(0));   // TODO: what should nil return?
}

Tr_exp Tr_Int(int i) {
    return Tr_Ex(T_Const(i));
}

Tr_exp Tr_String(string str) {
    Temp_label lab = Temp_newlabel();
    // TODO: fragments???
    printf("\tadd string fragment: %s\n", str);
    fragList = F_FragList(F_StringFrag(lab, str), fragList);
    return Tr_Ex(T_Name(lab));
}

Tr_exp Tr_Call(Temp_label func, Tr_expList inParams, Tr_level caller, Tr_level callee) {
    T_expList params = NULL;
    Tr_expList tr_expList = inParams;
    // TODO: note the params order
    while (tr_expList && tr_expList->head) {
        params = T_ExpList(unEx(tr_expList->head), params);
        tr_expList = tr_expList->tail;
    }
    T_exp sl = makeStaticLink(caller, callee->parent);
    return Tr_Ex(T_Call(T_Name(func), T_ExpList(sl, params)));
}

Tr_exp Tr_Op(A_oper oper, Tr_exp left, Tr_exp right) {
    T_exp l = unEx(left);
    T_exp r = unEx(right);
    switch (oper) {
        case A_plusOp: {
            return Tr_Ex(T_Binop(T_plus, l, r));
        }
        case A_minusOp: {
            return Tr_Ex(T_Binop(T_minus, l, r));
        }
        case A_timesOp: {
            return Tr_Ex(T_Binop(T_mul, l, r));
        }
        case A_divideOp: {
            return Tr_Ex(T_Binop(T_div, l, r));
        }
        default: return NULL; // TODO
    }
}

Tr_exp Tr_OpCom(A_oper oper, Tr_exp left, Tr_exp right, bool isString) {
    if (isString) {
        T_expList args = T_ExpList(unEx(left), T_ExpList(unEx(right), NULL));
        if (oper == A_eqOp)
            return Tr_Ex(F_externalCall("stringEqual", args));
        else {
            /* mov (call res ^ 1) to reg r
             * r */
            Temp_temp r = Temp_newtemp();
            return Tr_Ex(T_Eseq(T_Move(T_Temp(r),
                                       T_Binop(T_xor, F_externalCall("stringEqual", args), T_Const(1))),
                                T_Temp(r)));
        }
    }

    T_exp l = unEx(left);
    T_exp r = unEx(right);
    switch (oper){
        case A_eqOp: {
            T_stm nx = T_Cjump(T_eq, l, r, NULL, NULL);
            patchList trues = PatchList(&(nx->u.CJUMP.true), NULL);
            patchList falses = PatchList(&(nx->u.CJUMP.false), NULL);
            return Tr_Cx(trues, falses, nx);
        }
        case A_neqOp: {
            T_stm nx = T_Cjump(T_ne, l, r, NULL, NULL);
            patchList trues = PatchList(&(nx->u.CJUMP.true), NULL);
            patchList falses = PatchList(&(nx->u.CJUMP.false), NULL);
            return Tr_Cx(trues, falses, nx);
        }
        case A_ltOp: {
            T_stm nx = T_Cjump(T_lt, l, r, NULL, NULL);
            patchList trues = PatchList(&(nx->u.CJUMP.true), NULL);
            patchList falses = PatchList(&(nx->u.CJUMP.false), NULL);
            return Tr_Cx(trues, falses, nx);
        }
        case A_leOp: {
            T_stm nx = T_Cjump(T_le, l, r, NULL, NULL);
            patchList trues = PatchList(&(nx->u.CJUMP.true), NULL);
            patchList falses = PatchList(&(nx->u.CJUMP.false), NULL);
            return Tr_Cx(trues, falses, nx);
        }
        case A_gtOp: {
            T_stm nx = T_Cjump(T_gt, l, r, NULL, NULL);
            patchList trues = PatchList(&(nx->u.CJUMP.true), NULL);
            patchList falses = PatchList(&(nx->u.CJUMP.false), NULL);
            return Tr_Cx(trues, falses, nx);
        }
        case A_geOp: {
            T_stm nx = T_Cjump(T_ge, l, r, NULL, NULL);
            patchList trues = PatchList(&(nx->u.CJUMP.true), NULL);
            patchList falses = PatchList(&(nx->u.CJUMP.false), NULL);
            return Tr_Cx(trues, falses, nx);
        }
        default: return NULL; //TODO
    }
    assert(0); /* can’t get here */

}

Tr_exp Tr_record(Tr_expList fields) {
    /* mov malloc addr to reg r
     * mov r to a
     * mov field to mem(a)
     * a = a + i * ws
     * ...
     * r */
    printf("\tenter Tr_record\n");
    Temp_temp r = Temp_newtemp();
    Temp_temp a = Temp_newtemp();
    T_exp res = T_Eseq(NULL, NULL);
    T_exp tmp = res;

    tmp->u.ESEQ.exp = T_Eseq(NULL, NULL);
    tmp = tmp->u.ESEQ.exp;

    tmp->u.ESEQ.stm = T_Move(T_Temp(a), T_Temp(r));
    tmp->u.ESEQ.exp = T_Eseq(NULL, NULL);
    tmp = tmp->u.ESEQ.exp;

    // TODO: now the order maybe right
    T_expList params = NULL;
    Tr_expList tr_expList = fields;
    int cnt = 0;
    while (tr_expList && tr_expList->head) {
        cnt++;
        params = T_ExpList(unEx(tr_expList->head), params);
        tr_expList = tr_expList->tail;
    }
    // TODO: alloc
    res->u.ESEQ.stm = T_Move(T_Temp(r), F_externalCall("malloc", T_ExpList(T_Binop(T_minus, T_Const(cnt), T_Const(F_wordSize)), NULL)));


    while (params && params->head) {
        tmp->u.ESEQ.stm = T_Move(T_Mem(T_Temp(a)), params->head);
        tmp->u.ESEQ.exp = T_Eseq(NULL, NULL);
        tmp = tmp->u.ESEQ.exp;
        params = params->tail;
    }

    // TODO: awkward, get one more T_eseq. like this seq(a, seq(null, null)) => seq(a, useful)
    tmp->u.ESEQ.stm = T_Exp(T_Temp(r));
    tmp->u.ESEQ.exp = T_Temp(r);

    return Tr_Ex(res);
}

Tr_exp Tr_array(Tr_exp size, Tr_exp init) {
    return Tr_Ex(F_externalCall("initArray", T_ExpList(unEx(size), T_ExpList(unEx(init), NULL))));
}

Tr_exp Tr_assign(Tr_exp lhs, Tr_exp rhs) {
    return Tr_Nx(T_Move(unEx(lhs), unEx(rhs)));
}

Tr_exp Tr_if(Tr_exp test, Tr_exp then, Tr_exp elsee) {
    /* test(true goto t, false goto f)
     * t: then, mov res to reg r, goto done
     * f: elsee, mov res to reg r
     * done: r
     */
    if (elsee) {
        Temp_temp r = Temp_newtemp();
        Temp_label t = Temp_newlabel();
        Temp_label f = Temp_newlabel();
        Temp_label done = Temp_newlabel();

        struct Cx testCx = unCx(test);
        doPatch(testCx.trues, t);
        doPatch(testCx.falses, f);

        return Tr_Ex(T_Eseq(testCx.stm,
                        T_Eseq(T_Label(t),
                          T_Eseq(T_Move(T_Temp(r), unEx(then)),
                            T_Eseq(T_Jump(T_Name(done), Temp_LabelList(done, NULL)),
                              T_Eseq(T_Label(f),
                                T_Eseq(T_Move(T_Temp(r), unEx(elsee)),
                                  T_Eseq(T_Label(done),
                                    T_Temp(r)))))))));
    } else {
    /* test(true goto t, false goto done)
     * t: then
     * done:
     */
        Temp_label t = Temp_newlabel();
        Temp_label done = Temp_newlabel();

        struct Cx testCx = unCx(test);
        doPatch(testCx.trues, t);
        doPatch(testCx.falses, done);

        return Tr_Nx(T_Seq(testCx.stm,
                       T_Seq(T_Label(t),
                         T_Seq(unNx(then), T_Label(done)))));
    }

}

Tr_exp Tr_while(Tr_exp test, Tr_exp body, Temp_label breakk) {
    /* testLabel:
     * test (true goto t, false goto breakk)
     * t: body, goto testLabel
     * breakk:
     */
    Temp_label testLabel = Temp_newlabel();
    Temp_label t = Temp_newlabel();

    struct Cx testCx = unCx(test);
    doPatch(testCx.trues, t);
    doPatch(testCx.falses, breakk);

    return Tr_Nx(T_Seq(T_Label(testLabel),
                  T_Seq(testCx.stm,
                    T_Seq(T_Label(t),
                      T_Seq(unNx(body),
                        T_Seq(T_Jump(T_Name(testLabel), Temp_LabelList(testLabel, NULL)),
                              T_Label(breakk)))))));

}

Tr_exp Tr_for(Tr_exp lo, Tr_exp hi, Tr_exp body, Tr_exp loopVar, Temp_label breakk) {
    /* mov lo to loopvar, mov hi to reg r
     * lo <= hi(true goto loopbody, false goto breakk)
     * loopbody: body, loopvar < r(true goto t,false goto breakk)
     * t: mov loopvar to a, a + 1, mov a to loopvar, goto loopbody
     * breakk
     */
    // TODO: should I move hi or loopvar to a reg for optimization?
    // TODO: optimization.
    Temp_temp r = Temp_newtemp();
    Temp_temp a = Temp_newtemp();
    Temp_label t = Temp_newlabel();
    Temp_label loopbody = Temp_newlabel();

    T_exp loEx = unEx(lo);
    T_exp hiEx = unEx(hi);
    T_exp loopVarEx = unEx(loopVar);

    return Tr_Nx(T_Seq(T_Move(loopVarEx, loEx),
                  T_Seq(T_Move(T_Temp(r), hiEx),
                    T_Seq(T_Cjump(T_le, loEx, hiEx, loopbody, breakk),
                      T_Seq(T_Label(loopbody),
                        T_Seq(unNx(body),
                          T_Seq(T_Cjump(T_lt, loopVarEx, T_Temp(r), t, breakk),
                            T_Seq(T_Label(t),
                              T_Seq(T_Move(T_Temp(a), loopVarEx),
                                T_Seq(T_Move(loopVarEx, T_Binop(T_plus, T_Temp(a), T_Const(1))),
                                  T_Seq(T_Jump(T_Name(loopbody), Temp_LabelList(loopbody, NULL)),
                                        T_Label(breakk))))))))))));

}

Tr_exp Tr_break(Temp_label breakk) {
    // TODO: break handling.
    return Tr_Nx(T_Jump(T_Name(breakk), Temp_LabelList(breakk, NULL)));
}

// retrieve the @index th element of a given @var (array or record)
Tr_exp Tr_indexVar(Tr_exp var, Tr_exp index) {
    // mem(var + i * ws)
    return Tr_Ex(T_Mem(T_Binop(T_plus,
                               unEx(var),
                               T_Binop(T_mul, unEx(index), T_Const(F_wordSize)))));
}

// turn an var(@access) used in @level to a Tr_exp
Tr_exp Tr_simpleVar(Tr_access access, Tr_level level) {
    if (access->level == level)
        return Tr_Ex(F_Exp(access->access, T_Temp(F_FP())));
    else
        return Tr_Ex(F_Exp(access->access, makeStaticLink(level, access->level)));
}




// TODO!
void Tr_procEntryExit(Tr_level level, Tr_exp body, Tr_accessList formals) {
    printf("\tadd prog fragment\n");
    fragList = F_FragList(F_ProcFrag(NULL, NULL), fragList);
    return;
}

F_fragList Tr_getResult(void) {
    return fragList;
}
