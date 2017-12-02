#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "util.h"
#include "errormsg.h"
#include "symbol.h"
#include "absyn.h"
#include "types.h"
#include "env.h"
#include "semant.h"
#include "helper.h"
#include "translate.h"

/*Lab5: Your implementation of lab5.*/

// TODO: break handling.
// TODO: () exp (12,20,43)

/*Lab4: Your implementation of lab4*/

// for debug usage:
// static char str_ty[][12] = {
//    "ty_record", "ty_nil", "ty_int", "ty_string",
//    "ty_array", "ty_name", "ty_void"};
//printf("ty kind: %s\n", str_ty[ty->kind]);


//In Lab4, the first argument exp should always be **NULL**.
//typedef void* Tr_exp;
struct expty {
	Tr_exp exp;
	Ty_ty ty;
};
struct expty expTy(Tr_exp exp, Ty_ty ty) {
	struct expty e;

	e.exp = exp;
	e.ty = ty;

	return e;
}

/* Return the actual type under nameTy,
 * except nameTy(name, recordTy) and nameTy(name, arrayTy). */
Ty_ty actual_ty(Ty_ty ty) {
    Ty_ty t = ty;
    while(get_ty_type(t) == Ty_name) {
        Ty_ty temp = get_namety_ty(t);
        if (!temp || get_ty_type(temp) == Ty_record || get_ty_type(temp) == Ty_array)
            break;
        t = temp;
    }
    return t;
}

/* Compare two types.
 * For int, string types etc., compare their kind.
 * For array and record type (in namety form), directly compare then in pointers. */
bool tyEqual(Ty_ty a, Ty_ty b) {
    Ty_ty tya = actual_ty(a);
    Ty_ty tyb = actual_ty(b);

    switch(tya->kind) {
        case Ty_nil: {
            if (tya->kind == tyb->kind)
                return TRUE;
            // nil, record
            if (tyb->kind == Ty_name &&
                get_namety_ty(tyb) &&
                get_namety_ty(tyb)->kind == Ty_record)
                return TRUE;
            return FALSE;
        }
        case Ty_int:
        case Ty_string:
        case Ty_void: {
            if (tya->kind == tyb->kind)
                return TRUE;
            return FALSE;
        }
        // use pointers to compare Ty_name(array and record).
        case Ty_name: {
            if (tya == tyb)
                return TRUE;

            // (a, NULL) = (c, NULL)
            if (!get_namety_ty(tya))
                return FALSE;

            // record, nil
            if (get_namety_ty(tya)->kind == Ty_record && tyb->kind == Ty_nil)
                return TRUE;

            return FALSE;
        }
        default:
            return FALSE;
    }
}

/* Whether the var is readonly.
 * For assignexp of loop var(forexp) usage. */
bool isROVar(S_table venv, A_var v) {
    if (v->kind == A_simpleVar) {
        S_symbol simple = get_simplevar_sym(v);
        E_enventry enventry = (E_enventry)S_look(venv, simple);
        if (enventry && enventry->kind == E_varEntry && enventry->readonly == 1)
            return TRUE;
    }
    return FALSE;
}

// turn a list of params in func dec to tyList in the params order.
Ty_tyList makeFormalTyList(S_table tenv, A_fieldList params) {
    A_fieldList fieldList = params;
    Ty_tyList tyList = NULL;
    if (fieldList && fieldList->head) {
        Ty_ty ty = transTy(tenv, A_NameTy(fieldList->head->pos, fieldList->head->typ));
        if (!ty) {
            /* error */
            return NULL;
        }
        tyList = Ty_TyList(ty, NULL);
        fieldList = fieldList->tail;
    }
    Ty_tyList temp = tyList;
    while (fieldList && fieldList->head) {
        Ty_ty ty = transTy(tenv, A_NameTy(fieldList->head->pos, fieldList->head->typ));
        if (!ty) {
            /* error */
            return NULL;
        }
        temp->tail = Ty_TyList(ty, NULL);
        temp = temp->tail;
        fieldList = fieldList->tail;
    }
    return tyList;
}

U_boolList makeFormalEscapeList(A_fieldList params) {
    A_fieldList fieldList = params;
    U_boolList boolList = NULL;
    if (fieldList && fieldList->head) {
        boolList = U_BoolList(fieldList->head->escape, NULL);
        fieldList = fieldList->tail;
    }
    U_boolList temp = boolList;
    while (fieldList && fieldList->head) {
        temp->tail = U_BoolList(fieldList->head->escape, NULL);
        temp = temp->tail;
        fieldList = fieldList->tail;
    }
    return boolList;
}

/* Turn a A_ty type into Ty_ty type, peel nameTy. */
Ty_ty transTy(S_table tenv, A_ty a) {
    switch(a->kind) {
        case A_nameTy: {
            /* 1. find this type in tenv
             * 2. return the type (iterate until single namety) */
            //printf("enter transTy-nameTy.\n");
            Ty_ty ty = (Ty_ty)S_look(tenv, get_ty_name(a));
            //if (ty) printf("name ty kind: %s\n", str_ty[ty->kind]);

            if (!ty) {
                EM_error(a->pos, "undefined type %s", S_name(get_ty_name(a)));
                return NULL;
            }
            ty = actual_ty(ty);
            switch (ty->kind) {
                case Ty_nil: {
                    ty = Ty_Nil();
                    break;
                }
                case Ty_int: {
                    ty = Ty_Int();
                    break;
                }
                case Ty_string: {
                    ty = Ty_String();
                    break;
                }
                case Ty_void: {
                    ty = Ty_Void();
                    break;
                }
                default: ;
            }
            /* return format:
             * (newly created)Ty_nil, Ty_int, Ty_string, Ty_void
             * (pointers)Ty_name(name, Ty_record), Ty_name(name, Ty_array)
             */
            return ty;
        }
        case A_recordTy: {
            //printf("enter transTy-recordTy.\n");
            /* Place them in corresponding order. */
            A_fieldList record = get_ty_record(a);
            Ty_fieldList ty_fieldList = NULL;
            if (record && record->head) {
                A_field field = record->head;
                //printf("***%s\n",S_name(field->name));
                Ty_ty fieldTy = transTy(tenv, A_NameTy(field->pos, field->typ));
                if (!fieldTy) {
                    /* error */
                    return Ty_Record(ty_fieldList);
                }
                Ty_field ty_field = Ty_Field(field->name, fieldTy);
                //printf("***ty kind: %s\n", str_ty[ty_field->ty->kind]);
                ty_fieldList = Ty_FieldList(ty_field, NULL);
                record = record->tail;
            }
            Ty_fieldList temp = ty_fieldList;
            while (record && record->head) {
                A_field field = record->head;
                //printf("***%s\n",S_name(field->name));
                Ty_ty fieldTy = transTy(tenv, A_NameTy(field->pos, field->typ));
                if (!fieldTy) {
                    /* error */
                    return Ty_Record(ty_fieldList);
                }
                Ty_field ty_field = Ty_Field(field->name, fieldTy);
                temp->tail = Ty_FieldList(ty_field, NULL);
                temp = temp->tail;
                record = record->tail;
            }
            temp = ty_fieldList;
            while (temp) {
                //printf("record field: %s\n", S_name(temp->head->name));
                temp = temp->tail;
            }
            return Ty_Record(ty_fieldList);
        }
        case A_arrayTy: {
            //printf("enter transTy-arrayTy.\n");
            S_symbol array = get_ty_array(a);
            return Ty_Array(transTy(tenv, A_NameTy(a->pos, array)));
            /* error condition: transTy returns NULL */
        }
        default: ;
    }
}

struct expty transExp(S_table venv, S_table tenv, A_exp a, Tr_level level, Temp_label breakk) {
    switch(a->kind) {
        case A_seqExp: {
            //printf("enter transExp-seqExp.\n");
            A_expList seq = get_seqexp_seq(a);
            if (!seq)
                return expTy(Tr_nop(), Ty_Void());
            while (seq && seq->head && seq->tail) {
                transExp(venv, tenv, seq->head, level, breakk);
                seq = seq->tail;
            }
            // TODO: let in expseq end, the expseq can is zero or more exps.
            // but the (expseq) have two or more exps.
            return transExp(venv, tenv, seq->head, level, breakk);
        }
        case A_varExp: {
            //printf("enter transExp-varExp.\n");
            return transVar(venv, tenv, a->u.var, level, breakk);
        }
        case A_nilExp: {
            //printf("enter transExp-nilExp.\n");
            return expTy(Tr_Nil(), Ty_Nil());
        }
        case A_intExp: {
            //printf("enter transExp-intExp.\n");
            return expTy(Tr_Int(a->u.intt), Ty_Int());
        }
        case A_stringExp: {
            //printf("enter transExp-stringExp.\n");
            return expTy(Tr_String(a->u.stringg), Ty_String());
        }
        case A_callExp: {
            //printf("enter transExp-callExp.\n");
            /* find the label in venv, compare argument types */
            S_symbol func = get_callexp_func(a);
            A_expList args = get_callexp_args(a);
            E_enventry enventry = (E_enventry)S_look(venv, func);
            if (!enventry || enventry->kind != E_funEntry) {
                EM_error(a->pos, "undefined function %s", S_name(func));
                return expTy(NULL, Ty_Int());
            }

            Ty_tyList formals = get_func_tylist(enventry);
            Ty_ty result = get_func_res(enventry);

            A_expList expList = args;
            Ty_tyList tyList = formals;
            Tr_expList tr_expList = NULL;
            while (expList && expList->head) {
                // more params
                if (!(tyList && tyList->head)) {
                    EM_error(expList->head->pos, "too many params in function %s", S_name(func));
                    return expTy(NULL, Ty_Int());
                }
                struct expty paramTy = transExp(venv, tenv, expList->head, level, breakk);
                if (!tyEqual(paramTy.ty, tyList->head)) {
                    EM_error(expList->head->pos, "para type mismatch");
                    return expTy(NULL, Ty_Int());
                }
                // TODO: the order of params?
                tr_expList = Tr_ExpList(paramTy.exp, tr_expList);
                tyList = tyList->tail;
                expList = expList->tail;
            }

            // fewer params
            if (tyList) {
                EM_error(a->pos, "(transExp-callexp)Function params not match(fewer).");
                return expTy(NULL, Ty_Int());
            }
            return expTy(Tr_Call(get_func_label(enventry), tr_expList, level, get_func_level(enventry)),
                         result);
        }
    	case A_opExp: {
            //printf("enter transExp-opExp.\n");
            A_oper oper = get_opexp_oper(a);
            struct expty left = transExp(venv, tenv, get_opexp_left(a), level, breakk);
            struct expty right = transExp(venv, tenv, get_opexp_right(a), level, breakk);
            if (oper == A_plusOp || oper == A_minusOp || oper == A_timesOp || oper == A_divideOp) {
                if (get_expty_kind(left) != Ty_int)
                    EM_error(get_opexp_leftpos(a), "integer required");
                if (get_expty_kind(right) != Ty_int)
                    EM_error(get_opexp_rightpos(a), "integer required");
                return expTy(Tr_Op(oper, left.exp, right.exp), Ty_Int());
            }
            if (oper == A_ltOp || oper == A_leOp || oper == A_gtOp || oper == A_geOp) {
                if (!(get_expty_kind(left) == Ty_int || get_expty_kind(left) == Ty_string)) {
                    EM_error(get_opexp_leftpos(a), "integer or string required");
                }
                if (!(get_expty_kind(right) == Ty_int || get_expty_kind(right) == Ty_string)) {
                    EM_error(get_opexp_rightpos(a), "integer or string required");
                }
                if (!tyEqual(left.ty, right.ty))
                    EM_error(get_opexp_rightpos(a), "same type required");
                return expTy(Tr_OpCom(oper, left.exp, right.exp, FALSE), Ty_Int());
                //TODO: what will happen when compare between two strings?
            }

            if (oper == A_eqOp || oper == A_neqOp) {
                if (!((get_expty_kind(left) == Ty_int ||
                       get_expty_kind(left) == Ty_string ||
                       get_expty_kind(left) == Ty_nil ||
                       get_expty_kind(left) == Ty_name)
                    && tyEqual(left.ty, right.ty))) {
                    EM_error(get_opexp_rightpos(a), "same type required");
                }
                return expTy(Tr_OpCom(oper, left.exp, right.exp, get_expty_kind(left) == Ty_string), Ty_Int());
            }
            EM_error(get_opexp_leftpos(a), "(transExp-opexp)type not match");
            return expTy(NULL, Ty_Int());
        }
        case A_recordExp: {
            //printf("enter transExp-recordExp.\n");
            S_symbol typ = get_recordexp_typ(a);
            A_efieldList fields = get_recordexp_fields(a);

            Ty_ty ty = transTy(tenv, A_NameTy(a->pos, typ));
            if (!ty || ty->kind != Ty_name || ((Ty_ty)get_namety_ty(ty))->kind != Ty_record) {
                //EM_error(a->pos, "(transExp-recordExp)no such record id");
                /* (ins) without error twice. */
                return expTy(NULL, Ty_Nil()); // TODO: what should I return?
            }

            Ty_fieldList record = ((Ty_ty)get_namety_ty(ty))->u.record;
            A_efieldList efieldList = fields;
            Ty_fieldList fieldList = record;
            Tr_expList tr_expList = NULL;
            while (efieldList && efieldList->head) {
                // TODO: Here assume they are in the same order
                if(!(fieldList && fieldList->head)) {
                    EM_error(a->pos, "(transExp-recordExp)too many params");
                    return expTy(NULL, Ty_Nil()); // or nil?? what does nil do?
                }
                //printf("efieldList:%s\n", S_name(efieldList->head->name));
                //printf("fieldList:%s\n", S_name(fieldList->head->name));
                if (strcmp(S_name(efieldList->head->name), S_name(fieldList->head->name))) {
                    EM_error(a->pos, "(transExp-recordExp)names not match");
                    return expTy(NULL, Ty_Nil()); // TODO: what should I return?
                }
                struct expty fieldExpTy = transExp(venv, tenv, efieldList->head->exp, level, breakk);
                if (!tyEqual(fieldExpTy.ty, fieldList->head->ty)) {
                    EM_error(a->pos, "(transExp-recordExp)types not match");
                    return expTy(NULL, Ty_Nil()); // TODO: what should I return?
                }
                // TODO: the order of fields?
                tr_expList = Tr_ExpList(fieldExpTy.exp, tr_expList);
                efieldList = efieldList->tail;
                fieldList = fieldList->tail;
            }
            if (fieldList) {
                EM_error(a->pos, "(transExp-recordExp)fewer params");
                return expTy(NULL, Ty_Nil()); // TODO: what should I return?
            }
            //printf("finish recordExp\n");
            return expTy(Tr_record(tr_expList), ty); // return nameTy
        }
        case A_arrayExp: {
            //printf("enter transExp-arrayExp.\n");
            S_symbol typ = get_arrayexp_typ(a);
            A_exp size = get_arrayexp_size(a);
            A_exp init = get_arrayexp_init(a);

            Ty_ty ty = transTy(tenv, A_NameTy(a->pos, typ));
            if (!ty || ty->kind != Ty_name || ((Ty_ty)get_namety_ty(ty))->kind != Ty_array) {
                EM_error(a->pos, "(transExp-arrayexp)no such array id");
                return expTy(NULL, Ty_Nil());// TODO
            }

            struct expty sizety = transExp(venv, tenv, size, level, breakk);
            struct expty initty = transExp(venv, tenv, init, level, breakk);

            if (sizety.ty->kind != Ty_int) {
                EM_error(size->pos, "(transExp-arrayexp)size type wrong");
                return expTy(NULL, Ty_Nil());// TODO
            }
            if (!tyEqual(initty.ty, ((Ty_ty)get_namety_ty(ty))->u.array)) {
                EM_error(init->pos, "type mismatch");
                return expTy(NULL, Ty_Nil());// TODO
            }
            return expTy(Tr_array(sizety.exp, initty.exp), ty);
        }
        case A_assignExp: {
            //printf("enter transExp-assignExp.\n");
            A_var var = get_assexp_var(a);
            A_exp exp = get_assexp_exp(a);
            struct expty lhsTy = transVar(venv, tenv, var, level, breakk);
            if (isROVar(venv, var)) {
                /* hacking. In loop body, the var cannot be assigned. */
                EM_error(a->pos, "loop variable can't be assigned");
                return expTy(NULL, Ty_Void());
            }
            struct expty rhsTy = transExp(venv, tenv, exp, level, breakk);
            if (!tyEqual(lhsTy.ty, rhsTy.ty))
                EM_error(a->pos, "unmatched assign exp");
            return expTy(Tr_assign(lhsTy.exp, rhsTy.exp), Ty_Void());
        }
        case A_ifExp: {
            //printf("enter transExp-ifExp.\n");
            A_exp test = get_ifexp_test(a);
            A_exp then = get_ifexp_then(a);
            A_exp elsee = get_ifexp_else(a);
            struct expty testTy = transExp(venv, tenv, test, level, breakk);
            if (testTy.ty->kind != Ty_int) {
                EM_error(test->pos, "(transExp-ifexp)test condition return wrong type.");
                return expTy(NULL, Ty_Void());
            }

            struct expty thenTy = transExp(venv, tenv, then, level, breakk);
            if (!elsee) {
                if (thenTy.ty->kind != Ty_void)
                    EM_error(then->pos, "if-then exp's body must produce no value");
                return expTy(Tr_if(testTy.exp, thenTy.exp, NULL),
                             Ty_Void());
            }
            struct expty elseTy = transExp(venv, tenv, elsee, level, breakk);
            //printf("elseTy: %s\n", str_ty[elseTy.ty->kind]);

            if (!tyEqual(thenTy.ty, elseTy.ty)) {
                EM_error(elsee->pos, "then exp and else exp type mismatch");
                return expTy(NULL, Ty_Void());
            }
            return expTy(Tr_if(testTy.exp, thenTy.exp, elseTy.exp),
                         thenTy.ty);

        }
        case A_whileExp: {
            //printf("enter transExp-whileExp.\n");

            A_exp test = get_whileexp_test(a);
            A_exp body = get_whileexp_body(a);
            struct expty testTy = transExp(venv, tenv, test, level, breakk);
            struct expty bodyTy = transExp(venv, tenv, body, level, breakk);
            if (testTy.ty->kind != Ty_int)
                EM_error(test->pos, "(transExp-whileexp)test condition return wrong type.");
            if (bodyTy.ty->kind != Ty_void)
                EM_error(body->pos, "while body must produce no value");
            return expTy(Tr_while(testTy.exp, bodyTy.exp, breakk), Ty_Void());
        }
        case A_forExp: {
            //printf("enter transExp-forExp.\n");

            S_symbol var = get_forexp_var(a);
            A_exp lo = get_forexp_lo(a);
            A_exp hi = get_forexp_hi(a);
            A_exp body = get_forexp_body(a);
            struct expty loTy = transExp(venv, tenv, lo, level, breakk);
            struct expty hiTy = transExp(venv, tenv, hi, level, breakk);
            if (loTy.ty->kind != Ty_int)
                EM_error(lo->pos, "for exp's range type is not integer");
                //return expTy(NULL, Ty_Void()); continue running !
            if (hiTy.ty->kind != Ty_int)
                EM_error(hi->pos, "for exp's range type is not integer");
                //return expTy(NULL, Ty_Void()); continue running !

            S_beginScope(venv);
                Tr_access loopVarAccess = Tr_allocLocal(level, TRUE);
                S_enter(venv, var, E_ROVarEntry(loopVarAccess, loTy.ty));
                struct expty bodyTy = transExp(venv, tenv, body, level, breakk);
                if (bodyTy.ty->kind != Ty_void)
                    EM_error(body->pos, "(transExp-forExp)body must have no return values.");
            S_endScope(venv);
            return expTy(Tr_for(loTy.exp, hiTy.exp, bodyTy.exp, Tr_simpleVar(loopVarAccess, level), breakk),
                         Ty_Void());
        }
        case A_breakExp: {
            //printf("enter transExp-breakExp.\n");

            // TODO: handle break
            return expTy(Tr_break(breakk), Ty_Void());
        }
        case A_letExp: {
            //printf("enter transExp-letExp.\n");
            A_decList decList = get_letexp_decs(a);
            A_exp body = get_letexp_body(a);

            S_beginScope(venv);
            S_beginScope(tenv);
            while (decList && decList -> head) {
                transDec(venv, tenv, decList->head, level, breakk);
                decList = decList->tail;
            }
            struct expty bodyTy = transExp(venv, tenv, body, level, breakk);
            S_endScope(tenv);
            S_endScope(venv);
            return bodyTy;
        }
        default: ;//printf("no match kind\n");//return expTy(NULL, Ty_Void());
    }
}

Tr_exp transDec(S_table venv, S_table tenv, A_dec d, Tr_level level, Temp_label breakk) {
    switch (d->kind) {
        case A_varDec: {
            //printf("enter transDec-varDec.\n");

            A_exp init = get_vardec_init(d);
            S_symbol var = get_vardec_var(d);
            S_symbol typ = get_vardec_typ(d);
            struct expty initTy = transExp(venv, tenv, init, level, breakk);
            //printf("%s\n", str_ty[initTy.ty->kind]);

            if (strcmp(S_name(typ), "")) {
                Ty_ty ty = transTy(tenv, A_NameTy(d->pos, typ));
                if (!ty) {
                    return NULL;    //TODO. Actually, I return all NULL in error cases.
                }
                if (!tyEqual(initTy.ty, ty)) {
                    EM_error(init->pos, "type mismatch");
                    /* do not return directly */
                }
            } else {
                if (initTy.ty->kind == Ty_nil)
                    EM_error(init->pos, "init should not be nil without type specified");
            }

            Tr_access acc = Tr_allocLocal(level, get_vardec_escape(d));
            S_enter(venv, var, E_VarEntry(acc, initTy.ty));
            return Tr_assign(Tr_simpleVar(acc, level), initTy.exp);
        }
        case A_typeDec: {
            //printf("enter transDec-typeDec.\n");

            /* Deduplicate and enter all declarations of nameTy */
            char* decHistory[32];   // TODO: not big enough though
            int cnt = 0;
            A_nametyList nametyList = get_typedec_list(d);
            while (nametyList && nametyList->head) {
                int i;
                for (i = 0; i < cnt; i++) {
                    // equivalent to history entry
                    if (!strcmp(S_name(nametyList->head->name), decHistory[i])) {
                        EM_error(d->pos, "two types have the same name");
                        return NULL; //TODO
                    }
                }
                S_enter(tenv, nametyList->head->name, Ty_Name(nametyList->head->name, NULL));
                decHistory[cnt] = S_name(nametyList->head->name);
                cnt ++;
                //printf("nametyList->head->name: %s\n", S_name(nametyList->head->name));
                nametyList = nametyList->tail;
            }

            nametyList = get_typedec_list(d);
            while (nametyList && nametyList->head) {
                A_namety namety = nametyList->head;
                Ty_ty ty = (Ty_ty)S_look(tenv, namety->name);
                //printf("lhsTy: %s\n", str_ty[ty->kind]);
                if (get_ty_type(ty) != Ty_name)
                    EM_error(namety->ty->pos, "(transDec-typeDec)error.");

                Ty_ty typeTy = transTy(tenv, namety->ty);
                // printf("rhsTy: %s\n", str_ty[typeTy->kind]);
                if (!typeTy) {
                    /* error */
                    return NULL; //TODO
                }
                /* cycle detection */
                if (tyEqual(typeTy, ty)) {
                    EM_error(namety->ty->pos, "illegal type cycle");
                    return NULL; //TODO
                }
                ty->u.name.ty = typeTy;
                nametyList = nametyList->tail;
            }
            return Tr_nop();
            break;
        }
        case A_functionDec: {
            /* 1. enter all function in venv with params and return type (if have)
             * 2. for each func, begin scope
             * 3. enter params into venv
             * 4. transExp, check whether return type is equivalent
             * 5. endscope */
            //printf("enter transDec-functionDec.\n");

            /* Deduplicate and enter all declarations of nameTy */
            char* decHistory[32];   // TODO: not big enough though
            int cnt = 0;
            A_fundecList fundecList = get_funcdec_list(d);
            while (fundecList && fundecList->head) {
                A_fundec fundec = fundecList->head;
                int i;
                for (i = 0; i < cnt; i++) {
                    // equivalent to history entry
                    if (!strcmp(S_name(fundec->name), decHistory[i])) {
                        EM_error(d->pos, "two functions have the same name");
                        return NULL; //TODO
                    }
                }
                Ty_ty resultTy = Ty_Void();
                if (strcmp(S_name(fundec->result), "")) {
                    resultTy = transTy(tenv, A_NameTy(fundec->pos, fundec->result));
                    if (!resultTy)
                        EM_error(fundec->pos, "(transDec-func)result type not found.");
                }
                Ty_tyList formalTys = makeFormalTyList(tenv, fundec->params);
                //printf("enter venv of fundec: %s\n", S_name(fundec->name));

                //TODO label?
                Temp_label label = Temp_newlabel();
                U_boolList l =  makeFormalEscapeList(fundec->params);
                Tr_level cur = Tr_newLevel(level, label, U_BoolList(TRUE ,l));  // add static link in dec
                // while (l && l->head) {
                //     printf("boollist:%d\n", l->head);
                //     l = l->tail;
                // }
                S_enter(venv, fundec->name, E_FunEntry(cur, label, formalTys, resultTy));

                decHistory[cnt] = S_name(fundec->name);
                cnt ++;
                fundecList = fundecList->tail;
            }


            fundecList = get_funcdec_list(d);
            while (fundecList && fundecList->head) {
                A_fundec fundec = fundecList->head;

                E_enventry enventry = (E_enventry)S_look(venv, fundec->name);
                if (!enventry || enventry->kind != E_funEntry)
                    {/* error message */printf("I can't find the func in the dec process!\n");}
                Ty_tyList formalTys = get_func_tylist(enventry);
                Ty_ty resultTy = get_func_res(enventry);
                Tr_level curLevel = get_func_level(enventry);
                Tr_accessList paramsAcc = Tr_formals(curLevel);

                S_beginScope(venv);
                {
                    // printf("wha\n");

                    A_fieldList l;
                    Ty_tyList t;
                    Tr_accessList a;
                    for (l = fundec->params, t = formalTys, a = paramsAcc; l; l = l->tail, t = t->tail, a = a->tail) {
                        S_enter(venv, l->head->name, E_VarEntry(a->head, t->head));
                    }

                }
                struct expty bodyTy = transExp(venv, tenv, fundec->body, curLevel, NULL); // TODO
                if (!tyEqual(bodyTy.ty, resultTy)) {
                    //printf("bodyTy: %s, resultTy: %s\n", str_ty[bodyTy.ty->kind], str_ty[resultTy->kind]);
                    if (resultTy->kind == Ty_void)
                        EM_error(fundec->pos, "procedure returns value");
                    else
                        EM_error(fundec->pos, "(transDec-func)result type is incorrect.");
                }
                S_endScope(venv);
                Tr_procEntryExit(NULL, NULL, NULL);
                fundecList = fundecList->tail;
            }
            return Tr_nop();
        }
        default: ;
    }
}

/* Translate the var, peel off until nameTy. */
struct expty transVar(S_table venv, S_table tenv, A_var v, Tr_level level, Temp_label breakk) {
    switch(v->kind) {
        case A_simpleVar: {
            //printf("enter transVar-simpleVar.\n");

            S_symbol simple = get_simplevar_sym(v);
            E_enventry enventry = (E_enventry)S_look(venv, simple);
            if (enventry && enventry->kind == E_varEntry) {
                Ty_ty temp = get_varentry_type(enventry);
                Tr_exp tr_simpleVar = Tr_simpleVar(get_varentry_access(enventry), level);
                return expTy(tr_simpleVar, actual_ty(get_varentry_type(enventry)));
            } else {
                EM_error(v->pos, "undefined variable %s", S_name(get_simplevar_sym(v)));
                return expTy(NULL, Ty_Int());
            }
        }
        case A_fieldVar: {
            //printf("enter transVar-fieldVar.\n");
            A_var var = get_fieldvar_var(v);
            struct expty fieldVarTy = transVar(venv, tenv, var, level, breakk);

            if (fieldVarTy.ty->kind != Ty_name){
                /* error */;
                EM_error(v->pos, "not a record type");
                return expTy(NULL, Ty_Nil());
            }

            Ty_ty ty = get_namety_ty(fieldVarTy.ty);
            if (ty->kind != Ty_record) {
                /* error */;
                EM_error(v->pos, "not a record type");
                return expTy(NULL, Ty_Nil());
            }

            S_symbol sym = get_fieldvar_sym(v);
            Ty_fieldList fieldList = ty->u.record;
            int index = 0;
            while (fieldList && fieldList->head) {
                Ty_field field = fieldList->head;
                if (field->name == sym) {// TODO: is this right?
                    Tr_exp tr_fieldVar = Tr_indexVar(fieldVarTy.exp, Tr_Int(index));
                    return expTy(tr_fieldVar, actual_ty(field->ty));
                }
                fieldList = fieldList->tail;
                index ++;
            }
            EM_error(v->pos, "field %s doesn't exist", S_name(sym));
            return expTy(NULL, Ty_Int());
            // TODO: what should I return for rec1.nam := "asd"?
        }
        case A_subscriptVar: {
            //printf("enter transVar-subscriptVar.\n");

            //TODO: shall we detect overflow exception?
            A_var var = get_subvar_var(v);
            struct expty subscriptVarTy = transVar(venv, tenv, var, level, breakk);

            if (subscriptVarTy.ty->kind != Ty_name) {
                /* error */;
                EM_error(v->pos, "array type required");
                return expTy(NULL, Ty_Nil());
            }
            Ty_ty ty = get_namety_ty(subscriptVarTy.ty);
            if (ty->kind != Ty_array) {
                /* error */;
                EM_error(v->pos, "array type required");
                return expTy(NULL, Ty_Nil());
            }
            A_exp indexExp = get_subvar_exp(v);
            struct expty indexTy = transExp(venv, tenv, indexExp, level, breakk);
            // TODO check whether it is int

            Tr_exp tr_subscriptVar = Tr_indexVar(subscriptVarTy.exp, indexTy.exp);
            return expTy(tr_subscriptVar, actual_ty(get_ty_array(ty)));
        }
        default: ;
    }
}


F_fragList SEM_transProg(A_exp exp){
	//TODO LAB5: do not forget to add the main frame
    S_table tenv = E_base_tenv();
    S_table venv = E_base_venv();
    transExp(venv, tenv, exp, Tr_outermost(), Temp_newlabel());
        // TODO: not knowing much about breakk
    Tr_procEntryExit(NULL, NULL, NULL); // call the main func
    return Tr_getResult();
}
