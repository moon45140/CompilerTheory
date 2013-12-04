// Filename: parser.cpp
// Author: Himanshu Narayana
// This file is the Parser for the Compiler Project in Professor Wilsey's Compiler Theory class.
// The parser performs grammar/syntax checking on the input source file in the style of an LL(1) recursive decent compiler.

#include "compiler.h"

using namespace std;

TokenFrame currentToken;
TokenFrame nextToken;

// This function begins parsing of the grammar/syntax with the first grammar rule
void readProgram()
{
	try
	{
		readProgramHeader(); // First read the program header
		readProgramBody(); // Next, read the program body
	}
	catch( CompileErrorException& e )
	{
		// Display the compiler's error message
		reportError( e.what() );
	}
}

void readProgramHeader()
{
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
			addSymbolEntry( new Token( RESERVE, currentToken.name, true ) );
			
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

void readProgramBody()
{
	// currentToken is pointing to first declaration or begin
	
	// Check if there are any declarations
	if( currentToken.name.compare( "global" ) == 0 || currentToken.name.compare( "procedure" ) == 0 || currentToken.name.compare( "integer" ) == 0 || currentToken.name.compare( "float" ) == 0 || currentToken.name.compare( "bool" ) == 0 || currentToken.name.compare( "string" ) == 0 )
	{
		readDeclarations();
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
	
	// Look for block of statements
	if( currentToken.tokenType == IDENTIFIER || currentToken.name.compare( "if" ) == 0 || currentToken.name.compare( "for" ) == 0 || currentToken.name.compare( "return" ) == 0 || currentToken.tokenType == NONE )
	{
		readStatements();
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

void readDeclarations()
{
	// currentToken is pointing to the first declaration
	
	while( inFile.good() )
	{
		// Check if it's a global declaration
		if( currentToken.name.compare( "global" ) == 0 )
		{
			isGlobal = true;
			
			// Advance Token for after "global"
			currentToken = nextToken;
			nextToken = getToken();
		}
		
		// Check if it's a procedure declaration
		if( currentToken.name.compare( "procedure" ) == 0 )
		{
			readProcedureDeclaration();
			
			isGlobal = false;
		}
		// Check if it's a variable declaration
		else if( currentToken.name.compare( "integer" ) == 0 || currentToken.name.compare( "float" ) == 0 || currentToken.name.compare( "bool" ) == 0 || currentToken.name.compare( "string" ) == 0 )
		{
			isParameter = false;
			readVariableDeclaration();
			
			isGlobal = false;
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

void readProcedureDeclaration()
{
	Token* myProcedure = 0;
	SymbolTable::iterator janitor;
	bool checkName;
	int nestedCount;
	
	currentProcedure = 0;
	
	try
	{
		// create new scope and its associated symbol table
		currentScope++;
		localSymbolTable.push_back( SymbolTable() );
		localSymbolTable[currentScope].clear();
		
		readProcedureHeader(); // First read the procedure header
		
		myProcedure = currentProcedure;
		
		readProcedureBody(); // Second, read the procedure body
	}
	catch( CompileErrorException& e )
	{
		myProcedure = currentProcedure;
		
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
	if( myProcedure == 0 )
	{
		throw CompileErrorException( "Unable to remove scope" );
	}
	else
	{
		for( janitor = localSymbolTable[currentScope].begin(); janitor != localSymbolTable[currentScope].end(); ++janitor )
		{
			if( janitor->second->getName().compare( myProcedure->getName() ) == 0 )
			{
				checkName = true;
			}
			else
			{
				checkName = false;
			}
			
			if( checkName == false && janitor->second->getGlobal() == false )
			{
				delete janitor->second;
			}
			else
			{
			}
			janitor->second = 0;
		}
	}
	
	localSymbolTable[currentScope].clear();
	localSymbolTable.pop_back();
	currentScope--;
}

void readProcedureHeader()
{
	string myName; // stores the name of the procedure
	bool location = isGlobal; // true if the procedure was declared global
	
	isGlobal = false;
	
	// Advance Token to after "procedure"
	currentToken = nextToken;
	nextToken = getToken();
	
	// Next token should be an identifier
	if( currentToken.tokenType == NONE )
	{
		myName = currentToken.name;
		currentProcedure = new Procedure( IDENTIFIER, currentToken.name, location );
		
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
	
	// Read the Parameter List (starts with a type mark if it is not an empty list)
	if( currentToken.name.compare( "integer" ) == 0 || currentToken.name.compare( "float" ) == 0 || currentToken.name.compare( "bool" ) == 0 || currentToken.name.compare( "string" ) == 0 )
	{
		readParameterList();
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
	if( location )
	{
		globalSymbolTable[myName] = currentProcedure;
	}
	else
	{
		localSymbolTable[currentScope - 1][myName] = currentProcedure;
	}
}

void readParameterList()
{
	readParameter();
	
	if( currentToken.name.compare( "," ) == 0 )
	{
		// Advance Token to after ","
		currentToken = nextToken;
		nextToken = getToken();
		
		readParameterList();
	}
}

void readParameter()
{
	isParameter = true;
	readVariableDeclaration();
	isParameter = false;
	
	if( currentToken.name.compare( "in" ) == 0 || currentToken.name.compare( "out" ) == 0 )
	{
		// Advance Token to after "in" or "out"
		currentToken = nextToken;
		nextToken = getToken();
	}
	else
	{
		throw CompileErrorException( "Invalid parameter direction: " + currentToken.name );
	}
}

void readProcedureBody()
{
	// Check if there are any declarations
	if( currentToken.name.compare( "global" ) == 0 || currentToken.name.compare( "procedure" ) == 0 || currentToken.name.compare( "integer" ) == 0 || currentToken.name.compare( "float" ) == 0 || currentToken.name.compare( "bool" ) == 0 || currentToken.name.compare( "string" ) == 0 )
	{
		readDeclarations();
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

	// Look for block of statements
	if( currentToken.tokenType == IDENTIFIER || currentToken.name.compare( "if" ) == 0 || currentToken.name.compare( "for" ) == 0 || currentToken.name.compare( "return" ) == 0 || currentToken.tokenType == NONE )
	{
		readStatements();
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

void readVariableDeclaration()
{
	string myName;
	DataType myDataType = INVALID;
	int myArraySize = 0;
	
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
				stringstream convert( currentToken.name );
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
			
			addSymbolEntry( new Array( IDENTIFIER, myName, myDataType, myArraySize, isGlobal ) );
		}
		else
		{
			addSymbolEntry( new Variable( IDENTIFIER, myName, myDataType, isGlobal ) );
		}
		
		if( isParameter )
		{
			currentProcedure->addParameter( myDataType );
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

void readStatements()
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
					readProcedureCall();
				}
				// If next token is ":=" or "[", then this is an assignment statement
				else if( nextToken.name.compare( ":=" ) == 0 || nextToken.name.compare( "[" ) == 0 )
				{
					readAssignment();
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
				
				readIf();
			}
			// Check if it's a loop statement
			else if( currentToken.name.compare( "for" ) == 0 )
			{
				// Advance Token to after "for"
				currentToken = nextToken;
				nextToken = getToken();
				
				readLoop();
			}
			// Check if it's a return statement
			else if( currentToken.name.compare( "return" ) == 0 )
			{
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
	}
}

void readProcedureCall()
{
	TokenFrame calledProcedure = currentToken;
	Token* myProcedure = 0;
	int argumentCount = 0;
	
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
		myProcedure = globalSymbolTable[calledProcedure.name];
	}
	else
	{
		myProcedure = localSymbolTable[currentScope][calledProcedure.name];
	}
	
	// Check for null pointer
	if( myProcedure == 0 )
	{
		throw CompileErrorException( "Unable to locate procedure \'" + calledProcedure.name + "\'" );
	}
	else
	{
		if( myProcedure->getParameterListSize() < 0 )
		{
			throw CompileErrorException( "\'" + myProcedure->getName() + "\' is not a procedure" );
		}
	}
	
	// Advance Token to after "("
	currentToken = getToken();
	nextToken = getToken();
	
	// Check if the argument list contains the start of an expression
	if( currentToken.name.compare( "(" ) == 0 || currentToken.name.compare( "-" ) == 0 || currentToken.tokenType == IDENTIFIER || currentToken.tokenType == NUMBER || currentToken.tokenType == STRING || currentToken.name.compare( "true" ) == 0 || currentToken.name.compare( "false" ) == 0 )
	{
		readArgumentList( dynamic_cast<Procedure*>(myProcedure), 0, argumentCount );
		
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
}

void readArgumentList( Procedure* myProcedure, const int& parameterNumber, int& argumentCount )
{
	// Check if there's an entry in the parameter list to match this argument
	if( parameterNumber >= myProcedure->getParameterListSize() )
	{
		throw CompileErrorException( "Too many arguments in procedure call" );
	}
	
	// Parse the argument and check types
	if( readExpression() != myProcedure->getParameter( parameterNumber ) )
	{
		stringstream convert;
		convert << parameterNumber;
		reportError( "Incompatible data type in argument " + convert.str() );
	}
	
	// Increment the counter after successfully parsing and checking the argument
	argumentCount++;
	
	if( currentToken.name.compare( "," ) == 0 )
	{
		// Advance Token to after ","
		currentToken = nextToken;
		nextToken = getToken();
		
		// If there was a comma, expect another argument
		readArgumentList( myProcedure, parameterNumber + 1, argumentCount );
	}
}

void readAssignment()
{
	DataType destinationType = INVALID;
	DataType expressionType = INVALID;
	
	try
	{
		destinationType = readName();
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
	
	expressionType = readExpression();
	
	switch( destinationType )
	{
		case BOOL:
			switch( expressionType )
			{
				case BOOL:
				case INTEGER:
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
				case INTEGER:
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
				case FLOAT:
				case INTEGER:
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
			break;
			
		default:
			reportError( "Unknown data type in destination of assignment statement" );
			break;
	}
}

void readIf()
{
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
		switch( readExpression() )
		{
			case BOOL:
			case INTEGER:
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
		readStatements();
		
		// check if there is an "else" section
		if( currentToken.name.compare( "else" ) == 0 )
		{
			// Advance Token to after "else"
			currentToken = nextToken;
			nextToken = getToken();
			
			// next is one or more statements
			readStatements();
		}
		
		// finally, look for "end if"
		if( currentToken.name.compare( "end" ) == 0 )
		{
			// Advance Token to after "end"
			currentToken = nextToken;
			nextToken = getToken();
			
			if( currentToken.name.compare( "if" ) == 0 )
			{
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

void readLoop()
{
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
			readAssignment();
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
		switch( readExpression() )
		{
			case BOOL:
			case INTEGER:
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
			readStatements();
		}
		
		// finally, look for "end for"
		if( currentToken.name.compare( "end" ) == 0 )
		{
			// Advance Token to after "end"
			currentToken = nextToken;
			nextToken = getToken();
			
			if( currentToken.name.compare( "for" ) == 0 )
			{
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

DataType readExpression()
{
	DataType myType = INVALID; // Data type for an operand
	DataType expressionType = INVALID; // Data type for the whole expression
	bool restricted = false; // flags whether there was a & or | operator in the expression
	int operandCount = 0; // Counts number of operands
	
	// flag to check when we've reached the end of the expression
	bool terminate = false;
	
	// Check if there is a "not"
	if( currentToken.name.compare( "not" ) == 0 )
	{
		terminate = true;
		
		// Advance Token to after "not"
		currentToken = nextToken;
		nextToken = getToken();
		
		expressionType = readArithOp();
		switch( myType )
		{
			case INTEGER:
				break;
				
			default:
				reportError( "Operand of \'not\' must be an integer" );
				break;
		}
		
		// if there was a "not", grammar rule specifies no operator afterwards
	}
	else
	{
		do
		{
			myType = readArithOp();
			operandCount++;
			
			// check if there is a "&" or "|" next
			if( currentToken.name.compare( "&" ) == 0 || currentToken.name.compare( "|" ) == 0 )
			{
				restricted = true;
				
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
				switch( myType )
				{
					case INTEGER:
						break;
						
					default:
						reportError( "Operand of \'" + currentToken.name + "\' must be an integer" );
						break;
				}
			}
			
			// Update the data type of the expression
			if( operandCount == 1 )
			{
				expressionType = myType;
			}
			else if( operandCount > 1 )
			{
				expressionType = INTEGER;
			}
			else
			{
				expressionType = INVALID;
			}
		} while( terminate == false );
	}
	
	return expressionType;
}

DataType readArithOp()
{
	DataType myType = INVALID; // Data type for an operand
	DataType arithType = INVALID; // Data type for the whole ArithOp
	bool restricted = false; // flags whether there was a + or - in the ArithOp
	int operandCount = 0; // Counts number of operands
	
	// flag to check when we've  reached the end of the ArithOp
	bool terminate = false;
	
	do
	{
		myType = readRelation();
		operandCount++;
		
		// check if there is a "+" or "-" next
		if( currentToken.name.compare( "+" ) == 0 || currentToken.name.compare( "-" ) == 0 )
		{
			restricted = true;
			
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
			switch( myType )
			{
				case FLOAT:
				case INTEGER:
					break;
					
				default:
					reportError( "Operand of \'" + currentToken.name + "\' must be an integer or a float" );
					break;
			}
		}
		
		// Update the data type of the ArithOp
		if( operandCount == 1 )
		{
			arithType = myType;
		}
		else if( operandCount > 1 )
		{
			if( arithType < myType )
			{
				arithType = myType;
			}
		}
		else
		{
			arithType = INVALID;
		}
	} while( terminate == false );
	
	return arithType;
}

DataType readRelation()
{
	DataType myType = INVALID; // Data type for an operand
	DataType relationType = INVALID; // Data type for the whole relation
	bool restricted = false; // flags whether there was a relational operator
	int operandCount = 0; // Counts number of operands
	
	// flag to check when we've  reached the end of the Relation
	bool terminate = false;
	
	do
	{
		myType = readTerm();
		operandCount++;
		
		// check if there is an inequality symbol next
		if( currentToken.name.compare( "<" ) == 0 || currentToken.name.compare( ">=" ) == 0 || currentToken.name.compare( "<=" ) == 0 || currentToken.name.compare( ">" ) == 0 || currentToken.name.compare( "==" ) == 0 || currentToken.name.compare( "!=" ) == 0 )
		{
			restricted = true;
			
			// Advance Token to after the inequality symbol
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
			switch( myType )
			{
				case BOOL:
				case INTEGER:
					break;
					
				default:
					reportError( "Operand of \'" + currentToken.name + "\' must be a boolean or an integer" );
					break;
			}
		}
		
		// Update the data type of the ArithOp
		if( operandCount == 1 )
		{
			relationType = myType;
		}
		else if( operandCount > 1 )
		{
			if( relationType < myType )
			{
				relationType = myType;
			}
		}
		else
		{
			relationType = INVALID;
		}
	} while( terminate == false );
	
	return relationType;
}

DataType readTerm()
{
	DataType myType = INVALID; // Data type for an operand
	DataType termType = INVALID; // Data type for the whole term
	bool restricted = false; // flags whether there was a * or / in the term
	int operandCount = 0; // Counts number of operands
	
	// flag to check when we've  reached the end of the Term
	bool terminate = false;
	
	do
	{
		myType = readFactor();
		operandCount++;
		
		// check if there is a "*" or "/" next
		if( currentToken.name.compare( "*" ) == 0 || currentToken.name.compare( "/" ) == 0 )
		{
			restricted = true;
			
			// Advance Token to after the "*" or "/"
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
			switch( myType )
			{
				case FLOAT:
				case INTEGER:
					break;
					
				default:
					reportError( "Operand of \'" + currentToken.name + "\' must be a float or an integer" );
					break;
			}
		}
		
		// Update the data type of the expression
		if( operandCount == 1 )
		{
			termType = myType;
		}
		else if( operandCount > 1 )
		{
			if( termType < myType )
			{
				termType = myType;
			}
		}
		else
		{
			termType = INVALID;
		}
	} while( terminate == false );
	
	return termType;
}

DataType readFactor()
{
	DataType factorType = INVALID;
	
	// Check for parenthetical expression
	if( currentToken.name.compare( "(" ) == 0 )
	{
		// Advance Token to after "("
		currentToken = nextToken;
		nextToken = getToken();
		
		factorType = readExpression();
		
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
			factorType = readName();
		}
		else if( currentToken.tokenType == NUMBER )
		{
			// If there is a decimal point in the number, this is a float
			if( currentToken.name.find_first_of( '.' ) != string::npos )
			{
				factorType = FLOAT;
			}
			else // Otherwise it's an integer
			{
				factorType = INTEGER;
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
		factorType = readName();
	}
	else if( currentToken.tokenType == NUMBER )
	{
		// If there is a decimal point in the number, this is a float
		if( currentToken.name.find_first_of( '.' ) != string::npos )
		{
			factorType = FLOAT;
		}
		else // Otherwise it's an integer
		{
			factorType = INTEGER;
		}
		
		// Advance Token to after NUMBER
		currentToken = nextToken;
		nextToken = getToken();
	}
	else if( currentToken.tokenType == STRING )
	{
		factorType = STRINGT;
		
		// Advance Token to after STRING
		currentToken = nextToken;
		nextToken = getToken();
	}
	else if( currentToken.name.compare( "true" ) == 0 || currentToken.name.compare( "false" ) == 0 )
	{
		factorType = BOOL;
		
		// Advance Token to after "true" or "false"
		currentToken = nextToken;
		nextToken = getToken();
	}
	else
	{
		throw CompileErrorException( "Invalid factor: " + currentToken.name );
	}
	
	return factorType;
}

DataType readName()
{
	Token* myName = 0;
	DataType nameType = INVALID;
	
	// currentToken is the identifier. Get its symbol table entry
	if( currentToken.isGlobal )
	{
		myName = globalSymbolTable[currentToken.name];
	}
	else
	{
		myName = localSymbolTable[currentScope][currentToken.name];
	}
	
	nameType = myName->getDataType();
	
	if( myName->getDataType() == INVALID )
	{
		reportError( "\'" + myName->getName() + "\' is not a valid variable" );
	}
	
	// Advance Token to after IDENTIFIER
	currentToken = nextToken;
	nextToken = getToken();
	
	// Check if there is a "[" for an array element
	if( currentToken.name.compare( "[" ) == 0 )
	{
		if( myName->getArraySize() < 1 )
		{
			reportError( "\'" + myName->getName() + "\' is not an array" );
		}
		
		// Advance Token to after "["
		currentToken = nextToken;
		nextToken = getToken();
		
		if( readExpression() != INTEGER )
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
	}
	
	return nameType;
}