#ifndef TRANSLATE_H
#define TRANSLATE_H

#include "util.h"
#include "absyn.h"
#include "temp.h"
#include "frame.h"

/* Lab5: your code below */

typedef struct Tr_exp_ *Tr_exp;
typedef struct Tr_access_ *Tr_access;
typedef struct Tr_accessList_ *Tr_accessList;
struct Tr_accessList_ {
	Tr_access head;
	Tr_accessList tail;
};

typedef struct Tr_level_ *Tr_level;
struct Tr_level_ {
	F_frame frame;
	Tr_level parent;
};
typedef struct Tr_expList_ *Tr_expList;
struct Tr_expList_ {Tr_exp head; Tr_expList tail;};

Tr_expList Tr_ExpList(Tr_exp head, Tr_expList tail);
Tr_accessList Tr_AccessList(Tr_access head, Tr_accessList tail);

Tr_level Tr_outermost(void);
Tr_level Tr_newLevel(Tr_level parent, Temp_label name, U_boolList formals);
Tr_accessList Tr_formals(Tr_level level);
Tr_access Tr_allocLocal(Tr_level level, bool escape);

Tr_exp Tr_nop();
Tr_exp Tr_Nil();
Tr_exp Tr_Int(int i);
Tr_exp Tr_String(string str);
Tr_exp Tr_Call(Temp_label func, Tr_expList inParams, Tr_level caller, Tr_level callee);
Tr_exp Tr_Op(A_oper oper, Tr_exp left, Tr_exp right);
Tr_exp Tr_OpCom(A_oper oper, Tr_exp left, Tr_exp right, bool isString);
Tr_exp Tr_record(Tr_expList fields);
Tr_exp Tr_array(Tr_exp size, Tr_exp init);
Tr_exp Tr_assign(Tr_exp lhs, Tr_exp rhs);
Tr_exp Tr_if(Tr_exp test, Tr_exp then, Tr_exp elsee);
Tr_exp Tr_while(Tr_exp test, Tr_exp body, Temp_label breakk);
Tr_exp Tr_for(Tr_exp lo, Tr_exp hi, Tr_exp body, Tr_exp loopVar, Temp_label breakk);
Tr_exp Tr_break(Temp_label breakk);
Tr_exp Tr_indexVar(Tr_exp var, Tr_exp index);

Tr_exp Tr_simpleVar(Tr_access access, Tr_level level);

void Tr_procEntryExit(Tr_level level, Tr_exp body);
F_fragList Tr_getResult(void);




#endif
