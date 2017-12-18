
/*Lab5: This header file is not complete. Please finish it with more definition.*/

#ifndef FRAME_H
#define FRAME_H

#include "tree.h"
#include "assem.h"

typedef struct F_frame_ *F_frame;

typedef struct F_access_ *F_access;
typedef struct F_accessList_ *F_accessList;
struct F_accessList_ {F_access head; F_accessList tail;};
F_accessList F_AccessList(F_access head, F_accessList tail);

F_frame F_newFrame(Temp_label name, U_boolList formals);
Temp_label F_name(F_frame f);
F_accessList F_formals(F_frame f);
F_access F_allocLocal(F_frame f, bool escape);
// return offset
int F_allocSpill(F_frame f);

/* declaration for fragments */
typedef struct F_frag_ *F_frag;
struct F_frag_ {enum {F_stringFrag, F_procFrag} kind;
			union {
				struct {Temp_label label; string str;} stringg;
				struct {T_stm body; F_frame frame;} proc;
			} u;
};

F_frag F_StringFrag(Temp_label label, string str);
F_frag F_ProcFrag(T_stm body, F_frame frame);

typedef struct F_fragList_ *F_fragList;
struct F_fragList_
{
	F_frag head;
	F_fragList tail;
};

F_fragList F_FragList(F_frag head, F_fragList tail);

Temp_map F_tempMap;

Temp_map F_initialTempMap();
Temp_tempList F_CalleeSaves();
Temp_tempList F_CallerSaves();

Temp_temp F_FP(void);
Temp_temp F_SP(void);
Temp_temp F_RV(void);

Temp_temp F_EAX(void);
Temp_temp F_ECX(void);
Temp_temp F_EDX(void);
Temp_temp F_EBX(void);
Temp_temp F_ESI(void);
Temp_temp F_EDI(void);

extern const int F_wordSize;
//const int F_wordSize = 4;
T_exp F_Exp(F_access acc, T_exp framePtr);

T_exp F_externalCall(string s, T_expList args);

// move calleesaves to or from registers,
// before register allocation, after codegen
AS_instrList F_procEntryExit1(AS_instrList body);

// add live to end,
// before register allocation, after codegen
AS_instrList F_procEntryExit2(AS_instrList body);

// entry and exit for func, after register allocation
AS_instrList F_procEntryExit3(F_frame f, AS_instrList body);


#endif
