#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "mpi.h"

#include "rmdefs.h"
#include "rnamot.h"
#include "fmap.h"

#define	MIN(a,b)	((a)<(b)?(a):(b))
#define	MAX(a,b)	((a)>(b)?(a):(b))

#define	MT_GETHOSTID	0
#define	MT_HOSTID	1
#define	MT_SETUP	2
#define	MT_RUN	 	3
#define	MT_READY	4
#define	MT_RESULT	5
#define	MT_QUIT		6	/* sent from 0 -> 1,np-1	*/
#define	MT_ERROR	7	/* sent from 1,np-1 -> 0	*/

static	int	my_rank, n_proc, n_wproc;
static	char	hostname[ 256 ];
static	char	xhostname[ 256 ];

static	int	err;
static	char	emsg[ 1024 ];
static	int	err;

#define	CMD_SIZE	1024
static	char	rncmd[ CMD_SIZE ];
static	int	s_rncmd;

static	char	*xdescr;
static	int	s_xdescr;
static	char	xdfname[ 256 ];	/* used only by r1..rnp-1	*/
static	char	**hosts;

static	FMAP_T	*fmap;

#define	JS_UNDEF	UNDEF
#define	JS_READY	0
#define	JS_RUNNING	1
#define	JS_DONE		2

typedef	struct	job_t	{
	struct	job_t	*j_next;	
	int	j_num;
} JOB_T;

typedef	struct	jobq_t	{
	JOB_T	*j_first;	
	JOB_T	*j_last;	
} JOBQ_T;

static	JOBQ_T	*jobqs;
static	int	n_jobqs;
static	int	*jobs;
static	int	*jidx;
static	int	s_jobs;
static	int	n_jobs;
static	int	*actjobs;

static	int	qbuf;
static	char	*sbuf;
static	int	s_sbuf;
static	int	rcnt, scnt;
static	char	*rbuf;
static	int	s_rbuf;

#define	WORK_SIZE	102400
static	char	work[ WORK_SIZE ];

ARGS_T	*RM_getargs( int, char *[] );

static	int	all_init( void );
static	int	r0_init( int, char *[], int, char [] );
static	FMAP_T	*get_fmap( ARGS_T * );
static	int	mk_commands( ARGS_T *,
	char [], char [], int*, char [], int *, char [] );
static	int	compile_descr( char *, int, char *, int, char * );
static	int	setup_jobs( ARGS_T *, FMAP_T * );
static	int	mk_jobqs( void );
static	int	qjob( int, int );
static	JOB_T	*dqjob( int );
static	int	r0_handle_msg( MPI_Status * );
static	int	rn_handle_msg( MPI_Status * );
static	int	rn_setup( char * );
static	int	rn_run( char * );
static	int	nextjob( int );
static	void	dumpjobs( FILE * );
static	void	dumpjidx( FILE *, char * );
static	void	dumpjqs( FILE *, char * );

char	*getenv();

main( int argc, char *argv[] )
{
	int	p;
	MPI_Status	pstatus, rstatus;

	MPI_Init( &argc, &argv );
	MPI_Comm_rank( MPI_COMM_WORLD, &my_rank );
	MPI_Comm_size( MPI_COMM_WORLD, &n_proc );

	gethostname( hostname, sizeof( hostname ) );
	sprintf( xhostname, "%s_%d", hostname, my_rank );

	if( n_proc < 2 ){
		fprintf( stderr, "%s: n_proc must be > 1\n", xhostname );
		err = 1;
		goto CLEAN_UP;
	}else
		n_wproc = n_proc - 1;

	if( all_init() ){
		err = 1;
		goto CLEAN_UP;
	}

	if( my_rank == 0 ){
		if( r0_init( argc, argv, CMD_SIZE, rncmd ) ){
			err = 1;
			goto CLEAN_UP;
		}
		s_sbuf = s_rncmd + s_xdescr;

fprintf( stderr, "%s: n_jobs = %d\n", xhostname, n_jobs );

	}

	MPI_Bcast( &s_sbuf, 1, MPI_INT, 0, MPI_COMM_WORLD );
	sbuf = ( char * )malloc( s_sbuf * sizeof( char ) );
	if( sbuf == NULL ){
		fprintf( stderr, "%s: can't allocate sbuf\n", xhostname );
		err = 1;
		goto CLEAN_UP;
	}

	if( my_rank == 0 ){
		strcpy( sbuf, rncmd );
		strcpy( &sbuf[ s_rncmd ], xdescr );
	}
	MPI_Bcast( sbuf, s_sbuf, MPI_CHAR, 0, MPI_COMM_WORLD );
	if( my_rank != 0 ){
		if( rn_setup( sbuf ) ){
			err = 1;
			goto CLEAN_UP;
		}
	}

	if( my_rank == 0 ){
		for( rcnt = 0; ; ){
			MPI_Probe( MPI_ANY_SOURCE, MPI_ANY_TAG,
				MPI_COMM_WORLD, &pstatus );

			if( r0_handle_msg( &pstatus ) ){
				err = 1;
				goto CLEAN_UP;
			}
		}
	}else{
		while( !MPI_Probe( MPI_ANY_SOURCE, MPI_ANY_TAG,
			MPI_COMM_WORLD, &pstatus ) )
		{
			if( rn_handle_msg( &pstatus ) ){
				err = 1;
				break;
			}
		}
	}

CLEAN_UP : ;

	if( my_rank == 0 ){
		for( p = 1; p < n_proc; p++ ){
			qbuf = -1;
			MPI_Send( &qbuf, 1, MPI_INT,
				p, MT_QUIT, MPI_COMM_WORLD );
		}
	}

	MPI_Finalize();

	exit( err );
}

static	int	all_init( void )
{
	int	p, buf = 0;
	char	*hp, hname[ 256 ];
	MPI_Status	status;
	int	rval = 0;

	if( my_rank == 0 ){
		hosts = ( char ** )malloc( n_proc * sizeof( char * ) );
		if( hosts == NULL ){
			fprintf( stderr, "%s: all_init: can't allocate hosts\n",
				xhostname );
			rval = 1;
			goto CLEAN_UP;
		}

		hp = strdup( hostname );
		if( hp == NULL ){
			fprintf( stderr,
				"%s: all_init: can't allocate hp for %s_%d\n",
				xhostname, hostname, 0 );
			rval = 1;
			goto CLEAN_UP;
		}
		hosts[ 0 ] = hp;

		for( p = 1; p < n_proc; p++ ){
			MPI_Send( &buf, 1, MPI_INT,
				p, MT_GETHOSTID, MPI_COMM_WORLD );
			MPI_Recv( hname, sizeof( hname ), MPI_CHAR,
				p, MT_HOSTID, MPI_COMM_WORLD, &status );
			hp = strdup( hname );
			if( hp == NULL ){
				fprintf( stderr,
			"%s: all_init: can't allocate hp for host %s_%d\n",
					hname, p );
				rval = 1;
				goto CLEAN_UP;
			}
			hosts[ p ] = hp;
		}
	}else{
		MPI_Recv( &buf, 1, MPI_INT,
			0, MT_GETHOSTID, MPI_COMM_WORLD, &status );
		MPI_Send( hostname, strlen( hostname ) + 1, MPI_CHAR,
			0, MT_HOSTID, MPI_COMM_WORLD );
	}

CLEAN_UP : ;

	return( rval );
}

static	int	r0_init( int argc, char *argv[], int s_rncmd, char rncmd[] )
{
	ARGS_T	*args;
	char	r0cmd[ CMD_SIZE ];
	char	xdfname[ 256 ];
	int	xdfd;
	char	r0efname[ 256 ];
	int	r0efd;

	*xdfname = *r0efname = '\0';

	if( ( args = RM_getargs( argc, argv ) ) == NULL ){
		err = 1;
		goto CLEAN_UP;
	}

	if( ( fmap = get_fmap( args ) ) == NULL ){
		err = 1;
		goto CLEAN_UP;
	}

	if( mk_commands( args,
		r0cmd, rncmd, &xdfd, xdfname, &r0efd, r0efname ) )
	{
		err = 1;
		goto CLEAN_UP;
	}

	if( compile_descr( r0cmd, xdfd, xdfname, r0efd, r0efname ) ){
		err = 1;
		goto CLEAN_UP;
	}

	if( setup_jobs( args, fmap ) ){
		err = 1;
		goto CLEAN_UP;
	}

CLEAN_UP : ;

	if( *xdfname )
		unlink( xdfname );
	if( *r0efname )
		unlink( r0efname );

	return( err );
}

static	FMAP_T	*get_fmap( ARGS_T *args )
{
	FMAP_T	*fmap = NULL;
	int	fmt;
	int	err = 0;

	if( ( fmap = FMread_fmap( args->a_fmfname ) ) == NULL ){
		err = 1;
		goto CLEAN_UP;
	}

	if( !strcmp( fmap->f_format, DT_FASTN ) )
		args->a_dbfmt = DT_FASTN;
	else if( !strcmp( fmap->f_format, DT_PIR ) )
		args->a_dbfmt = DT_PIR;
	else if( !strcmp( fmap->f_format, DT_GENBANK ) )
		args->a_dbfmt = DT_GENBANK;
	else{
		fprintf( stderr, "%s: get_fmap: unknown format '%s'\n",
			xhostname, fmap->f_format );
		err = 1;
		goto CLEAN_UP;
	}

CLEAN_UP : ;

	if( err && fmap != NULL )
		fmap = FMfree_fmap( fmap );

	return( fmap );
}

static	int	mk_commands( ARGS_T *args, char r0cmd[], char rncmd[],
	int	*xdfd, char xdfname[], int *r0efd, char r0efname[] )
{
	char	*cp0, *cpn;
	int	done = FALSE;
	INCDIR_T	*ip;
	char	*cdp, *e_cdp;

	*r0cmd = *rncmd = '\0';
	*xdfname = *r0efname = '\0';
	*xdfd = *r0efd = 0;

	cp0 = r0cmd;
	strcpy( cp0, "rnamotif" );
	cp0 += strlen( cp0 );

	/* these options produce information only; no compile; no run:	*/
	if( args->a_sopt ){
		strcpy( cp0, " -s" );
		cp0 += strlen( cp0 );
		done = TRUE;
	}
	if( args->a_vopt ){
		strcpy( cp0, " -v" );
		cp0 += strlen( cp0 );
		done = TRUE;
	}
	if( done )
		return( 0 );

	/* options used on r0:	*/
	strcpy( cp0, " -c" );
	cp0 += strlen( cp0 );
	if( args->a_dopt ){
		strcpy( cp0, " -d" );
		cp0 += strlen( cp0 );
	}
	if( args->a_hopt ){
		strcpy( cp0, " -h" );
		cp0 += strlen( cp0 );
	}
	sprintf( cp0, " -O%g", args->a_o_emin );
	cp0 += strlen( cp0 );
	if( args->a_popt ){
		strcpy( cp0, " -p" );
		cp0 += strlen( cp0 );
	}
	if( args->a_show_context ){
		strcpy( cp0, " -context" );
		cp0 += strlen( cp0 );
	}
	if( args->a_strict_helices ){
		strcpy( cp0, " -sh" );
		cp0 += strlen( cp0 );
	}
	for( ip = args->a_idlist; ip; ip = ip->i_next ){
		sprintf( cp0, " -I%s", ip->i_name );
		cp0 += strlen( cp0 );
	}
	for( cdp = args->a_cldefs; cdp && *cdp;  cdp = e_cdp + 2 ){
		e_cdp = strchr( cdp, ';' );
		sprintf( cp0, " -D%.*s", e_cdp - cdp, cdp );
		cp0 += strlen( cp0 );
	}
	if( args->a_dfname == NULL ){
		return( 1 );
	}else{
		sprintf( cp0, " -descr %s", args->a_dfname );
		cp0 += strlen( cp0 );
	}
	if( args->a_xdfname != NULL ){
		return( 1 );
	}else{
		strcpy( xdfname, "/tmp/rmxdf_XXXXXX" );
		*xdfd = mkstemp( xdfname );
		sprintf( cp0, " -xdfname %s", xdfname );
		cp0 += strlen( cp0 );
	}
	strcpy( r0efname, "/tmp/rmr0ef_XXXXXX" );
	*r0efd = mkstemp( r0efname );
	sprintf( cp0, " >& %s", r0efname );

	if( args->a_copt )
		return( 0 );

	/* options used on rn:	*/
	cpn = rncmd;
	strcpy( cpn, "rnamotif" );
	cpn += strlen( cpn );

	sprintf( cpn, " -O%g", args->a_o_emin );
	cpn += strlen( cpn );
	if( args->a_show_context ){
		strcpy( cpn, " -context" );
		cpn += strlen( cpn );
	}
	if( args->a_strict_helices ){
		strcpy( cpn, " -sh" );
		cpn += strlen( cpn );
	}
	strcpy( cpn, " -xdescr %s 2> /dev/null" );
	cpn += strlen( cpn );
	strcpy( cpn, " -fmt" );
	cpn += strlen( cpn );
	if( !strcmp( args->a_dbfmt, DT_FASTN ) )
		sprintf( cpn, " %s", DT_FASTN );
	else if( !strcmp( args->a_dbfmt, DT_PIR ) )
		sprintf( cpn, " %s", DT_PIR );
	else 
		sprintf( cpn, " %s", DT_GENBANK );
	cpn += strlen( cpn );
	strcpy( cpn, " %s" );
	cpn += strlen( cpn );

	s_rncmd = strlen( rncmd ) + 1;

	return( 0 );
}

static	int	compile_descr( char *r0cmd,
	int xdfd, char *xdfname, int r0efd, char *r0efname )
{
	FILE	*fp = NULL;
	char	line[ 256 ];
	int	lcnt;
	int	cdlmsg;
	struct	stat	sbuf;

	system( r0cmd );

	if( ( fp = fdopen( r0efd, "r" ) ) == NULL ){
		fprintf( stderr,
			"%s: r0_init: can't read compile step errfile %s\n",
			xhostname, r0efname );
		return( 1 );
	}
	for( cdlmsg = 0, lcnt = 0; fgets( line, sizeof( line ), fp ); lcnt++ ){
		if( strstr( line, ": complete descr length: min/max = " ) )
			cdlmsg = 1;
		fputs( line, stderr );
	}
	fclose( fp );
	fp = NULL;
	if( lcnt > 1 || !cdlmsg )
		return( 1 );

	fstat( xdfd, &sbuf );
	s_xdescr = sbuf.st_size + 1;
	xdescr = ( char * )malloc( s_xdescr * sizeof( char * ) );
	if( xdescr == NULL ){
		fprintf( stderr, "%s: r0_init: can't allocate xdescr\n",
			xhostname );
		return( 1 );
	}
	fp = fdopen( xdfd, "r" );
	fread( xdescr, sizeof( char ), ( long )( s_xdescr - 1 ), fp );
	xdescr[ s_xdescr - 1 ] = '\0';
	fclose( fp );
	fp = NULL;

	return( 0 );
}

static	int	setup_jobs( ARGS_T *args, FMAP_T *fmap )
{
	int	*active = NULL;
	int	i, j, ji, h;
	char	*dp, *pp, *pc, *pd;
	int	dlen;
	char	dname[ 256 ];
	FM_ENTRY_T	*fme;
	int	p, p1, pcnt, pl, ph;
	char	range[ 256 ];
	int	err = 0;

	s_jobs = fmap->f_nentries;
	jobs = ( int * )calloc( ( long )s_jobs, sizeof( int ) );
	if( jobs == NULL ){
		fprintf( stderr, "%s: setup_jobs: can't allocate jobs\n",
			xhostname );
		err = 1;
		goto CLEAN_UP;
	}
	for( i = 0; i < s_jobs; i++ )
		jobs[ i ] = JS_UNDEF;

	active = ( int * )calloc( ( s_jobs ), sizeof( int ) );
	if( active == NULL ){
		fprintf( stderr, "%s: setup_jobs: can't allocate active\n",
			xhostname );
		err = 1;
		goto CLEAN_UP;
	}

	actjobs = ( int * )malloc( n_proc * sizeof( int ) );
	if( actjobs == NULL ){
		fprintf( stderr, "%s: setup_jobs: can't allocate actjobs\n",
			xhostname );
		err = 1;
		goto CLEAN_UP;
	}
	for( i = 0; i < n_proc; i++ )
		actjobs[ i ] = UNDEF;

	if( args->a_n_dbfname == 0 ){
		for( j = 0; j < s_jobs; j++ )
			jobs[ j ] = JS_READY; 
		n_jobs = s_jobs;
	}else{
		for( i = 0; i < args->a_n_dbfname; i++ ){
			if( FMmark_active(fmap, args->a_dbfname[ i ], active) ){
				err = 1;
				goto CLEAN_UP;
			}
		}
		for( n_jobs = 0, i = 0; i < s_jobs; i++ ){
			if( active[ i ] ){
				jobs[ i ] = JS_READY;
				n_jobs++;
			}
		}
	}

	jidx = ( int * )malloc( n_jobs * sizeof( int ) );
	if( jidx == NULL ){
		fprintf( stderr, "%s: setup_jobs: can't allocate jidx\n",
			xhostname );
		err = 1;
		goto CLEAN_UP;
	}
	for( j = i = 0; i < s_jobs; i++ ){
		if( jobs[ i ] == JS_READY ){
			jidx[ j ] = i;
			j++;
		}
	}

	if( mk_jobqs() ){
		err = 1;
		goto CLEAN_UP;
	}

CLEAN_UP : ;

	if( active != NULL ){
		free( active );
		active = NULL;
	}

	return( err );
}

static	int	mk_jobqs( void )
{
	int	j, ji, h;
	int	needqs;
	FM_ENTRY_T	*fme;
	int	err = 0;

	for( needqs = 0, j = 0; j < n_jobs; j++ ){
		ji =  jidx[ j ];
		fme = &fmap->f_entries[ ji ];
		if( fme->f_hosts != NULL ){
			needqs = 1;
			break;
		}
	}
	if( !needqs )
		return( err );

	n_jobqs = n_proc;	/* jobqs[0] is not used	*/
	jobqs = ( JOBQ_T * )calloc( ( long )n_jobqs, sizeof( JOBQ_T ) );
	if( jobqs == NULL ){
		fprintf( stderr, "%s: mk_jobqs: can't allocate jobqs\n",
			xhostname );
		err = 1;
		goto CLEAN_UP;
	}

	for( j = 0; j < n_jobs; j++ ){
		ji =  jidx[ j ];
		fme = &fmap->f_entries[ ji ];
		for( h = 1; h < n_proc; h++ ){
			if( strstr( fme->f_hosts, hosts[ h ] ) ){
				if( qjob( h, ji ) ){
					err = 1;
					goto CLEAN_UP;
				}
			}
		}
	}

CLEAN_UP : ;

	return( err );
}

static	int	qjob( int qnum, int jnum )
{
	FM_ENTRY_T	*fme;
	JOB_T	*jp;
	JOBQ_T	*jqp;

	jp = ( JOB_T * )malloc( sizeof( JOB_T ) );
	if( jp == NULL )
		return( 1 );
	jp->j_next = NULL;
	jp->j_num = jnum;
	jqp = &jobqs[ qnum ];
	if( jqp->j_first == NULL )
		jqp->j_first = jp;
	else
		jqp->j_last->j_next = jp;
	jqp->j_last = jp;
	return( 0 );
}

static	JOB_T	*dqjob( int qnum )
{
	JOB_T	*jp;

	jp = jobqs[ qnum ].j_first;
	if( jp != NULL ){
		jobqs[ qnum ].j_first = jp->j_next;
		if( jp->j_next == NULL )
			jobqs[ qnum ].j_last = NULL;
	}
	return( jp );
}

static	int	r0_handle_msg( MPI_Status *pstatus )
{
	char	sbuf[ 256 ];
	MPI_Status	rstatus;
	int	count;
	int	s, j;
	int	rval = 0;

	/* expect MT_READY, MT_RESULT, MT_ERROR	*/

	switch( pstatus->MPI_TAG ){
	case MT_READY :
	case MT_ERROR :
		MPI_Get_count( pstatus, MPI_CHAR, &count );
		if( count > s_rbuf ){
			if( rbuf != NULL )
				free( rbuf );
			s_rbuf = count;
			rbuf = ( char * )malloc( s_rbuf * sizeof( char ) );
			if( rbuf == NULL ){
				fprintf( stderr,
				"%s: r0_handle_msg: can't allocate rbuf\n",
					xhostname );
				rval = 1;
				break;
			}
		}
		MPI_Recv( rbuf, s_rbuf, MPI_CHAR,
			pstatus->MPI_SOURCE,
			pstatus->MPI_TAG, MPI_COMM_WORLD, &rstatus );

		if( pstatus->MPI_TAG == MT_READY ){
			rcnt++;

fprintf( stderr, "%s: READY: %s_%d: rcnt = %d\n",
	xhostname, hosts[ pstatus->MPI_SOURCE ], pstatus->MPI_SOURCE, rcnt );

			s = pstatus->MPI_SOURCE;
			j = actjobs[ s ];
			if( j != UNDEF ){
				jobs[ j ] = JS_DONE;
				actjobs[ s ] = UNDEF;
			}

			if( rcnt >= n_wproc + n_jobs ){
				rval = 1;
				break;
			}
			if( scnt < n_jobs ){

				if( ( j = nextjob( s ) ) == UNDEF )
					break;

				sprintf( sbuf, "%s/%s", fmap->f_root,
					fmap->f_entries[ j ].f_fname );
				jobs[ j ] = JS_RUNNING;
				actjobs[ s ] = j;

fprintf( stderr, "%s: RUN: search '%s' on %s_%d\n", xhostname, sbuf,
	hosts[ pstatus->MPI_SOURCE ], pstatus->MPI_SOURCE );

				scnt++;
				MPI_Send( sbuf, strlen( sbuf ) + 1, MPI_CHAR,
					pstatus->MPI_SOURCE,
					MT_RUN, MPI_COMM_WORLD );
			}
		}else{

fprintf( stderr, "%s: ERROR: %s\n", xhostname, rbuf );

			rval = 1;
		}

		break;
	case MT_RESULT :

		MPI_Recv( work, sizeof( work ), MPI_CHAR,
			pstatus->MPI_SOURCE,
			MT_RESULT, MPI_COMM_WORLD, &rstatus );
		fputs( work, stdout );
		break;

	case MT_SETUP :
	case MT_RUN :
	case MT_QUIT :
	default :
		fprintf( stderr,
			"%s: r0_handle_msg: unexpected mesg tag %d\n",
			pstatus->MPI_TAG );
		rval = 1;
		break;
	}

	return( rval );
}

static	int	rn_handle_msg( MPI_Status *pstatus )
{
	int	count;
	MPI_Status	rstatus;
	int	rval = 0;

	/* expect MT_SETUP, MT_RUN, MT_QUIT	*/

	switch( pstatus->MPI_TAG ){
	case MT_QUIT :
		if( *xdfname )
			unlink( xdfname );
		rval = 1;
		break;
/*
	case MT_SETUP :
		MPI_Get_count( pstatus, MPI_CHAR, &count );
		if( count > s_rbuf ){
			if( rbuf != NULL )
				free( rbuf );
			s_rbuf = count;
			rbuf = ( char * )malloc( s_rbuf * sizeof( char ) );
			if( rbuf == NULL ){
				rval = 1;
				break;
			}
		}
		MPI_Recv( rbuf, s_rbuf, MPI_CHAR, 0,
			pstatus->MPI_TAG, MPI_COMM_WORLD, &rstatus );
		if( rn_setup( rbuf ) ){
			rval = 1;
			break;
		}
		break;
*/
	case MT_RUN :
		MPI_Get_count( pstatus, MPI_CHAR, &count );
		if( count > s_rbuf ){
			if( rbuf != NULL )
				free( rbuf );
			s_rbuf = count;
			rbuf = ( char * )malloc( s_rbuf * sizeof( char ) );
			if( rbuf == NULL ){
				rval = 1;
				break;
			}
		}
		MPI_Recv( rbuf, s_rbuf, MPI_CHAR, 0,
			pstatus->MPI_TAG, MPI_COMM_WORLD, &rstatus );
		if( rn_run( rbuf ) ){
			rval = 1;
			break;
		}
		break;

	case MT_SETUP :
	case MT_RESULT :
	case MT_READY :
	case MT_ERROR :
	default :
		rval = 1;	/* create errmsg, send back to 0 */
		break;
	}

	return( rval );
}

static	int	rn_setup( char *rbuf )
{
	char	*rp, *rp1;
	int	s_rncmd, s_xdescr;
	int	xdfd;
	FILE	*xdfp = NULL;
	int	rval = 0;

	for( rp = rbuf; *rp; rp++ )
		;
	s_rncmd = rp - rbuf + 1;
	strcpy( rncmd, rbuf );

	for( rp1 = ++rp; *rp1; rp1++ )
		;
	s_xdescr = rp1 - rp + 1;
	xdescr = ( char * )malloc( s_xdescr * sizeof( char ) );
	if( xdescr == NULL ){
		rval = 1;
		goto CLEAN_UP;
	}
	strcpy( xdescr, rp );

	strcpy( xdfname, "/tmp/rmxdf_XXXXXX" );
	xdfd = mkstemp( xdfname );
	if( ( xdfp = fdopen( xdfd, "w" ) ) == NULL ){
		rval = 1;
		goto CLEAN_UP;
	}
	fwrite( xdescr, sizeof( char ), ( long )( s_xdescr - 1 ), xdfp );
	fclose( xdfp );
	xdfp = NULL;

	*work = '\0';
	MPI_Send( work, 1, MPI_CHAR, 0, MT_READY, MPI_COMM_WORLD );

CLEAN_UP : ;

	if( xdfp != NULL ){
		fclose( xdfp );
		xdfp = NULL;
	}
	if( rval == 1 ){
		if( *xdfname )
			unlink( xdfname );
	}

	return( rval );
}

static	int	rn_run( char *fname )
{
	char	dbfname[ 256 ];
	char	*wp, *cp, cmd[ 1024 ];
	FILE	*fp = NULL;
	char	line[ 10240 ];
	int	rval = 0;

	sprintf( cmd, rncmd, xdfname, fname );

	if( ( fp = popen( cmd, "r" ) ) == NULL ){
		fprintf( stderr, "%s: popen failed\n", xhostname );
		rval = 1;
		goto CLEAN_UP;
	}
	for( wp = work; fgets( line, sizeof( line ), fp ); ){
		strcpy( wp, line );
		wp += strlen( line );
		fgets( line, sizeof( line ), fp );
		strcpy( wp, line );
		wp += strlen( line );
		if( wp - work > .95 * WORK_SIZE ){
			MPI_Send( work, strlen( work ) + 1, MPI_CHAR,
				0, MT_RESULT, MPI_COMM_WORLD );
			wp = work;
		}
	}
	if( wp > work ){
		MPI_Send( work, strlen( work ) + 1, MPI_CHAR,
			0, MT_RESULT, MPI_COMM_WORLD );
		wp = work;
	}
	pclose( fp );
	fp = NULL;

	MPI_Send( cmd, strlen( cmd ) + 1, MPI_CHAR,
		0, MT_READY, MPI_COMM_WORLD );

CLEAN_UP : ;

	if( fp != NULL ){
		pclose( fp );
		fp = NULL;
	}

	return( rval );
}

static	int	nextjob( int s )
{
	JOB_T	*jp = NULL;
	int	j, nj;

	if( jobqs == NULL ){
		nj = jidx[ scnt ];
	}else{
		for( nj = UNDEF; jobqs[ s ].j_first; ){
			jp = dqjob( s );
			j = jp->j_num;
			free( jp );
			if( jobs[ j ] == JS_READY ){
				nj = j;
				break;
			}
		}
	}
	return( nj );
}

static	void	dumpjobs( FILE *fp )
{
	int	ji, j;

	fprintf( fp, "%s: %d jobs\n", xhostname, n_jobs );
	for( ji = 0; ji < n_jobs; ji++ ){
		j = jidx[ ji ];
		fprintf( fp, "%s: job[%3d] = fme[%3d] = %s:%d\n",
			xhostname, ji, j, fmap->f_entries[ j ].f_dname,
			fmap->f_entries[ j ].f_part );
	}
}

static	void	dumpjidx( FILE *fp, char *msg )
{
	int	j;

	fprintf( fp, "jidx: %d entries: %s\n", n_jobs, msg );
	for( j = 0; j < n_jobs; j++ ){
		fprintf( fp, "jidx[%3d] = %3s, status = %d\n",
			j, fmap->f_entries[ jidx[ j ] ].f_fname,
			jobs[ jidx[ j ] ] );
	}
}

static	void	dumpjqs( FILE *fp, char *msg )
{
	int	p;
	JOB_T	*jp;

	fprintf( fp, "%d jobqs: %s\n", n_proc, msg );
	for( p = 1; p < n_proc; p++ ){
		fprintf( fp, "jobsq['%s']:\n", hosts[p] );
		for( jp = jobqs[ p ].j_first; jp; jp = jp->j_next ){
			fprintf( fp, "\t%s %d\n",
				fmap->f_entries[ jp->j_num ].f_fname,
				jobs[ jp->j_num ] );
		}
	}
}
