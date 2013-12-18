// Filename: scanner.cpp
// Author: Himanshu Narayana
// This file is the Scanner for the Compiler Project in Professor Wilsey's Compiler Theory class.
// The scanner reads an input source code file, tokenizes it, and outputs the tokens.
// The specification for the tokens is in the project description on Professor Wilsey's website.

#include "compiler.h"

using namespace std;

// This function initializes global counters and sets up file I/O for the scanner
void initializeScanner( const char* inputFile )
{
	Token* myToken = NULL;
	Variable* myVariable = NULL;
	Procedure* myProcedure = NULL;
	lineNumber = 1;
	warningCount = 0;
	errorCount = 0;
	currentScope = 0;
	
	inFile.open( inputFile, ios::in );
	
	// Make sure the global symbol table starts out empty
	globalSymbolTable.clear();
	
	// Make sure the vector of local symbol tables starts with one element and empty it.
	localSymbolTable.clear();
	localSymbolTable.push_back( SymbolTable() );
	localSymbolTable[currentScope].clear();
	
	// Populate the symbol table with the reserve words and operators
	myToken = new Token( RESERVE, "and", true );
	addSymbolEntry( myToken );
	
	myToken = new Token( RESERVE, "begin", true );
	addSymbolEntry( myToken );
	
	myToken = new Token( RESERVE, "bool", true );
	addSymbolEntry( myToken );
	
	myToken = new Token( RESERVE, "case", true );
	addSymbolEntry( myToken );
	
	myToken = new Token( RESERVE, "else", true );
	addSymbolEntry( myToken );
	
	myToken = new Token( RESERVE, "end", true );
	addSymbolEntry( myToken );
	
	myToken = new Token( RESERVE, "false", true );
	addSymbolEntry( myToken );
	
	myToken = new Token( RESERVE, "float", true );
	addSymbolEntry( myToken );
	
	myToken = new Token( RESERVE, "for", true );
	addSymbolEntry( myToken );
	
	myToken = new Token( RESERVE, "global", true );
	addSymbolEntry( myToken );
	
	myToken = new Token( RESERVE, "if", true );
	addSymbolEntry( myToken );
	
	myToken = new Token( RESERVE, "in", true );
	addSymbolEntry( myToken );
	
	myToken = new Token( RESERVE, "integer", true );
	addSymbolEntry( myToken );
	
	myToken = new Token( RESERVE, "is", true );
	addSymbolEntry( myToken );
	
	myToken = new Token( RESERVE, "not", true );
	addSymbolEntry( myToken );
	
	myToken = new Token( RESERVE, "or", true );
	addSymbolEntry( myToken );
	
	myToken = new Token( RESERVE, "out", true );
	addSymbolEntry( myToken );
	
	myToken = new Token( RESERVE, "procedure", true );
	addSymbolEntry( myToken );
	
	myToken = new Token( RESERVE, "program", true );
	addSymbolEntry( myToken );
	
	myToken = new Token( RESERVE, "return", true );
	addSymbolEntry( myToken );
	
	myToken = new Token( RESERVE, "string", true );
	addSymbolEntry( myToken );
	
	myToken = new Token( RESERVE, "then", true );
	addSymbolEntry( myToken );
	
	myToken = new Token( RESERVE, "true", true );
	addSymbolEntry( myToken );
	
	myToken = new Token( OPERATOR, ":", true );
	addSymbolEntry( myToken );
	
	myToken = new Token( OPERATOR, ";", true );
	addSymbolEntry( myToken );
	
	myToken = new Token( OPERATOR, ",", true );
	addSymbolEntry( myToken );
	
	myToken = new Token( OPERATOR, "+", true );
	addSymbolEntry( myToken );
	
	myToken = new Token( OPERATOR, "-", true );
	addSymbolEntry( myToken );
	
	myToken = new Token( OPERATOR, "*", true );
	addSymbolEntry( myToken );
	
	myToken = new Token( OPERATOR, "/", true );
	addSymbolEntry( myToken );
	
	myToken = new Token( OPERATOR, "(", true );
	addSymbolEntry( myToken );
	
	myToken = new Token( OPERATOR, ")", true );
	addSymbolEntry( myToken );
	
	myToken = new Token( OPERATOR, "<", true );
	addSymbolEntry( myToken );
	
	myToken = new Token( OPERATOR, "<=", true );
	addSymbolEntry( myToken );
	
	myToken = new Token( OPERATOR, ">", true );
	addSymbolEntry( myToken );
	
	myToken = new Token( OPERATOR, ">=", true );
	addSymbolEntry( myToken );
	
	myToken = new Token( OPERATOR, "!=", true );
	addSymbolEntry( myToken );
	
	myToken = new Token( OPERATOR, "=", true );
	addSymbolEntry( myToken );
	
	myToken = new Token( OPERATOR, ":=", true );
	addSymbolEntry( myToken );
	
	myToken = new Token( OPERATOR, "{", true );
	addSymbolEntry( myToken );
	
	myToken = new Token( OPERATOR, "}", true );
	addSymbolEntry( myToken );
	
	myToken = new Token( OPERATOR, "&", true );
	addSymbolEntry( myToken );
	
	myToken = new Token( OPERATOR, "|", true );
	addSymbolEntry( myToken );
	
	myToken = new Token( OPERATOR, "[", true );
	addSymbolEntry( myToken );
	
	myToken = new Token( OPERATOR, "]", true );
	addSymbolEntry( myToken );
	
	// Put runtime functions in the global symbol table
	myProcedure = new Procedure( IDENTIFIER, "getBool", true );
	myVariable = new Variable( IDENTIFIER, "newBool", BOOL, false, myProcedure->getParameterAddress(), true );
	myProcedure->advanceParameterAddress();
	myProcedure->addParameter( myVariable );
	myProcedure->addDirection( false );
	addSymbolEntry( myProcedure );
	
	myProcedure = new Procedure( IDENTIFIER, "getInteger", true );
	myVariable = new Variable( IDENTIFIER, "newInteger", INTEGER, false, myProcedure->getParameterAddress(), true );
	myProcedure->advanceParameterAddress();
	myProcedure->addParameter( myVariable );
	myProcedure->addDirection( false );
	addSymbolEntry( myProcedure );
	
	myProcedure = new Procedure( IDENTIFIER, "getFloat", true );
	myVariable = new Variable( IDENTIFIER, "newFloat", FLOAT, false, myProcedure->getParameterAddress(), true );
	myProcedure->advanceParameterAddress();
	myProcedure->addParameter( myVariable );
	myProcedure->addDirection( false );
	addSymbolEntry( myProcedure );
	
	myProcedure = new Procedure( IDENTIFIER, "getString", true );
	myVariable = new Variable( IDENTIFIER, "newString", STRINGT, false, myProcedure->getParameterAddress(), true );
	myProcedure->advanceParameterAddress();
	myProcedure->addParameter( myVariable );
	myProcedure->addDirection( false );
	addSymbolEntry( myProcedure );
	
	myProcedure = new Procedure( IDENTIFIER, "putBool", true );
	myVariable = new Variable( IDENTIFIER, "oldBool", BOOL, false, myProcedure->getParameterAddress(), true );
	myProcedure->advanceParameterAddress();
	myProcedure->addParameter( myVariable );
	myProcedure->addDirection( true );
	addSymbolEntry( myProcedure );
	
	myProcedure = new Procedure( IDENTIFIER, "putInteger", true );
	myVariable = new Variable( IDENTIFIER, "oldInteger", INTEGER, false, myProcedure->getParameterAddress(), true );
	myProcedure->advanceParameterAddress();
	myProcedure->addParameter( myVariable );
	myProcedure->addDirection( true );
	addSymbolEntry( myProcedure );
	
	myProcedure = new Procedure( IDENTIFIER, "putFloat", true );
	myVariable = new Variable( IDENTIFIER, "oldFloat", FLOAT, false, myProcedure->getParameterAddress(), true );
	myProcedure->advanceParameterAddress();
	myProcedure->addParameter( myVariable );
	myProcedure->addDirection( true );
	addSymbolEntry( myProcedure );
	
	myProcedure = new Procedure( IDENTIFIER, "putString", true );
	myVariable = new Variable( IDENTIFIER, "oldString", STRINGT, false, myProcedure->getParameterAddress(), true );
	myProcedure->advanceParameterAddress();
	myProcedure->addParameter( myVariable );
	myProcedure->addDirection( true );
	addSymbolEntry( myProcedure );
}

// This function retrieves the next token from the input file ( already open by initializeScanner() ) and returns it to the calling function
TokenFrame getToken( void )
{
	char currentCharacter;
	char nextCharacter;
	
	TokenFrame newToken;
	
	while( true )
	{
		// Initialize the container for the new token to retrieve
		newToken.tokenType = UNKNOWN;
		newToken.name.clear();
		newToken.isGlobal = false;
		
		// This block of code skips through white space until a real character is detected
		while( inFile.good() )
		{
			nextCharacter = inFile.peek(); // Check what the next character is
			if( isspace( nextCharacter ) ) // Check if it's white space
			{
				if( nextCharacter == '\n' ) // Check if the whitespace is a newline in order to keep track of line number
				{
					lineNumber++;
				}
				
				inFile.ignore(); // Pull the white space character out of the stream
			}
			else // If it's not white space
			{
				break;
			}
		}
		
		// Determine what kind of character is next.
		switch( getCharacterClass( nextCharacter ) )
		{
			case LETTER: // Only identifiers and reserved words start with a letter.
				// Get the token value
				do
				{
					// Read the next character in
					inFile >> currentCharacter;
					newToken.name += currentCharacter;
					
					// Look at what the next character is
					nextCharacter = inFile.peek();
				} while( inFile.good() && ( isalnum( nextCharacter ) || nextCharacter == '_' ) );
				
				// Search for the token in the symbol table
				// Determine whether it's a reserved word
				findSymbolEntry( newToken );
				return newToken;
				break;
				
			case DIGIT: // Only numbers start with a digit
				newToken.tokenType = NUMBER;
				
				// Get the token value
				do
				{
					if( nextCharacter != '_' )
					{
						// Read the next character in and add it to the token value
						inFile >> currentCharacter;
						newToken.name += currentCharacter;
					}
					else
					{
						inFile.ignore( 1 );
					}
					
					// Look at what the next character is
					nextCharacter = inFile.peek();
				} while( inFile.good() && ( isdigit( nextCharacter ) || nextCharacter == '_' ) ); // keep going as long as there are digits or underscores
				
				// Check if there's a decimal point next
				if( nextCharacter == '.' )
				{
					do
					{
						if( nextCharacter != '_' )
						{
							// Read the next character in and add it to the token value
							inFile >> currentCharacter;
							newToken.name += currentCharacter;
						}
						else
						{
							inFile.ignore( 1 );
						}
						
						// Look at what the next character is
						nextCharacter = inFile.peek();
					} while( inFile.good() && ( isdigit( nextCharacter ) || nextCharacter == '_' ) ); // keep going as long as there are digits or underscores
				}
				
				return newToken;
				break;
				
			case PUNCTUATION:
				switch( nextCharacter )
				{
					case '/': // This is either a division symbol or part of a comment marker
						inFile >> currentCharacter;
						nextCharacter = inFile.peek();
						
						if( nextCharacter == '/' ) // This is a comment marker. Skip to the next line
						{
							inFile.ignore( numeric_limits<streamsize>::max(), '\n' ); // skip the rest of the line
							
							lineNumber++;
							continue; // Start over looking for a token because we haven't found one yet, just a comment.
						}
						else // It's just a division operator
						{
							newToken.tokenType = OPERATOR;
							newToken.name = '/';
							
							return newToken;
						}
						break;
						
					case '<': // These can be one-character or two-character operators. Check which one
					case '>':
					case '!':
					case ':':
						inFile >> currentCharacter;
						nextCharacter = inFile.peek();
						
						newToken.tokenType = OPERATOR;
						newToken.name += currentCharacter;
						
						if( nextCharacter == '=' ) // This is a two-character operator
						{
							inFile >> currentCharacter;
							newToken.name += currentCharacter;
							
							return newToken;
						}
						else // It's just a one-character operator
						{
							if( currentCharacter == '!' )
							{
								reportWarning( "Illegal character: \'!\'. Assuming whitespace." );
								continue; // Start over looking for a token because we haven't gotten one yet.
							}
							else
							{
								return newToken;
							}
						}
						break;
						
					case '\"': // Double quotes means the beginning of a string literal
						newToken.tokenType = STRING;
						
						// Start retrieving the string literal
						do
						{
							inFile >> currentCharacter;
							nextCharacter = inFile.peek();
							
							newToken.name += currentCharacter;
							if( isalnum( nextCharacter ) ) // Check if we are continuing to get valid string characters
							{
								continue;
							}
							switch( nextCharacter )
							{
								// These are also valid string characters
								case ' ':
								case '_':
								case ',':
								case ';':
								case ':':
								case '.':
								case '\'':
									continue;
									
								case '\"':
									inFile >> currentCharacter;
									newToken.name += currentCharacter;
									
									return newToken;
									
								case '\n': // Unexpected inside a string literal
									reportWarning( "Unexpected end of line in string literal. Assuming end of string literal." );
									
									inFile.ignore();
									lineNumber++;
									
									newToken.name += '\"';
									
									return newToken;
									
								default:
									reportWarning( "Encountered illegal character in string literal. Assuming end of string literal." );
									
									newToken.name += '\"';
									
									return newToken;
							}
						} while( inFile.good() );
						break;
						
					default: // This will cover all other legal punctuation
						inFile >> currentCharacter;
						
						newToken.tokenType = OPERATOR;
						newToken.name = currentCharacter;
						
						return newToken;
				}
				break;
				
			default: // Illegal character
				if( nextCharacter == char_traits<char>::eof() )
				{
					throw EOFException();
				}
				else
				{
					inFile >> currentCharacter;
					
					reportWarning( "Illegal character found. Assuming whitespace." );
				}
				break;
		}
	}
}

// This function determines the character class of the specified character
CharacterClass getCharacterClass( const char& ch )
{
	if( isalpha( ch ) != 0 )
		return LETTER;
	else if( isdigit( ch ) != 0 )
		return DIGIT;
	else if( ispunct( ch ) != 0 )
	{
		switch( ch )
		{
			case ':':
			case ';':
			case ',':
			case '+':
			case '-':
			case '*':
			case '/':
			case '(':
			case ')':
			case '<':
			case '>':
			case '!':
			case '=':
			case '{':
			case '}':
			case '\"':
			case '&':
			case '|':
			case '[':
			case ']':
				return PUNCTUATION;
				break;
				
			default:
				return ILLEGAL;
				break;
		}
	}
}
