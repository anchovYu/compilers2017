%{
/* Lab2 Attention: You are only allowed to add code in this file and start at Line 26.*/
#include <string.h>
#include "util.h"
#include "symbol.h"
#include "errormsg.h"
#include "absyn.h"
#include "y.tab.h"

int charPos = 1;

int yywrap(void) {
    charPos = 1;
    return 1;
}

void adjust(void) {
    EM_tokPos = charPos;
    charPos += yyleng;
}

/*
* Please don't modify the lines above.
* You can add C declarations of your own below.
*/

/* @function: getstr
 * @input: a string literal
 * @output: the string value for the input which has all the escape sequences
 * translated into their meaning.
 */
char *getstr(const char *str) {
	// not implemented
    return NULL;
}

/* variable for string storage.
 * @parameter max length
 */
char stringBuf[1024];
int stringPos = 0;

int commentLayer = 0; /* TODO: change a name */

%}
/* lex definitions */

/* digit RE */
digits [0-9]+
/* comment and string state */
%Start COMMENT STRINGEXP

%%
    /* initial states, basic symbols */
<INITIAL>","        {adjust(); return COMMA;}
<INITIAL>":"        {adjust(); return COLON;}
<INITIAL>";"        {adjust(); return SEMICOLON;}
<INITIAL>"("        {adjust(); return LPAREN;}
<INITIAL>")"        {adjust(); return RPAREN;}
<INITIAL>"["        {adjust(); return LBRACK;}
<INITIAL>"]"        {adjust(); return RBRACK;}
<INITIAL>"{"        {adjust(); return LBRACE;}
<INITIAL>"}"        {adjust(); return RBRACE;}
<INITIAL>"."        {adjust(); return DOT;}
<INITIAL>"+"        {adjust(); return PLUS;}
<INITIAL>"-"        {adjust(); return MINUS;}
<INITIAL>"*"        {adjust(); return TIMES;}
<INITIAL>"/"        {adjust(); return DIVIDE;}
<INITIAL>"="        {adjust(); return EQ;}
<INITIAL>"<>"       {adjust(); return NEQ;}
<INITIAL>"<"        {adjust(); return LT;}
<INITIAL>"<="       {adjust(); return LE;}
<INITIAL>">"        {adjust(); return GT;}
<INITIAL>">="       {adjust(); return GE;}
<INITIAL>"&"        {adjust(); return AND;}
<INITIAL>"|"        {adjust(); return OR;}
<INITIAL>":="       {adjust(); return ASSIGN;}

    /* initial states, reserved words */
<INITIAL>array      {adjust(); return ARRAY;}
<INITIAL>if         {adjust(); return IF;}
<INITIAL>then       {adjust(); return THEN;}
<INITIAL>else       {adjust(); return ELSE;}
<INITIAL>while      {adjust(); return WHILE;}
<INITIAL>for        {adjust(); return FOR;}
<INITIAL>to         {adjust(); return TO;}
<INITIAL>do         {adjust(); return DO;}
<INITIAL>let        {adjust(); return LET;}
<INITIAL>in         {adjust(); return IN;}
<INITIAL>end        {adjust(); return END;}
<INITIAL>of         {adjust(); return OF;}
<INITIAL>break      {adjust(); return BREAK;}
<INITIAL>nil        {adjust(); return NIL;}
<INITIAL>function   {adjust(); return FUNCTION;}
<INITIAL>var        {adjust(); return VAR;}
<INITIAL>type       {adjust(); return TYPE;}

    /* initial states, variables */
<INITIAL>[a-zA-Z][a-zA-Z0-9"_"]*    {adjust(); yylval.sval=String(yytext); return ID;}

    /* initial states, int */
<INITIAL>{digits}   {adjust(); yylval.ival=atoi(yytext); return INT;}

    /* initial states, comment */
<INITIAL>"/*"       {adjust(); commentLayer ++; BEGIN COMMENT;}

    /* initial states, string */
<INITIAL>"\""       {adjust(); BEGIN STRINGEXP;}

    /* initial states, space */
<INITIAL>(" "|"\t")+ {adjust();}
<INITIAL>"\n"       {adjust(); EM_newline(); continue;}

    /* initial states, error */
<INITIAL>.          {adjust(); EM_error(EM_tokPos, "illegal character");}


    /* comment states, end comment */
<COMMENT>"*/"       {adjust(); commentLayer --; if (commentLayer == 0) BEGIN INITIAL;}
    /* comment states, nested comment */
<COMMENT>"/*"       {adjust(); commentLayer ++;}
<COMMENT>"\n"       {adjust(); EM_newline();}
    /* comment states, ignore comment */
<COMMENT>.          {adjust();}


    /* string states, end string */
<STRINGEXP>"\""     {charPos+=yyleng;
                     stringBuf[stringPos] = '\0';
                     yylval.sval = String(stringBuf);
                     stringPos = 0;
                     BEGIN INITIAL;
                     return STRING;}
    /* string states, special char */
    /* TODO: add more. */
<STRINGEXP>"\\n"    {charPos+=yyleng; stringBuf[stringPos++] = '\n';}
<STRINGEXP>"\\t"    {charPos+=yyleng; stringBuf[stringPos++] = '\t';}
    /* string states, other char */
<STRINGEXP>.        {charPos+=yyleng; stringBuf[stringPos++] = yytext[0];}


"\n"                {adjust(); EM_newline(); continue;}
.                   {BEGIN INITIAL; yyless(1);}
