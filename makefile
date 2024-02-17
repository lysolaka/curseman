cman = cman.o shared_f.o
editor = mapeditor.o shared_f.o

main : $(cman) $(editor)
	cc -o curseman $(cman) -lncursesw
	cc -o editor $(editor) -lncursesw

cman.o : cmanutils.h

mapeditor.o : cmanutils.h

shared_f.o : cmanutils.h

.PHONY : clean
clean : 
	rm -r cman.o mapeditor.o shared_f.o editor curseman
