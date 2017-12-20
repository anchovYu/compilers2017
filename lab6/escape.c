#include <stdio.h>
#include <string.h>
#include "util.h"
#include "symbol.h"
#include "absyn.h"
#include "escape.h"
#include "table.h"
#include "helper.h"
#include "env.h"

static void traverseExp(S_table env, int depth, A_exp e);
static void traverseDec(S_table env, int depth, A_dec d);
static void traverseVar(S_table env, int depth, A_var v);

void Esc_findEscape(A_exp exp) {
	S_table env = S_empty();
    traverseExp(env, 0, exp);
}


static void traverseExp(S_table env, int depth, A_exp e) {
    switch(e->kind) {
        case A_seqExp: {
            A_expList seq = get_seqexp_seq(e);
            while (seq && seq->head) {
                traverseExp(env, depth, seq->head);
                seq = seq->tail;
            }
            return;
        }
        case A_varExp: {
            traverseVar(env, depth, e->u.var);
            return;
        }
        case A_nilExp:    case A_intExp:
        case A_stringExp: case A_breakExp: {
            return;
        }
        case A_callExp: {
            A_expList args = get_callexp_args(e);
            while (args && args->head) {
                traverseExp(env, depth, args->head);
                args = args->tail;
            }
            return;
        }
        case A_opExp: {
            A_exp left = get_opexp_left(e);
            A_exp right = get_opexp_right(e);
            traverseExp(env, depth, left);
            traverseExp(env, depth, right);
            return;
        }
        case A_recordExp: {
            A_efieldList fields = get_recordexp_fields(e);
            while (fields && fields->head) {
                traverseExp(env, depth, fields->head->exp);
                fields = fields->tail;
            }
            return;
        }
        case A_arrayExp: {
            A_exp size = get_arrayexp_size(e);
            A_exp init = get_arrayexp_init(e);
            traverseExp(env, depth, size);
            traverseExp(env, depth, init);
            return;
        }
        case A_assignExp: {
            A_var var = get_assexp_var(e);
            A_exp exp = get_assexp_exp(e);
            traverseVar(env, depth, var);
            traverseExp(env, depth, exp);
            return;
        }
        case A_ifExp: {
            A_exp test = get_ifexp_test(e);
            A_exp then = get_ifexp_then(e);
            A_exp elsee = get_ifexp_else(e);
            traverseExp(env, depth, test);
            traverseExp(env, depth, then);
            if (elsee) traverseExp(env, depth, elsee);
            return;
        }
        case A_whileExp: {
            A_exp test = get_whileexp_test(e);
            A_exp body = get_whileexp_body(e);
            traverseExp(env, depth, test);
            traverseExp(env, depth, body);
            return;
        }
        case A_forExp: {
            S_symbol var = get_forexp_var(e);
            A_exp lo = get_forexp_lo(e);
            A_exp hi = get_forexp_hi(e);
            A_exp body = get_forexp_body(e);

            traverseExp(env, depth, lo);
            traverseExp(env, depth, hi);
            S_beginScope(env);
                e->u.forr.escape = FALSE;
                S_enter(env, var, E_Escapeentry(depth, &(e->u.forr.escape)));
                traverseExp(env, depth, body);
            S_endScope(env);

            return;
        }
        case A_letExp: {
            A_decList decList = get_letexp_decs(e);
            A_exp body = get_letexp_body(e);

            S_beginScope(env);
                while (decList && decList->head) {
                    traverseDec(env, depth, decList->head);
                    decList = decList->tail;
                }
                traverseExp(env, depth, body);
            S_endScope(env);
            return;
        }
        assert(0); /* cannot reach here */
    }
}

static void traverseDec(S_table env, int depth, A_dec d) {
    switch (d->kind) {
        case A_varDec: {
            S_symbol var = get_vardec_var(d);
            A_exp init = get_vardec_init(d);
            traverseExp(env, depth, init);
            d->u.var.escape = FALSE;
            S_enter(env, var, E_Escapeentry(depth, &(d->u.var.escape)));
            return;
        }
        case A_typeDec: {
            return;
        }
        case A_functionDec: {
            A_fundecList fundecList = get_funcdec_list(d);
            while (fundecList && fundecList->head) {
                A_fundec fundec = fundecList->head;
                S_beginScope(env);
                    A_fieldList l = fundec->params;
                    while (l && l->head) {
                        l->head->escape = FALSE;
                        S_enter(env, l->head->name, E_Escapeentry(depth+1, &(l->head->escape)));
                        l = l->tail;
                    }
                    traverseExp(env, depth+1, fundec->body);
                S_endScope(env);

                fundecList = fundecList->tail;
            }
            return;
        }
    }
    assert(0); /* cannot reach here */

}

static void traverseVar(S_table env, int depth, A_var v) {
    switch(v->kind) {
        case A_simpleVar: {
            S_symbol simple = get_simplevar_sym(v);
            E_escapeentry escapeentry = (E_escapeentry)S_look(env, simple);
            if (depth > escapeentry->depth)
                *(escapeentry->escape) = TRUE;
            return;
        }
        case A_fieldVar: {
            A_var var = get_fieldvar_var(v);
            traverseVar(env, depth, var);
            return;
        }
        case A_subscriptVar: {
            A_var var = get_subvar_var(v);
            A_exp indexExp = get_subvar_exp(v);
            traverseVar(env, depth, var);
            traverseExp(env, depth, indexExp);
            return;
        }
    }
    assert(0); /* cannot reach here */
}
