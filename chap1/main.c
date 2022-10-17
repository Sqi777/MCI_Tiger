#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "util.h"
#include "prog1.h"

typedef struct table * Table_;
struct  table {string id; int value; Table_ tail;}; // environment
Table_ Table(string id, int value, struct table *tail); // environment constructor
struct IntAndTable {int i; Table_ t;}; // return value of interExp

int lookup(Table_ t, string key); // find option of environment
Table_ update(Table_ t, string key, int i); // linker optional function

struct IntAndTable interExp(A_exp s,Table_ t);  // recursive function
Table_ interStm(A_stm s,Table_ t);  // recursive function
struct IntAndTable interExpList(A_expList s, Table_ t);


int main(){
    A_stm stm = prog();
    Table_ t = NULL;
    t = interStm(stm, t);
   
    return 0;
}

struct IntAndTable interExp(A_exp s, Table_ t){
    struct IntAndTable intandtable;
    // 4 cases of kind of A_exp
    if (s->kind == A_numExp){
        intandtable.i = s->u.num;
        intandtable.t = t;
        return intandtable;
    } else if (s->kind == A_opExp){
        if(s->u.op.oper == A_plus){
            intandtable.i = interExp(s->u.op.left, t).i + interExp(s->u.op.right, t).i;
            intandtable.t = t;
            return intandtable;
        }else if(s->u.op.oper == A_minus){
            intandtable.i = interExp(s->u.op.left, t).i - interExp(s->u.op.right, t).i;
            intandtable.t = t;
            return intandtable;
        }else if(s->u.op.oper == A_times){
            intandtable.i = interExp(s->u.op.left, t).i * interExp(s->u.op.right, t).i;
            intandtable.t = t;
            return intandtable;
        }else {
            intandtable.i = interExp(s->u.op.left, t).i/ interExp(s->u.op.right, t).i;
            intandtable.t = t;
            return intandtable;
        }

    } else if (s->kind == A_idExp){
        intandtable.i  = lookup(t, s->u.id);
        intandtable.t = t;
        return intandtable;
    }else {
        intandtable.i = interExp(s->u.eseq.exp, t).i;
        intandtable.t = interStm(s->u.eseq.stm, t);
        return intandtable;
    }
   
}
Table_ interStm(A_stm s,Table_ t){
    if(s->kind ==  A_assignStm){
        t = update(t, s->u.assign.id, interExp(s->u.assign.exp,t).i);
        return t;
    }else if(s->kind == A_compoundStm){
        t = interStm(s->u.compound.stm1, t);
        t = interStm(s->u.compound.stm2, t);
        return t;
    }else {
        t = interExpList(s->u.print.exps, t).t;
        return t;
    }
}

Table_ Table(string id, int value, struct table *tail){
    Table_ t = malloc(sizeof(*t));
    t->id = id; t->value = value; t->tail = tail;
    return t;
}

int lookup(Table_ t, string key){
    Table_ new = t;
    if(new->id != key && new->tail == NULL){
        return 0;
    }else if(new->id != key && new->tail != NULL){
        return lookup(new->tail, key);
    }else {
        return new->value;
    }
}
Table_ update(Table_ t, string key, int i){
    Table_ new = Table(key, i, t);
    return new;
}

struct IntAndTable interExpList(A_expList s, Table_ t){
    struct IntAndTable intandtable;
    if(s->kind == A_lastExpList){
        intandtable = interExp(s->u.last, t);
        printf("%d\n",intandtable.i);
        return intandtable;
    } else {
        intandtable = interExp(s->u.pair.head, t);
        printf("%d  ", intandtable.i);
        intandtable = interExpList(s->u.pair.tail, t);
        return intandtable;
    }
}
