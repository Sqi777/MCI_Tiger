#include <stdio.h>
#include "frame.h"
#include "temp.h"

// F_frame are some information
struct F_frame_ {
    Temp_label label;
    int local_count;
    F_accessList formals; 
    F_accessList locals; // useless?
};

struct F_access_ {
    enum {inFrame, inReg} kind;
    union{
        int offset;
        Temp_temp reg;
    } u;
};
const int F_WORD_SIZE = 4;		//i386: 32bit
static const int F_KEEP = 4;	//number of parameters kept in regs;

static F_access InFrame(int offset);
static F_access InReg(Temp_temp reg);
// 记录一个参数或者变量的偏移量，至于该偏移量的起始位置是没有记录的
F_access InFrame(int offset){
    F_access p = checked_malloc(sizeof(*p));
    p->kind = inFrame;
    p->u.offset = offset;
    return p;
}
// 只是简单的记录了一下，该变量或者参数会分配到抽象机的寄存器中，抽象机有无限多寄存器
// 或者更直接的理解就是该变量或者参数会分配到寄存器中，至于Temp_temp的编号有什么用，现在还不清楚
F_access InReg(Temp_temp reg){
    F_access p = checked_malloc(sizeof(*p));
    p->kind = inReg;
    p->u.reg = reg;
    return p;
}

F_accessList F_AccessList(F_access head, F_accessList tail) {
	F_accessList l = checked_malloc(sizeof(*l));
	l->head = head;
	l->tail = tail;
	return l;
}
// 参数数量是固定不变的，因此构造frame时，可以直接确定formals属性，locals是后确定的，可以调用F_allocloca()添加，所以
// F_accessList locals 属性感觉无用
F_frame F_newFrame(Temp_label name, U_boolList formals) {
	F_frame fr = checked_malloc(sizeof(*fr));
	fr->label = name;		
	fr->formals = NULL;
	fr->locals = NULL;
	fr->local_count = 0;

    F_accessList head = NULL, tail = NULL;
	int rn = 0, fn = 0;
    U_boolList ptr;
    for (ptr = formals; ptr; ptr = ptr->tail) {
		F_access ac = NULL;
		if (rn < F_KEEP && !(ptr->head)) {
			ac = InReg(Temp_newtemp());	
			rn++;
		} else {
			fn++;
			ac = InFrame((fn+1)*F_WORD_SIZE);	//1 return address
		}

		if (head) {
			tail->tail = F_AccessList(ac, NULL);
			tail = tail->tail;
		} else {
			head = F_AccessList(ac, NULL);
			tail = head;
		}
	}
	fr->formals = head;

	return fr;
}

F_access F_allocLocal(F_frame f, bool escape) {
	//f->local_count++;
	if (escape){
		f->local_count++;	
		return InFrame(-F_WORD_SIZE * (f->local_count)); // why minus?
	}else 
		return InReg(Temp_newtemp());
}

Temp_label F_name(F_frame f){
	return f->label;
}

F_accessList F_formals(F_frame f){
	return f->formals;
}