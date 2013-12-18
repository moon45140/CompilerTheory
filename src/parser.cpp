// Filename: parser.cpp
// Author: Himanshu Narayana
// This file is the Parser for the Compiler Project in Professor Wilsey's Compiler Theory class.
// The parser performs grammar/syntax checking on the input source file in the style of an LL(1) recursive decent compiler.

#include "compiler.h"

using namespace std;

static TokenFrame currentToken;
static TokenFrame nextToken;

static int registerPointer = 2; // Keeps track of the next available register
static int memoryPointer = 1; // Keeps track of first address of available memory for global variables for the runtime environment.
static int localMemoryPointer = 0; // Keeps track of next available address for local variables in the top-level scope

// Stored CODEGEN for string literal storage in memory
static string literalStorage;

// Stores information for the code generator when processing procedure call arguments
static bool isArgument = false;
static Variable* argumentName;
static int argumentOperands = 0;
static int arrayIndexPointer = 6000000;

// Keeps track of next available IF block ID number
static int ifID = 0;

// Keeps track of next available LOOP block ID number
static int loopID = 0;

// Flags to determine which runtime functions to add to the program
static bool getBool = false;
static bool getInteger = false;
static bool getFloat = false;
static bool getString = false;
static bool putBool = false;
static bool putInteger = false;
static bool putFloat = false;
static bool putString = false;

static void generateRuntime( void );

// Functions for different stages of the parser. Declared static because they don't need to be visible outside of this file.
// readProgram() is declared extern in compiler.h because it is called from the main function in a different file.
static void readProgramHeader( void );
static void readProgramBody( void );
static void readDeclarations( Procedure*& currentProcedure );
static void readProcedureDeclaration( const bool isGlobal );
static void readProcedureHeader( Procedure*& currentProcedure, const bool isGlobal );
static void readParameterList( Procedure*& currentProcedure );
static void readParameter( Procedure*& currentProcedure );
static void readProcedureBody( Procedure*& currentProcedure );
static void readVariableDeclaration( Procedure*& currentProcedure, const bool isGlobal, const bool isParameter );
static void readStatements( Procedure*& currentProcedure );
static void readProcedureCall( Procedure*& currentProcedure );
static void readArgumentList( Procedure*& currentProcedure, Procedure*& myProcedure, int parameterNumber, int& argumentCount, string& returnCode );
static void readAssignment( Procedure*& currentProcedure );
static DataType readDestination( Procedure*& currentProcedure, Variable*& myVariable, string& destinationCode );
static void readIf( Procedure*& currentProcedure );
static void readLoop( Procedure*& currentProcedure ); // ****
static DataType readExpression( Procedure*& currentProcedure, int& resultRegister );
static DataType readArithOp( Procedure*& currentProcedure, int& resultRegister );
static DataType readRelation( Procedure*& currentProcedure, int& resultRegister ); // ****
static DataType readTerm( Procedure*& currentProcedure, int& resultRegister );
static DataType readFactor( Procedure*& currentProcedure, int& resultRegister );
static DataType readName( Procedure*& currentProcedure, int& resultRegister );

// This function begins parsing of the grammar/syntax with the first grammar rule
void readProgram( void )
{
	try
	{
		readProgramHeader(); // First read the program header
		readProgramBody(); // Next, read the program body
		
		// CODEGEN: Output the rest of the program setup code (string literals)
		if( errorCount == 0 )
		{
			outFile << "\treturn 0;" << endl;
			outFile << endl;
			outFile << "\tprogramsetup:" << endl;
			outFile << "\tR[1].intVal = " << memoryPointer << ";" << endl;
			outFile << literalStorage;
			outFile << "\tgoto programbody;" << endl;
			outFile << endl;
			
			generateRuntime();
		}
	}
	catch( CompileErrorException& e )
	{
		// Display the compiler's error message
		reportError( e.what() );
	}
}

void readProgramHeader( void )
{
	Token* myToken = NULL;
	
	try
	{
		// Start by getting the first token
		currentToken = getToken();
		nextToken = getToken();
		
		// The first token must be "program"
		if( currentToken.name.compare( "program" ) == 0 )
		{
			// Advance token to after "program"
			currentToken = nextToken;
			nextToken = getToken();
		}
		else
		{
			throw CompileErrorException( "Incorrect or missing program header" );
		}
		
		// Second token of the header must be an identifier
		if( currentToken.tokenType == NONE )
		{
			myToken = new Token( RESERVE, currentToken.name, true );
			addSymbolEntry( myToken );
			
			// Advance token to after the identifier
			currentToken = nextToken;
			nextToken = getToken();
		}
		else
		{
			throw CompileErrorException( "Illegal program identifier: " + currentToken.name );
		}
		
		// Third token of the header must be "is"
		if( currentToken.name.compare( "is" ) == 0 )
		{
			// Advance token to after "is"
			currentToken = nextToken;
			nextToken = getToken();
		}
		else
		{
			throw CompileErrorException( "Program header must end with keyword \"is\"" );
		}
	}
	catch( CompileErrorException& e )
	{
		// Display the compiler's error message
		reportError( e.what() );
		
		// Resync to Program Body
		while( inFile.good() )
		{
			if( currentToken.name.compare( "global" ) == 0 || currentToken.name.compare( "procedure" ) == 0 || currentToken.name.compare( "integer" ) == 0 || currentToken.name.compare( "float" ) == 0 || currentToken.name.compare( "bool" ) == 0 || currentToken.name.compare( "string" ) == 0 || currentToken.name.compare( "begin" ) == 0 )
			{
				break;
			}
			
			currentToken = nextToken;
			nextToken = getToken();
		}
	}
}

void readProgramBody( void )
{
	Procedure* currentProcedure = NULL;
	// currentToken is pointing to first declaration or begin
	
	// Check if there are any declarations
	if( currentToken.name.compare( "global" ) == 0 || currentToken.name.compare( "procedure" ) == 0 || currentToken.name.compare( "integer" ) == 0 || currentToken.name.compare( "float" ) == 0 || currentToken.name.compare( "bool" ) == 0 || currentToken.name.compare( "string" ) == 0 )
	{
		readDeclarations( currentProcedure );
	}
	
	// Look for "begin"
	if( currentToken.name.compare( "begin" ) == 0 )
	{
		// Advance Token to after "begin"
		currentToken = nextToken;
		nextToken = getToken();
	}
	else
	{
		throw CompileErrorException( "Expected \'begin\'" );
	}
	
	// CODEGEN: Update stack pointer and array declaration code
	if( errorCount == 0 )
	{
		outFile << "\tprogrambody:" << endl;
		outFile << "\tR[0].intVal = R[0].intVal - " << localMemoryPointer << ";" << endl;
		outFile << endl;
	}
	
	// Look for block of statements
	if( currentToken.tokenType == IDENTIFIER || currentToken.name.compare( "if" ) == 0 || currentToken.name.compare( "for" ) == 0 || currentToken.name.compare( "return" ) == 0 || currentToken.tokenType == NONE )
	{
		readStatements( currentProcedure );
	}
	
	// Check if there are any declarations in the statement section
	if( currentToken.name.compare( "global" ) == 0 || currentToken.name.compare( "procedure" ) == 0 || currentToken.name.compare( "integer" ) == 0 || currentToken.name.compare( "float" ) == 0 || currentToken.name.compare( "bool" ) == 0 || currentToken.name.compare( "string" ) == 0 )
	{
		reportError( "Incorrect Program Body: Declarations must be before \'begin\'" );
		
		// resync to "end program" because the parse cannot recover from this position
		while( inFile.good() )
		{
			if( currentToken.name.compare( "end" ) == 0 )
			{
				currentToken = nextToken;
				nextToken = getToken();
				
				if( currentToken.name.compare( "program" ) == 0 )
				{
					return;
				}
			}
			
			currentToken = getToken();
		}
	}
	
	// Look for "end program"
	if( currentToken.name.compare( "end" ) == 0 )
	{
		// Advance Token for after "end"
		currentToken = nextToken;
		nextToken = getToken();
		
		if( currentToken.name.compare( "program" ) != 0 )
		{
			throw CompileErrorException( "Incorrect end of program body" );
		}
	}
	else
	{
		throw CompileErrorException( "Incorrect end of program body" );
	}
}

void readDeclarations( Procedure*& currentProcedure )
{
	bool isGlobal; // Flag to tell whether declaration is global.
	// currentToken is pointing to the first declaration
	
	while( inFile.good() )
	{
		isGlobal = false;
		
		// Check if it's a global declaration
		if( currentToken.name.compare( "global" ) == 0 )
		{
			if( currentScope == 0 )
			{
				isGlobal = true;
			}
			else
			{
				isGlobal = false;
				reportWarning( "Variables and functions may only be declared global in the outermost scope. Setting to local." );
			}
			
			// Advance Token for after "global"
			currentToken = nextToken;
			nextToken = getToken();
		}
		
		// Check if it's a procedure declaration
		if( currentToken.name.compare( "procedure" ) == 0 )
		{
			readProcedureDeclaration( isGlobal );
		}
		// Check if it's a variable declaration
		else if( currentToken.name.compare( "integer" ) == 0 || currentToken.name.compare( "float" ) == 0 || currentToken.name.compare( "bool" ) == 0 || currentToken.name.compare( "string" ) == 0 )
		{
			readVariableDeclaration( currentProcedure, isGlobal, false );
		}
		// This block is for invalid syntax in the declaration section
		else
		{
			throw CompileErrorException( "Unrecognized declaration" );
		}
		
		// Check for a ";" after the declaration
		if( currentToken.name.compare( ";" ) == 0 )
		{
			// Advance Token to after the ";"
			currentToken = nextToken;
			nextToken = getToken();
		}
		else
		{
			reportError( "Expected ';' before \'" + currentToken.name + "\'. Not found" );
		}
		
		// Finished with declarations if we don't see anymore declaration keywords
		if( currentToken.name.compare( "global" ) != 0 && currentToken.name.compare( "procedure" ) != 0 && currentToken.name.compare( "integer" ) != 0 && currentToken.name.compare( "float" ) != 0 && currentToken.name.compare( "bool" ) != 0 && currentToken.name.compare( "string" ) != 0 )
		{
			break;
		}
	}
}

void readProcedureDeclaration( const bool isGlobal )
{
	Procedure* currentProcedure = NULL; // Pointer to the symbol table entry for the current procedure being declared
	SymbolTable::iterator janitor;
	bool checkName;
	int nestedCount;
	
	try
	{
		// create new scope, its associated symbol table, and associated local variable counter
		currentScope++;
		localSymbolTable.push_back( SymbolTable() );
		localSymbolTable[currentScope].clear();
		
		readProcedureHeader( currentProcedure, isGlobal ); // First read the procedure header
		
		readProcedureBody( currentProcedure ); // Second, read the procedure body
	}
	catch( CompileErrorException& e )
	{
		// Keep track of nested procedure blocks when resyncing
		nestedCount = 0;
		
		// Display the compiler's error message
		reportError( e.what() );
		reportWarning( "Encountered error in procedure declaration. Remainder of procedure has not been checked." );
		
		// Resync to Follow(declaration) which is ";"
		while( inFile.good() )
		{
			if( currentToken.name.compare( "procedure" ) == 0 )
			{
				nestedCount++;
			}
			else if( currentToken.name.compare( "end" ) == 0 )
			{
				currentToken = nextToken;
				nextToken = getToken();
				
				if( currentToken.name.compare( "procedure" ) == 0 )
				{
					currentToken = nextToken;
					nextToken = getToken();
					
					// Determine whether this "end procedure" is the end of our block or a nested one
					if( nestedCount == 0 )
					{
						break;
					}
					else
					{
						nestedCount--;
						continue;
					}
				}
			}
			
			currentToken = nextToken;
			nextToken = getToken();
		}
	}
	
	// Remove the scope and its associated symbol table
	for( janitor = localSymbolTable[currentScope].begin(); janitor != localSymbolTable[currentScope].end(); ++janitor )
	{
		// Don't remove the entry for the function whose scope is being deleted so that its containing scope will still be able to refer to it
		if( currentProcedure->getName().compare( janitor->second->getName() ) != 0 )
		{
			delete janitor->second;
			janitor->second = NULL;
		}
	}
	
	localSymbolTable[currentScope].clear();
	localSymbolTable.pop_back();
	currentScope--;
}

void readProcedureHeader( Procedure*& currentProcedure, const bool isGlobal )
{
	Token* myToken;
	string myName; // stores the name of the procedure
	
	// Advance Token to after "procedure"
	currentToken = nextToken;
	nextToken = getToken();
	
	// Next token should be an identifier
	if( currentToken.tokenType == NONE )
	{
		myName = currentToken.name;
		currentProcedure = new Procedure( IDENTIFIER, currentToken.name, isGlobal );
		
		// Add the procedure to its own symbol table
		addSymbolEntry( currentProcedure );
		
		// Advance Token to after IDENTIFIER
		currentToken = nextToken;
		nextToken = getToken();
	}
	else if( currentToken.tokenType == IDENTIFIER )
	{
		throw CompileErrorException( "Identifier \'" + currentToken.name + "\' has already been declared." );
	}
	else if( currentToken.tokenType == RESERVE )
	{
		throw CompileErrorException( "Invalid procedure identifier. \'" + currentToken.name + "\' is a reserve word." );
	}
	else
	{
		throw CompileErrorException( "Invalid procedure identifier \'" + currentToken.name + "\'" );
	}
	
	// Next token should be "("
	if( currentToken.name.compare( "(" ) == 0 )
	{
		// Advance Token to after "("
		currentToken = nextToken;
		nextToken = getToken();
	}
	else
	{
		throw CompileErrorException( "Invalid or missing parameter list" );
	}
	
	// CODEGEN: Create jump target to enter procedure
	if( errorCount == 0 )
	{
		if( currentProcedure == NULL )
		{
			throw CompileErrorException( "Unable to locate procedure \'" + myName + "\'" );
		}
		else
		{
			outFile << "\t" << currentProcedure->getName() << "_start:" << endl;
		}
	}
	
	// Read the Parameter List (starts with a type mark if it is not an empty list)
	if( currentToken.name.compare( "integer" ) == 0 || currentToken.name.compare( "float" ) == 0 || currentToken.name.compare( "bool" ) == 0 || currentToken.name.compare( "string" ) == 0 )
	{
		readParameterList( currentProcedure );
		
		// CODEGEN: Load procedure call arguments from registers into parameter locations in the stack
		if( errorCount == 0 )
		{
			for( int i = 0; i < currentProcedure->getParameterListSize(); i++ )
			{
				if( currentProcedure->getDirection( i ) == true )
				{
					outFile << "\tMM[R[0].intVal + " << i << "] = R[" << 200 + i << "];" << endl;
				}
			}
			
			outFile << endl;
		}
	}
	
	if( currentToken.name.compare( ")" ) == 0 )
	{
		// Advance Token to after ")"
		currentToken = nextToken;
		nextToken = getToken();
	}
	else
	{
		throw CompileErrorException( "Expected \')\' or \',\' before \'" + currentToken.name + "\'. Not found" );
	}
	
	// Copy this procedure's symbol table entry to its parent scope
	if( isGlobal )
	{
		globalSymbolTable[myName] = currentProcedure;
	}
	else
	{
		localSymbolTable[currentScope - 1][myName] = currentProcedure;
	}
}

void readParameterList( Procedure*& currentProcedure )
{
	readParameter( currentProcedure );
	
	if( currentToken.name.compare( "," ) == 0 )
	{
		// Advance Token to after ","
		currentToken = nextToken;
		nextToken = getToken();
		
		readParameterList( currentProcedure );
	}
}

void readParameter( Procedure*& currentProcedure )
{
	readVariableDeclaration( currentProcedure, false, true );
	
	if( currentToken.name.compare( "in" ) == 0 )
	{
		currentProcedure->addDirection( true );
		
		// Advance Token to after "in" or "out"
		currentToken = nextToken;
		nextToken = getToken();
	}
	else if( currentToken.name.compare( "out" ) == 0 )
	{
		currentProcedure->addDirection( false );
		
		// Advance Token to after "in" or "out"
		currentToken = nextToken;
		nextToken = getToken();
	}
	else
	{
		throw CompileErrorException( "Invalid parameter direction: " + currentToken.name );
	}
}

void readProcedureBody( Procedure*& currentProcedure )
{
	// Check if there are any declarations
	if( currentToken.name.compare( "global" ) == 0 || currentToken.name.compare( "procedure" ) == 0 || currentToken.name.compare( "integer" ) == 0 || currentToken.name.compare( "float" ) == 0 || currentToken.name.compare( "bool" ) == 0 || currentToken.name.compare( "string" ) == 0 )
	{
		readDeclarations( currentProcedure );
	}
	
	// Look for "begin"
	if( currentToken.name.compare( "begin" ) == 0 )
	{
		// Advance Token to after "begin"
		currentToken = nextToken;
		nextToken = getToken();
	}
	else
	{
		throw CompileErrorException( "Expected \'begin\'" );
	}
	
	// CODEGEN: Update stack pointer and array declaration code
	if( errorCount == 0 )
	{
		if( currentProcedure != NULL )
		{
			outFile << "\tR[0].intVal = R[0].intVal - " << currentProcedure->getLocalAddress() << ";" << endl;
			outFile << endl;
		}
	}
	
	// Look for block of statements
	if( currentToken.tokenType == IDENTIFIER || currentToken.name.compare( "if" ) == 0 || currentToken.name.compare( "for" ) == 0 || currentToken.name.compare( "return" ) == 0 || currentToken.tokenType == NONE )
	{
		readStatements( currentProcedure );
	}
	
	if( currentToken.name.compare( "global" ) == 0 || currentToken.name.compare( "procedure" ) == 0 || currentToken.name.compare( "integer" ) == 0 || currentToken.name.compare( "float" ) == 0 || currentToken.name.compare( "bool" ) == 0 || currentToken.name.compare( "string" ) == 0 )
	{
		reportError( "Incorrect Procedure Body: Declarations must be before \'begin\'" );
		
		// resync to "end procedure"
		while( inFile.good() )
		{
			int nestedCount = 0;
			
			if( currentToken.name.compare( "procedure" ) == 0 )
			{
				nestedCount++;
			}
			else if( currentToken.name.compare( "end" ) == 0 )
			{
				currentToken = nextToken;
				nextToken = getToken();
				
				if( currentToken.name.compare( "procedure" ) == 0 )
				{
					// Determine whether this "end procedure" is the end of our block or a nested one
					if( nestedCount == 0 )
					{
						currentToken = nextToken;
						nextToken = getToken();
						return;
					}
					else
					{
						currentToken = nextToken;
						nextToken = getToken();
						nestedCount--;
						continue;
					}
				}
			}
			
			currentToken = nextToken;
			nextToken = getToken();
		}
	}
	
	// Look for "end procedure"
	if( currentToken.name.compare( "end" ) == 0 )
	{
		// Advance Token to after end
		currentToken = nextToken;
		nextToken = getToken();
		
		if( currentToken.name.compare( "procedure" ) == 0 )
		{
			// CODEGEN: Update stack pointer at end of procedure
			// CODEGEN: Add return code for end of procedure
			if( errorCount == 0 )
			{
				if( currentProcedure != NULL )
				{
					outFile << "\tR[0].intVal = R[0].intVal + " << currentProcedure->getLocalAddress() << ";" << endl << endl;
					
					for( int i = 0; i < currentProcedure->getParameterListSize(); i++ )
					{
						if( currentProcedure->getDirection( i ) == false )
						{
							outFile << "\tR[" << 200 + i << "] = MM[R[0].intVal + " << i << "];" << endl;
						}
					}
					
					outFile << "\tjumpRegister = MM[R[0].intVal + " << currentProcedure->getParameterAddress() << "].jumpTarget;" << endl;
					outFile << "\tgoto *jumpRegister;" << endl << endl;
				}
			}
			
			// Advance Token to after "procedure"
			currentToken = nextToken;
			nextToken = getToken();
		}
		else
		{
			throw CompileErrorException( "Incorrect end of procedure body" );
		}
	}
	else
	{
		throw CompileErrorException( "Incorrect end of procedure body" );
	}
}

void readVariableDeclaration( Procedure*& currentProcedure, const bool isGlobal, const bool isParameter )
{
	string myName;
	DataType myDataType = INVALID;
	Token* myToken = NULL;
	Variable* myVariable = NULL;
	Array* myArray = NULL;
	int myArraySize = 1;
	
	stringstream convert;
	
	try
	{
		// First token should be data type
		if( currentToken.name.compare( "integer" ) == 0 || currentToken.name.compare( "float" ) == 0 || currentToken.name.compare( "bool" ) == 0 || currentToken.name.compare( "string" ) == 0 )
		{
			if( currentToken.name.compare( "integer" ) == 0 )
			{
				myDataType = INTEGER;
			}
			else if( currentToken.name.compare( "float" ) == 0 )
			{
				myDataType = FLOAT;
			}
			else if( currentToken.name.compare( "bool" ) == 0 )
			{
				myDataType = BOOL;
			}
			else if( currentToken.name.compare( "string" ) == 0 )
			{
				myDataType = STRINGT;
			}
			else
			{
				myDataType = INVALID;
				throw CompileErrorException( "Invalid data type: " + currentToken.name );
			}
			
			// Advance Token to after the type mark
			currentToken = nextToken;
			nextToken = getToken();
		}
		else
		{
			throw CompileErrorException( "Invalid data type: " + currentToken.name );
		}
		
		// Second token is variable name
		if( currentToken.tokenType == NONE )
		{
			myName = currentToken.name;
			
			// Advance Token to after IDENTIFIER
			currentToken = nextToken;
			nextToken = getToken();
		}
		else if( currentToken.tokenType == IDENTIFIER )
		{
			throw CompileErrorException( "Identifier \'" + currentToken.name + "\' has already been declared." );
		}
		else if( currentToken.tokenType == RESERVE )
		{
			throw CompileErrorException( "Invalid variable identifier. \'" + currentToken.name + "\' is a reserve word." );
		}
		else
		{
			throw CompileErrorException( "Invalid variable identifier: " + currentToken.name );
		}
		
		// Check if there is an array size
		if( currentToken.name.compare( "[" ) == 0 )
		{
			// Advance Token to after "["
			currentToken = nextToken;
			nextToken = getToken();
			
			// continue to parse array size
			if( currentToken.tokenType == NUMBER )
			{
				// Grammar allows any number for the array size, but throw warning if it's a float
				if( currentToken.name.find_first_of( '.' ) != string::npos )
				{
					reportWarning( "Array size is of type \'float\'. Decimal will be truncated." );
				}
				
				// Convert the number token into an integer to store the array size
				convert.str( currentToken.name );
				convert >> myArraySize;
				
				// Advance Token to after NUMBER
				currentToken = nextToken;
				nextToken = getToken();
			}
			else
			{
				throw CompileErrorException( "Invalid array size: " + currentToken.name );
			}
			
			// array closer
			if( currentToken.name.compare( "]" ) == 0 )
			{
				// Advance Token to after "]"
				currentToken = nextToken;
				nextToken = getToken();
			}
			else
			{
				throw CompileErrorException( "Unexpected end of array declaration. Expected \']\'" );
			}
			
			// Add the array to the parameter list of the current procedure if it's a parameter
			if( isParameter )
			{
				myArray = new Array( IDENTIFIER, myName, myDataType, myArraySize, isGlobal, memoryPointer, true );
				currentProcedure->addParameter( myArray ); // Add the array to the procedure's parameter list
				
				myArray = new Array( IDENTIFIER, myName, myDataType, myArraySize, isGlobal, memoryPointer, true );
				addSymbolEntry( myArray );
				
				memoryPointer += myArraySize; // Allocate one unit of memory for each array element
			}
			// Otherwise add it as a regular array
			else
			{
				if( isGlobal )
				{
					myArray = new Array( IDENTIFIER, myName, myDataType, myArraySize, isGlobal, memoryPointer, false );
					addSymbolEntry( myArray );
					memoryPointer += myArraySize; // Allocate one unit of memory for each array element
				}
				else
				{
					// If we're in the top-level scope, we must not use currentProcedure because it will be NULL.
					// Use the global counter defined in this file
					if( currentScope == 0 )
					{
						myArray = new Array( IDENTIFIER, myName, myDataType, myArraySize, isGlobal, memoryPointer, false );
						addSymbolEntry( myArray );
						memoryPointer += myArraySize; // One unit of memory for each array element
					}
					else if( currentScope > 0 )
					{
						myArray = new Array( IDENTIFIER, myName, myDataType, myArraySize, isGlobal, memoryPointer, false );
						addSymbolEntry( myArray );
						memoryPointer += myArraySize; // One unit of memory for each array element
					}
				}
			}
		}
		// Otherwise add as a variable since it's not an array
		else
		{
			// Add the variable to the parameter list of the current procedure if it is one.
			if( isParameter )
			{
				myVariable = new Variable( IDENTIFIER, myName, myDataType, isGlobal, currentProcedure->getParameterAddress(), true );
				currentProcedure->addParameter( myVariable ); // Add the data type to the procedure's parameter list
				
				myVariable = new Variable( IDENTIFIER, myName, myDataType, isGlobal, currentProcedure->getParameterAddress(), true );
				addSymbolEntry( myVariable );
				
				currentProcedure->advanceParameterAddress();
			}
			// Otherwise add it as a regular variable
			else
			{
				if( isGlobal )
				{
					myVariable = new Variable( IDENTIFIER, myName, myDataType, isGlobal, memoryPointer, false );
					addSymbolEntry( myVariable );
					memoryPointer++;
				}
				else
				{
					// If we're in the top-level scope, we must not use currentProcedure because it will be NULL.
					// Use the global counter defined in this file
					if( currentScope == 0 )
					{
						myVariable = new Variable( IDENTIFIER, myName, myDataType, isGlobal, localMemoryPointer, false );
						addSymbolEntry( myVariable );
						localMemoryPointer++;
					}
					else if( currentScope > 0 )
					{
						myVariable = new Variable( IDENTIFIER, myName, myDataType, isGlobal, currentProcedure->getLocalAddress(), false );
						addSymbolEntry( myVariable );
						currentProcedure->advanceLocalAddress();
					}
				}
			}
		}
	}
	catch( CompileErrorException& e )
	{
		// Display the compiler's error message
		reportError( e.what() );
		
		if( isParameter )
		{
			// Resync to Follow(variable_declaration) in parameter, which is "in" or out"
			while( inFile.good() )
			{
				if( currentToken.name.compare( "in" ) == 0 || currentToken.name.compare( "out" ) == 0 )
				{
					return;
				}
				else if( currentToken.name.compare( "," ) == 0 || currentToken.name.compare( ")" ) == 0 )
				{
					reportError( "Expected parameter direction before \'" + currentToken.name + "\'. Not found." );
					return;
				}
				
				currentToken = nextToken;
				nextToken = getToken();
			}
		}
		else
		{
			// Resync to Follow(variable_declaration) which is ";"
			while( inFile.good() )
			{
				if( currentToken.name.compare( ";" ) == 0 )
				{
					return;
				}
				else if( currentToken.name.compare( "global" ) == 0 || currentToken.name.compare( "integer" ) == 0 || currentToken.name.compare( "float" ) == 0 || currentToken.name.compare( "bool" ) == 0 || currentToken.name.compare( "string" ) == 0 || currentToken.name.compare( "begin" ) == 0 )
				{
					reportError( "Expected \';\' before \'" + currentToken.name + "\'. Not found." );
					return;
				}
				
				currentToken = nextToken;
				nextToken = getToken();
			}
		}
	}
}

void readStatements( Procedure*& currentProcedure )
{
	while( inFile.good() )
	{
		try
		{
			// Check if it's an assignment statement or procedure call
			if( currentToken.tokenType == IDENTIFIER )
			{
				// look ahead to determine procedure call or assignment statement
				
				// If next token is "(", then this is a procedure call
				if( nextToken.name.compare( "(" ) == 0 )
				{
					readProcedureCall( currentProcedure );
				}
				// If next token is ":=" or "[", then this is an assignment statement
				else if( nextToken.name.compare( ":=" ) == 0 || nextToken.name.compare( "[" ) == 0 )
				{
					readAssignment( currentProcedure );
				}
				else
				{
					throw CompileErrorException( "Unrecognized statement" );
				}
			}
			// Check if it's an if statement
			else if( currentToken.name.compare( "if" ) == 0 )
			{
				// Advance Token to after "if"
				currentToken = nextToken;
				nextToken = getToken();
				
				readIf( currentProcedure );
			}
			// Check if it's a loop statement
			else if( currentToken.name.compare( "for" ) == 0 )
			{
				// Advance Token to after "for"
				currentToken = nextToken;
				nextToken = getToken();
				
				readLoop( currentProcedure );
			}
			// Check if it's a return statement
			else if( currentToken.name.compare( "return" ) == 0 )
			{
				// CODEGEN: Generate return code for procedures
				// CODEGEN: Update stack pointer at end of procedure
				// CODEGEN: Add return code for end of procedure
				if( errorCount == 0 )
				{
					if( currentScope == 0 )
					{
						outFile << "\treturn 0;" << endl << endl;
					}
					else if( currentScope > 0 )
					{
						outFile << "\tR[0].intVal = R[0].intVal + " << currentProcedure->getLocalAddress() << ";" << endl << endl;
					
						for( int i = 0; i < currentProcedure->getParameterListSize(); i++ )
						{
							if( currentProcedure->getDirection( i ) == false )
							{
								outFile << "\tR[" << 200 + i << "] = MM[R[0].intVal + " << i << "];" << endl;
							}
						}
						
						outFile << "\tjumpRegister = MM[R[0].intVal + " << currentProcedure->getParameterAddress() << "].jumpTarget;" << endl;
						outFile << "\tgoto *jumpRegister;" << endl << endl;
					}
				}
				
				// Advance Token to after "return"
				currentToken = nextToken;
				nextToken = getToken();
			}
			else if( currentToken.tokenType == NONE )
			{
				throw CompileErrorException( "Undeclared identifier \'" + currentToken.name + "\'" );
			}
			else
			{
				throw CompileErrorException( "Unrecognized statement" + currentToken.name + nextToken.name );
			}
		}
		catch( CompileErrorException& e )
		{
			// Display the compiler's error message
			reportError( e.what() );
			
			// Resync to Follow(statement) which is ";"
			while( inFile.good() )
			{
				if( currentToken.name.compare( ";" ) == 0 )
				{
					break;
				}
				
				currentToken = nextToken;
				nextToken = getToken();
			}
		}
		
		// Check for ; at end of statement
		if( currentToken.name.compare( ";" ) == 0 )
		{
			// Advance Token to after ";"
			currentToken = nextToken;
			nextToken = getToken();
		}
		else
		{
			throw CompileErrorException( "Expected ';' before \'" + currentToken.name + "\'. Not found" );
		}
		
		// Finished with statements if we don't see anymore statement keywords
		if( currentToken.tokenType != IDENTIFIER && currentToken.name.compare( "if" ) != 0 && currentToken.name.compare( "for" ) != 0 && currentToken.name.compare( "return" ) != 0 )
		{
			break;
		}
		
		// If we're back at the top-level scope, reset procedure call information
		if( currentScope == 0 )
		{
			isArgument = false;
			argumentName = NULL;
			argumentOperands = 0;
			arrayIndexPointer = 6000000;
		}
	}
}

void readProcedureCall( Procedure*& currentProcedure )
{
	TokenFrame calledProcedure = currentToken;
	Token* apparentProcedure = NULL;
	Procedure* myProcedure = NULL;
	int argumentCount = 0;
	string returnCode;
	
	registerPointer = 2;
	
	// Locate the symbol table entry for the called procedure
	findSymbolEntry( calledProcedure );
	
	// If there is no such entry...
	if( calledProcedure.tokenType == NONE )
	{
		throw CompileErrorException( "Procedure \'" + calledProcedure.name + "\' not found" );
	}
	
	// Retrieve symbol table entry depending on whether it is global or local
	if( calledProcedure.isGlobal )
	{
		apparentProcedure = globalSymbolTable[calledProcedure.name];
	}
	else
	{
		apparentProcedure = localSymbolTable[currentScope][calledProcedure.name];
	}
	
	// Check for null pointer
	if( apparentProcedure == NULL )
	{
		throw CompileErrorException( "Unable to locate procedure \'" + calledProcedure.name + "\'" );
	}
	else
	{
		if( typeid( *apparentProcedure ) != typeid( Procedure ) )
		{
			throw CompileErrorException( "\'" + apparentProcedure->getName() + "\' is not a procedure" );
		}
		else
		{
			myProcedure = dynamic_cast<Procedure*>(apparentProcedure);
		}
	}
	
	// Check if it is a runtime function
	if( myProcedure->getName().compare( "getBool" ) == 0 )
	{
		getBool = true;
	}
	else if( myProcedure->getName().compare( "getInteger" ) == 0 )
	{
		getInteger = true;
	}
	else if( myProcedure->getName().compare( "getFloat" ) == 0 )
	{
		getFloat = true;
	}
	else if( myProcedure->getName().compare( "getString" ) == 0 )
	{
		getString = true;
	}
	else if( myProcedure->getName().compare( "putBool" ) == 0 )
	{
		putBool = true;
	}
	else if( myProcedure->getName().compare( "putInteger" ) == 0 )
	{
		putInteger = true;
	}
	else if( myProcedure->getName().compare( "putFloat" ) == 0 )
	{
		putFloat = true;
	}
	else if( myProcedure->getName().compare( "putString" ) == 0 )
	{
		putString = true;
	}
	
	// Advance Token to after "("
	currentToken = getToken();
	nextToken = getToken();
	
	// Check if the argument list contains the start of an expression
	if( currentToken.name.compare( "(" ) == 0 || currentToken.name.compare( "-" ) == 0 || currentToken.tokenType == IDENTIFIER || currentToken.tokenType == NUMBER || currentToken.tokenType == STRING || currentToken.name.compare( "true" ) == 0 || currentToken.name.compare( "false" ) == 0 )
	{
		readArgumentList( currentProcedure, myProcedure, 0, argumentCount, returnCode );
		
		// Check how many arguments were read
		if( argumentCount < myProcedure->getParameterListSize() )
		{
			reportError( "Too few arguments in procedure call" );
		}
	}
	
	if( currentToken.name.compare( ")" ) == 0 )
	{
		// Advance Token to after ")"
		currentToken = nextToken;
		nextToken = getToken();
	}
	else
	{
		throw CompileErrorException( "Mismatched Parentheses" );
	}
	
	// CODEGEN: Move Stack Pointer for and Add stack entry for return address
	// CODEGEN: Move Stack Pointer for procedure parameters
	if( errorCount == 0 )
	{
		outFile << "\tR[0].intVal = R[0].intVal - 1;" << endl;
		outFile << "\tMM[R[0].intVal].jumpTarget = &&" << myProcedure->getName() << "_return" << myProcedure->getReturnAddress() << ";" << endl;
		outFile << "\tR[0].intVal = R[0].intVal - " << myProcedure->getParameterAddress() << ";" << endl;
		outFile << "\tgoto " << myProcedure->getName() << "_start;" << endl;
		outFile << "\t" << myProcedure->getName() << "_return" << myProcedure->getReturnAddress() << ":" << endl;
		outFile << "\tR[0].intVal = R[0].intVal + " << myProcedure->getParameterAddress() + 1 << ";" << endl;
		outFile << returnCode;
		outFile << endl;
		
		myProcedure->advanceReturnAddress();
	}
}

void readArgumentList( Procedure*& currentProcedure, Procedure*& myProcedure, int parameterNumber, int& argumentCount, string& returnCode )
{
	int resultRegister = 2;
	stringstream convert;
	
	registerPointer = 2;
	
	// Check if there's an entry in the parameter list to match this argument
	if( parameterNumber >= myProcedure->getParameterListSize() )
	{
		throw CompileErrorException( "Too many arguments in procedure call" );
	}
	
	isArgument = true;
	argumentOperands = 0;
	argumentName = NULL;
	
	// Parse the argument and check types
	if( readExpression( currentProcedure, resultRegister ) != myProcedure->getParameterType( parameterNumber ) )
	{
		convert.str( string() );
		convert << parameterNumber;
		reportError( "Incompatible data type in argument " + convert.str() );
	}
	
	isArgument = false;
	
	// CODEGEN: Store this argument in a register for the called procedure to grab later
	// CODEGEN: Buffer code for storing output parameters after returning
	if( errorCount == 0 )
	{
		outFile << "\tR[" << 200 + argumentCount << "] = R[" << resultRegister << "];" << endl;
		
		convert.str( string() );
		if( argumentOperands == 1 && myProcedure->getDirection( argumentCount ) == false && argumentName != NULL )
		{
			if( typeid( *argumentName ) == typeid( Array ) )
			{
				Array* myArray = dynamic_cast<Array*>(argumentName);
				
				convert << "\tR[2].intVal = MM[" << arrayIndexPointer << "].intVal;" << endl;
				convert << "\tMM[R[2].intVal + " << myArray->getAddress() << "] = R[" << 200 + argumentCount << "];" << endl;
				
				arrayIndexPointer++;
			}
			else
			{
				if( argumentName->getGlobal() )
				{
					convert << "\tMM[" << argumentName->getAddress() << "] = R[" << 200 + argumentCount << "];" << endl;
				}
				else
				{
					if( argumentName->getParameter() )
					{
						convert << "\tMM[R[0].intVal + " << currentProcedure->getLocalAddress() + argumentName->getAddress() << "];" << endl;
					}
					else 
					{
						convert << "\tMM[R[0].intVal + " << argumentName->getAddress() << "];" << endl;
					}
				}
			}
			
			returnCode += convert.str();
		}
	}
	
	// Increment the counter after successfully parsing and checking the argument
	argumentCount++;
	
	if( currentToken.name.compare( "," ) == 0 )
	{
		// Advance Token to after ","
		currentToken = nextToken;
		nextToken = getToken();
		
		// If there was a comma, expect another argument
		readArgumentList( currentProcedure, myProcedure, parameterNumber + 1, argumentCount, returnCode );
	}
}

void readAssignment( Procedure*& currentProcedure )
{
	DataType destinationType = INVALID;
	DataType expressionType = INVALID;
	Variable* destinationVariable = NULL;
	int resultRegister = 2;
	string destinationCode;
	
	registerPointer = 2;
	
	try
	{
		destinationType = readDestination( currentProcedure, destinationVariable, destinationCode );
	}
	catch( CompileErrorException& e )
	{
		// Display the compiler's error message
		reportError( e.what() );
		
		// Resync to Follow(destination) which is ":=" or the ";" at the end of the statement
		while( inFile.good() )
		{
			if( currentToken.name.compare( ":=" ) == 0 )
			{
				break;
			}
			else if( currentToken.name.compare( ";" ) == 0 )
			{
				throw;
			}
		}
	}
	
	if( currentToken.name.compare( ":=" ) == 0 )
	{
		// Advance Token to after ":="
		currentToken = nextToken;
		nextToken = getToken();
	}
	else
	{
		throw CompileErrorException( "Invalid statement" );
	}
	
	expressionType = readExpression( currentProcedure, resultRegister );
	
	switch( destinationType )
	{
		case BOOL:
			switch( expressionType )
			{
				case BOOL:
					if( errorCount == 0 )
					{
						outFile << destinationCode << ".intVal = R[" << resultRegister << "].intVal;" << endl << endl;
					}
					break;
					
				case INTEGER:
					if( errorCount == 0 )
					{
						outFile << "\tif( R[resultRegister] != 0 ) goto secondcheck;" << endl;
						outFile << "\tgoto endcheck;" << endl;
						outFile << "\tsecondcheck:" << endl;
						outFile << "\tif( R[resultRegister] != 1 ) goto runtimeerror;" << endl;
						outFile << "endcheck:" << endl;
						
						outFile << destinationCode << ".intVal = R[" << resultRegister << "].intVal;" << endl << endl;
					}
					break;
					
				default:
					reportError( "Incompatible data types in assignment statement" );
					break;
			}
			break;
			
		case FLOAT:
			switch( expressionType )
			{
				case FLOAT:
					if( errorCount == 0 )
					{
						outFile << destinationCode << ".floatVal = R[" << resultRegister << "].floatVal;" << endl << endl;
					}
					break;
					
				case INTEGER:
					if( errorCount == 0 )
					{
						outFile << destinationCode << ".floatVal = R[" << resultRegister << "].intVal;" << endl << endl;
					}
					break;
					
				default:
					reportError( "Incompatible data types in assignment statement" );
					break;
			}
			break;
			
		case INTEGER:
			switch( expressionType )
			{
				case BOOL:
					if( errorCount == 0 )
					{
						outFile << destinationCode << ".intVal = R[" << resultRegister << "].intVal;" << endl << endl;
					}
					break;
					
				case FLOAT:
					if( errorCount == 0 )
					{
						outFile << destinationCode << ".intVal = R[" << resultRegister << "].floatVal;" << endl << endl;
					}
					break;
					
				case INTEGER:
					if( errorCount == 0 )
					{
						outFile << destinationCode << ".intVal = R[" << resultRegister << "].intVal;" << endl << endl;
					}
					break;
					
				default:
					reportError( "Incompatible data types in assignment statement" );
					break;
			}
			break;
			
		case STRINGT:
			if( expressionType != STRINGT )
			{
				reportError( "Incompatible data types in assignment statement" );
			}
			
			if( errorCount == 0 )
			{
				outFile << destinationCode << ".stringPointer = R[" << resultRegister << "].stringPointer;" << endl << endl;
			}
			break;
			
		default:
			reportError( "Unknown data type in destination of assignment statement" );
			break;
	}
}

DataType readDestination( Procedure*& currentProcedure, Variable*& myVariable, string& destinationCode )
{
	Token* myName = NULL;
	Array* myArray = NULL;
	DataType nameType = INVALID;
	int resultRegister = 2;
	stringstream convert;
	
	// currentToken is the identifier. Get its symbol table entry
	if( currentToken.isGlobal )
	{
		myName = globalSymbolTable[currentToken.name];
	}
	else
	{
		myName = localSymbolTable[currentScope][currentToken.name];
	}
	
	if( typeid( *myName ) == typeid( Variable ) )
	{
		myVariable = dynamic_cast<Variable*>(myName);
		nameType = myVariable->getDataType();
	}
	else
	{
		reportError( "\'" + myName->getName() + "\' is not a valid variable" );
	}
	
	// Advance Token to after IDENTIFIER
	currentToken = nextToken;
	nextToken = getToken();
	
	// Check if there is a "[" for an array element
	if( currentToken.name.compare( "[" ) == 0 )
	{
		if( typeid( *myVariable ) != typeid( Array ) )
		{
			reportError( "\'" + myVariable->getName() + "\' is not an array" );
		}
		else
		{
			myArray = dynamic_cast<Array*>(myVariable);
		}
		
		// Advance Token to after "["
		currentToken = nextToken;
		nextToken = getToken();
		
		
		if( readExpression( currentProcedure, resultRegister ) != INTEGER )
		{
			reportError( "Array index must evaluate to an integer" );
		}
		
		// Check for "]" after expression
		if( currentToken.name.compare( "]" ) == 0 )
		{
			// Advance Token to after "]"
			currentToken = nextToken;
			nextToken = getToken();
		}
		else
		{
			throw CompileErrorException( "Mismatched square brackets for array subscript" );
		}
		
		// CODEGEN: Generate code to store result of assignment into array element (will be output later)
		if( errorCount == 0 )
		{
			convert << "\tMM[R[" << resultRegister << "].intVal + " << myArray->getAddress() << "]";
			destinationCode = convert.str();
		}
	}
	// CODEGEN: Generate code to store result of assignment into variable (will be output later)
	else if( errorCount == 0 )
	{
		if( typeid( *myVariable ) == typeid( Array ) )
		{
			reportWarning( string( "No array index specified for " + myVariable->getName() ) );
		}
		
		if( myVariable->getGlobal() )
		{
			convert << "\tMM[" << myVariable->getAddress() << "]";
		}
		else
		{
			if( myVariable->getParameter() )
			{
				convert << "\tMM[R[0].intVal + " << currentProcedure->getLocalAddress() + myVariable->getAddress() << "]";
			}
			else
			{
				convert << "\tMM[R[0].intVal + " << myVariable->getAddress() << "]";
			}
		}
		
		destinationCode = convert.str();
	}
	
	return nameType;
}

void readIf( Procedure*& currentProcedure )
{
	int resultRegister = 2;
	// Grab the next available ID number
	int myID = ifID;
	ifID++;
	
	registerPointer = 2;
	
	try
	{
		// next token should be "("
		if( currentToken.name.compare( "(" ) == 0 )
		{
			// Advance Token to after "("
			currentToken = nextToken;
			nextToken = getToken();
		}
		else
		{
			throw CompileErrorException( "\'(\' is required before conditional expression" );
		}
		
		// next is the conditional expression
		// CODEGEN: Begin the code generation for the if block
		switch( readExpression( currentProcedure, resultRegister ) )
		{
			case BOOL:
				if( errorCount == 0 )
				{
					outFile << "\tif( R[" << resultRegister << "].intVal == 1 ) goto if" << myID << "_start;" << endl;
					outFile << "\tgoto else" << myID << "_start;" << endl;
					outFile << "\tif" << myID << "_start:" << endl << endl;
				}
				break;
				
			case INTEGER:
				if( errorCount == 0 )
				{
					outFile << "\tif( R[resultRegister] != 0 ) goto secondcheck;" << endl;
					outFile << "\tgoto endcheck;" << endl;
					outFile << "\tsecondcheck:" << endl;
					outFile << "\tif( R[resultRegister] != 1 ) goto runtimeerror;" << endl;
					outFile << "endcheck:" << endl;
					
					outFile << "\tif( R[" << resultRegister << "].intVal == 1 ) goto if" << myID << "_start;" << endl;
					outFile << "\tgoto else" << myID << "_start;" << endl;
					outFile << "\tif" << myID << "_start:" << endl << endl;
				}
				break;
				
			default:
				reportError( "Conditional expression must evaluate to boolean data type" );
				break;
		}
		
		// next is the ")"
		if( currentToken.name.compare( ")" ) == 0 )
		{
			// Advance Token to after ")"
			currentToken = nextToken;
			nextToken = getToken();
		}
		else
		{
			throw CompileErrorException( "\')\' is required after conditional expression" );
		}
		
		// next is "then"
		if( currentToken.name.compare( "then" ) == 0 )
		{
			// Advance Token to after "then"
			currentToken = nextToken;
			nextToken = getToken();
		}
		else
		{
			throw CompileErrorException( "keyword \'then\' is required after \')\' of conditional expression" );
		}
		
		// next is one or more statements
		registerPointer = 2;
		readStatements( currentProcedure );
		
		// CODEGEN: Begin the else block
		if( errorCount == 0 )
		{
			outFile << "\tgoto endif" << myID << ";" << endl;
			outFile << "\telse" << myID << "_start:" << endl << endl;
		}
		
		// check if there is an "else" section
		if( currentToken.name.compare( "else" ) == 0 )
		{
			// Advance Token to after "else"
			currentToken = nextToken;
			nextToken = getToken();
			
			// next is one or more statements
			readStatements( currentProcedure );
		}
		
		// finally, look for "end if"
		if( currentToken.name.compare( "end" ) == 0 )
		{
			// Advance Token to after "end"
			currentToken = nextToken;
			nextToken = getToken();
			
			if( currentToken.name.compare( "if" ) == 0 )
			{
				// CODEGEN: End the entire if block
				if( errorCount == 0 )
				{
					outFile << "\tendif" << myID << ":" << endl << endl;
				}
				
				currentToken = nextToken;
				nextToken = getToken();
			}
			else
			{
				throw CompileErrorException( "Incorrect end of if statement" );
			}
		}
		else
		{
			throw CompileErrorException( "Incorrect end of if statement" );
		}
	}
	catch( CompileErrorException& e )
	{
		// Display the compiler's error message
		reportError( e.what() );
		
		// Keep track of nested blocks while resyncing
		int nestedCount = 0;
		
		while( inFile.good() )
		{
			// Resync to Follow(if_statement) which is ";" while accounting for nested ifs
			if( currentToken.name.compare( "if" ) == 0 )
			{
				nestedCount++;
			}
			else if( currentToken.name.compare( "end" ) == 0 )
			{
				currentToken = nextToken;
				nextToken = getToken();
				
				if( currentToken.name.compare( "if" ) == 0 )
				{
					currentToken = nextToken;
					nextToken = getToken();
					
					// Determine wehther this "end if" is the end of ours or a nested one
					if( nestedCount == 0 )
					{
						break;
					}
					else
					{
						nestedCount--;
						continue;
					}
				}
				
			}
			
			currentToken = nextToken;
			nextToken = getToken();
		}
	}
}

void readLoop( Procedure*& currentProcedure )
{
	int resultRegister = 2;
	// Grab the next available ID number
	int myID = loopID;
	loopID++;
	
	registerPointer = 2;
	
	try
	{
		// next token should be "("
		if( currentToken.name.compare( "(" ) == 0 )
		{
			// Advance Token to after "("
			currentToken = nextToken;
			nextToken = getToken();
		}
		else
		{
			throw CompileErrorException( "\'(\' is required before assignment statement" );
		}
		
		// next is an assignment statement
		try
		{
			readAssignment( currentProcedure );
		}
		catch( CompileErrorException& e )
		{
			// Display the compiler's error message
			reportError( e.what() );
			
			// Resync to Follow(assignment_statement) which is ";"
			while( inFile.good() )
			{
				if( currentToken.name.compare( ";" ) == 0 )
				{
					break;
				}
				
				currentToken = nextToken;
				nextToken = getToken();
			}
		}
		
		// followed by a ";"
		if( currentToken.name.compare( ";" ) == 0 )
		{
			// Advance Token to after ";"
			currentToken = nextToken;
			nextToken = getToken();
		}
		else
		{
			throw CompileErrorException( "Missing \';\' after assignment statement" );
		}
		
		// next is the conditional expression
		// CODEGEN: Begin the code generation for the loop block
		if( errorCount == 0 )
		{
			outFile << "\tloop" << myID << "_check:" << endl << endl;
		}
		switch( readExpression( currentProcedure, resultRegister ) )
		{
			case BOOL:
				if( errorCount == 0 )
				{
					outFile << "\tif( R[" << resultRegister << "].intVal == 1 ) goto loop" << myID << "_start;" << endl;
					outFile << "\tgoto endloop" << myID << ";" << endl;
					outFile << "\tloop" << myID << "_start:" << endl << endl;
				}
				break;
				
			case INTEGER:
				if( errorCount == 0 )
				{
					outFile << "\tif( R[resultRegister] != 0 ) goto secondcheck;" << endl;
					outFile << "\tgoto endcheck;" << endl;
					outFile << "\tsecondcheck:" << endl;
					outFile << "\tif( R[resultRegister] != 1 ) goto runtimeerror;" << endl;
					outFile << "endcheck:" << endl;
					
					outFile << "\tif( R[" << resultRegister << "].intVal == 1 ) goto loop" << myID << "_start;" << endl;
					outFile << "\tgoto endloop" << myID << ";" << endl;
					outFile << "\tloop" << myID << "_start:" << endl << endl;
				}
				break;
				
			default:
				reportError( "Conditional expression must evaluate to boolean data type" );
				break;
		}
		
		// next is the ")"
		if( currentToken.name.compare( ")" ) == 0 )
		{
			// Advance Token to after ")"
			currentToken = nextToken;
			nextToken = getToken();
		}
		else
		{
			throw CompileErrorException( "Missing \')\' after conditional expression" );
		}
		
		// Check if there are any statements inside the loop
		if( currentToken.tokenType == IDENTIFIER || currentToken.name.compare( "if" ) == 0 || currentToken.name.compare( "for" ) == 0 || currentToken.name.compare( "return" ) == 0 )
		{
			registerPointer = 2;
			readStatements( currentProcedure );
		}
		
		// finally, look for "end for"
		if( currentToken.name.compare( "end" ) == 0 )
		{
			// Advance Token to after "end"
			currentToken = nextToken;
			nextToken = getToken();
			
			if( currentToken.name.compare( "for" ) == 0 )
			{
				// CODEGEN: End the entire loop block
				if( errorCount == 0 )
				{
					outFile << "\tgoto loop" << myID << "_check;" << endl;
					outFile << "\tendloop" << myID << ":" << endl << endl;
				}
				currentToken = nextToken;
				nextToken = getToken();
			}
			else
			{
				throw CompileErrorException( "Incorrect end of for loop" );
			}
		}
		else
		{
			throw CompileErrorException( "Incorrect end of for loop" );
		}
	}
	catch( CompileErrorException& e )
	{
		// Display the compiler's error message
		reportError( e.what() );
		
		// Keep track of nested blocks while resyncing
		int nestedCount = 0;
		
		while( inFile.good() )
		{
			// Resync to Follow(loop_statement) which is ";" while accounting for nested loops
			if( currentToken.name.compare( "for" ) == 0 )
			{
				nestedCount++;
			}
			else if( currentToken.name.compare( "end" ) == 0 )
			{
				currentToken = nextToken;
				nextToken = getToken();
				
				if( currentToken.name.compare( "for" ) == 0 )
				{
					currentToken = nextToken;
					nextToken = getToken();
					
					// Determine wehther this "end for" is the end of ours or a nested one
					if( nestedCount == 0 )
					{
						break;
					}
					else
					{
						nestedCount--;
						continue;
					}
				}
				
			}
			
			currentToken = getToken();
		}
	}
}

DataType readExpression( Procedure*& currentProcedure, int& resultRegister )
{
	DataType myType1 = INVALID; // Data type for an operand
	DataType myType2 = INVALID; // Data type for another operand
	int myRegister1 = 2; // Keeps track of the register of one of the operands
	int myRegister2 = 2; // Keeps track of the register of one of the operands
	DataType expressionType = INVALID; // Data type for the whole expression
	bool restricted = false; // flags whether there was a & or | operator in the expression
	int operandCount = 0; // Counts number of operands
	string operation; // Stores the selected operation
	
	// flag to check when we've reached the end of the expression
	bool terminate = false;
	
	// Check if there is a "not"
	if( currentToken.name.compare( "not" ) == 0 )
	{
		terminate = true;
		
		// Advance Token to after "not"
		currentToken = nextToken;
		nextToken = getToken();
		
		expressionType = readArithOp( currentProcedure, myRegister1 );
		
		switch( expressionType )
		{
			case BOOL:
			case INTEGER:
				break;
				
			default:
				reportError( "Operand of \'not\' must be a boolean or integer" );
				break;
		}
		
		// CODEGEN: Generate code for "not" operator
		if( errorCount == 0 )
		{
			outFile << "\tR[" << myRegister1 << "].intVal = !R[" << myRegister1 << "];" << endl;
		}
		
		// if there was a "not", grammar rule specifies no operator afterwards
	}
	else
	{
		do
		{
			myType2 = myType1;
			myRegister2 = myRegister1;
			myType1 = readArithOp( currentProcedure, myRegister1 );
			operandCount++;
			
			// Update the data type of the expression
			if( operandCount == 1 )
			{
				expressionType = myType1;
			}
			else if( operandCount > 1 )
			{
				expressionType = myType1;
				
				// CODEGEN: Generate code for bitwise/logical operators
				if( errorCount == 0 )
				{
					outFile << "\tR[" << myRegister1 << "].intVal = R[" << myRegister1 << "].intVal " << operation << " R[" << myRegister2 << "].intVal;" << endl;
				}
			}
			else
			{
				expressionType = INVALID;
			}
			
			// check if there is a "&" or "|" next
			if( currentToken.name.compare( "&" ) == 0 || currentToken.name.compare( "|" ) == 0 )
			{
				restricted = true;
				operation = currentToken.name;
				
				// Advance Token to after "&" or "|"
				currentToken = nextToken;
				nextToken = getToken();
			}
			else
			{
				terminate = true;
			}
			
			// Check the data type of the operand
			if( restricted )
			{
				switch( myType1 )
				{
					case BOOL:
					case INTEGER:
						break;
						
					default:
						reportError( "Operand of logical expression must be a boolean or integer" );
						break;
				}
			}
		} while( terminate == false );
	}
	
	resultRegister = myRegister1;
	return expressionType;
}

DataType readArithOp( Procedure*& currentProcedure, int& resultRegister )
{
	DataType myType1 = INVALID; // Data type for an operand
	DataType myType2 = INVALID; // Data type for another operand
	int myRegister1 = 2; // Keeps track of the register of one of the operands
	int myRegister2 = 2; // Keeps track of the register of one of the operands
	DataType arithType = INVALID; // Data type for the whole ArithOp
	bool restricted = false; // flags whether there was a + or - in the ArithOp
	int operandCount = 0; // Counts number of operands
	string operation; // Stores the selected operation
	
	// flag to check when we've  reached the end of the ArithOp
	bool terminate = false;
	
	do
	{
		myType2 = myType1;
		myRegister2 = myRegister1;
		myType1 = readRelation( currentProcedure, myRegister1 );
		operandCount++;
		
		// Update the data type of the ArithOp
		if( operandCount == 1 )
		{
			arithType = myType1;
		}
		else if( operandCount > 1 )
		{
			if( arithType < myType1 )
			{
				arithType = myType1;
			}
			
			// CODEGEN: Generate lines for computing addition or subtraction
			if( errorCount == 0 )
			{
				switch( arithType )
				{
					case FLOAT:
						if( myType1 == FLOAT && myType2 == INTEGER )
						{
							outFile << "\tR[" << myRegister1 << "].floatVal = R[" << myRegister1 << "].floatVal " << operation << " R[" << myRegister2 << "].intVal;" << endl;
						}
						else if( myType1 == INTEGER && myType2 == FLOAT )
						{
							outFile << "\tR[" << myRegister1 << "].floatVal = R[" << myRegister1 << "].intVal " << operation << " R[" << myRegister2 << "].floatVal;" << endl;
						}
						break;
						
					case INTEGER:
						outFile << "\tR[" << myRegister1 << "].intVal = R[" << myRegister1 << "].intVal " << operation << " R[" << myRegister2 << "].intVal;" << endl;
						break;
						
					default:
						break;
				}
			}
		}
		else
		{
			arithType = INVALID;
		}
		
		// check if there is a "+" or "-" next
		if( currentToken.name.compare( "+" ) == 0 || currentToken.name.compare( "-" ) == 0 )
		{
			restricted = true;
			operation = currentToken.name;
			
			// Advance Token to after "+" or "-"
			currentToken = nextToken;
			nextToken = getToken();
		}
		else
		{
			terminate = true;
		}
		
		// Check the data type of the operand
		if( restricted )
		{
			switch( myType1 )
			{
				case FLOAT:
				case INTEGER:
					break;
					
				default:
					reportError( "Operand of arithmetic expression must be an integer or a float" );
					break;
			}
		}
	} while( terminate == false );
	
	resultRegister = myRegister1;
	return arithType;
}

DataType readRelation( Procedure*& currentProcedure, int& resultRegister )
{
	DataType myType1 = INVALID; // Data type for an operand
	DataType myType2 = INVALID; // Data type for another operand
	int myRegister1 = 2; // Keeps track of the register of one of the operands
	int myRegister2 = 2; // Keeps track of the register of one of the operands
	DataType relationType = INVALID; // Data type for the whole relation
	bool restricted = false; // flags whether there was a relational operator
	int operandCount = 0; // Counts number of operands
	string operation; // Stores the selected operation
	
	// flag to check when we've  reached the end of the Relation
	bool terminate = false;
	
	do
	{
		myType2 = myType1;
		myRegister2 = myRegister1;
		myType1 = readTerm( currentProcedure, myRegister1 );
		operandCount++;
		
		// Update the data type of the ArithOp
		if( operandCount == 1 )
		{
			relationType = myType1;
		}
		else if( operandCount > 1 )
		{
			relationType = BOOL;
			
			// CODEGEN: Generate lines for computing multiplication or division
			// **** Add code for data conversion check for integers in boolean expression
			if( errorCount == 0 )
			{
				outFile << "\tR[" << myRegister1 << "].intVal = R[" << myRegister1 << "].intVal " << operation << " R[" << myRegister2 << "].intVal;" << endl;
			}
		}
		else
		{
			relationType = INVALID;
		}
		
		// check if there is a relational operator next
		if( currentToken.name.compare( "<" ) == 0 || currentToken.name.compare( ">=" ) == 0 || currentToken.name.compare( "<=" ) == 0 || currentToken.name.compare( ">" ) == 0 || currentToken.name.compare( "==" ) == 0 || currentToken.name.compare( "!=" ) == 0 )
		{
			restricted = true;
			operation = currentToken.name;
			
			// Advance Token to after the relational operator
			currentToken = nextToken;
			nextToken = getToken();
		}
		else
		{
			terminate = true;
		}
		
		// Check the data type of the operand
		if( restricted )
		{
			switch( myType1 )
			{
				case BOOL:
				case INTEGER:
					break;
					
				default:
					reportError( "Operand of relational expression must be a boolean or an integer" );
					break;
			}
		}
	} while( terminate == false );
	
	resultRegister = myRegister1;
	return relationType;
}

DataType readTerm( Procedure*& currentProcedure, int& resultRegister )
{
	DataType myType1 = INVALID; // Data type for an operand
	DataType myType2 = INVALID; // Data type for another operand
	DataType termType = INVALID; // Data type for the whole term
	bool restricted = false; // flags whether there was a * or / in the term
	int operandCount = 0; // Counts number of operands
	string operation; // Stores the selected operation (multiplication or division)
	
	// flag to check when we've  reached the end of the Term
	bool terminate = false;
	
	do
	{
		myType2 = myType1;
		myType1 = readFactor( currentProcedure, resultRegister );
		operandCount++;
		
		// Update the data type of the expression
		if( operandCount == 1 )
		{
			termType = myType1;
		}
		else if( operandCount > 1 )
		{
			if( termType < myType1 )
			{
				termType = myType1;
			}
			
			// CODEGEN: Generate lines for computing multiplication or division
			if( errorCount == 0 )
			{
				switch( termType )
				{
					case FLOAT:
						if( myType1 == FLOAT && myType2 == INTEGER )
						{
							outFile << "\tR[" << registerPointer - 1 << "].floatVal = R[" << registerPointer - 1 << "].floatVal " << operation << " R[" << registerPointer - 2 << "].intVal;" << endl;
						}
						else if( myType1 == INTEGER && myType2 == FLOAT )
						{
							outFile << "\tR[" << registerPointer - 1 << "].floatVal = R[" << registerPointer - 1 << "].intVal " << operation << " R[" << registerPointer - 2 << "].floatVal;" << endl;
						}
						break;
						
					case INTEGER:
						outFile << "\tR[" << registerPointer - 1 << "].intVal = R[" << registerPointer - 1 << "].intVal " << operation << " R[" << registerPointer - 2 << "].intVal;" << endl;
						break;
						
					default:
						break;
				}
			}
		}
		else
		{
			termType = INVALID;
		}
		
		// check if there is a "*" or "/" next
		if( currentToken.name.compare( "*" ) == 0 || currentToken.name.compare( "/" ) == 0 )
		{
			restricted = true;
			operation = currentToken.name;
			
			// Advance Token to after the "*"
			currentToken = nextToken;
			nextToken = getToken();
		}
		else
		{
			terminate = true;
		}
		
		// Check the data type of the operand
		if( restricted )
		{
			switch( myType1 )
			{
				case FLOAT:
				case INTEGER:
					break;
					
				default:
					reportError( "Operand of arithmetic expression must be a float or an integer" );
					break;
			}
		}
	} while( terminate == false );
	
	// resultRegister got set by readFactor
	return termType;
}

DataType readFactor( Procedure*& currentProcedure, int& resultRegister )
{
	Token* myToken = NULL;
	Variable* myVariable = NULL;
	DataType factorType = INVALID;
	
	// Check for parenthetical expression
	if( currentToken.name.compare( "(" ) == 0 )
	{
		// Advance Token to after "("
		currentToken = nextToken;
		nextToken = getToken();
		
		factorType = readExpression( currentProcedure, resultRegister );
		
		// Check for ")" after expression
		if( currentToken.name.compare( ")" ) == 0 )
		{
			// Advance Token to after ")"
			currentToken = nextToken;
			nextToken = getToken();
		}
		else
		{
			throw CompileErrorException( "Mismatched parentheses" );
		}
	}
	// Check for negation
	else if( currentToken.name.compare( "-" ) == 0 )
	{
		// Advance Token to after "-"
		currentToken = nextToken;
		nextToken = getToken();
		
		if( currentToken.tokenType == IDENTIFIER )
		{
			factorType = readName( currentProcedure, resultRegister );
			
			// CODEGEN: Negate the variable value in the register
			if( errorCount == 0 )
			{
				switch( factorType )
				{
					case BOOL:
						outFile << "\tR[" << resultRegister  << "].intVal = !R[" << resultRegister << "].intVal;" << endl;
						break;
						
					case INTEGER:
						outFile << "\tR[" << resultRegister  << "].intVal = -1 * R[" << resultRegister << "].intVal;" << endl;
						break;
						
					case FLOAT:
						outFile << "\tR[" << resultRegister  << "].floatVal = -1 * R[" << resultRegister << "].floatVal;" << endl;
						break;
						
					default:
						reportError( "Invalid data type to negate" );
						break;
				}
			}
		}
		else if( currentToken.tokenType == NUMBER )
		{
			// If there is a decimal point in the number, this is a float
			if( currentToken.name.find_first_of( '.' ) != string::npos )
			{
				factorType = FLOAT;
				
				// CODEGEN: Put the negated number in a register
				if( errorCount == 0 )
				{
					outFile << "\tR[" << registerPointer << "].floatVal = -1 * " << currentToken.name << ";" << endl;
					resultRegister = registerPointer;
					registerPointer++;
				}
			}
			else // Otherwise it's an integer
			{
				factorType = INTEGER;
				
				// CODEGEN: Put the negated number in a register
				if( errorCount == 0 )
				{
					outFile << "\tR[" << registerPointer << "].intVal = -1 * " << currentToken.name << ";" << endl;
					resultRegister = registerPointer;
					registerPointer++;
				}
			}
			
			// Advance Token to after NUMBER
			currentToken = nextToken;
			nextToken = getToken();
		}
		else
		{
			throw CompileErrorException( "Invalid operand for \'-\': " + currentToken.name );
		}
	}
	else if( currentToken.tokenType == IDENTIFIER )
	{
		factorType = readName( currentProcedure, resultRegister );
	}
	else if( currentToken.tokenType == NUMBER )
	{
		// If there is a decimal point in the number, this is a float
		if( currentToken.name.find_first_of( '.' ) != string::npos )
		{
			factorType = FLOAT;
			
			// CODEGEN: Put the number in a register
			if( errorCount == 0 )
			{
				outFile << "\tR[" << registerPointer << "].floatVal = " << currentToken.name << ";" << endl;
				resultRegister = registerPointer;
				registerPointer++;
			}
		}
		else // Otherwise it's an integer
		{
			factorType = INTEGER;
			
			// CODEGEN: Put the number in a register
			if( errorCount == 0 )
			{
				outFile << "\tR[" << registerPointer << "].intVal = " << currentToken.name << ";" << endl;
				resultRegister = registerPointer;
				registerPointer++;
			}
		}
		
		// Advance Token to after NUMBER
		currentToken = nextToken;
		nextToken = getToken();
	}
	else if( currentToken.tokenType == STRING )
	{
		factorType = STRINGT;
		
		// Check if the string literal is already in the symbol table
		findSymbolEntry( currentToken );
		
		if( currentToken.tokenType == NONE )
		{
			myVariable = new Variable( STRING, currentToken.name, STRINGT, true, memoryPointer, false );
			addSymbolEntry( myVariable );
			
			// CODEGEN: Generate code to put literal strings in memory. (hold for output later)
			if( errorCount == 0 )
			{
				ostringstream convert;
				
				for( int i = 1; i < currentToken.name.size() - 1; i++ )
				{
					switch( currentToken.name[i] )
					{
						case '\'':
							convert << "\tR[2].charVal = \'\\\'\';" << endl;
							break;
							
						case '\"':
							convert << "\tR[2].charVal = \'\\\"\';" << endl;
							break;
							
						default:
							convert << "\tR[2].charVal = \'" << currentToken.name[i] << ";" << endl;
							break;
					}
					
					convert << "\tMM[" << memoryPointer + i - 1 << "] = R[2];" << endl;
				}
				
				convert << "\tR[2].charVal = \'\\0\';" << endl;
				convert << "\tMM[" << memoryPointer + currentToken.name.size() - 2 << "] = R[2];" << endl;
				
				literalStorage += convert.str();
			}
			
			memoryPointer += ( currentToken.name.size() - 1 );
		}
		else if( currentToken.tokenType == STRING )
		{
			myToken = globalSymbolTable[currentToken.name];
			if( typeid( *myToken ) == typeid( Variable ) )
			{
				myVariable = dynamic_cast<Variable*>(myToken);
			}
			
			// CODEGEN: Load the address of the string literal into a register
			if( errorCount == 0 )
			{
				outFile << "\tR[" << registerPointer << "].stringPointer = " << myVariable->getAddress() << ";" << endl;
				resultRegister = registerPointer;
				registerPointer++;
			}
		}
		
		// Advance Token to after STRING
		currentToken = nextToken;
		nextToken = getToken();
	}
	else if( currentToken.name.compare( "true" ) == 0 )
	{
		factorType = BOOL;
		
		// CODEGEN: Put "true" in a register as 1
		if( errorCount == 0 )
		{
			outFile << "\tR[" << registerPointer << "].intVal = 1;" << endl;
			resultRegister = registerPointer;
			registerPointer++;
		}
		
		// Advance Token to after "true" or "false"
		currentToken = nextToken;
		nextToken = getToken();
	}
	else if( currentToken.name.compare( "false" ) == 0 )
	{
		factorType = BOOL;
		
		// CODEGEN: Put "false" in a register as 0
		if( errorCount == 0 )
		{
			outFile << "\tR[" << registerPointer << "].intVal = 0;" << endl;
			resultRegister = registerPointer;
			registerPointer++;
		}
		
		// Advance Token to after "true" or "false"
		currentToken = nextToken;
		nextToken = getToken();
	}
	else
	{
		throw CompileErrorException( "Invalid factor: " + currentToken.name );
	}
	
	if( isArgument )
	{
		argumentOperands++;
	}
	
	return factorType;
}

DataType readName( Procedure*& currentProcedure, int& resultRegister )
{
	Token* myName = NULL;
	Variable* myVariable = NULL;
	Array* myArray = NULL;
	DataType nameType = INVALID;
	int tempArgumentOperands = 0;
	
	// currentToken is the identifier. Get its symbol table entry
	if( currentToken.isGlobal )
	{
		myName = globalSymbolTable[currentToken.name];
	}
	else
	{
		myName = localSymbolTable[currentScope][currentToken.name];
	}
	
	if( typeid( *myName ) == typeid( Variable ) )
	{
		myVariable = dynamic_cast<Variable*>(myName);
		nameType = myVariable->getDataType();
	}
	else
	{
		reportError( "\'" + myName->getName() + "\' is not a valid variable" );
	}
	
	if( isArgument )
	{
		argumentName = myVariable;
	}
	
	// Advance Token to after IDENTIFIER
	currentToken = nextToken;
	nextToken = getToken();
	
	// Check if there is a "[" for an array element
	if( currentToken.name.compare( "[" ) == 0 )
	{
		if( typeid( *myVariable ) != typeid( Array ) )
		{
			reportError( "\'" + myVariable->getName() + "\' is not an array" );
		}
		else
		{
			myArray = dynamic_cast<Array*>(myVariable);
		}
		
		// Advance Token to after "["
		currentToken = nextToken;
		nextToken = getToken();
		
		tempArgumentOperands = argumentOperands;
		if( readExpression( currentProcedure, resultRegister ) != INTEGER )
		{
			reportError( "Array index must evaluate to an integer" );
		}
		argumentOperands = tempArgumentOperands;
		
		// CODEGEN: Load the array element into a register
		if( errorCount == 0 )
		{
			outFile << "\tR[" << registerPointer << "] = MM[R[" << resultRegister << "].intVal + " << myArray->getAddress() << "];" << endl;
			
			if( isArgument )
			{
				outFile << "\tMM[" << arrayIndexPointer << "].intVal = R[" << resultRegister << "].intVal;" << endl;
			}
			
			resultRegister = registerPointer;
			registerPointer++;
		}
		
		// Check for "]" after expression
		if( currentToken.name.compare( "]" ) == 0 )
		{
			// Advance Token to after "]"
			currentToken = nextToken;
			nextToken = getToken();
		}
		else
		{
			throw CompileErrorException( "Mismatched square brackets for array index" );
		}
	}
	// CODEGEN: Load the variable into a register
	else if( errorCount == 0 )
	{
		if( typeid( *myVariable ) == typeid( Array ) )
		{
			reportWarning( "No array index specified for " + myVariable->getName() );
		}
		
		if( myVariable->getGlobal() )
		{
			outFile << "\tR[" << registerPointer << "] = MM[" << myVariable->getAddress() << "];" << endl;
		}
		else
		{
			if( myVariable->getParameter() )
			{
				outFile << "\tR[" << registerPointer << "] = MM[R[0].intVal + " << currentProcedure->getLocalAddress() + myVariable->getAddress() << "];" << endl;
			}
			else
			{
				outFile << "\tR[" << registerPointer << "] = MM[R[0].intVal + " << myVariable->getAddress() << "];" << endl;
			}
		}
		
		resultRegister = registerPointer;
		registerPointer++;
	}
	
	return nameType;
}

// This function runs after the parse has successfully completed without errors
// It adds code for the input code to call the runtime functions.
void generateRuntime( void )
{
	outFile << "\tgetBool_start:" << endl;
	outFile << "\tMM[R[0].intVal].intVal = getBool();" << endl;
	outFile << "\tjumpRegister = MM[R[0].intVal + 1].jumpTarget;" << endl;
	outFile << "\tgoto *jumpRegister;" << endl << endl;
	
	outFile << "\tgetInteger_start:" << endl;
	outFile << "\tMM[R[0].intVal].intVal = getInteger();" << endl;
	outFile << "\tjumpRegister = MM[R[0].intVal + 1].jumpTarget;" << endl;
	outFile << "\tgoto *jumpRegister;" << endl << endl;
	
	outFile << "\tgetFloat_start:" << endl;
	outFile << "\tMM[R[0].intVal].floatVal = getFloat();" << endl;
	outFile << "\tjumpRegister = MM[R[0].intVal + 1].jumpTarget;" << endl;
	outFile << "\tgoto *jumpRegister;" << endl << endl;
	
	outFile << "\tgetString_start:" << endl;
	outFile << "\tMM[R[0].intVal].stringPointer = getString();" << endl;
	outFile << "\tjumpRegister = MM[R[0].intVal + 1].jumpTarget;" << endl;
	outFile << "\tgoto *jumpRegister;" << endl << endl;
	
	outFile << "\tputBool_start:" << endl;
	outFile << "\tputBool( MM[R[0].intVal].intVal );" << endl;
	outFile << "\tjumpRegister = MM[R[0].intVal + 1].jumpTarget;" << endl;
	outFile << "\tgoto *jumpRegister;" << endl << endl;
	
	outFile << "\tputInteger_start:" << endl;
	outFile << "\tputInteger( MM[R[0].intVal].intVal );" << endl;
	outFile << "\tjumpRegister = MM[R[0].intVal + 1].jumpTarget;" << endl;
	outFile << "\tgoto *jumpRegister;" << endl << endl;
	
	outFile << "\tputFloat_start:" << endl;
	outFile << "\tputFloat( MM[R[0].intVal].floatVal );" << endl;
	outFile << "\tjumpRegister = MM[R[0].intVal + 1].jumpTarget;" << endl;
	outFile << "\tgoto *jumpRegister;" << endl << endl;
	
	outFile << "\tputString_start:" << endl;
	outFile << "\tputString( MM[R[0].intVal].stringPointer );" << endl;
	outFile << "\tjumpRegister = MM[R[0].intVal + 1].jumpTarget;" << endl;
	outFile << "\tgoto *jumpRegister;" << endl << endl;
	
	outFile << "\truntimeerror:" << endl;
	outFile << "\tputString( 0 );" << endl;
	
	outFile << "}" << endl << endl;
	
	outFile << "#include \"runtime.c\"" << endl;
	outFile << endl;	
}
