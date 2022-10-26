#include <stdio.h>
#include "util.h"
#include "symbol.h"
#include "types.h"
#include "env.h"
#include "translate.h"

E_enventry E_VarEntry(Tr_access access,Ty_ty ty){
    E_enventry p = checked_malloc(sizeof(*p));
    p->kind = E_varEntry;
    p->u.var.access = access;
    p->u.var.ty = ty;
    return p;
}

E_enventry E_FunEntry(Tr_level level, Temp_label label, Ty_tyList formals, Ty_ty result){
    E_enventry p = checked_malloc(sizeof(*p));
    p->kind = E_FunEntry;
    p->u.fun.level = level;
    p->u.fun.label = label;
    p->u.fun.formals = formals;
    p->u.fun.result = result;
    return p;
}

S_table E_base_tenv(void){
    S_table t = TAB_empty();
    S_symbol s1 =  S_Symbol("int");
    S_symbol s2 = S_Symbol("string");
    Ty_ty ty1 = Ty_Int();
    Ty_ty ty2 = Ty_String();
    S_enter(t, s1, ty1);
    S_enter(t, s2, ty2);
    return t;
}

S_table E_base_venv() {
	S_table t = S_empty();
	S_enter(t, S_Symbol("print"), E_FunEntry(Tr_outermost(), Temp_newlabel(), Ty_TyList(Ty_String(), NULL), Ty_Void()));
	S_enter(t, S_Symbol("flush"), E_FunEntry(Tr_outermost(), Temp_newlabel(), NULL, Ty_Void()));
	S_enter(t, S_Symbol("getchar"), E_FunEntry(Tr_outermost(), Temp_newlabel(), NULL, Ty_String()));
	S_enter(t, S_Symbol("ord"), E_FunEntry(Tr_outermost(), Temp_newlabel(), Ty_TyList(Ty_String(), NULL), Ty_Int()));
	S_enter(t, S_Symbol("chr"), E_FunEntry(Tr_outermost(), Temp_newlabel(), Ty_TyList(Ty_Int(), NULL), Ty_String()));
	S_enter(t, S_Symbol("size"), E_FunEntry(Tr_outermost(), Temp_newlabel(), Ty_TyList(Ty_String(), NULL), Ty_Int()));
	S_enter(t, S_Symbol("substring"), E_FunEntry(Tr_outermost(), Temp_newlabel(), Ty_TyList(Ty_String(), Ty_TyList(Ty_Int(), Ty_TyList(Ty_Int(), NULL))), Ty_String()));
	S_enter(t, S_Symbol("concat"), E_FunEntry(Tr_outermost(), Temp_newlabel(), Ty_TyList(Ty_String(), Ty_TyList(Ty_String(), NULL)), Ty_String()));
	S_enter(t, S_Symbol("not"), E_FunEntry(Tr_outermost(), Temp_newlabel(), Ty_TyList(Ty_Int(), NULL), Ty_Int()));
	S_enter(t, S_Symbol("exit"), E_FunEntry(Tr_outermost(), Temp_newlabel(), Ty_TyList(Ty_Int(), NULL), Ty_Void()));
	return t;
}