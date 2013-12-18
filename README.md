# Compiler Theory Project

Project for Compiler Theory class taught by Dr. Philip Wilsey at the University of Cincinnati.

The project language specification from Dr. Wilsey is given in BNF in [this PDF file](https://github.com/narayaha/CompilerTheory/blob/master/Language%20Specification.pdf)

# Building in Linux

For Linux, this compiler is built with G++ version 4.7.2 and Make.

To build the compiler either clone a local copy of the git repository or download the source files in the `src` directory

Navigate to the `src` directory and run `make` to build the compiler executable.

# Building in Windows

For Windows, this compiler is built with G++ 4.8.1 from the MinGW tool set. Please refer to [this site](http://www.mingw.org/) for installation instructions.

To build the compiler into a Windows executable, run the following command from the `src` directory:

	g++ -o narcomp.exe compiler.cpp scanner.cpp parser.cpp

# Usage

Running the without any arguments gives usage information for the compiler.

To compile an input file in Linux:

	./narcomp <filename>

To compile an input file in Windows:

	narcomp <filename>

If there are no compiler errors, it will produce an output file named `narcomp_output.c`.

Compiling this into an executable will require the `runtime.c` file that came with the compiler source code.

To build the output file with the runtime file in Linux, simply type `make final`.

In Windows using MinGW, you will have to type `gcc -o final.exe narcomp_output.c`.
