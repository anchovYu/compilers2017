%{
#include <stdio.h>
#include "util.h"
#include "errormsg.h"
#include "symbol.h"
#include "absyn.h"

int yylex(void); /* function prototype */

A_exp absyn_root;

void yyerror(char *s)
{
 EM_error(EM_tokPos, "%s", s);
}
%}

%union {
	int pos;
	int ival;
	string sval;

	A_exp exp;             //exp

	A_expList explist;     //explist

	A_var var;             //var(lvalue)

	A_decList declist;     // dec
	A_dec  dec;

    A_nametyList nametylist;   // type dec
    A_namety namety;
	A_efieldList efieldlist;           //record field
	A_efield  efield;
	A_fieldList fieldlist;         //field type
	A_field field;
	A_fundecList fundeclist;       //function type
	A_fundec fundec;
	A_ty ty;                       //id type
}

%token <sval> ID STRING
%token <ival> INT

%token
  COMMA COLON SEMICOLON LPAREN RPAREN LBRACK RBRACK
  LBRACE RBRACE DOT
  PLUS MINUS TIMES DIVIDE EQ NEQ LT LE GT GE
  AND OR ASSIGN
  ARRAY IF THEN ELSE WHILE FOR TO DO LET IN END OF
  BREAK NIL
  FUNCTION VAR TYPE

%type <exp> exp callexp opexp recordexp arrayexp assignexp ifexp whileexp forexp letexp
%type <explist> expseq explist
%type <var> lvalue
%type <declist> decs
%type <dec> dec vardec
%type <efieldlist> rec
%type <efield> rec_one
%type <nametylist> tydecs
%type <namety> tydec
%type <fieldlist> tyfields tyfields_nonempty
%type <field> field
%type <ty> ty
%type <fundeclist> fundecs
%type <fundec> fundec

/*%nonassoc THEN ELSE DO OF    hacking */
%nonassoc ASSIGN
%left OR
%left AND
%nonassoc EQ NEQ LT LE GT GE
%left PLUS MINUS
%left TIMES DIVIDE
%left UMINUS

%start program

%%

program :   exp     {absyn_root = $1;};
        ;

exp     :   LPAREN exp RPAREN   /* emm.. */
                {$$ = $2;}
        |   LPAREN expseq RPAREN
                {$$ = A_SeqExp(EM_tokPos, $2);}
        |   lvalue
                {$$ = A_VarExp(EM_tokPos, $1);}
        |   NIL
                {$$ = A_NilExp(EM_tokPos);}
        |   INT
                {$$ = A_IntExp(EM_tokPos, $1);}
        |   STRING
                {$$ = A_StringExp(EM_tokPos, $1);}
        |   callexp
                {$$ = $1;}
        |   opexp
                {$$ = $1;}
        |   recordexp
                {$$ = $1;}
        |   arrayexp
                {$$ = $1;}
        |   assignexp
                {$$ = $1;}
        |   ifexp
                {$$ = $1;}
        |   whileexp
                {$$ = $1;}
        |   forexp
                {$$ = $1;}
        |   BREAK
                {$$ = A_BreakExp(EM_tokPos);}
        |   letexp
                {$$ = $1;} /* acutally miss the () expression */
        |   LPAREN RPAREN
                {$$ = A_SeqExp(EM_tokPos, NULL);}  /* hacking.. */

        ;

/* Difference between expseq and explist in nonterminal:
 * (exp;exp) => expseq
 * let decs in exp;exp end => expseq
 * func(exp,exp) => explist
 */
expseq  :   exp SEMICOLON expseq
                {$$ = A_ExpList($1, $3);}
        |   exp
                {$$ = A_ExpList($1, NULL);}
        ;

explist :   exp COMMA explist
                {$$ = A_ExpList($1, $3);}
        |   exp
                {$$ = A_ExpList($1, NULL);}
        ;

lvalue  :   ID
                {$$ = A_SimpleVar(EM_tokPos, S_Symbol($1));}
        |   lvalue DOT ID
                {$$ = A_FieldVar(EM_tokPos, $1, S_Symbol($3));}
        |   lvalue LBRACK exp RBRACK
                {$$ = A_SubscriptVar(EM_tokPos, $1, $3);}
        |   ID LBRACK exp RBRACK    /* hacking */
                {$$ = A_SubscriptVar(EM_tokPos, A_SimpleVar(EM_tokPos, S_Symbol($1)), $3);}
        ;

callexp :   ID LPAREN RPAREN
                {$$ = A_CallExp(EM_tokPos, S_Symbol($1), NULL);}
        |   ID LPAREN explist RPAREN
                {$$ = A_CallExp(EM_tokPos, S_Symbol($1), $3);}
        ;


opexp   :   exp PLUS exp
                {$$ = A_OpExp(EM_tokPos, A_plusOp, $1, $3);}
        |   exp MINUS exp
                {$$ = A_OpExp(EM_tokPos, A_minusOp, $1, $3);}
        |   exp TIMES exp
                {$$ = A_OpExp(EM_tokPos, A_timesOp, $1, $3);}
        |   exp DIVIDE exp
                {$$ = A_OpExp(EM_tokPos, A_divideOp, $1, $3);}
        |   exp EQ exp
                {$$ = A_OpExp(EM_tokPos, A_eqOp, $1, $3);}
        |   exp NEQ exp
                {$$ = A_OpExp(EM_tokPos, A_neqOp, $1, $3);}
        |   exp LT exp
                {$$ = A_OpExp(EM_tokPos, A_ltOp, $1, $3);}
        |   exp LE exp
                {$$ = A_OpExp(EM_tokPos, A_leOp, $1, $3);}
        |   exp GT exp
                {$$ = A_OpExp(EM_tokPos, A_gtOp, $1, $3);}
        |   exp GE exp
                {$$ = A_OpExp(EM_tokPos, A_geOp, $1, $3);}
        |   exp AND exp
                /* if e1 then e2 else 0 */
                {$$ = A_IfExp(EM_tokPos, $1, $3, A_IntExp(EM_tokPos, 0));}
        |   exp OR exp
                /* if e1 then 1 else e2 */
                {$$ = A_IfExp(EM_tokPos, $1, A_IntExp(EM_tokPos, 1), $3);}
        |   MINUS exp %prec UMINUS
                {$$ = A_OpExp(EM_tokPos, A_minusOp, A_IntExp(EM_tokPos, 0), $2);}
        ;

recordexp   :  ID LBRACE rec RBRACE
                {$$ = A_RecordExp(EM_tokPos, S_Symbol($1), $3);}
            |  ID LBRACE RBRACE
                {$$ = A_RecordExp(EM_tokPos, S_Symbol($1), NULL);}
            ;
rec     :   rec_one COMMA rec
                {$$ = A_EfieldList($1, $3);}
        |   rec_one
                {$$ = A_EfieldList($1, NULL);}
        ;
rec_one :   ID EQ exp
                {$$ = A_Efield(S_Symbol($1), $3);};


arrayexp:   ID LBRACK exp RBRACK OF exp
                {$$ = A_ArrayExp(EM_tokPos, S_Symbol($1), $3, $6);};

assignexp:  lvalue ASSIGN exp
                {$$ = A_AssignExp(EM_tokPos, $1, $3);}
         /*|  lvalue ASSIGN arrayexp  stop hacking */
         ;

ifexp   :   IF exp THEN exp ELSE exp
                {$$ = A_IfExp(EM_tokPos, $2, $4, $6);}
        |   IF exp THEN exp
                {$$ = A_IfExp(EM_tokPos, $2, $4, NULL);}
        ;

whileexp:   WHILE exp DO exp
                {$$ = A_WhileExp(EM_tokPos, $2, $4);};

forexp  :   FOR ID ASSIGN exp TO exp DO exp
                {$$ = A_ForExp(EM_tokPos, S_Symbol($2), $4, $6, $8);};

letexp  :   LET decs IN expseq END
                {$$ = A_LetExp(EM_tokPos, $2, A_SeqExp(EM_tokPos, $4));};

decs    :   dec decs
                {$$ = A_DecList($1, $2);}
        |
                {$$ = NULL;}    // it that right?
        ;
dec     :   vardec
                {$$ = $1;}
        |   fundecs
                {$$ = A_FunctionDec(EM_tokPos, $1);}
        |   tydecs
                {$$ = A_TypeDec(EM_tokPos, $1);}
        ;

vardec  :   VAR ID ASSIGN exp
                {$$ = A_VarDec(EM_tokPos, S_Symbol($2), S_Symbol(""), $4);}
        /*|   VAR ID ASSIGN arrayexp  stop hacking */
        |   VAR ID COLON ID ASSIGN exp
                {$$ = A_VarDec(EM_tokPos, S_Symbol($2), S_Symbol($4), $6);}
        /*|   VAR ID COLON ID ASSIGN arrayexp stop hacking */

        ;

fundecs :   fundec fundecs
                {$$ = A_FundecList($1, $2);}
        |   fundec
                {$$ = A_FundecList($1, NULL);}
        ;
fundec  :   FUNCTION ID LPAREN tyfields RPAREN EQ exp
                {$$ = A_Fundec(EM_tokPos, S_Symbol($2), $4, S_Symbol(""), $7);}
        |   FUNCTION ID LPAREN tyfields RPAREN COLON ID EQ exp
                {$$ = A_Fundec(EM_tokPos, S_Symbol($2), $4, S_Symbol($7), $9);}
        ;

tydecs  :   tydec tydecs
                {$$ = A_NametyList($1, $2);}
        |   tydec
                {$$ = A_NametyList($1, NULL);}
        ;
tydec   :   TYPE ID EQ ty
                {$$ = A_Namety(S_Symbol($2), $4);};
ty      :   ID
                {$$ = A_NameTy(EM_tokPos, S_Symbol($1));}
        |   LBRACE tyfields RBRACE
                {$$ = A_RecordTy(EM_tokPos, $2);}
        |   ARRAY OF ID
                {$$ = A_ArrayTy(EM_tokPos, S_Symbol($3));}
        ;
tyfields:
            {$$ = NULL;}
        |   tyfields_nonempty
            {$$ = $1;}
        ;
tyfields_nonempty   :   field COMMA tyfields_nonempty
                            {$$ = A_FieldList($1, $3);}
                    |   field
                            {$$ = A_FieldList($1, NULL);}
                    ;
field   :   ID COLON ID
                {$$ = A_Field(EM_tokPos, S_Symbol($1), S_Symbol($3));};
