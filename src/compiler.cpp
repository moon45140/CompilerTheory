// Filename: compiler.cpp
// Author: Himanshu Narayana
// This file is the main source file for the compiler project for Professor Wilsey's Compiler Theory class.
// The project description and language specification are on Professor Wilsey's website.

#include "compiler.h"

using namespace std;

int lineNumber;  // To keep track of the scanner's current line number
int warningCount; // To keep track of number of warnings found
int errorCount; // To keep track of number of errors found

SymbolTable globalSymbolTable;
vector<SymbolTable> localSymbolTable;

int currentScope;

ifstream inFile; // file handler for input file to tokenize
ofstream outFile; // file handler for output file

static void initializeOutput();

int main( int argc, char** argv )
{
	try
	{
		// Give usage information if no command line arguments were given
		if( argc == 1 )
		{
			cerr << "Usage: " << argv[0] << " [filename]" << endl;
			return 0;
		}
		
		// pass input filename to the initialization function
		initializeScanner( argv[1] );
		
		// Prepare output file
		initializeOutput();
		
		// Check status of input and output files
		if( inFile.good() == false )
		{
			cerr << "Error opening input file." << endl;
			return 0;
		}
		if( outFile.good() == false )
		{
			cerr << "Error opening file for output." << endl;
			
			inFile.close();
			return 0;
		}
		
		readProgram();
	}
	catch( exception& e )
	{
		cerr << e.what() << endl;
	}
	
	// If there were warnings and/or errors, leave a blank line before printing the summary.
	if( warningCount > 0 || errorCount > 0 )
	{
		cerr << endl;
	}
	
	// Output summary of number of lines read, number of errors, and number of warnings
	cout << "Summary" << endl;
	cout << "=======" << endl;
	cout << "Lines Read: " << lineNumber << endl;
	cout << "Errors: " << errorCount << endl;
	cout << "Warnings: " << warningCount << endl;
	
	inFile.close();
	outFile.close();
	
	if( errorCount > 0 )
	{
		remove( "narcomp_output.c" );
	}
	
	// Empty Symbol Tables
	// Global Symbol Table Entries
	for( SymbolTable::iterator janitor = globalSymbolTable.begin(); janitor != globalSymbolTable.end(); janitor++ )
	{
		delete janitor->second;
	}
	globalSymbolTable.clear();
	
	// Local Symbol Table Entries
	for( int i = 0; i < localSymbolTable.size(); i++ )
	{
		for( SymbolTable::iterator janitor = localSymbolTable[i].begin(); janitor != localSymbolTable[i].end(); janitor++ )
		{
			delete janitor->second;
		}
	}
	localSymbolTable.clear();
	
	return 0;
}

// This function adds an entry to the symbol table with the specified token type
void addSymbolEntry( Token* newToken )
{
	if( newToken->getGlobal() )
	{
		globalSymbolTable[newToken->getName()] = newToken;
	}
	else
	{
		localSymbolTable[currentScope][newToken->getName()] = newToken;
	}
}

// This function searches the symbol table for the specified entry.
// It modifies the token passed with the token's type (IDENTIFIER, RESERVE, or STRING) if it exists.
// The token passed gets a type of NONE if it is not found in the symbol table
void findSymbolEntry( TokenFrame& newToken )
{
	
	SymbolTable::iterator result;
	
	// get a pointer to the symbol table entry with the specified key
	// search the local scope first
	result = localSymbolTable[currentScope].find( newToken.name );
	
	// If not found in local scope
	if( result == localSymbolTable[currentScope].end() )
	{
		// search the global scope
		result = globalSymbolTable.find( newToken.name );
		
		// If not found in global scope either
		if( result == globalSymbolTable.end() )
		{
			newToken.tokenType = NONE;
		}
		else // If found in global scope
		{
			newToken.tokenType = result->second->getTokenType();
			newToken.isGlobal = true;
		}
	}
	else // If found in local scope
	{
		newToken.tokenType = result->second->getTokenType();
		newToken.isGlobal = false;
	}
}

// Reports warnings by printing line number and message to stderr
void reportWarning( const string& message )
{
	warningCount++;
	cerr << "Warning: Line " << lineNumber << ": " << message << endl;
}

// Reports errors by printing line number and message to stderr
void reportError( const string& message )
{
	errorCount++;
	cerr << "Error: Line " << lineNumber << ": " << message << endl;
}

void initializeOutput()
{
	outFile.open( "narcomp_output.c", ios::out | ios::trunc );
	
	if( outFile.good() == true )
	{
		// Prepare the beginning of the output
		outFile << "typedef union" << endl;
		outFile << "{" << endl;
		outFile << "\tchar charVal;" << endl;
		outFile << "\tint intVal;" << endl;
		outFile << "\tfloat floatVal;" << endl;
		outFile << "\tint stringPointer;" << endl;
		outFile << "\tvoid* jumpTarget;" << endl;
		outFile << "} MemoryFrame;" << endl;
		outFile << endl;
		outFile << "static MemoryFrame R[" << REGISTER_SIZE << "];" << endl;
		outFile << "static MemoryFrame MM[" << MEMORY_SIZE << "];" << endl;
		outFile << "static void* jumpRegister;" << endl;
		outFile << endl;
		outFile << "int getBool( void );" << endl;
		outFile << "int getInteger( void );" << endl;
		outFile << "float getFloat( void );" << endl;
		outFile << "int getString( void );" << endl;
		outFile << "int putBool( int oldBool );" << endl;
		outFile << "int putInteger( int oldInteger );" << endl;
		outFile << "int putFloat( float oldFloat );" << endl;
		outFile << "int putString( int oldString );" << endl;
		outFile << endl;
		outFile << "int main( int argc, char** argv )" << endl;
		outFile << "{" << endl;
		outFile << "\tR[0].intVal = " << MEMORY_SIZE << ";" << endl;
		outFile << "\tgoto programsetup;" << endl;
		outFile << endl;
	}
}
