#include <stdio.h>

#define	UNBOUNDED	0x7fffffff

#define	CBRA	2	/* 0002, \(      */
#define	CCHR	4	/* 0004  ord chr */
#define	CDOT	8	/* 0010  .       */
#define	CCL	12	/* 0014  [...]   */
#define	CXCL	16	/* 0020  [.8bt.] */
#define	CDOL	20	/* 0024  $       */
#define	CCEOF	22	/* 0026  <eof>   */
#define	CKET	24	/* 0030  \)      */
#define	CBRC	28	/* 0034  \<      */
#define	CLET	30	/* 0036  \>      */
#define	CBACK	36	/* 0044  *?      */
#define	NCCL	40	/* 0050  [^...]  */

#define	STAR	01	/* 0001 */
#define	RANGE	03	/* 0003 */

#define	ISTHERE(c)	(ep[c >> 3] & bittab[ c & 07])
static	char	bittab[] = { 1, 2, 4, 8, 16, 32, 64, 128 };

extern	char	*loc1, *loc2;
extern	int	circf;

void	mm_seqlen();
void	mm_dumppat();
int	mm_step();
static	int	mm_advance();

void	mm_seqlen( expbuf, minl, maxl, exact, mmok )
char	expbuf[];
int	*minl;
int	*maxl;
int	*exact;
int	*mmok;
{
	char	*ep;
	int	i, dol;

	*minl = *maxl = 0;
	*exact = 0;
	*mmok = 1;
	for( ep = expbuf; *ep != CCEOF; ep++ ){
		switch( *ep ){
		case CBRA :
			ep++;
			break;
		case CKET :
			break;

		case CBRC :
			*mmok = 0;
			break;

		case CLET :
			*mmok = 0;
			break;

		case CCHR :
		case CCHR | STAR :
		case CCHR | RANGE :
			if( *ep == ( CCHR | STAR ) ){
				if( *maxl != UNBOUNDED )
					( *maxl )++;
				*mmok = 0;
			}else if( *ep == ( CCHR | RANGE ) ){
				*minl += ep[2];
				if( *maxl != UNBOUNDED )
					*maxl += ep[3];
				if( ep[2] != ep[3] )
					*mmok = 0;
				ep += 2;
			}else{
				( *minl )++;
				if( *maxl != UNBOUNDED )
					( *maxl )++;
			}
			ep++;
			break;
		case CDOT :
		case CDOT | STAR :
		case CDOT | RANGE :
			if( *ep == ( CDOT | STAR ) ){
				if( *maxl != UNBOUNDED )
					( *maxl )++;
				*mmok = 0;
			}else if( *ep == ( CDOT | RANGE ) ){
				*minl += ep[2];
				if( *maxl != UNBOUNDED )
					*maxl += ep[3];
				if( ep[2] != ep[3] )
					*mmok = 0;
				ep += 2;
			}else{
				( *minl )++;
				if( *maxl != UNBOUNDED )
					( *maxl )++;
			}
			break;
		case CCL :
		case CCL | STAR :
		case CCL | RANGE :
		case NCCL :
		case NCCL | STAR :
		case NCCL | RANGE :
			if( *ep == (CCL|STAR) || *ep == (NCCL|STAR)){
				if( *maxl != UNBOUNDED )
					( *maxl )++;
				*mmok = 0;
			}else if( *ep == (CCL|RANGE) || *ep == (NCCL|RANGE) ){
				*minl += ep[16+1];
				if( *maxl != UNBOUNDED )
					*maxl += ep[3];
				if( ep[16+1] != ep[16+2] )
					*mmok = 0;
				ep += 2;	
			}else{
				( *minl )++;
				if( *maxl != UNBOUNDED )
					( *maxl )++;
			}
			ep += 16;
			break;
		case CDOL :
			*exact = circf;
			break;
		default :
			printf( " %2d", *ep );
			break;
		}
	}
}

void	mm_dumppat( fp, expbuf, e_expbuf )
FILE	*fp;
char	expbuf[];
char	*e_expbuf;
{
	char	*ep;
	int	i;

	for( ep = expbuf; ep < e_expbuf && *ep != CCEOF; ep++ ){
		switch( *ep ){
		case CBRA :
			ep++;
			printf( " \\(%d", *ep );
			break;
		case CKET :
			printf( " \\)" );
			break;

		case CBRC :
			printf( " \\<" );
			break;

		case CLET :
			printf( " \\>" );
			break;

		case CCHR :
		case CCHR | STAR :
		case CCHR | RANGE :
			printf( " %c", ep[1] );
			if( *ep == ( CCHR | STAR ) )
				printf( "*" );
			else if( *ep == ( CCHR | RANGE ) ){
				printf( "\\{%d,%d\\}", ep[2], ep[3] );
				ep += 2;
			}
			ep++;
			break;
		case CDOT :
		case CDOT | STAR :
		case CDOT | RANGE :
			printf( " ." );
			if( *ep == ( CDOT | STAR ) )
				printf( "*" );
			else if( *ep == ( CDOT | RANGE ) ){
				printf( "\\{%d,%d\\}", ep[2], ep[3] );
				ep += 2;
			}
			break;
		case CCL :
		case CCL | STAR :
		case CCL | RANGE :
		case NCCL :
		case NCCL | STAR :
		case NCCL | RANGE :
			if( ( *ep & ~RANGE ) == CCL )
				printf( " [" );
			else
				printf( " [^" );
			for( i = 1; i <= 16; i++ ){
				printf( " %02x", ep[i] & 0xff );
			}
			printf( " ]" );
			if( *ep == (CCL|STAR) || *ep == (NCCL|STAR))
				printf( "*" );
			else if( *ep == (CCL|RANGE) || *ep == (NCCL|RANGE) ){
				printf( "\\{%d,%d\\}", ep[16+1], ep[16+2] );
				ep += 2;	
			}
			ep += 16;
			break;
		case CDOL :
			printf( " $" );
			break;
		default :
			printf( " %2d", *ep );
			break;
		}
	}
	printf( " %s\n", "<eof>" );
}

int	mm_step( p1, p2, mm )
char	*p1;
char	*p2;
int	mm;
{

	loc1 = p1;
	if( circf )
		return( mm_advance( p1, p2, mm ) );

	do{
		if( mm_advance( p1, p2, mm ) ){
			loc1 = p1;
			return( 1 );
		}
	}while( *p1++ );
	return( 0 );
}

static	int	mm_advance( lp, ep, mm )
char	*lp;
char	*ep;
int	mm;
{
	int	neg, low;
	int	c, m;

	for( m  = 0; ; ){
		neg = 0;
		switch( *ep++ ){

		case CCHR :
			if( *ep++ == *lp++ )
				continue;
			else{
				m++;
				if( m > mm )
					return( 0 );
			}
			break;

		case CCHR | RANGE :
			c = *ep++;
			low = *ep;
			while( low-- ){
				if(*lp++ != c){
					m++;
					if( m > mm )
						return( 0 ); 
				}
			}
			ep += 2;
			break;

		case CDOT :
			if( *lp++ )
				continue;
			return( 0 );
			break;

		case CDOT | RANGE :
			low = *ep;
			while( low-- ){
				if( *lp++ == '\0' )
					return( 0 );
			}
			ep += 2;
			break;

		case CDOL :
			if( *lp == '\0' )
				continue;
			return( 0 );
			break;

		case CCEOF :
			loc2 = lp;
			return( 1 );
			break;

		case NCCL :
			neg = 1;
		case CCL :
			c = *lp++;
			if( ( ( c & 0200 ) == 0 && ISTHERE( c ) ) ^ neg ){
				ep += 16;
				continue;
			}else{
				m++;
				if( m > mm )
					return( 0 );
			}
			break;

		case NCCL | RANGE :
			neg = 1;
		case CCL | RANGE :
			low = ep[16];
			while( low-- ){
				c = *lp++;
				if((( c & 0200 ) || !ISTHERE(c)) ^ neg ){
					m++;
					if( m > mm )
						return( 0 );
				}
			}
			ep += 18;
			break;

		default :
			break;
		}
	}
	return( 0 );
}