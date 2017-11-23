#include <stdio.h>
#include "util.h"
#include "symbol.h"
#include "env.h"

/*Lab4: Your implementation of lab4*/

E_enventry E_VarEntry(Tr_access access, Ty_ty ty) {
	E_enventry entry = checked_malloc(sizeof(*entry));

    entry->kind = E_varEntry; // ????
	entry->u.var.access = access;
	entry->u.var.ty = ty;
	return entry;
}

E_enventry E_ROVarEntry(Tr_access access, Ty_ty ty) {
	E_enventry entry = checked_malloc(sizeof(*entry));

    entry->kind = E_varEntry; // ????
	entry->u.var.access = access;
	entry->u.var.ty = ty;
	entry->readonly = 1;
	return entry;
}

E_enventry E_FunEntry(Tr_level level, Temp_label label, Ty_tyList formals, Ty_ty result) {
	E_enventry entry = checked_malloc(sizeof(*entry));

    entry->kind = E_funEntry; // ????
	entry->u.fun.level = level;
	entry->u.fun.label = label;
	entry->u.fun.formals = formals;
	entry->u.fun.result = result;
	return entry;
}

//sym->value
//type_id(name, S_symbol) -> type (Ty_ty)
S_table E_base_tenv(void) {
	S_table table;
	S_symbol ty_int;
	S_symbol ty_string;

	table = S_empty();

	//basic type: string
	ty_int = S_Symbol("int");
	S_enter(table, ty_int, Ty_Int());

	//basic type: string
	ty_string = S_Symbol("string");
	S_enter(table, ty_string, Ty_String());

	return table;
}

S_table E_base_venv(void) {
	S_table venv;
    S_symbol funcname;
    Ty_tyList formalTys;
	Ty_ty resultTy;

	Temp_label label = NULL;
    U_boolList formals;
	Tr_level level;

	Tr_level parent = Tr_outermost();
	venv = S_empty();

    //basic func: flush
    formals = NULL;
    level = Tr_newLevel(parent, label, formals);
    funcname = S_Symbol("flush");
    formalTys = NULL;
    resultTy = Ty_Void();
	S_enter(venv, funcname, E_FunEntry(level, label, formalTys, resultTy));

    //basic func: exit
    formals = U_BoolList(TRUE, NULL);   //TODO: assume all params escape
    level = Tr_newLevel(parent, label, formals);
    funcname = S_Symbol("exit");
    formalTys = Ty_TyList(Ty_Int(), NULL);
    resultTy = Ty_Void();
    S_enter(venv, funcname, E_FunEntry(level, label, formalTys, resultTy));

    //basic func: not
    formals = U_BoolList(TRUE, NULL);   //TODO: assume all params escape
    level = Tr_newLevel(parent, label, formals);
    funcname = S_Symbol("not");
    formalTys = Ty_TyList(Ty_Int(), NULL);
    resultTy = Ty_Int();
    S_enter(venv, funcname, E_FunEntry(level, label, formalTys, resultTy));

    //basic func: chr
    formals = U_BoolList(TRUE, NULL);   //TODO: assume all params escape
    level = Tr_newLevel(parent, label, formals);
    funcname = S_Symbol("chr");
    formalTys = Ty_TyList(Ty_Int(), NULL);
    resultTy = Ty_String();
    S_enter(venv, funcname, E_FunEntry(level, label, formalTys, resultTy));

    //basic func: getchar
    formals = NULL;
    level = Tr_newLevel(parent, label, formals);
    funcname = S_Symbol("getchar");
    formalTys = NULL;
    resultTy = Ty_String();
    S_enter(venv, funcname, E_FunEntry(level, label, formalTys, resultTy));

    //basic func: print
    formals = U_BoolList(TRUE, NULL);   //TODO: assume all params escape
    level = Tr_newLevel(parent, label, formals);
    funcname = S_Symbol("print");
    formalTys = Ty_TyList(Ty_String(), NULL);
    resultTy = Ty_Void();
    S_enter(venv, funcname, E_FunEntry(level, label, formalTys, resultTy));

    //basic func: ord
    formals = U_BoolList(TRUE, NULL);   //TODO: assume all params escape
    level = Tr_newLevel(parent, label, formals);
    funcname = S_Symbol("ord");
    formalTys = Ty_TyList(Ty_String(), NULL);
    resultTy = Ty_Int();
    S_enter(venv, funcname, E_FunEntry(level, label, formalTys, resultTy));

    //basic func: size
    formals = U_BoolList(TRUE, NULL);   //TODO: assume all params escape
    level = Tr_newLevel(parent, label, formals);
    funcname = S_Symbol("size");
    formalTys = Ty_TyList(Ty_String(), NULL);
    resultTy = Ty_Int();
    S_enter(venv, funcname, E_FunEntry(level, label, formalTys, resultTy));

    //basic func: concat
    formals = U_BoolList(TRUE, U_BoolList(TRUE, NULL));   //TODO: assume all params escape
    level = Tr_newLevel(parent, label, formals);
    funcname = S_Symbol("concat");
    formalTys = Ty_TyList(Ty_String(), Ty_TyList(Ty_String(), NULL));
    resultTy = Ty_String();
    S_enter(venv, funcname, E_FunEntry(level, label, formalTys, resultTy));

    //basic func: substring
    formals = U_BoolList(TRUE, U_BoolList(TRUE, U_BoolList(TRUE, NULL)));   //TODO: assume all params escape
    level = Tr_newLevel(parent, label, formals);
    funcname = S_Symbol("substring");
    formalTys = Ty_TyList(Ty_String(), Ty_TyList(Ty_Int(), Ty_TyList(Ty_Int(), NULL)));
    resultTy = Ty_String();
    S_enter(venv, funcname, E_FunEntry(level, label, formalTys, resultTy));

	return venv;
}
