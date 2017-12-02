#include <stdio.h>
#include <stdlib.h>
#include "util.h"
#include "symbol.h"
#include "absyn.h"
#include "temp.h"
#include "errormsg.h"
#include "tree.h"
#include "assem.h"
#include "frame.h"
#include "codegen.h"
#include "table.h"

static AS_instrList iList = NULL, last = NULL;
static void emit(AS_instr inst) {
    if (last != NULL)
        last = last->tail = AS_InstrList(inst, NULL);
    else
        last = iList = AS_InstrList(inst, NULL);
}

static Temp_tempList L(Temp_temp temp, Temp_tempList l) {
    return Temp_TempList(temp, l);
}
static Temp_tempList reverseTempList(Temp_tempList l);
static void pushCallerSaves();
static void popCallerSaves();

static void munchStm(T_stm s);
static Temp_temp munchExp(T_exp e);
static Temp_tempList munchArgs(int i, T_expList args);


static Temp_tempList reverseTempList(Temp_tempList l) {
    Temp_tempList res, restmp;
    Temp_tempList tmp = l;
    while (tmp && tmp->head) {
        restmp = Temp_TempList(tmp->head, NULL);
        restmp = restmp->tail;
        tmp = tmp->tail;
    }
    return res;
}
static void pushCallerSaves() {
    Temp_tempList callersaves = F_CallerSaves();
    while (callersaves && callersaves->head) {
        emit(AS_Oper("push `s0", L(F_SP(), NULL), L(callersaves->head, NULL), NULL));
        callersaves = callersaves->tail;
    }
}
static void popCallerSaves() {
    Temp_tempList callersaves = F_CallerSaves();
    Temp_tempList reverseList = reverseTempList(callersaves);
    while (reverseList && reverseList->head) {
        emit(AS_Oper("pop `s0", L(F_SP(), NULL), L(reverseList->head, NULL), NULL));
        reverseList = reverseList->tail;
    }
}

static void munchStm(T_stm s) {
    char* inst1 = checked_malloc(80);
    switch (s->kind) {
        case T_SEQ: {
            munchStm(s->u.SEQ.left);
            munchStm(s->u.SEQ.right);
            return;
        }
        case T_LABEL: {
            sprintf(inst1, "%s:\n", Temp_labelstring(s->u.LABEL));
            emit(AS_Label(inst1, s->u.LABEL));
            return;
        }
        case T_JUMP: {
            // T_Jump(T_Name(a), Temp_LabelList(a, NULL)
            // TODO: jmp *e
            sprintf(inst1, "jmp `j0\n");
            emit(AS_Oper(inst1, NULL, NULL, AS_Targets(s->u.JUMP.jumps)));
            return;
        }
        case T_CJUMP: {
            Temp_temp r = Temp_newtemp();
            Temp_temp left = munchExp(s->u.CJUMP.left);
            Temp_temp right = munchExp(s->u.CJUMP.right);

            emit(AS_Oper("cmpl `s1, `s0\n", NULL, L(left, L(right, NULL)), NULL));

            switch (s->u.CJUMP.op) {
                case T_eq: {
                    emit(AS_Oper("je `j0\n", NULL, NULL, AS_Targets(Temp_LabelList(s->u.CJUMP.true, NULL))));
                    break;
                }
                case T_ne: {
                    emit(AS_Oper("jne `j0\n", NULL, NULL, AS_Targets(Temp_LabelList(s->u.CJUMP.true, NULL))));
                    break;
                }
                case T_lt: {
                    emit(AS_Oper("jl `j0\n", NULL, NULL, AS_Targets(Temp_LabelList(s->u.CJUMP.true, NULL))));
                    break;
                }
                case T_le: {
                    emit(AS_Oper("jle `j0\n", NULL, NULL, AS_Targets(Temp_LabelList(s->u.CJUMP.true, NULL))));
                    break;
                }
                case T_gt: {
                    emit(AS_Oper("jg `j0\n", NULL, NULL, AS_Targets(Temp_LabelList(s->u.CJUMP.true, NULL))));
                    break;
                }
                case T_ge: {
                    emit(AS_Oper("jge `j0\n", NULL, NULL, AS_Targets(Temp_LabelList(s->u.CJUMP.true, NULL))));
                    break;
                }
                case T_ult: {
                    emit(AS_Oper("jb `j0\n", NULL, NULL, AS_Targets(Temp_LabelList(s->u.CJUMP.true, NULL))));
                    break;
                }
                case T_ule: {
                    emit(AS_Oper("jbe `j0\n", NULL, NULL, AS_Targets(Temp_LabelList(s->u.CJUMP.true, NULL))));
                    break;
                }
                case T_ugt: {
                    emit(AS_Oper("ja `j0\n", NULL, NULL, AS_Targets(Temp_LabelList(s->u.CJUMP.true, NULL))));
                    break;
                }
                case T_uge: {
                    emit(AS_Oper("jae `j0\n", NULL, NULL, AS_Targets(Temp_LabelList(s->u.CJUMP.true, NULL))));
                    break;
                }
                default: assert(0); /* cannot reach here */
            }
            emit(AS_Oper("jmp `j0\n", NULL, NULL, AS_Targets(Temp_LabelList(s->u.CJUMP.false, NULL))));
            return;
        }
        case T_MOVE: {
            // for ebp + 8 or esp - 4, I design special pattern for mem(binop())
            /* mov(dst, scr):
             * mov(mem(binop(+, e, const)), e)
             * mov(mem(binop(-, e, const)), e)
             * mov(e, mem(binop(+, e, const)))
             * mov(e, mem(binop(-, e, const)))
             * mov(mem(e), e)
             * mov(e, mem(e))
             * mov(e, const)
             * mov(e, e)    */
            if (s->u.MOVE.dst->kind == T_MEM &&
                s->u.MOVE.dst->u.MEM->kind == T_BINOP &&
                s->u.MOVE.dst->u.MEM->u.BINOP.right->kind == T_CONST &&
                s->u.MOVE.dst->u.MEM->u.BINOP.op == T_plus) {
                // mov(mem(binop(+, e, const)), e)
                Temp_temp src = munchExp(s->u.MOVE.src);
                Temp_temp dst = munchExp(s->u.MOVE.dst->u.MEM->u.BINOP.left);
                sprintf(inst1, "movl `s0, $%d(`d0)", s->u.MOVE.dst->u.MEM->u.BINOP.right->u.CONST);
                emit(AS_Oper(inst1, L(dst, NULL), L(src, NULL), NULL));
            } else if (s->u.MOVE.dst->kind == T_MEM &&
                s->u.MOVE.dst->u.MEM->kind == T_BINOP &&
                s->u.MOVE.dst->u.MEM->u.BINOP.right->kind == T_CONST &&
                s->u.MOVE.dst->u.MEM->u.BINOP.op == T_minus) {
                // mov(mem(binop(-, e, const)), e)
                Temp_temp src = munchExp(s->u.MOVE.src);
                Temp_temp dst = munchExp(s->u.MOVE.dst->u.MEM->u.BINOP.left);
                sprintf(inst1, "movl `s0, $-%d(`d0)", s->u.MOVE.dst->u.MEM->u.BINOP.right->u.CONST);
                emit(AS_Oper(inst1, L(dst, NULL), L(src, NULL), NULL));                return;
            } else if (s->u.MOVE.src->kind == T_MEM &&
                s->u.MOVE.src->u.MEM->kind == T_BINOP &&
                s->u.MOVE.src->u.MEM->u.BINOP.right->kind == T_CONST &&
                s->u.MOVE.src->u.MEM->u.BINOP.op == T_plus) {
                // mov(e, mem(binop(+, e, const)))
                Temp_temp src = munchExp(s->u.MOVE.src->u.MEM->u.BINOP.left);
                Temp_temp dst = munchExp(s->u.MOVE.dst);
                sprintf(inst1, "movl $%d(`s0), `d0", s->u.MOVE.src->u.MEM->u.BINOP.right->u.CONST);
                emit(AS_Oper(inst1, L(dst, NULL), L(src, NULL), NULL));
            } else if (s->u.MOVE.src->kind == T_MEM &&
                s->u.MOVE.src->u.MEM->kind == T_BINOP &&
                s->u.MOVE.src->u.MEM->u.BINOP.right->kind == T_CONST &&
                s->u.MOVE.src->u.MEM->u.BINOP.op == T_minus) {
                // mov(e, mem(binop(-, e, const)))
                Temp_temp src = munchExp(s->u.MOVE.src->u.MEM->u.BINOP.left);
                Temp_temp dst = munchExp(s->u.MOVE.dst);
                sprintf(inst1, "movl $-%d(`s0), `d0", s->u.MOVE.src->u.MEM->u.BINOP.right->u.CONST);
                emit(AS_Oper(inst1, L(dst, NULL), L(src, NULL), NULL));
            } else if (s->u.MOVE.dst->kind == T_MEM) {
                // mov(mem(e), e)
                Temp_temp src = munchExp(s->u.MOVE.src);
                Temp_temp dst = munchExp(s->u.MOVE.dst->u.MEM);
                sprintf(inst1, "movl `s0, (`d0)");
                emit(AS_Oper(inst1, L(dst, NULL), L(src, NULL), NULL));
            } else if (s->u.MOVE.src->kind == T_MEM) {
                // mov(e, mem(e))
                Temp_temp src = munchExp(s->u.MOVE.src->u.MEM);
                Temp_temp dst = munchExp(s->u.MOVE.dst);
                sprintf(inst1, "movl (`s0), `d0");
                emit(AS_Oper(inst1, L(dst, NULL), L(src, NULL), NULL));
            } else if (s->u.MOVE.src->kind == T_CONST) {
                // mov(e, const)
                Temp_temp dst = munchExp(s->u.MOVE.dst);
                sprintf(inst1, "movl $%d, `d0", s->u.MOVE.src->u.CONST);
                emit(AS_Oper(inst1, L(dst, NULL), NULL, NULL));
            } else {
                // mov(e, e)
                Temp_temp src = munchExp(s->u.MOVE.src);
                Temp_temp dst = munchExp(s->u.MOVE.dst);
                sprintf(inst1, "movl `s0, `d0");
                emit(AS_Move(inst1, L(dst, NULL), L(src, NULL)));
            }
            return;
        }
        case T_EXP: {
            munchExp(s->u.EXP);
            return;
        }
        default:
            assert(0); /* cannot reach here */
    }
}

static Temp_temp munchExp(T_exp e) {
    char* inst1 = checked_malloc(80);
    char* inst2 = checked_malloc(80);
    switch (e->kind) {
        case T_BINOP: {
            if (e->u.BINOP.op == T_plus && e->u.BINOP.left->kind == T_CONST) {
                // binop(+, const, e)
                Temp_temp r = Temp_newtemp();
                sprintf(inst1, "movl `s0, `d0\n");
                sprintf(inst2, "addl $%d, `d0\n", e->u.BINOP.left->u.CONST);
                emit(AS_Move(inst1, L(r, NULL), L(munchExp(e->u.BINOP.right), NULL)));
                emit(AS_Oper(inst2, L(r, NULL), NULL, NULL));
                return r;
            } else if (e->u.BINOP.op == T_plus && e->u.BINOP.right->kind == T_CONST) {
                // binop(+, e, const)
                Temp_temp r = Temp_newtemp();
                sprintf(inst1, "movl `s0, `d0\n");
                sprintf(inst2, "addl $%d, `d0\n", e->u.BINOP.right->u.CONST);
                emit(AS_Move(inst1, L(r, NULL), L(munchExp(e->u.BINOP.left), NULL)));
                emit(AS_Oper(inst2, L(r, NULL), NULL, NULL));
                return r;
            } else if (e->u.BINOP.op == T_minus && e->u.BINOP.right->kind == T_CONST) {
                // binop(-, e, const)
                Temp_temp r = Temp_newtemp();
                sprintf(inst1, "movl `s0, `d0\n");
                sprintf(inst2, "subl $%d, `d0\n", e->u.BINOP.right->u.CONST);
                emit(AS_Move(inst1, L(r, NULL), L(munchExp(e->u.BINOP.left), NULL)));
                emit(AS_Oper(inst2, L(r, NULL), NULL, NULL));
                return r;
            } else if (e->u.BINOP.op == T_mul && e->u.BINOP.left->kind == T_CONST) {
                // binop(*, const, e)
                Temp_temp r = Temp_newtemp();
                sprintf(inst1, "movl `s0, `d0\n");
                sprintf(inst2, "imull $%d, `d0\n", e->u.BINOP.left->u.CONST);
                emit(AS_Move(inst1, L(r, NULL), L(munchExp(e->u.BINOP.right), NULL)));
                emit(AS_Oper(inst2, L(r, NULL), NULL, NULL));
                return r;
            } else if (e->u.BINOP.op == T_mul && e->u.BINOP.right->kind == T_CONST) {
                // binop(*, e, const)
                Temp_temp r = Temp_newtemp();
                sprintf(inst1, "movl `s0, `d0\n");
                sprintf(inst2, "imull $%d, `d0\n", e->u.BINOP.right->u.CONST);
                emit(AS_Move(inst1, L(r, NULL), L(munchExp(e->u.BINOP.left), NULL)));
                emit(AS_Oper(inst2, L(r, NULL), NULL, NULL));
                return r;
            } else if (e->u.BINOP.op == T_plus) {
                // binop(+, e1, e2)
                Temp_temp r = Temp_newtemp();
                sprintf(inst1, "movl `s0, `d0\n");
                sprintf(inst2, "addl `s0, `d0\n");
                emit(AS_Move(inst1, L(r, NULL), L(munchExp(e->u.BINOP.left), NULL)));
                emit(AS_Oper(inst2, L(r, NULL), L(munchExp(e->u.BINOP.right), L(r, NULL)), NULL));
                return r;
            } else if (e->u.BINOP.op == T_minus) {
                // binop(-, e1, e2)
                Temp_temp r = Temp_newtemp();
                sprintf(inst1, "movl `s0, `d0\n");
                sprintf(inst2, "subl `s0, `d0\n");
                emit(AS_Move(inst1, L(r, NULL), L(munchExp(e->u.BINOP.left), NULL)));
                emit(AS_Oper(inst2, L(r, NULL), L(munchExp(e->u.BINOP.right), L(r, NULL)), NULL));
                return r;
            } else if (e->u.BINOP.op == T_mul) {
                // binop(*, e1, e2)
                Temp_temp r = Temp_newtemp();
                sprintf(inst1, "movl `s0, `d0\n");
                sprintf(inst2, "imull `s0, `d0\n");
                emit(AS_Move(inst1, L(r, NULL), L(munchExp(e->u.BINOP.left), NULL)));
                emit(AS_Oper(inst2, L(r, NULL), L(munchExp(e->u.BINOP.right), L(r, NULL)), NULL));
                return r;
            } else if (e->u.BINOP.op == T_div) {
                // binop(/, e1, e2)
                // R[%edx] ← R[%edx]:R[%eax] mod S;
                // R[%eax] ← R[%edx]:R[%eax] ÷ S
                Temp_temp r = Temp_newtemp();
                emit(AS_Move("movl `s0, `d0\n", L(F_EAX(), NULL), L(munchExp(e->u.BINOP.left), NULL)));
                emit(AS_Oper("movl $0, `d0\n", L(F_EDX(), NULL), NULL, NULL));
                emit(AS_Oper("idivl `s0\n", L(F_EAX(), L(F_EDX(), NULL)), L(munchExp(e->u.BINOP.right), NULL), NULL));
                emit(AS_Move("movl `s0, `d0\n", L(r, NULL), L(F_EAX(), NULL)));
                return r;
            } else {
                assert(0); /* will not reach here */
            }
            // else if () {
            //     // binop(&, e1, e2) (dummy)
            // } else if () {
            //     // binop(|, e1, e2) (dummy)
            // } else if () {
            //     // binop(<<, e1, e2) (dummy)
            // } else if () {
            //     // binop(>>, e1, e2) (dummy)
            // } else if () {
            //     // binop(>>(a), e1, e2) (dummy)
            // } else if () {
            //     // binop(^, e1, e2) (dummy)
            // }
        }
        case T_MEM: {
            if (e->u.MEM->kind == T_BINOP &&
                e->u.MEM->u.BINOP.op == T_plus &&
                e->u.MEM->u.BINOP.left->kind == T_CONST) {
                // mem(binop(+, const, e))
                Temp_temp r = Temp_newtemp();
                sprintf(inst1, "movl %d(`s0), `d0\n", e->u.BINOP.left->u.CONST);
                emit(AS_Oper(inst1, L(r, NULL), L(munchExp(e->u.MEM->u.BINOP.right), NULL), NULL));
                return r;
            } else if (e->u.MEM->kind == T_BINOP &&
                       e->u.MEM->u.BINOP.op == T_plus &&
                       e->u.MEM->u.BINOP.right->kind == T_CONST) {
                // mem(binop(+, e, const))
                Temp_temp r = Temp_newtemp();
                sprintf(inst1, "movl %d(`s0), `d0\n", e->u.BINOP.right->u.CONST);
                emit(AS_Oper(inst1, L(r, NULL), L(munchExp(e->u.MEM->u.BINOP.left), NULL), NULL));
                return r;
            } else if (e->u.MEM->kind == T_BINOP &&
                       e->u.MEM->u.BINOP.op == T_minus &&
                       e->u.MEM->u.BINOP.right->kind == T_CONST) {
                // mem(binop(-, e, const))
                Temp_temp r = Temp_newtemp();
                sprintf(inst1, "movl -%d(`s0), `d0\n", e->u.BINOP.right->u.CONST);
                emit(AS_Oper(inst1, L(r, NULL), L(munchExp(e->u.MEM->u.BINOP.left), NULL), NULL));
                return r;
            } else if (e->u.MEM->kind == T_BINOP &&
                       e->u.MEM->u.BINOP.op == T_plus) {
                // mem(binop(+, e, e))
                Temp_temp r = Temp_newtemp();
                sprintf(inst1, "movl (`s0,`s1), `d0\n");
                emit(AS_Oper(inst1, L(r, NULL), L(munchExp(e->u.MEM->u.BINOP.left), L(munchExp(e->u.MEM->u.BINOP.right), NULL)), NULL));
                return r;
            } else if (e->u.MEM->kind == T_CONST) {
                // mem(const)
                Temp_temp r = Temp_newtemp();
                sprintf(inst1, "movl %d, `d0\n", e->u.MEM->u.CONST);
                emit(AS_Oper(inst1, L(r, NULL), NULL, NULL));
                return r;
            } else {
                // mem(e)
                Temp_temp r = Temp_newtemp();
                sprintf(inst1, "movl (`s0), `d0\n");
                emit(AS_Oper(inst1, L(r, NULL), L(munchExp(e->u.MEM), NULL), NULL));
                return r;
            }
        }
        case T_CALL: {
            // call(name, args)
            // should all be direct call
            // TODO: callersave(push, pop)
            // TODO: should I return the rv directly?
            pushCallerSaves();
            // Temp_temp r = Temp_newtemp();
            Temp_tempList l = munchArgs(0, e->u.CALL.args);
            Temp_tempList calldefs = L(F_RV(), F_CallerSaves());
            sprintf(inst1, "call %s\n", Temp_labelstring(e->u.CALL.fun->u.NAME));
            //sprintf(inst2, "movl `s0, `d0");
            emit(AS_Oper(inst1, calldefs, l, NULL));
            popCallerSaves();
            // TODO: what about the args?
            // emit(AS_Oper(inst2, L(r, NULL), L(F_RV(), NULL), NULL));
            return F_RV();
        }
        case T_TEMP: {
            return e->u.TEMP;
        }
        case T_NAME: {
            // TODO: not know what to do?
            // moving the label imm to a reg?
            Temp_temp r = Temp_newtemp();
            sprintf(inst1, "movl $%s, `d0", Temp_labelstring(e->u.NAME));
            emit(AS_Oper(inst1, L(r, NULL), NULL, NULL));
            return r;
        }
        case T_CONST: {
            Temp_temp r = Temp_newtemp();
            sprintf(inst1, "movl $%d, `d0", e->u.CONST);
            emit(AS_Oper(inst1, L(r, NULL), NULL, NULL));
            return r;
        }
    }
    assert(0); /* cannot reach here */
}

static Temp_tempList munchArgs(int i, T_expList args) {
    // Note: the push order!
    if (!args) return NULL;
    T_expList next = args->tail;
    Temp_tempList l = munchArgs(i+1, next);

    Temp_temp r = munchExp(args->head);
    emit(AS_Oper("push `s0", L(F_SP(), NULL), L(r, NULL), NULL));
    return Temp_TempList(r, l);
}


//Lab 6: put your code here
AS_instrList F_codegen(F_frame f, T_stmList stmList) {

}
