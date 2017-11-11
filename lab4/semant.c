#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "util.h"
#include "errormsg.h"
#include "symbol.h"
#include "absyn.h"
#include "types.h"
#include "helper.h"
#include "env.h"
#include "semant.h"

// TODO: nil handling.
// TODO: break handling.
// TODO: cycle detection.

/*Lab4: Your implementation of lab4*/

//In Lab4, the first argument exp should always be **NULL**.
typedef void* Tr_exp;
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

// Return the actual type under nameTy,
// except nameTy(name, recordTy) and nameTy(name, arrayTy).
Ty_ty actual_ty(Ty_ty ty) {
    Ty_ty t = ty;
    while(get_ty_type(t) == Ty_name) {
        //Ty_ty temp = t->u.name.ty;
        Ty_ty temp = get_namety_ty(t);
        if (get_ty_type(temp) == Ty_record || get_ty_type(temp) == Ty_array)
            break;
        t = temp;
    }
    return t;
}

// Compare two types.
// For int, string types etc., compare their kind.
// For array and record type (in namety form), directly compare then in pointers.
bool tyEqual(Ty_ty a, Ty_ty b) {
    Ty_ty tya = actual_ty(a);
    Ty_ty tyb = actual_ty(b);
    switch(tya->kind) {
        case Ty_nil:
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
            return FALSE;
        }
        default:
            return FALSE;
    }
}

// turn a list of params in func dec to tyList
Ty_tyList makeFormalTyList(S_table tenv, A_fieldList params) {
    A_fieldList fieldList = params;
    Ty_tyList tyList = NULL;
    while (fieldList && fieldList->head) {
        Ty_ty ty = transTy(tenv, A_NameTy(fieldList->head->pos, fieldList->head->typ));
        tyList = Ty_TyList(ty, tyList);
        fieldList = fieldList->tail;
    }
    return tyList;
}

// turn a A_ty type into Ty_ty type, peel nameTy
Ty_ty transTy(S_table tenv, A_ty a) {
    switch(a->kind) {
        case A_nameTy: {
            /* 1. find this type in tenv
             * 2. return the type (iterate until single namety)
             */
            Ty_ty ty = (Ty_ty)S_look(tenv, get_ty_name(a));
            if (!ty) {
                /* error message */
                EM_error(a->pos, "(transTy-namety)type not defined.");
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
            A_fieldList record = get_ty_record(a);
            Ty_fieldList ty_fieldList = NULL;
            while (record && record->head) {
                A_field field = record->head;
                Ty_field ty_field = Ty_Field(field->name, transTy(tenv, A_NameTy(field->pos, field->typ)));
                ty_fieldList = Ty_FieldList(ty_field, ty_fieldList);
                record = record->tail;
            }
            return Ty_Record(ty_fieldList);
        }
        case A_arrayTy: {
            S_symbol array = get_ty_array(a);
            return Ty_Array(transTy(tenv, A_NameTy(a->pos, array)));
        }
        default: ;
    }
}

struct expty transExp(S_table venv, S_table tenv, A_exp a) {
    switch(a->kind) {
        case A_seqExp: {
            A_expList seq = get_seqexp_seq(a);
            while (seq && seq->head && seq->tail) {
                transExp(venv, tenv, seq->head);
                seq = seq->tail;
            }
            // here found a bug.
            // TODO: let in expseq end, the expseq can is zero or more exps.
            // but the (expseq) have two or more exps.
            return transExp(venv, tenv, seq->head);
        }
        case A_varExp: {
            return transVar(venv, tenv, a->u.var);
        }
        case A_nilExp: {
            return expTy(NULL, Ty_Nil());
        }
        case A_intExp: {
            return expTy(NULL, Ty_Int());
        }
        case A_stringExp: {
            return expTy(NULL, Ty_String());
        }
        case A_callExp: {
            // find the label in venv, compare argument types
            S_symbol func = get_callexp_func(a);
            A_expList args = get_callexp_args(a);
            E_enventry enventry = (E_enventry)S_look(venv, func);
            if (!enventry || enventry->kind != E_funEntry) {
                EM_error(a->pos, "(transExp-callexp)No such function.");
                return expTy(NULL, Ty_Int());
            }

            Ty_tyList formals = get_func_tylist(enventry);
            Ty_ty result = get_func_res(enventry);

            A_expList expList = args;
            Ty_tyList tyList = formals;
            while (expList && expList->head) {
                if (!(tyList && tyList->head)) {
                    EM_error(a->pos, "(transExp-callexp)Function params not match.");
                    return expTy(NULL, Ty_Int());
                }
                struct expty paramTy = transExp(venv, tenv, expList->head);
                if (!tyEqual(paramTy.ty, tyList->head)) {
                    EM_error(a->pos, "(transExp-callexp)Function params type error.");
                    return expTy(NULL, Ty_Int());
                }
                tyList = tyList->tail;
                expList = expList->tail;
            }
            // fewer params
            if (expList) {
                EM_error(a->pos, "(transExp-callexp)Function params not match.");
                return expTy(NULL, Ty_Int());
            }
            return expTy(NULL, result);
        }
    	case A_opExp: {
            A_oper oper = get_opexp_oper(a);
            struct expty left = transExp(venv, tenv, get_opexp_left(a));
            struct expty right = transExp(venv, tenv, get_opexp_right(a));
            if (oper == A_plusOp || oper == A_minusOp || oper == A_timesOp || oper == A_divideOp ||
                oper == A_ltOp || oper == A_leOp || oper == A_gtOp || oper == A_geOp) {
                if (get_expty_kind(left) != Ty_int)
                    EM_error(get_opexp_leftpos(a), "integer required");
                if (get_expty_kind(right) != Ty_int)
                    EM_error(get_opexp_rightpos(a), "integer required");
                return expTy(NULL, Ty_Int());
            }
            if (oper == A_eqOp || oper == A_neqOp) {
                if (!((get_expty_kind(left) == Ty_int ||
                       get_expty_kind(left) == Ty_string ||
                       get_expty_kind(left) == Ty_name)
                    && tyEqual(left.ty, right.ty))) {
                    //TODO: in this func, expty in transExp must return types which can be decided equal or not.
                    // or write another function to do this. This is finally on transVar. Think about when encountering
                    // record and array type. Better to return a nametype? Or should them all be nametype?
                    // This also has something to do with typedec..
                    EM_error(get_opexp_leftpos(a), "(transExp-opexp)type not match");
                }
                return expTy(NULL, Ty_Int());
            }
            EM_error(get_opexp_leftpos(a), "(transExp-opexp)type not match");
            return expTy(NULL, Ty_Int());
        }
        case A_recordExp: {
            S_symbol typ = get_recordexp_typ(a);
            A_efieldList fields = get_recordexp_fields(a);

            Ty_ty ty = transTy(tenv, A_NameTy(a->pos, typ));
            if (ty->kind != Ty_name || ((Ty_ty)get_namety_ty(ty))->kind != Ty_record) {
                EM_error(a->pos, "(transExp-recordExp)no such record id");
                return expTy(NULL, Ty_Void()); // or nil?? what does nil do?
            }
            Ty_fieldList record = ((Ty_ty)get_namety_ty(ty))->u.record;

            A_efieldList efieldList = fields;
            Ty_fieldList fieldList = record;
            while (efieldList && efieldList->head) {
                // assume they are in the same order..
                if(!(fieldList && fieldList->head)) {
                    EM_error(a->pos, "(transExp-recordExp)");
                    return expTy(NULL, Ty_Nil()); // or nil?? what does nil do?
                }
                if (efieldList->head->name != fieldList->head->name) {
                    EM_error(a->pos, "(transExp-recordExp)names not match");
                    return expTy(NULL, Ty_Nil()); // or nil?? what does nil do?
                }
                struct expty fieldExpTy = transExp(venv, tenv, efieldList->head->exp);
                if (!tyEqual(fieldExpTy.ty, fieldList->head->ty)) {
                    EM_error(a->pos, "(transExp-recordExp)types not match");
                    return expTy(NULL, Ty_Nil()); // or nil?? what does nil do?
                }
                efieldList = efieldList->tail;
                fieldList = fieldList->tail;
            }
            if (fieldList) {
                EM_error(a->pos, "(transExp-recordExp)fewer params");
                return expTy(NULL, Ty_Nil()); // or nil?? what does nil do?
            }
            return expTy(NULL, Ty_Nil());

        }
        case A_arrayExp: {
            S_symbol typ = get_arrayexp_typ(a);
            A_exp size = get_arrayexp_size(a);
            A_exp init = get_arrayexp_init(a);

            Ty_ty ty = transTy(tenv, A_NameTy(a->pos, typ));
            if (ty->kind != Ty_name || ((Ty_ty)get_namety_ty(ty))->kind != Ty_array) {
                EM_error(a->pos, "(transExp-arrayexp)no such array id");
                return expTy(NULL, Ty_Int());// TODO
            }

            struct expty sizety = transExp(venv, tenv, size);
            struct expty initty = transExp(venv, tenv, init);

            if (sizety.ty->kind != Ty_int) {
                EM_error(size->pos, "(transExp-arrayexp)size type wrong");
                return expTy(NULL, Ty_Int());// TODO
            }
            if (!tyEqual(initty.ty, ty->u.array)) {
                EM_error(size->pos, "(transExp-arrayexp)init type wrong");
                return expTy(NULL, Ty_Int());// TODO
            }
            return expTy(NULL, initty.ty);
        }
        case A_assignExp: {
            A_var var = get_assexp_var(a);
            A_exp exp = get_assexp_exp(a);
            struct expty varTy = transVar(venv, tenv, var);
            struct expty rhsTy = transExp(venv, tenv, exp);
            if (!tyEqual(varTy.ty, rhsTy.ty))
                EM_error(a->pos, "(transExp-assignExo)types not match.");
            return expTy(NULL, Ty_Void());
        }
        case A_ifExp: {
            A_exp test = get_ifexp_test(a);
            A_exp then = get_ifexp_then(a);
            A_exp elsee = get_ifexp_else(a);
            struct expty testTy = transExp(venv, tenv, test);
            if (testTy.ty->kind != Ty_int) {
                EM_error(test->pos, "(transExp-ifexp)test condition return wrong type.");
            }
            struct expty thenTy = transExp(venv, tenv, then);
            if (!elsee) {
                if (thenTy.ty->kind != Ty_void)
                    EM_error(then->pos, "(transExp-ifexp)then branch should produce no value.");
                return expTy(NULL, Ty_Void());
            }
            struct expty elseTy = transExp(venv, tenv, elsee);
            if (!tyEqual(thenTy.ty, elseTy.ty))
                EM_error(then->pos, "(transExp-ifexp)two branch return values not the same type.");
            return expTy(NULL, thenTy.ty);
        }
        case A_whileExp: {
            A_exp test = get_whileexp_test(a);
            A_exp body = get_whileexp_body(a);
            struct expty testTy = transExp(venv, tenv, test);
            struct expty bodyTy = transExp(venv, tenv, body);
            if (testTy.ty->kind != Ty_int)
                EM_error(test->pos, "(transExp-whileexp)test condition return wrong type.");
            if (bodyTy.ty->kind != Ty_void)
                EM_error(body->pos, "(transExp-whileexp)body must return no value.");
            return expTy(NULL, Ty_Void());
        }
        case A_forExp: {
            S_symbol var = get_forexp_var(a);
            A_exp lo = get_forexp_lo(a);
            A_exp hi = get_forexp_hi(a);
            A_exp body = get_forexp_body(a);
            struct expty loTy = transExp(venv, tenv, lo);
            struct expty hiTy = transExp(venv, tenv, hi);
            if (!tyEqual(loTy.ty, hiTy.ty))
                EM_error(lo->pos, "(transExp-forExp)lo and hi value types not match.");
            S_beginScope(venv);
                S_enter(venv, var, E_VarEntry(loTy.ty));
                struct expty bodyTy = transExp(venv, tenv, body);
                if (bodyTy.ty->kind != Ty_void)
                    EM_error(body->pos, "(transExp-forExp)body must have no return values.");
            S_endScope(venv);
            return expTy(NULL, Ty_Void());
        }
        case A_breakExp: {
            //TODO??how to handle this??
            return expTy(NULL, Ty_Void());
        }
        case A_letExp: {
            struct expty exp;
            A_decList decList;
            S_beginScope(venv);
            S_beginScope(tenv);
            for (decList = get_letexp_decs(a); decList; decList = decList->tail)
                transDec(venv, tenv, decList->head);
            exp = transExp(venv, tenv, get_letexp_body(a));
            S_endScope(tenv);
            S_endScope(venv);
            return exp;
        }
        default: ;
    }
}

void transDec(S_table venv, S_table tenv, A_dec d) {
    switch (d->kind) {
        case A_varDec: {
            A_exp init = get_vardec_init(d);
            S_symbol var = get_vardec_var(d);
            S_symbol typ = get_vardec_typ(d);
            struct expty initTy = transExp(venv, tenv, init);
            if (strcmp(S_name(typ), "")) {
                Ty_ty ty = transTy(tenv, A_NameTy(d->pos, typ));
                // TODO: this part also requires transTy and transExp
                // results the same in record and array type.
                if (!tyEqual(initTy.ty, ty))
                    EM_error(d->pos, "(transDec) error.");
            }
            S_enter(venv, var, E_VarEntry(initTy.ty));
            break;
        }
        case A_typeDec: {
            A_nametyList nametyList = get_typedec_list(d);
            // enter all dec of name type
            while (nametyList && nametyList->head) {
                S_enter(tenv, nametyList->head->name, Ty_Name(nametyList->head->name, NULL));
                nametyList = nametyList->tail;
            }
            nametyList = get_typedec_list(d);
            while (nametyList && nametyList->head) {
                A_namety namety = nametyList->head;
                Ty_ty ty = (Ty_ty)S_look(tenv, namety->name);
                if (get_ty_type(ty) != Ty_name)
                    EM_error(namety->ty->pos, "(transDec-typeDec)error.");
                ty = get_namety_ty(ty);
                if (get_ty_type(ty) != Ty_name)
                    /*EM_error();*/;
                ty->u.name.ty = transTy(tenv, namety->ty);
                // translate A_ty to Ty_ty.
                // TODO: how? translate to which step? if we have type a = b, type c = a,
                // should we return for c nameTy(a) or nameTy(b)?
                // Then one day if we compare a's type with c's type, can we safely return OK?
                nametyList = nametyList->tail;
            }
            // TODO: cycle detection.
            break;
        }
        case A_functionDec: {
            /* 1. enter all function in venv with params and return type (if have)
             * 2. for each func, begin scope
             * 3. enter params into venv
             * 4. transExp, check whether return type is equivalent
             * 5. endscope
             */
            A_fundecList fundecList = get_funcdec_list(d);
            while (fundecList && fundecList->head) {
                A_fundec fundec = fundecList->head;
                Ty_ty resultTy = Ty_Void();
                if (strcmp(S_name(fundec->result), "")) {
                    Ty_ty resultTy = transTy(tenv, A_NameTy(fundec->pos, fundec->result));
                    if (!resultTy)
                        EM_error(fundec->pos, "(transDec-func)result type not found.");
                }
                Ty_tyList formalTys = makeFormalTyList(tenv, fundec->params);
                S_enter(venv, fundec->name, E_FunEntry(formalTys, resultTy));

                fundecList = fundecList->tail;
            }

            fundecList = get_funcdec_list(d);
            while (fundecList && fundecList->head) {
                A_fundec fundec = fundecList->head;
                E_enventry enventry = (E_enventry)S_look(venv, fundec->name);
                if (!enventry || enventry->kind != E_funEntry)
                    /* error message */;
                Ty_tyList formalTys = get_func_tylist(enventry);
                Ty_ty resultTy = get_func_res(enventry);

                S_beginScope(venv);
                {
                    A_fieldList l;
                    Ty_tyList t;
                    for (l = fundec->params, t = formalTys; l; l = l->tail, t = t->tail) {
                        S_enter(venv, l->head->name, E_VarEntry(t->head));
                    }
                }
                struct expty bodyTy = transExp(venv, tenv, fundec->body);
                if (!tyEqual(bodyTy.ty, resultTy)) {
                    EM_error(fundec->pos, "(transDec-func)result type is incorrect.");
                }
                S_endScope(venv);
                fundecList = fundecList->tail;
            }
            break;
        }
        default: ;
    }
}

// actual ty of var
struct expty transVar(S_table venv, S_table tenv, A_var v) {
    switch(v->kind) {
        case A_simpleVar: {
            S_symbol simple = get_simplevar_sym(v);
            E_enventry enventry = (E_enventry)S_look(venv, simple);
            if (enventry && enventry->kind == E_varEntry)
                return expTy(NULL, actual_ty(get_varentry_type(enventry)));
            else {
                EM_error(v->pos, "undefined variable %s", S_name(get_simplevar_sym(v)));
                return expTy(NULL, Ty_Int());
            }
        }
        case A_fieldVar: {
            A_var var = get_fieldvar_var(v);
            struct expty fieldVarTy = transVar(venv, tenv, var);

            if (fieldVarTy.ty->kind != Ty_name)
                /* error */;
            Ty_ty ty = get_namety_ty(fieldVarTy.ty);
            if (ty->kind != Ty_record)
                /* error */;

            S_symbol sym = get_fieldvar_sym(v);
            Ty_fieldList fieldList = ty->u.record;
            while (fieldList && fieldList->head) {
                Ty_field field = fieldList->head;
                if (field->name == sym) // TODO: is this right?
                    return expTy(NULL, actual_ty(field->ty));
                fieldList = fieldList->tail;
            }
            EM_error(v->pos, "no match field");
            return expTy(NULL, Ty_Int());
        }
        case A_subscriptVar: {
            A_var var = get_subvar_var(v);
            struct expty subscriptVarTy = transVar(venv, tenv, var);

            if (subscriptVarTy.ty->kind != Ty_name)
                /* error */;
            Ty_ty ty = get_namety_ty(subscriptVarTy.ty);
            if (ty->kind != Ty_array)
                /* error */;

            return expTy(NULL, actual_ty(get_ty_array(ty)));
        }
        default: ;
    }
}

void SEM_transProg(A_exp exp) {
    S_table tenv = E_base_tenv();
    S_table venv = E_base_venv();
    transExp(venv, tenv, exp);
}
