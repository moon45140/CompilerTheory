# Compiler Theory Project

Project for Compiler Theory class taught by Dr. Philip Wilsey at the University of Cincinnati.

The project language specification from Dr. Wilsey is given in BNF in ([this file] https://github.com/narayaha/CompilerTheory/blob/master/Language%20Specification.pdf)

# Building

This compiler is built with G++ and Make.

To build the compiler either clone a local copy of the git repository or download the source files in the `src` directory

Navigate to the `src` directory and run `make` to build the compiler executable.

# Usage

By default the compiler executable is `narcomp`. Running this without any arguments gives usage information for the compiler.

To compile an input file:

	./narcomp <filename>

At this stage of development, the compiler does not produce an output file. However it does do syntax and type checking.
