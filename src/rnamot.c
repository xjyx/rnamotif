#include <stdio.h>

#include "rnamot.h"

#include "dbase.h"

int	rm_error;
VALUE_T	rm_tokval;
int	rm_lineno;
int	rm_emsg_lineno;
char	rm_fname[ 256 ] = "--stdin--";
int	rm_copt = 0;
int	rm_dopt = 0;
int	rm_hopt = 0;	/* dump hierarchy	*/

#define	RM_GLOBAL_IDS_SIZE	50
IDENT_T	rm_global_ids[ RM_GLOBAL_IDS_SIZE ] = {
	{ "wc", T_PAIR, C_VAR, S_GLOBAL, { T_PAIR, NULL } },
	{ "gu", T_PAIR, C_VAR, S_GLOBAL, { T_PAIR, NULL } },
	{ "tr", T_PAIR, C_VAR, S_GLOBAL, { T_PAIR, NULL } },
	{ "qu", T_PAIR, C_VAR, S_GLOBAL, { T_PAIR, NULL } },
 /*5*/	{ "database", T_STRING, C_VAR, S_GLOBAL, { T_STRING, "RNA" } },
	{ "overlap", T_INT, C_VAR, S_GLOBAL, { T_INT, 0 } },
	{ "wc_minlen", T_INT, C_VAR, S_GLOBAL, { T_INT, 3 } },
	{ "wc_maxlen", T_INT, C_VAR, S_GLOBAL, { T_INT, 30 } },
 /*9*/	{ "windowsize", T_INT, C_VAR, S_GLOBAL, { T_INT, 6000 } }
};
int	rm_s_global_ids = RM_GLOBAL_IDS_SIZE;
int	rm_n_global_ids = 9;

#define	RM_DESCR_SIZE 100
STREL_T	rm_descr[ RM_DESCR_SIZE ];
int	rm_s_descr = RM_DESCR_SIZE;
int	rm_n_descr;
int	rm_dminlen;	/* min. len. of entire motif	*/
int	rm_dmaxlen;	/* max. len. of entire motif	*/

#define	RM_POS_SIZE	10
POS_T	rm_pos[ RM_POS_SIZE ];
int	rm_s_pos = RM_POS_SIZE;
int	rm_n_pos;

SITE_T	*rm_sites = NULL;

#define	RM_S_B2C	256	
int	rm_b2bc[ RM_S_B2C ];
int	rm_s_b2bc = RM_S_B2C;
char	rm_bc2b[ N_BCODES ] = { 'a', 'c', 'g', 't', 'n' };

SEARCH_T	**rm_searches;
int		rm_n_searches;

extern	int	yydebug;

char	*getenv();

#define	SBUFSIZE	2000000
static	char	sbuf[ SBUFSIZE ];
static	int	slen;

IDENT_T	*find_id();

main( argc, argv )
int	argc;
char	*argv[];
{
	DBASE_T	*dbp;
	IDENT_T	*ip;
	char	*dbnp;
	char	locus[ 20 ];

	RM_init( argc, argv );

	if( yyparse() ){
		errormsg( 1, "syntax error." );
	}
	if( !rm_error ){
		if( SE_link( rm_n_descr, rm_descr ) )
			exit( 1 );
		fprintf( stderr, "%s: complete descr length: min/max = %d/",
			rm_fname, rm_dminlen );
		if( rm_dmaxlen == UNBOUNDED )
			fprintf( stderr, "UNBND\n" );
		else
			fprintf( stderr, "%d\n", rm_dmaxlen );
	}

	if( rm_dopt || rm_hopt )
		RM_dump( stderr, rm_dopt, rm_dopt, rm_dopt, rm_hopt );

	if( rm_error )
		exit( 1 );

	if( rm_copt )
		exit( 0 );

	ip = find_id( "database" );
	if( ip == NULL ){
		fprintf( stderr, "rnamot: 'database' not defined.\n" );
		exit( 1 );
	}else
		dbnp = ip->i_val.v_value.v_pval;

	for( dbp = DB_open( dbnp ); dbp; ){
		if( slen = DB_getseq( dbp, locus, SBUFSIZE, sbuf ) ){
			find_motif_driver( rm_n_searches, rm_searches, rm_sites,
				locus, slen, sbuf );
		}
		dbp = DB_next( dbp );
	}

	exit( 0 );
}
