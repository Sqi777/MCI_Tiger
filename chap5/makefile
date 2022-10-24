a.out: parsetest.o semant.o env.o types.o parse.o absyn.o symbol.o table.o y.tab.o errormsg.o lex.yy.o util.o
	cc -g parsetest.o semant.o env.o types.o parse.o absyn.o symbol.o table.o y.tab.o errormsg.o lex.yy.o util.o

parsetest.o: parsetest.c
	cc -g -c parsetest.c

semant.o: semant.c semant.h
	cc -g -c semant.c

env.o: env.c env.h
	cc -g -c env.c

types.o: types.c types.h
	cc -g -c types.c

parse.o: parse.c parse.h
	cc -g -c parse.c

absyn.o: absyn.c absyn.h
	cc -g -c absyn.c

symbol.o: symbol.c symbol.h
	cc -g -c symbol.c

prabsyn.o: prabsyn.c prabsyn.h
	cc -g -c prabsyn.c

table.o: table.c table.h
	cc -g -c table.c

y.tab.o: y.tab.c
	cc -g -c y.tab.c

y.tab.c: tiger.grm
	yacc -dv tiger.grm

y.tab.h: y.tab.c
	echo "y.tab.h was created at the same time as y.tab.c"

errormsg.o: errormsg.c errormsg.h util.h
	cc -g -c errormsg.c

lex.yy.o: lex.yy.c y.tab.h errormsg.h util.h
	cc -g -c lex.yy.c

lex.yy.c: tiger.lex
	lex tiger.lex

util.o: util.c util.h
	cc -g -c util.c
