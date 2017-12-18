/*
 * temp.c - functions to create and manipulate temporary variables which are
 *          used in the IR tree representation before it has been determined
 *          which variables are to go into registers.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"
#include "symbol.h"
#include "temp.h"
#include "table.h"

struct Temp_temp_ {int num;};

int Temp_int(Temp_temp t)
{
	return t->num;
}

string Temp_labelstring(Temp_label s)
{return S_name(s);
}

static int labels = 0;

Temp_label Temp_newlabel(void)
{char buf[100];
 sprintf(buf,"L%d",labels++);
 return Temp_namedlabel(String(buf));
}

/* The label will be created only if it is not found. */
Temp_label Temp_namedlabel(string s)
{return S_Symbol(s);
}

static int temps = 100;

Temp_temp Temp_newtemp(void)
{Temp_temp p = (Temp_temp) checked_malloc(sizeof (*p));
 p->num=temps++;
 {char r[16];
  sprintf(r, "%d", p->num);
  Temp_enter(Temp_name(), p, String(r));
 }
 return p;
}



struct Temp_map_ {TAB_table tab; Temp_map under;};


Temp_map Temp_name(void) {
 static Temp_map m = NULL;
 if (!m) m=Temp_empty();
 return m;
}

Temp_map newMap(TAB_table tab, Temp_map under) {
  Temp_map m = checked_malloc(sizeof(*m));
  m->tab=tab;
  m->under=under;
  return m;
}

Temp_map Temp_empty(void) {
  return newMap(TAB_empty(), NULL);
}

Temp_map Temp_layerMap(Temp_map over, Temp_map under) {
  if (over==NULL)
      return under;
  else return newMap(over->tab, Temp_layerMap(over->under, under));
}

void Temp_enter(Temp_map m, Temp_temp t, string s) {
  assert(m && m->tab);
  TAB_enter(m->tab,t,s);
}

string Temp_look(Temp_map m, Temp_temp t) {
  string s;
  assert(m && m->tab);
  s = TAB_look(m->tab, t);
  if (s) return s;
  else if (m->under) return Temp_look(m->under, t);
  else return NULL;
}

Temp_tempList Temp_TempList(Temp_temp h, Temp_tempList t)
{Temp_tempList p = (Temp_tempList) checked_malloc(sizeof (*p));
 p->head=h; p->tail=t;
 return p;
}

Temp_labelList Temp_LabelList(Temp_label h, Temp_labelList t)
{Temp_labelList p = (Temp_labelList) checked_malloc(sizeof (*p));
 p->head=h; p->tail=t;
 return p;
}

static FILE *outfile;
void showit(Temp_temp t, string r) {
  fprintf(outfile, "t%d -> %s\n", t->num, r);
}

void Temp_dumpMap(FILE *out, Temp_map m) {
  outfile=out;
  TAB_dump(m->tab,(void (*)(void *, void*))showit);
  if (m->under) {
     fprintf(out,"---------\n");
     Temp_dumpMap(out,m->under);
  }
}

bool Temp_tempInList(Temp_tempList tempList, Temp_temp temp) {
    Temp_tempList tmp = tempList;
    while (tmp && tmp->head) {
        if (temp == tmp->head)
            return TRUE;
        tmp = tmp->tail;
    }
    return FALSE;
}
// A+B
Temp_tempList Temp_unionList(Temp_tempList a, Temp_tempList b) {
    Temp_tempList res = NULL;
    Temp_tempList tmp = a;
    while (tmp && tmp->head) {
        res = Temp_TempList(tmp->head, res);
        tmp = tmp->tail;
    }
    tmp = b;
    while (tmp && tmp->head) {
        if (!Temp_tempInList(res, tmp->head))
            res = Temp_TempList(tmp->head, res);
        tmp = tmp->tail;
    }
    return res;
}

// A-B
Temp_tempList Temp_diffList(Temp_tempList a, Temp_tempList b) {
    Temp_tempList res = NULL;
    Temp_tempList tmp = a;
    while (tmp && tmp->head) {
        if (!Temp_tempInList(b, tmp->head))
            res = Temp_TempList(tmp->head, res);
        tmp = tmp->tail;
    }
    return res;
}

// reverse list order
Temp_tempList Temp_reverseList(Temp_tempList a) {
    Temp_tempList res = NULL;
    Temp_tempList tmp = a;
    while (tmp && tmp->head) {
        res = Temp_TempList(tmp->head, res);
        tmp = tmp->tail;
    }
    return res;
}

bool Temp_listEqual(Temp_tempList a, Temp_tempList b) {
    Temp_tempList tmp = a;
    while (tmp && tmp->head) {
        if (!Temp_tempInList(b, tmp->head))
            return FALSE;
        tmp = tmp->tail;
    }
    tmp = b;
    while (tmp && tmp->head) {
        if (!Temp_tempInList(a, tmp->head))
            return FALSE;
        tmp = tmp->tail;
    }
    return TRUE;
}

void Temp_findAndReplace(Temp_tempList* tempList,
    Temp_temp oldtemp, Temp_temp newtemp) {
    Temp_tempList temps = *tempList;
    Temp_tempList last = NULL;
    while (temps && temps->head) {
        if (temps->head == oldtemp) {
            if (!last) {
                *tempList = Temp_TempList(newtemp, temps->tail);
                last = *tempList;
            }
            else {
                last->tail = Temp_TempList(newtemp, temps->tail);
                last = last->tail;
            }
        } else
            last = temps;
        temps = temps->tail;
    }
}
