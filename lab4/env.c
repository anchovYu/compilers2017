#include <stdio.h>
#include "util.h"
#include "symbol.h"
#include "env.h"

/*Lab4: Your implementation of lab4*/

E_enventry E_VarEntry(Ty_ty ty)
{
	E_enventry entry = checked_malloc(sizeof(*entry));
    entry->kind = E_varEntry; // ????
	entry->u.var.ty = ty;

	return entry;
}

E_enventry E_ROVarEntry(Ty_ty ty)
{
	E_enventry entry = checked_malloc(sizeof(*entry));
    entry->kind = E_varEntry; // ????
	entry->u.var.ty = ty;
	entry->readonly = 1;
	return entry;
}

E_enventry E_FunEntry(Ty_tyList formals, Ty_ty result)
{
	E_enventry entry = checked_malloc(sizeof(*entry));
    entry->kind = E_funEntry; // ????
	entry->u.fun.formals = formals;
	entry->u.fun.result = result;

	return entry;
}

//sym->value
//type_id(name, S_symbol) -> type (Ty_ty)
S_table E_base_tenv(void)
{
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

S_table E_base_venv(void)
{
	S_table table;
    S_symbol funcname;
    Ty_tyList formalTys;
    Ty_ty resultTy;


	table = S_empty();

    //basic func: print
    funcname = S_Symbol("print");
    formalTys = Ty_TyList(Ty_String(), NULL);
    resultTy = Ty_Void();
    S_enter(table, funcname, E_FunEntry(formalTys, resultTy));


    //basic func: flush
    funcname = S_Symbol("flush");
    formalTys = NULL;
    resultTy = Ty_Void();
    S_enter(table, funcname, E_FunEntry(formalTys, resultTy));

    //basic func: getchar
    funcname = S_Symbol("getchar");
    formalTys = NULL;
    resultTy = Ty_String();
    S_enter(table, funcname, E_FunEntry(formalTys, resultTy));

    //basic func: ord
    funcname = S_Symbol("ord");
    formalTys = Ty_TyList(Ty_String(), NULL);
    resultTy = Ty_Int();
    S_enter(table, funcname, E_FunEntry(formalTys, resultTy));

    //basic func: chr
    funcname = S_Symbol("chr");
    formalTys = Ty_TyList(Ty_Int(), NULL);
    resultTy = Ty_String();
    S_enter(table, funcname, E_FunEntry(formalTys, resultTy));

    //basic func: size
    funcname = S_Symbol("size");
    formalTys = Ty_TyList(Ty_String(), NULL);
    resultTy = Ty_Int();
    S_enter(table, funcname, E_FunEntry(formalTys, resultTy));

    //basic func: substring
    funcname = S_Symbol("substring");
    formalTys = Ty_TyList(Ty_String(), Ty_TyList(Ty_Int(), Ty_TyList(Ty_Int(), NULL)));
    resultTy = Ty_String();
    S_enter(table, funcname, E_FunEntry(formalTys, resultTy));

    //basic func: concat
    funcname = S_Symbol("concat");
    formalTys = Ty_TyList(Ty_String(), Ty_TyList(Ty_String(), NULL));
    resultTy = Ty_String();
    S_enter(table, funcname, E_FunEntry(formalTys, resultTy));

    //basic func: not
    funcname = S_Symbol("not");
    formalTys = Ty_TyList(Ty_Int(), NULL);
    resultTy = Ty_Int();
    S_enter(table, funcname, E_FunEntry(formalTys, resultTy));

    //basic func: exit
    funcname = S_Symbol("exit");
    formalTys = Ty_TyList(Ty_Int(), NULL);
    resultTy = Ty_Void();
    S_enter(table, funcname, E_FunEntry(formalTys, resultTy));

	return table;
}
