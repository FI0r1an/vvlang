#include <stdio.h>

#include <stdlib.h>
#include <assert.h>
#include "vvgen.h"
#include "vvlex.h"
#include "vvparser.h"
#include "vvcom.h"

/* int main( )
{
	char *src = vv_readFile( "test.vv" );
	vvLexTable *tbl = vv_newLexTable( );
	vvLexer *lex = vv_newLexer( src, ".", VV_CHAR_BUFFER_DEFAULT_LEN, tbl );
	vvParser *par = vv_newParser( lex );

	vvSyntaxContainer *stmt = vv_newSyntaxContainer( );
	parseStatement( par, stmt );
	vv_freeSyntaxContainer( stmt );

	vv_freeParser( par );
	vv_freeLexer( lex );
	vv_freeLexTable( tbl );

	char ignore = getchar( );

	return 0;
}*/