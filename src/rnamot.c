#include <stdio.h>

#include "rnamot.h"

extern	int	yydebug;

main( argc, argv )
int	argc;
char	*argv[];
{

	if( yyparse() ){
		fprintf( stderr, "syntax error.\n" );
	}

	SE_dump( stderr, 1, 1, 1, 1 );
}
