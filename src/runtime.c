#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int getBool( void )
{
	char inputBuffer[10];
	int test;
	
	do
	{
		fgets( inputBuffer, 10, stdin );
		
		test = strncmp( inputBuffer, "true", 4 );
		if( test == 0 )
		{
			return 1;
		}
		
		test = strncmp( inputBuffer, "false", 5 );
		if( test == 0 )
		{
			return 0;
		}
		
		test = -1;
	} while( test == -1 );
}

int getInteger( void )
{
	int newInteger;
	
	scanf( "%d", &newInteger );
	
	return newInteger;
}

float getFloat( void )
{
	float newFloat;
	
	scanf( "%f", &newFloat );
	
	return newFloat;
}

int getString( void )
{
	int newStringPointer = R[1].intVal;
	int finished = 0;
	int i = 0;
	char inputBuffer[256];
	
	fgets( inputBuffer, 256, stdin );
	
	for( i = 0; i < 256 && finished == 0; i++ )
	{
		if( inputBuffer[i] == '\0' )
		{
			finished = 1;
		}
		
		MM[newStringPointer + i].charVal = inputBuffer[i];
	}
	
	R[1].intVal += i;
	
	return newStringPointer;
}

int putBool( int oldBool )
{
	switch( oldBool )
	{
		case 0:
			printf( "false" );
			break;
			
		case 1:
			printf( "true" );
			break;
			
		default:
			printf( "Runtime Data Conversion Error: Converting Integer to Boolean\n" );
			exit( EXIT_FAILURE );
			break;
	}
	
	return 0;
}

int putInteger( int oldInteger )
{
	printf( "%d", oldInteger );
	
	return 0;
}

int putFloat( float oldFloat )
{
	printf( "%f", oldFloat );
	
	return 0;
}

int putString( int oldString )
{
	char outputCharacter;
	int i = 0;
	
	if( oldString == 0 )
	{
		printf( "\nRuntime Data Conversion Error: Converting Integer to Boolean\n" );
		exit( EXIT_FAILURE );
	}
	
	do
	{
		outputCharacter = MM[oldString + i].charVal;
		
		printf( "%c", outputCharacter );
		
		i++;
	} while( outputCharacter != '\0' );
}