#include "prog1.h"
#include <stdio.h>

#define MAX(x,y) (x > y ? x : y)

typedef struct table *Table_;
struct table {string id; int value; Table_ tail;};
Table_ Table(string id, int value, struct table *tail) {
	Table_ t = checked_malloc(sizeof(*t));
	t->id = id;
	t->value = value;
	t->tail = tail;
	return t;
}
int lookup(Table_ t, string key) {
	if (t->id == key) {
		return t->value;
	} else {
		return lookup(t->tail, key);
	}
}

struct IntAndTable_ {int i; Table_ t;};
typedef struct IntAndTable_ IntAndTable;
IntAndTable interpExp(A_exp e, Table_ t);
IntAndTable generateIntAndTable(int i, Table_ t) {
	IntAndTable result;
	result.i = i;
	result.t = t;
	return result;
}


Table_ interpStm(A_stm s, Table_ t) {
	if (s->kind == A_compoundStm) {
		return interpStm(s->u.compound.stm2, interpStm(s->u.compound.stm1, t));
	} else if (s->kind == A_assignStm) {
		IntAndTable result = interpExp(s->u.assign.exp, t);
		return Table(s->u.assign.id, result.i, result.t);
	} else {
		A_expList exps= s->u.print.exps;
		if (exps->kind == A_lastExpList) {
			IntAndTable result = interpExp(exps->u.last, t);
			printf("%d\n", result.i);
            return result.t;
		} else {
			IntAndTable result = interpExp(exps->u.pair.head, t);
			printf("%d ", result.i);
			return interpStm(A_PrintStm(exps->u.pair.tail), result.t);
		}
	}
}

IntAndTable interpExp(A_exp e, Table_ t) {
	if (e->kind == A_idExp) {
		return generateIntAndTable(lookup(t, e->u.id), t);
	} else if (e->kind == A_numExp) {
		return generateIntAndTable(e->u.num, t);
	} else if (e->kind == A_opExp) {
		IntAndTable leftResult = interpExp(e->u.op.left, t);
		IntAndTable rightResult = interpExp(e->u.op.right, leftResult.t);

		switch (e->u.op.oper) {
		case A_plus:
			return generateIntAndTable(leftResult.i + rightResult.i, rightResult.t);
			break;
		case A_minus:
			return generateIntAndTable(leftResult.i - rightResult.i, rightResult.t);
			break;
		case A_times:
			return generateIntAndTable(leftResult.i * rightResult.i, rightResult.t);
			break;
		case A_div:
			return generateIntAndTable(leftResult.i / rightResult.i, rightResult.t);
			break;
		}
	} else {
		return interpExp(e->u.eseq.exp, interpStm(e->u.eseq.stm, t));
	}
}



int maxargs_in_exp(A_exp exp);
int maxargs_in_expList(A_expList exps, int exp_count);
int maxargs(A_stm stm) {
	if (stm->kind == A_compoundStm) {
		return MAX(maxargs(stm->u.compound.stm1), maxargs(stm->u.compound.stm2));
	} else if (stm->kind == A_assignStm) {
		return maxargs_in_exp(stm->u.assign.exp);
	} else {
		return maxargs_in_expList(stm->u.print.exps, 1);
	}
}

int maxargs_in_exp(A_exp exp) {
	if (exp->kind == A_opExp) {
		return MAX(maxargs_in_exp(exp->u.op.left), maxargs_in_exp(exp->u.op.right));
	} else if (exp->kind == A_eseqExp) {
		return MAX(maxargs(exp->u.eseq.stm), maxargs_in_exp(exp->u.eseq.exp));
	}
	return 0;
}

int maxargs_in_expList(A_expList exps, int exp_count) {
	if (exps->kind == A_pairExpList) {
		return MAX(MAX(exp_count + 1, maxargs_in_exp(exps->u.pair.head)),maxargs_in_expList(exps->u.pair.tail, exp_count + 1));
	} else {
		return MAX(exp_count, maxargs_in_exp(exps->u.last));
	}
}

void interp(A_stm stm) {
    interpStm(stm, 0);
}
