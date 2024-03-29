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
// TODO: cycle detection. (16)
// TODO: () exp (12,20,43)

/*Lab4: Your implementation of lab4*/

static char str_ty[][12] = {
   "ty_record", "ty_nil", "ty_int", "ty_string",
   "ty_array", "ty_name", "ty_void"};

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
        //printf("***(actual_ty)name ty kind: %s\n", str_ty[t->kind]);
        //Ty_ty temp = t->u.name.ty;
        Ty_ty temp = get_namety_ty(t);
        if (!temp || get_ty_type(temp) == Ty_record || get_ty_type(temp) == Ty_array)
            break;
        t = temp;
    }
    return t;
}

// Compare two types.
// For int, string types etc., compare their kind.
// For array and record type (in namety form), directly compare then in pointers.
bool tyEqual(Ty_ty a, Ty_ty b) {
    printf("enter here\n");
    Ty_ty tya = actual_ty(a);
    printf("enter here1\n");

    Ty_ty tyb = actual_ty(b);
    printf("enter here2\n");

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
            printf("enter here3\n");

            if (tya == tyb) {
                printf("enter here4\n");

                return TRUE;
            }
            printf("enter here5\n");
            // (a, NULL) = (c, NULL)
            if (!get_namety_ty(tya))
                return FALSE;

            // record, nil
            if (get_namety_ty(tya)->kind == Ty_record && tyb->kind == Ty_nil) {
                printf("we should return true;\n");
                return TRUE;
            }
            printf("we should return false;\n");
            return FALSE;
        }
        default:
            return FALSE;
    }
}

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

// turn a A_ty type into Ty_ty type, peel nameTy
Ty_ty transTy(S_table tenv, A_ty a) {
    switch(a->kind) {
        case A_nameTy: {
            /* 1. find this type in tenv
             * 2. return the type (iterate until single namety)
             */
            printf("enter transTy-nameTy.\n");
            //printf("namety's ty name: %s\n", S_name(get_ty_name(a)));
            Ty_ty ty = (Ty_ty)S_look(tenv, get_ty_name(a));
            //if (ty) printf("***name ty kind: %s\n", str_ty[ty->kind]);

            if (!ty) {
                /* error message */
                EM_error(a->pos, "undefined type %s", S_name(get_ty_name(a)));
                return NULL;
            }
            ty = actual_ty(ty);
            //printf("enter here.\n");
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
            printf("enter transTy-recordTy.\n");
            // should place them in the right order.
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
                //temp = temp->tail;
                //temp = Ty_FieldList(ty_field, NULL);
                //printf("***record field: %s\n", S_name(temp->head->name));
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
            printf("enter transTy-arrayTy.\n");
            S_symbol array = get_ty_array(a);
            return Ty_Array(transTy(tenv, A_NameTy(a->pos, array)));
            /* error condition: transTy returns NULL */
        }
        default: ;
    }
}

struct expty transExp(S_table venv, S_table tenv, A_exp a) {
    switch(a->kind) {
        case A_seqExp: {
            printf("enter transExp-seqExp.\n");
            A_expList seq = get_seqexp_seq(a);
            if (!seq)
                return expTy(NULL, Ty_Void());
            while (seq && seq->head && seq->tail) {
                transExp(venv, tenv, seq->head);
                //printf("finish here\n");
                seq = seq->tail;
            }
            // here found a bug.
            // TODO: let in expseq end, the expseq can is zero or more exps.
            // but the (expseq) have two or more exps.
            return transExp(venv, tenv, seq->head);
        }
        case A_varExp: {
            printf("enter transExp-varExp.\n");
            return transVar(venv, tenv, a->u.var);
        }
        case A_nilExp: {
            printf("enter transExp-nilExp.\n");
            return expTy(NULL, Ty_Nil());
        }
        case A_intExp: {
            printf("enter transExp-intExp.\n");
            return expTy(NULL, Ty_Int());
        }
        case A_stringExp: {
            printf("enter transExp-stringExp.\n");
            return expTy(NULL, Ty_String());
        }
        case A_callExp: {
            printf("enter transExp-callExp.\n");
            // find the label in venv, compare argument types
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
            while (expList && expList->head) {
                if (!(tyList && tyList->head)) {
                    EM_error(expList->head->pos, "too many params in function %s", S_name(func));
                    return expTy(NULL, Ty_Int());
                }
                struct expty paramTy = transExp(venv, tenv, expList->head);
                if (!tyEqual(paramTy.ty, tyList->head)) {
                    EM_error(expList->head->pos, "para type mismatch");
                    return expTy(NULL, Ty_Int());
                }
                tyList = tyList->tail;
                expList = expList->tail;
            }
            // fewer params
            if (tyList) {
                EM_error(a->pos, "(transExp-callexp)Function params not match(fewer).");
                return expTy(NULL, Ty_Int());
            }
            return expTy(NULL, result);
        }
    	case A_opExp: {
            printf("enter transExp-opExp.\n");
            A_oper oper = get_opexp_oper(a);
            struct expty left = transExp(venv, tenv, get_opexp_left(a));
            struct expty right = transExp(venv, tenv, get_opexp_right(a));
            if (oper == A_plusOp || oper == A_minusOp || oper == A_timesOp || oper == A_divideOp) {
                if (get_expty_kind(left) != Ty_int)
                    EM_error(get_opexp_leftpos(a), "integer required");
                if (get_expty_kind(right) != Ty_int)
                    EM_error(get_opexp_rightpos(a), "integer required");
                return expTy(NULL, Ty_Int());
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
                return expTy(NULL, Ty_Int());
                //TODO: what will happen when compare between two strings?
            }

            if (oper == A_eqOp || oper == A_neqOp) {
                if (!((get_expty_kind(left) == Ty_int ||
                       get_expty_kind(left) == Ty_string ||
                       get_expty_kind(left) == Ty_nil ||
                       get_expty_kind(left) == Ty_name)
                    && tyEqual(left.ty, right.ty))) {
                    //TODO: in this func, expty in transExp must return types which can be decided equal or not.
                    // or write another function to do this. This is finally on transVar. Think about when encountering
                    // record and array type. Better to return a nametype? Or should them all be nametype?
                    // This also has something to do with typedec..
                    EM_error(get_opexp_rightpos(a), "same type required");
                }
                return expTy(NULL, Ty_Int());
            }
            EM_error(get_opexp_leftpos(a), "(transExp-opexp)type not match");
            return expTy(NULL, Ty_Int());
        }
        case A_recordExp: {
            printf("enter transExp-recordExp.\n");
            S_symbol typ = get_recordexp_typ(a);
            A_efieldList fields = get_recordexp_fields(a);

            Ty_ty ty = transTy(tenv, A_NameTy(a->pos, typ));
            if (!ty || ty->kind != Ty_name || ((Ty_ty)get_namety_ty(ty))->kind != Ty_record) {
                //EM_error(a->pos, "(transExp-recordExp)no such record id");
                // (ins) without error twice.
                return expTy(NULL, Ty_Nil()); // or nil?? what does nil do?
            }
            // Ty_ty whatever = (Ty_ty)get_namety_ty(ty);
            // printf("is this record type?: %s\n", str_ty[whatever->kind]);

            Ty_fieldList record = ((Ty_ty)get_namety_ty(ty))->u.record;
            A_efieldList efieldList = fields;
            Ty_fieldList fieldList = record;
            while (efieldList && efieldList->head) {
                // assume they are in the same order..
                // but they are not.
                if(!(fieldList && fieldList->head)) {
                    EM_error(a->pos, "(transExp-recordExp)too many params");
                    return expTy(NULL, Ty_Nil()); // or nil?? what does nil do?
                }
                //printf("efieldList:%s\n", S_name(efieldList->head->name));
                //printf("fieldList:%s\n", S_name(fieldList->head->name));
                if (strcmp(S_name(efieldList->head->name), S_name(fieldList->head->name))) {
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
            printf("finish recordExp\n");
            return expTy(NULL, ty); // return nameTy
        }
        case A_arrayExp: {
            printf("enter transExp-arrayExp.\n");
            S_symbol typ = get_arrayexp_typ(a);
            A_exp size = get_arrayexp_size(a);
            A_exp init = get_arrayexp_init(a);

            Ty_ty ty = transTy(tenv, A_NameTy(a->pos, typ));
            if (!ty || ty->kind != Ty_name || ((Ty_ty)get_namety_ty(ty))->kind != Ty_array) {
                EM_error(a->pos, "(transExp-arrayexp)no such array id");
                return expTy(NULL, Ty_Nil());// TODO
            }

            struct expty sizety = transExp(venv, tenv, size);
            struct expty initty = transExp(venv, tenv, init);

            if (sizety.ty->kind != Ty_int) {
                EM_error(size->pos, "(transExp-arrayexp)size type wrong");
                return expTy(NULL, Ty_Nil());// TODO
            }
            if (!tyEqual(initty.ty, ((Ty_ty)get_namety_ty(ty))->u.array)) {
                EM_error(init->pos, "type mismatch");
                return expTy(NULL, Ty_Nil());// TODO
            }
            return expTy(NULL, ty);
        }
        case A_assignExp: {
            printf("enter transExp-assignExp.\n");
            A_var var = get_assexp_var(a);
            A_exp exp = get_assexp_exp(a);
            struct expty lhsTy = transVar(venv, tenv, var);
            if (isROVar(venv, var)) {
                // hacking. In loop body, the var cannot be assigned.
                EM_error(a->pos, "loop variable can't be assigned");
                return expTy(NULL, Ty_Void());
            }
                //printf("finish transExp-assignExp: transVar.\n");
            struct expty rhsTy = transExp(venv, tenv, exp);
            if (!tyEqual(lhsTy.ty, rhsTy.ty))
                EM_error(a->pos, "unmatched assign exp");
            return expTy(NULL, Ty_Void());
        }
        case A_ifExp: {
            printf("enter transExp-ifExp.\n");
            A_exp test = get_ifexp_test(a);
            A_exp then = get_ifexp_then(a);
            A_exp elsee = get_ifexp_else(a);
            struct expty testTy = transExp(venv, tenv, test);
            if (testTy.ty->kind != Ty_int) {
                EM_error(test->pos, "(transExp-ifexp)test condition return wrong type.");
                return expTy(NULL, Ty_Void());
            }
            struct expty thenTy = transExp(venv, tenv, then);
            // printf("thenTy: %s\n", str_ty[thenTy.ty->kind]);
            // Ty_ty whatever = get_namety_ty(thenTy.ty);
            // if (whatever) printf("this is not null.\n");
            // printf("under thenTy: %s\n", str_ty[whatever->kind]);

            if (!elsee) {
                if (thenTy.ty->kind != Ty_void)
                    EM_error(then->pos, "if-then exp's body must produce no value");
                return expTy(NULL, Ty_Void());
            }
            struct expty elseTy = transExp(venv, tenv, elsee);
            printf("elseTy: %s\n", str_ty[elseTy.ty->kind]);

            if (!tyEqual(thenTy.ty, elseTy.ty)) {
                EM_error(elsee->pos, "then exp and else exp type mismatch");
                return expTy(NULL, Ty_Void());
            }
            return expTy(NULL, thenTy.ty);
        }
        case A_whileExp: {
            printf("enter transExp-whileExp.\n");

            A_exp test = get_whileexp_test(a);
            A_exp body = get_whileexp_body(a);
            struct expty testTy = transExp(venv, tenv, test);
            struct expty bodyTy = transExp(venv, tenv, body);
            if (testTy.ty->kind != Ty_int)
                EM_error(test->pos, "(transExp-whileexp)test condition return wrong type.");
            if (bodyTy.ty->kind != Ty_void)
                EM_error(body->pos, "while body must produce no value");
            return expTy(NULL, Ty_Void());
        }
        case A_forExp: {
            printf("enter transExp-forExp.\n");

            S_symbol var = get_forexp_var(a);
            A_exp lo = get_forexp_lo(a);
            A_exp hi = get_forexp_hi(a);
            A_exp body = get_forexp_body(a);
            struct expty loTy = transExp(venv, tenv, lo);
            struct expty hiTy = transExp(venv, tenv, hi);
            if (loTy.ty->kind != Ty_int)
                EM_error(lo->pos, "for exp's range type is not integer");
                //return expTy(NULL, Ty_Void()); continue running !
            if (hiTy.ty->kind != Ty_int)
                EM_error(hi->pos, "for exp's range type is not integer");
                //return expTy(NULL, Ty_Void()); continue running !

            S_beginScope(venv);
                S_enter(venv, var, E_ROVarEntry(loTy.ty));
                struct expty bodyTy = transExp(venv, tenv, body);
                if (bodyTy.ty->kind != Ty_void)
                    EM_error(body->pos, "(transExp-forExp)body must have no return values.");
            S_endScope(venv);
            return expTy(NULL, Ty_Void());
        }
        case A_breakExp: {
            printf("enter transExp-breakExp.\n");

            //TODO??how to handle this??
            return expTy(NULL, Ty_Void());
        }
        case A_letExp: {
            printf("enter transExp-letExp.\n");
            A_decList decList = get_letexp_decs(a);
            A_exp body = get_letexp_body(a);

            S_beginScope(venv);
            S_beginScope(tenv);
            //printf("enter here!\n");
            while (decList && decList -> head) {
                transDec(venv, tenv, decList->head);
                decList = decList->tail;
            }
            //printf("after transDec!\n");
            struct expty bodyTy = transExp(venv, tenv, body);
            S_endScope(tenv);
            S_endScope(venv);
            return bodyTy;
        }
        default: printf("no match kind\n");//return expTy(NULL, Ty_Void());
    }
}

void transDec(S_table venv, S_table tenv, A_dec d) {
    switch (d->kind) {
        case A_varDec: {
            printf("enter transDec-varDec.\n");

            A_exp init = get_vardec_init(d);
            S_symbol var = get_vardec_var(d);
            S_symbol typ = get_vardec_typ(d);
            struct expty initTy = transExp(venv, tenv, init);
            //printf("here\n");
            //printf("%s\n", str_ty[initTy.ty->kind]);

            if (strcmp(S_name(typ), "")) {
                Ty_ty ty = transTy(tenv, A_NameTy(d->pos, typ));
                // TODO: this part also requires transTy and transExp
                // results the same in record and array type.
                if (!ty) {
                    return;
                }
                if (!tyEqual(initTy.ty, ty)) {
                    EM_error(init->pos, "type mismatch");
                    //return; do not return directly
                }
            } else {
                //printf("here\n");
                if (initTy.ty->kind == Ty_nil)
                    EM_error(init->pos, "init should not be nil without type specified");
                //printf("here\n");

            }

            S_enter(venv, var, E_VarEntry(initTy.ty));
            break;
        }
        case A_typeDec: {
            printf("enter transDec-typeDec.\n");
            char* decHistory[32];   // TODO: not big enough though
            int cnt = 0;
            A_nametyList nametyList = get_typedec_list(d);
            // enter all dec of name type
            while (nametyList && nametyList->head) {
                int i;
                for (i = 0; i < cnt; i++) {
                    // equivalent to history entry
                    if (!strcmp(S_name(nametyList->head->name), decHistory[i])) {
                        EM_error(d->pos, "two types have the same name");
                        return;
                    }
                }
                S_enter(tenv, nametyList->head->name, Ty_Name(nametyList->head->name, NULL));
                decHistory[cnt] = S_name(nametyList->head->name);
                cnt ++;
                //printf("nametyList->head->name: %s\n", S_name(nametyList->head->name));
                nametyList = nametyList->tail;
            }
            printf("after first part of typedec!\n");
            nametyList = get_typedec_list(d);
            while (nametyList && nametyList->head) {
                A_namety namety = nametyList->head;

                printf("looking for namety->name: %s\n", S_name(namety->name));
                Ty_ty ty = (Ty_ty)S_look(tenv, namety->name);

                printf("lhsTy: %s\n", str_ty[ty->kind]);
                printf("under lhsTy: %s\n", S_name(ty->u.name.sym));
                Ty_ty whatever = get_namety_ty(ty);
                if (!whatever) printf("NOne\n");

                if (get_ty_type(ty) != Ty_name)
                    EM_error(namety->ty->pos, "(transDec-typeDec)error.");

                Ty_ty typeTy = transTy(tenv, namety->ty);
                // printf("rhsTy: %s\n", str_ty[typeTy->kind]);
                // printf("under rhsTy: %s\n", S_name(typeTy->u.name.sym));
                // Ty_ty whatever2 = get_namety_ty(typeTy);
                // if (!whatever2) printf("NOne\n");

                if (!typeTy) {
                    /* error */
                    return;
                }

                if (tyEqual(typeTy, ty)) {
                    EM_error(namety->ty->pos, "illegal type cycle");
                    return;
                }
                printf("enterhere\n");

                ty->u.name.ty = typeTy;

                // translate A_ty to Ty_ty.
                // TODO: how? translate to which step? if we have type a = b, type c = a,
                // should we return for c nameTy(a) or nameTy(b)?
                // Then one day if we compare a's type with c's type, can we safely return OK?
                nametyList = nametyList->tail;
            }
            // TODO: cycle detection.
            //printf("after typedec!\n");
            break;
        }
        case A_functionDec: {
            /* 1. enter all function in venv with params and return type (if have)
             * 2. for each func, begin scope
             * 3. enter params into venv
             * 4. transExp, check whether return type is equivalent
             * 5. endscope
             */
            printf("enter transDec-functionDec.\n");
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
                        return;
                    }
                }
                Ty_ty resultTy = Ty_Void();
                //printf("resultTy: %s\n", S_name(fundec->result));
                if (strcmp(S_name(fundec->result), "")) {
                    resultTy = transTy(tenv, A_NameTy(fundec->pos, fundec->result));
                    if (!resultTy)
                        EM_error(fundec->pos, "(transDec-func)result type not found.");
                }
                Ty_tyList formalTys = makeFormalTyList(tenv, fundec->params);
                //printf("enter venv of fundec: %s\n", S_name(fundec->name));
                S_enter(venv, fundec->name, E_FunEntry(formalTys, resultTy));
                decHistory[cnt] = S_name(fundec->name);
                cnt ++;
                fundecList = fundecList->tail;
            }

            fundecList = get_funcdec_list(d);
            while (fundecList && fundecList->head) {
                A_fundec fundec = fundecList->head;
                //printf("dec ing in fundec: %s\n", S_name(fundec->name));

                E_enventry enventry = (E_enventry)S_look(venv, fundec->name);
                if (!enventry || enventry->kind != E_funEntry)
                    {/* error message */printf("I can't find the func in the dec process!\n");}
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
                    //printf("bodyTy: %s, resultTy: %s\n", str_ty[bodyTy.ty->kind], str_ty[resultTy->kind]);
                    if (resultTy->kind == Ty_void)
                        EM_error(fundec->pos, "procedure returns value");
                    else
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
            printf("enter transVar-simpleVar.\n");

            S_symbol simple = get_simplevar_sym(v);
            E_enventry enventry = (E_enventry)S_look(venv, simple);
            if (enventry && enventry->kind == E_varEntry) {
                Ty_ty temp = get_varentry_type(enventry); ////
                //printf("%s\n", str_ty[temp->kind]);
                //printf("here\n");

                return expTy(NULL, actual_ty(get_varentry_type(enventry)));
            } else {
                EM_error(v->pos, "undefined variable %s", S_name(get_simplevar_sym(v)));
                return expTy(NULL, Ty_Int());
            }
        }
        case A_fieldVar: {
            printf("enter transVar-fieldVar.\n");
            A_var var = get_fieldvar_var(v);
            struct expty fieldVarTy = transVar(venv, tenv, var);

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
            while (fieldList && fieldList->head) {
                Ty_field field = fieldList->head;
                if (field->name == sym) // TODO: is this right?
                    return expTy(NULL, actual_ty(field->ty));
                fieldList = fieldList->tail;
            }
            EM_error(v->pos, "field %s doesn't exist", S_name(sym));
            return expTy(NULL, Ty_Int());
            // TODO: what should I return for rec1.nam := "asd"?
        }
        case A_subscriptVar: {
            printf("enter transVar-subscriptVar.\n");

            //TODO: shall we detect overflow exception?
            A_var var = get_subvar_var(v);
            struct expty subscriptVarTy = transVar(venv, tenv, var);

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
