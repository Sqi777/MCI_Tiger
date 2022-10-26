#include <stdio.h>
#include <stdlib.h>
#include "errormsg.h"
#include "util.h"
#include "symbol.h"
#include "types.h"
#include "absyn.h"
#include "env.h"
#include "translate.h"
#include "semant.h"

typedef void *Tr_exp;
struct expty {Tr_exp exp; Ty_ty ty};

struct expty expTy(Tr_exp exp, Ty_ty ty){
    struct expty e;
    e.exp = exp;
    e.ty = ty;
    return e;
}

static struct expty transVar(Tr_level level, S_table venv, S_table tenv, A_var v);
static struct expty transExp(Tr_level level, S_table venv, S_table tenv, A_exp a);
static void transDec(Tr_level level, S_table venv, S_table tenv, A_dec d);
static Ty_ty transTy(S_table tenv, A_ty a);

static Ty_ty actual_ty(Ty_ty t); 
static bool cmp_ty(Ty_ty a, Ty_ty b);

static Ty_fieldList transFieldList(S_table tenv, A_fieldList a_fl);
static Ty_tyList makeFormalTyList(S_table tenv, A_fieldList a_fl); 

static U_boolList makeFormalList(A_fieldList params);

// we need a set like which in python
struct set_ {
	S_symbol symbol[100];
	int pos;
};
typedef struct set_ *set;

static set setInit(void);
static int setPut(set s, S_symbol x);
static void setRset(set s);
static void setFree(set s);

//nested layers of loop
static int loop = 0;

struct expty transVar(Tr_level level, S_table venv, S_table tenv, A_var v){
    switch (v->kind){
        case A_simpleVar:{
            E_enventry x = S_look(venv, v->u.simple);
            if(x && x->kind == E_varEntry)
                return expTy(NULL, actual_ty(x->u.var.ty));
            else {
				EM_error(v->pos, "undefined variable '%s'", S_name(v->u.simple));
				return expTy(NULL, Ty_Int());
			}
        }
        case A_fieldVar: {
			struct expty exp = transVar(level, venv, tenv, v->u.field.var); // recursion from here
			Ty_ty t = exp.ty;
			if (t->kind != Ty_record) {
				EM_error(v->pos, "variable not record");
				return expTy(NULL, Ty_Int());
			} else {
				Ty_fieldList fl = t->u.record;
				for (; fl; fl = fl->tail)
					if (fl->head->name == v->u.field.sym) 
						return expTy(NULL, actual_ty(fl->head->ty));
				EM_error(v->pos, "'%s' was not a member", S_name(v->u.field.sym));
				return expTy(NULL, Ty_Int());
			}
        }  
        case A_subscriptVar:{
            struct expty var = transVar(level, venv, tenv, v->u.subscript.var); 
            struct expty exp = transExp(level, venv, tenv, v->u.subscript.exp); 
            if(var.ty->kind != Ty_array){
                EM_error(v->pos, "variable not array");
				return expTy(NULL, Ty_Int());
            } 
            if(exp.ty->kind != Ty_int)
                EM_error(v->pos, "Subscript was not an integer");
            return expTy(NULL, var.ty->u.array);
        }
    }
}           
struct expty transExp(Tr_level level, S_table venv, S_table tenv, A_exp a){
    switch(a->kind){
        case A_intExp:{
            return expTy(NULL, Ty_Int());
        }
        case A_varExp:{
            struct expty exp;
            A_var var = a->u.var;
            exp = transVar(level, venv, tenv, var);
            return exp;
        }
        case A_stringExp:{
            return expTy(NULL, Ty_String());
        }
        case A_nilExp:{
            return expTy(NULL, Ty_Nil());
        }
        case A_ifExp: {
			struct expty t = transExp(level, venv, tenv, a->u.iff.test);	
			struct expty h = transExp(level, venv, tenv, a->u.iff.then);	
			if (t.ty->kind != Ty_int)
				EM_error(a->pos, "if-exp was not an integer");
			if (a->u.iff.elsee) {
				struct expty e = transExp(level, venv, tenv, a->u.iff.elsee);
				if (!cmp_ty(h.ty, e.ty) && !(h.ty->kind == Ty_nil && e.ty->kind == Ty_nil))
					EM_error(a->pos, "then-else had different types");
				return expTy(NULL, h.ty);
			} else {
				if (h.ty->kind != Ty_void)
					EM_error(a->pos, "if-then shouldn't return a value");
				return expTy(NULL, Ty_Void());
			}
		}
        case A_letExp:{
            struct expty exp;
            A_decList d;
            S_beginScope(venv);
            S_beginScope(tenv);
            for(d = a->u.let.decs; d; d = d->tail)
                transDec(level, venv, tenv, d->head);
            exp = transExp(level, venv, tenv, a->u.let.body);    
            S_endScope(tenv);
            S_endScope(venv);    
            return exp;
        }
        case A_opExp:{
            A_oper oper = a->u.op.oper;
            struct expty left = transExp(level, venv, tenv, a->u.op.left);
            struct expty right = transExp(level, venv, tenv, a->u.op.right);
            switch (oper) {
				case A_plusOp:
				case A_minusOp:
				case A_timesOp:
				case A_divideOp: {
					if (left.ty->kind != Ty_int) 
						EM_error(a->u.op.left->pos, "integer required");
					if (right.ty->kind != Ty_int) 
						EM_error(a->u.op.right->pos, "integer required");
					return expTy(NULL, Ty_Int());
				} 
				case A_eqOp:
				case A_neqOp: {
					if (left.ty->kind == Ty_void) 
						EM_error(a->u.op.left->pos, "expression had no value");	
					else if (right.ty->kind == Ty_void) 
						EM_error(a->u.op.right->pos, "expression had no value");	
					else if (!cmp_ty(left.ty, right.ty))
						EM_error(a->u.op.right->pos, "comparison type mismatch");
					return expTy(NULL, Ty_Int());
				}
				case A_ltOp:
				case A_leOp:
				case A_gtOp:
				case A_geOp: {
					if (left.ty->kind != Ty_int && left.ty->kind != Ty_string) 
						EM_error(a->u.op.left->pos, "string or integer required");
					else if (right.ty->kind != left.ty->kind) 
						EM_error(a->u.op.right->pos, "comparison type mismatch");
					return expTy(NULL, Ty_Int());
				}
			} 
		}
        case A_callExp:{
            E_enventry env = S_look(tenv, a->u.call.func);
            // check if the function is declared
            if(!env){
                EM_error(a->pos, "undeclared function '%s",S_name(a->u.call.func));
                return expTy(NULL, Ty_Int());
            } else if (env->kind == E_varEntry) {
				EM_error(a->pos, "'%s' was a variable", S_name(a->u.call.func));
				return expTy(NULL, env->u.var.ty);
			}   
            Ty_tyList formals = env->u.fun.formals;
            A_expList el = a->u.call.args;
            // a loop check out all parameters
            for(; el && formals; el = el->tail, formals = formals->tail){
                struct expty exp = transExp(level, venv, tenv, el->head);
                if(!cmp_ty(formals->head, exp.ty)){
                    EM_error(a->pos, "argument type mismatch");
                }
            } 
            if(formals)
                EM_error(a->pos, "too few arguments");
			else if (el)
				EM_error(a->pos, "too many arguments");

            return expTy(NULL, env->u.fun.result); 
        }
        case A_arrayExp: {
			Ty_ty t = actual_ty(S_look(tenv, a->u.array.typ));
			if (!t) {
				EM_error(a->pos, "undefined type '%s'", S_name(a->u.array.typ));
				return expTy(NULL, Ty_Int());
			} else if (t->kind != Ty_array) {
				EM_error(a->pos, "'%s' was not a array type", S_name(a->u.array.typ));
				return expTy(NULL, Ty_Int());
			}
			struct expty z = transExp(level, venv, tenv, a->u.array.size);	
			struct expty i = transExp(level, venv, tenv, a->u.array.init);	
			if (z.ty->kind != Ty_int)
				EM_error(a->pos, "array size was not an integer value");
			if (!cmp_ty(i.ty, t->u.array)) // like something wrong here
				EM_error(a->pos, "array init type mismatch");
			return expTy(NULL, t);
		}
        case A_recordExp: {
			Ty_ty t = actual_ty(S_look(tenv, a->u.record.typ));
			if (!t) {
				EM_error(a->pos, "undefined type '%s'", S_name(a->u.record.typ));
				return expTy(NULL, Ty_Int());
			} else if (t->kind != Ty_record) {
				EM_error(a->pos, "'%s' was not a record type", S_name(a->u.record.typ));
				return expTy(NULL, t);
			}

			Ty_fieldList ti = t->u.record;
			A_efieldList ei = a->u.record.fields;
			for (; ti && ei; ti = ti->tail, ei = ei->tail) {
				if (ti->head->name != ei->head->name) {
					EM_error(a->pos, "need member '%s' but '%s'", S_name(ti->head->name), S_name(ei->head->name));
					continue;
				}
				struct expty exp = transExp(level, venv, tenv, ei->head->exp);
				if (!cmp_ty(ti->head->ty, exp.ty))
					EM_error(a->pos, "member '%s' type mismatch", S_name(ti->head->name));
			}

			if (ti) 
				EM_error(a->pos, "too few initializers for '%s'", S_name(a->u.record.typ));
			else if (ei)
				EM_error(a->pos, "too many initializers for '%s'", S_name(a->u.record.typ));

			return expTy(NULL, t);
		}
        case A_whileExp:{
            struct expty t = transExp(level, venv, tenv, a->u.whilee.test);
            loop++;
            struct expty b = transExp(level, venv, tenv, a->u.whilee.body);
            loop--;
            if (t.ty->kind != Ty_int)
				EM_error(a->pos, "while-exp was not an integer");
			if (b.ty->kind != Ty_void)
				EM_error(a->pos, "do-exp shouldn't return a value");
			return expTy(NULL, Ty_Void());
        }
        case A_breakExp: {
			if (!loop)
				EM_error(a->pos, "break statement not within loop");
			return expTy(NULL, Ty_Void());
		}
        case A_forExp:{
            struct expty l = transExp(level, venv, tenv, a->u.forr.lo);
			struct expty h = transExp(level, venv, tenv, a->u.forr.hi);
            if (l.ty->kind != Ty_int)
				EM_error(a->pos, "low bound was not an integer");
			if (l.ty->kind != Ty_int)
				EM_error(a->pos, "high bound was not an integer");
            S_beginScope(venv);
			loop++;
            Tr_access ac = Tr_allocLocal(level, a->u.forr.escape);
            S_enter(venv, a->u.forr.var, E_VarEntry(ac, Ty_Int()));
            struct expty b = transExp(level, venv, tenv, a->u.forr.body);
			S_endScope(venv);
			loop--;

			if (b.ty->kind != Ty_void)
				EM_error(a->pos, "body exp shouldn't return a value");
			return expTy(NULL, Ty_Void());
            // is there something wrong when you check out a linker ?
        }
    }
}
void transDec(Tr_level level, S_table venv, S_table tenv, A_dec d){
    switch(d->kind){
        case A_varDec:{
            struct expty e = transExp(level, venv, tenv, d->u.var.init);
            if(d->u.var.typ){
                Ty_ty t = S_look(tenv, d->u.var.typ); //kind of return different from S_look(venv,key)
                if (!t)
					EM_error(d->pos, "undefined type '%s'", S_name(d->u.var.typ));
				else {
					if (!cmp_ty(t, e.ty)) 
						EM_error(d->pos, "var init type mismatch");
                    Tr_access ac = Tr_allocLocal(level, d->u.var.escape);
					S_enter(venv, d->u.var.var, E_VarEntry(ac, t));
					break;
				}
            }
            if (e.ty == Ty_Void())
				EM_error(d->pos, "initialize with no value");
			else if (e.ty == Ty_Nil())
				EM_error(d->pos, "'%s' is not a record", S_name(d->u.var.var));
            Tr_access ac = Tr_allocLocal(level, d->u.var.escape);
            S_enter(venv, d->u.var.var, E_VarEntry(ac, e.ty));
            break;
        }
        case A_typeDec:{
            //only one dec in declist
            // S_enter(tenv, d->u.type->head->name, transTy(tenv, d->u.type->head->ty));
            set set = setInit();
            A_nametyList a_nl;
            // put head into tenv
            for(a_nl = d->u.type; a_nl; a_nl = a_nl->tail){
                if (!set_push(set, a_nl->head->name)) {
					EM_error(d->pos, "redefinition of '%s'", S_name(a_nl->head->name));
					continue;
				}
                Ty_ty t = Ty_Name(a_nl->head->name, NULL);
				S_enter(tenv, a_nl->head->name, t);
			}
            setRset(set);
            for(a_nl = d->u.type; a_nl; a_nl = a_nl->tail){
                if(!setPut(set, a_nl->head->name))
                    continue;
                Ty_ty t = S_look(tenv, a_nl->head->name);
				t->u.name.ty = transTy(tenv, a_nl->head->ty);
            }
            //check recursive definition
            for(a_nl = d->u.type; a_nl; a_nl = a_nl->tail){
                setRset(set);
                Ty_ty ty = S_look(tenv, a_nl->head->name);
                Ty_ty t = ty;
                t = t->u.name.ty;
                while(t->kind == Ty_name){
                    if (!setPut(set, t->u.name.sym)) {
						EM_error(d->pos, "illegal recursive definition '%s'", S_name(t->u.name.sym));
						t->u.name.ty = Ty_Int();
                        break;
                    }
                    t = S_look(tenv, t->u.name.sym);
                    t = t->u.name.ty;
                }
                ty->u.name.ty = t;
            }
            setFree(set);        
            break;
        }
        case A_functionDec:{
            set set = setInit();
            A_fundecList a_fdl;
            for(a_fdl = d->u.function; a_fdl; a_fdl = a_fdl->tail){
                if (!set_push(set, a_fdl->head->name)) {
					EM_error(d->pos, "redefinition of '%s'", S_name(a_fdl->head->name));
					continue;
				}
                Ty_ty resultTy = NULL;
                if(a_fdl->head->result){
                    resultTy = S_look(tenv, a_fdl->head->result);
                    if(!resultTy){
                        EM_error(d->pos, "function result: undefined type '%s'", S_name(a_fdl->head->result));
						resultTy = Ty_Int();
                    }
                }else{
                    resultTy = Ty_Void();
                }
                Ty_tyList formalTys = makeFormalTyList(tenv, a_fdl->head->params);
                Temp_label label = Temp_newlabel();
                // Tr_newLevel is an API from which semant modul get comunication with translate module
                Tr_level level_new = Tr_newLevel(level, label, makeFormalList(a_fdl->head->params));
                S_enter(venv, a_fdl->head->name, E_FunEntry(level_new, label, formalTys, resultTy));
            }
            setRset(set);
            for(a_fdl = d->u.function; a_fdl; a_fdl = a_fdl->tail){
                if(!setPut(set, a_fdl->head->name))
                    continue;
                S_beginScope(venv);
                E_enventry f = S_look(venv, a_fdl->head->name);
                Tr_level level_new = f->u.fun.level;
                {A_fieldList l; 
                 Ty_tyList t = f->u.fun.formals; 
                 Tr_accessList acl = Tr_formals(level_new); 
                 acl = acl->tail; //the first is static link
                 for(l = a_fdl->head->params, t, acl; l; l = l->tail,t = t->tail,acl = acl->tail){
                    S_enter(venv, l->head->name, E_VarEntry(acl->head, t->head));
                 }
                } 
                struct expty exp = transExp(level_new, venv, tenv, a_fdl->head->body);   
                if (f->u.fun.result->kind == Ty_void && exp.ty->kind != Ty_void)
					EM_error(a_fdl->head->pos, "procedure '%s' should't return a value", S_name(a_fdl->head->name));
				else if (!cmp_ty(f->u.fun.result, exp.ty))
					EM_error(a_fdl->head->pos, "body result type mismatch");
                S_endScope(venv);
            }
            setFree(set);
            break;
        }
    }
}

Ty_ty transTy(S_table tenv, A_ty a){
	switch (a->kind) {
		case A_nameTy: {
			Ty_ty t = S_look(tenv, a->u.name);
			if (!t) {
				EM_error(a->pos, "undefined type '%s'", S_name(a->u.name));
				return Ty_Int();
			} else
				return Ty_Name(a->u.name, t->u.name.ty);
		}
		case A_recordTy:
			return Ty_Record(transFieldList(tenv, a->u.record));
		case A_arrayTy: {
			Ty_ty t = S_look(tenv, a->u.array);
			if (!t) {
				EM_error(a->pos, "undefined type '%s'", S_name(a->u.array));
				return Ty_Array(Ty_Int());
			} else
				return Ty_Array(S_look(tenv, a->u.array));
		}
	}
	assert(0);
}
 
U_boolList makeFormalList(A_fieldList params) {
	U_boolList head = NULL, tail = NULL;
	while (params) {
		if (head) {
			tail->tail = U_BoolList(params->head->escape, NULL);
			tail = tail->tail;
		} else {
			head = U_BoolList(params->head->escape, NULL);
			tail = head;
		}
		params = params->tail;
	}
	return head;
}

Ty_fieldList transFieldList(S_table tenv, A_fieldList a_fl){
    if(!a_fl)
        return NULL;
    A_field a_f = a_fl->head;
    S_symbol name = a_f->name;
    S_symbol typ = a_f->typ;
    Ty_ty t = S_look(tenv, typ);
    Ty_field t_f = Ty_Field(name, t);
    if(!t){
        EM_error(a_fl->head->pos, "undefined type '%s'", S_name(typ));
		t = Ty_Int();
    }
    if(!a_fl->tail){
        Ty_fieldList t_fl = Ty_FieldList(t_f, NULL);
        return t_fl;
    } else {
        Ty_fieldList t_fl = Ty_FieldList(t_f ,transFieldList(tenv, a_fl->tail));
        return t_fl;
    }
}

Ty_tyList makeFormalTyList(S_table tenv, A_fieldList a_fl){
    if(!a_fl)
        return NULL;
    A_field a_f = a_fl->head;
    S_symbol name = a_f->name;
    S_symbol typ = a_f->typ;
    Ty_ty t = S_look(tenv, typ);
    if(!t){
        EM_error(a_fl->head->pos, "undefined type '%s'", S_name(typ));
		t = Ty_Int();
    }
    if(!a_fl->tail){
        Ty_tyList t_tl = Ty_TyList(t, NULL);
        return t_tl;
    } else {
        Ty_tyList t_tl = Ty_TyList(t ,transFieldList(tenv, a_fl->tail));
        return t_tl;
    }
}

set setInit(void){
    set set = checked_malloc(sizeof(*set)); // I don't known if the pointer is 悬空
    set->pos = 0;
    return set;
}

int setPut(set s, S_symbol x){
    int i;
    for(i = 0; i < s->pos; i++){
        if(s->symbol[i] == x)
            return FALSE;         
    }
    s->symbol[s->pos] = x;
    s->pos++;
    return TRUE;
}

void setRset(set s){
    s->pos = 0;
}

void setFree(set s){
    if(!s)
        free(s);
}