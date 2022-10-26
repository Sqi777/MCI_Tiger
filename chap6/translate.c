#include <stdio.h>
#include <stdlib.h>
#include "translate.h"
#include "frame.h"

struct Tr_level_{
    Tr_level parent;
    Temp_label name;
    F_frame frame;
    Tr_accessList formals;
};

struct Tr_access_ {
    Tr_level level;
    F_access access; //just formals access information ,not include locals access information
};

static Tr_access Tr_Access(Tr_level level, F_access access) {
	Tr_access a = checked_malloc(sizeof(*a));
	a->level = level;
	a->access = access;
	return a;
}

static Tr_accessList Tr_AccessList(Tr_access head, Tr_accessList tail) {
	Tr_accessList l = checked_malloc(sizeof(*l));
	l->head = head;
	l->tail = tail;
	return l; 
}

static Tr_accessList makeFormalList(Tr_level l) {
	Tr_accessList head = NULL, tail = NULL;
	F_accessList ptr = F_formals(l->frame);
	for (; ptr; ptr = ptr->tail) {
		Tr_access ac = Tr_Access(l, ptr->head);
		if (head) {
			tail->tail = Tr_AccessList(ac, NULL);
			tail = tail->tail;
		} else {
			head = Tr_AccessList(ac, NULL);
			tail = head;
		}
	}
	return head;
}

Tr_level Tr_newLevel(Tr_level parent, Temp_label name, U_boolList formals) {
	Tr_level l = checked_malloc(sizeof(*l));
	l->parent = parent;
	l->name = name;
	l->frame = F_newFrame(name, U_BoolList(TRUE, formals)); // static linker's offset
	l->formals = makeFormalList(l);

	return l;
}

static Tr_level outer = NULL;
Tr_level Tr_outermost(void) {
	if (!outer)
		outer = Tr_newLevel(NULL, Temp_newlabel(), NULL);

	return outer;
}

Tr_accessList Tr_formals(Tr_level level){
    return level->formals;
}
Tr_access Tr_allocLocal(Tr_level level, bool escape){
    return Tr_Access(level, F_allocLocal(level->frame, escape));
}
