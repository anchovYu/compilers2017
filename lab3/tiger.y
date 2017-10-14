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

%nonassoc THEN ELSE DO OF   /* hacking */
%nonassoc ASSIGN
%left OR
%left AND
%nonassoc EQ NEQ LT LE GT GE
%left PLUS MINUS
%left TIMES DIVIDE
%left UMINUS

%start program

%%

program :   exp
        ;

exp     :   LPAREN expseq RPAREN
        |   lvalue
        |   NIL
        |   INT
        |   STRING
        |   callexp
        |   opexp
        |   recordexp
        |   assignexp
        |   ifexp
        |   whileexp
        |   forexp
        |   BREAK
        |   letexp
        ;

/* Difference between expseq and explist in nonterminal:
 * (exp;exp) => expseq
 * let decs in exp;exp end => expseq
 * func(exp,exp) => explist
 */
expseq  :   exp SEMICOLON expseq
        |   exp
        ;

explist :   exp COMMA explist
        |   exp
        ;

lvalue  :   ID
        |   fieldvar
        |   subscriptvar
        ;

fieldvar     :  lvalue DOT ID;
subscriptvar :  lvalue LBRACK exp RBRACK;

callexp :   ID LPAREN RPAREN
        |   ID LPAREN explist RPAREN
        ;

opexp   :   exp PLUS exp
        |   exp MINUS exp
        |   exp TIMES exp
        |   exp DIVIDE exp
        |   exp EQ exp
        |   exp NEQ exp
        |   exp LT exp
        |   exp LE exp
        |   exp GT exp
        |   exp GE exp
        |   exp AND exp
        |   exp OR exp
        ;

recordexp   :  ID LBRACE rec RBRACE
            |  ID LBRACE RBRACE
            ;
rec     :   rec_one COMMA rec
        |   rec_one
        ;
rec_one :   ID EQ exp;

arrayexp:   ID LBRACK exp RBRACK OF exp;

assignexp:  lvalue ASSIGN exp
         |  lvalue ASSIGN arrayexp  /* hack */
         ;

ifexp   :   IF exp THEN exp ELSE exp
        |   IF exp THEN exp
        ;

whileexp:   WHILE exp DO exp;

forexp  :   FOR ID ASSIGN exp TO exp DO exp;

letexp  :   LET decs IN expseq END;

decs    :   dec decs
        |
        ;
dec     :   vardec
        |   fundecs
        |   tydecs
        ;

vardec  :   VAR ID ASSIGN exp
        |   VAR ID COLON ID ASSIGN exp
        ;

fundecs :   fundec fundecs
        |   fundec
        ;
fundec  :   FUNCTION ID LPAREN tyfields RPAREN EQ exp
        |   FUNCTION ID LPAREN tyfields RPAREN COLON ID EQ exp
        ;

tydecs  :   tydec tydecs
        |   tydec
        ;
tydec   :   TYPE ID EQ ty;
ty      :   ID
        |   LBRACE tyfields RBRACE
        |   ARRAY OF ID
        ;
tyfields:
        |   tyfields_nonempty
        ;
tyfields_nonempty   :   field COMMA tyfields_nonempty
                    |   field
                    ;
field   :   ID COLON ID;
