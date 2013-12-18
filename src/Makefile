objects = compiler.o scanner.o parser.o

narcomp : $(objects)
	g++ -o narcomp $(objects)

compiler.o : compiler.h compiler.cpp
	g++ -c compiler.cpp

scanner.o : compiler.h scanner.cpp
	g++ -c scanner.cpp

parser.o : compiler.h parser.cpp
	g++ -c parser.cpp

final : narcomp_output.c runtime.c
	gcc -o final narcomp_output.c

clean :
	rm narcomp narcomp_output.c $(objects)