#include <stdio.h>
#include <string.h>
#include "rnamot.h"
#include "y.tab.h"

#define	MIN(a,b)	((a)<(b)?(a):(b))
#define	MAX(a,b)	((a)>(b)?(a):(b))

extern	int	rm_emsg_lineno;
extern	STREL_T	rm_descr[];
extern	int	rm_n_descr;
extern	int	rm_dminlen;
extern	int	rm_dmaxlen;
extern	int	rm_b2bc[];

extern	SEARCH_T	**rm_searches;

static	char	fm_emsg[ 256 ];
static	char	*fm_locus;
static	int	fm_comp;
static	int	fm_slen;
static	char	*fm_sbuf;
static	int	*fm_winbuf;	/* windowsize + 2, 1 before, 1 after	*/
static	int	*fm_window;	/* fm_winbuf[1]				*/
static	int	fm_windowsize;
static	char	*fm_chk_seq;

static	int	find_motif();
static	int	find_1_motif();
static	int	find_ss();
static	int	find_wchlx();
static	int	find_pknot();
static	int	match_helix();
static	int	paired();

static	void	print_match();

IDENT_T	*find_id();

int	find_motif_driver( n_searches, searches, sites, locus, slen, sbuf )
int	n_searches;
SEARCH_T	*searches[];
SITE_T	*sites;
char	locus[];
int	slen;
char	sbuf[];
{
	int	i, w_winsize, slev;
	int	szero, l_szero;
	int	sdollar, f_sdollar, l_sdollar;
	IDENT_T	*ip;
	SEARCH_T	*srp;
	
	if( fm_winbuf == NULL ){
		ip = find_id( "windowsize" );
		if( ip == NULL )
			errormsg( 1,
				"find_motif_driver: windowsize undefined." );
	
			if( ip->i_val.v_value.v_ival <= 0 )
				errormsg( 1,
					"find_motif_driver: windowsize <= 0." );
		else
			fm_windowsize = ip->i_val.v_value.v_ival;
		fm_winbuf = ( int * )malloc( (fm_windowsize+2) * sizeof(int) );
		if( fm_winbuf == NULL )
			errormsg( 1,
				"find_motif_driver: can't allocate fm_winbuf.");
		fm_window = &fm_winbuf[ 1 ];
		fm_chk_seq = ( char * )malloc((fm_windowsize+1) * sizeof(char));
		if( fm_chk_seq == NULL )
			errormsg( 1,
			"find_motif_driver: can't allocate fm_chk_seq." );
	}

	fm_locus = locus;
	fm_slen = slen;
	fm_sbuf = sbuf;

	w_winsize = rm_dmaxlen < fm_windowsize ? rm_dmaxlen : fm_windowsize;

	srp = searches[ 0 ];
	l_szero = slen - w_winsize;
	for( szero = 0; szero < l_szero; szero++ ){
		srp->s_zero = szero;
		srp->s_dollar = szero + w_winsize - 1;
		fm_window[ srp->s_zero - 1 ] = UNDEF;
		fm_window[ srp->s_dollar + 1 ] = UNDEF;
		find_motif( srp );
	}

	l_szero = slen - rm_dminlen;
	srp->s_dollar = slen - 1;
	for( ; szero <= l_szero; szero++ ){
		srp->s_zero = szero;
		find_motif( srp );
	}

	return( 0 );
}

static	int	find_motif( srp )
SEARCH_T	*srp;
{
	STREL_T	*stp;
	SEARCH_T	*n_srp;
	int	sdollar, o_sdollar, f_sdollar, l_sdollar; 
	int	rv, loop;

	rv = 0;
	stp = srp->s_descr;
	if( stp->s_next != NULL ){
		n_srp = rm_searches[ stp->s_next->s_searchno ]; 
		loop = 1;
	}else if( stp->s_outer == NULL ){
		n_srp = NULL;
		loop = 1;
	}else{
		n_srp = NULL;
		loop = 0;
	}
	o_sdollar = srp->s_dollar;
	f_sdollar = MIN( srp->s_dollar, srp->s_zero + stp->s_maxglen - 1 );
	l_sdollar = srp->s_zero + stp->s_minglen - 1;

	if( loop ){
		for( sdollar = f_sdollar; sdollar >= l_sdollar; sdollar-- ){
			srp->s_dollar = sdollar;

			if( n_srp != NULL ){
				n_srp->s_zero = sdollar + 1;
				n_srp->s_dollar = o_sdollar;
			}else{

			}

			rv = find_1_motif( srp );
		}
	}else	
		rv = find_1_motif( srp );

	srp->s_dollar = o_sdollar;

	return( rv );
}

static	int	find_1_motif( srp )
SEARCH_T	*srp;
{
	STREL_T	*stp;
	int	rv;

	stp = srp->s_descr;

	switch( stp->s_type ){
	case SYM_SS :
		rv = find_ss( srp );
		break;
	case SYM_H5 :
		if( stp->s_proper ){
			rv = find_wchlx( srp );
		}else{
/*
			rv = find_pknot(  slev, n_searches, searches,
				szero, sdollar );
*/
		}
		break;
	case SYM_P5 :
		rm_emsg_lineno = stp->s_lineno;
		errormsg( 1, "parallel helix finder not implemented." );
		break;
	case SYM_T1 :
		rm_emsg_lineno = stp->s_lineno;
		errormsg( 1, "triple helix finder not implemented." );
		break;
	case SYM_Q1 :
		rm_emsg_lineno = stp->s_lineno;
		errormsg( 1, "quad helix finder not implemented." );
		break;
	case SYM_H3 :
	case SYM_P3 :
	case SYM_T2 :
	case SYM_T3 :
	case SYM_Q2 :
	case SYM_Q3 :
	case SYM_Q4 :
	default :
		rm_emsg_lineno = stp->s_lineno;
		sprintf( fm_emsg, "find_motif: illegal symbol %d.",
			stp->s_type );
		errormsg( 1, fm_emsg );
		break;
	}

	return( rv );
}

static	int	find_ss( srp )
SEARCH_T	*srp;
{
	STREL_T	*stp, *n_stp;
	SEARCH_T	*n_srp;
	int	s, slen, szero, sdollar;

	stp = srp->s_descr;
	szero = srp->s_zero;
	sdollar = srp->s_dollar;
	slen = sdollar - szero + 1;

	if( stp->s_seq != NULL ){
		strncpy( fm_chk_seq, &fm_sbuf[ szero ], slen );
		fm_chk_seq[ slen ] = '\0';
		if( !step( fm_chk_seq, stp->s_expbuf ) )
			return( 0 );
	}

	for( s = 0; s < slen; s++ ) 
		fm_window[ szero + s ] = stp->s_index;
	stp->s_matchoff = szero;
	stp->s_matchlen = slen;

	n_stp = srp->s_forward;
	if( n_stp != NULL ){
		n_srp = rm_searches[ n_stp->s_searchno ];
		if( find_motif( n_srp ) ){
			return( 1 );
		}else{
			for( s = 0; s < slen; s++ ) 
				fm_window[ szero + s ] = UNDEF;
			return( 0 );
		}
	}else{
		print_match( stdout, fm_locus, fm_comp,
			rm_n_descr, rm_descr );
	}

		return( 1 );
}

static	int	find_wchlx( srp )
SEARCH_T	*srp;
{
	STREL_T	*stp, *stp3, *i_stp, *n_stp;
	int	s, s3lim, slen, szero, sdollar;
	int	h_minl, h_maxl;
	int	i_minl, i_maxl, i_len;
	int	h, h3, hlen;
	SEARCH_T	*i_srp, *n_srp;

	szero = srp->s_zero;
	sdollar = srp->s_dollar;
	slen = sdollar - szero + 1;
	stp = srp->s_descr;

	h_minl = stp->s_minlen;
	h_maxl = stp->s_maxlen;
	i_minl = stp->s_minilen;
	i_maxl = stp->s_maxilen;

	s3lim = sdollar - szero + 1;
	s3lim = ( s3lim - i_minl ) / 2;
	s3lim = MIN( s3lim, h_maxl );
	s3lim = sdollar - s3lim + 1;

	if( match_helix( stp, szero, sdollar, s3lim, &h3, &hlen ) ){

		stp->s_matchoff = szero;
		stp->s_matchlen = hlen;
		stp3 = stp->s_mates[ 0 ];
		stp3->s_matchoff = h3 - hlen + 1;
		stp3->s_matchlen = hlen;

		i_len = h3 - szero - 2 * hlen + 1;
		if( i_len > i_maxl )
			return( 0 );

		for( h = 0; h < hlen; h++ ){
			fm_window[ szero+h ] = stp->s_index;
			fm_window[ sdollar-h ] = stp->s_index;
		}

		i_stp = stp->s_inner;
		i_srp = rm_searches[ i_stp->s_searchno ];
		i_srp->s_zero = szero + hlen;
		i_srp->s_dollar = h3 - hlen;

		if( find_motif( i_srp ) ){
			return( 1 );
		}else{
			for( h = 0; h < hlen; h++ ){
				fm_window[ szero+h ] = UNDEF;
				fm_window[ sdollar-h ] = UNDEF;
			}
			return( 0 );
		}
	}else
		return( 0 );
}

static	int	find_pknot(  slev, n_searches, searches, szero, sdollar )
int	slev;
int	n_searches;
SEARCH_T	*searches[];
int	szero;
int	sdollar;
{

}
static	int	match_helix( stp, s5, s3, s3lim, h3, hlen )
STREL_T	*stp;
int	s5;
int	s3;
int	s3lim;
int	*h3;
int	*hlen;
{
	STREL_T	*stp3;
	int	s, s3_5plim;
	int	b5, b3;
	int	bpcnt, mpr;

	stp3 = stp->s_scopes[ 1 ];
	b5 = fm_sbuf[ s5 ];
	b3 = fm_sbuf[ s3 ];
	if( !paired( stp, b5, b3 ) )
		return( 0 );

	*h3 = s3;
	bpcnt = 1;
	*hlen = 1;
	mpr = 0;
	for( bpcnt = 1, *hlen = 1, s = s3 - 1; s >= s3lim; s--, (*hlen)++ ){

		b5 = fm_sbuf[ s5 + *hlen ];
		b3 = fm_sbuf[ s3 - *hlen ];
		if( paired( stp, b5, b3 ) ){
			bpcnt++;
		}else{
			mpr++;
			if( mpr > stp->s_mispair ){
				if( *hlen < stp->s_minlen )
					return( 0 );
				else
					break;
			}
		}
	}

	if( stp->s_seq != NULL ){
		strncpy( fm_chk_seq,  &fm_sbuf[ s5 ], *hlen );
		fm_chk_seq[ *hlen ] = '\0';
		if( !step( fm_chk_seq, stp->s_expbuf ) )
			return( 0 );
	}else if( stp3->s_seq != NULL ){
		strncpy( fm_chk_seq,  &fm_sbuf[ s3 - *hlen + 1 ], *hlen );
		fm_chk_seq[ *hlen ] = '\0';
		if( !step( fm_chk_seq, stp3->s_expbuf ) )
			return( 0 );
	}

	return( 1 );
}

static	int	paired( stp, b5, b3 )
STREL_T	*stp;
int	b5;
int	b3;
{
	BP_MAT_T	*bpmatp;
	int	b5i, b3i;
	int	rv;
	
	bpmatp = stp->s_pairset->ps_mat;
	b5i = rm_b2bc[ b5 ];
	b3i = rm_b2bc[ b3 ];
	rv = (*bpmatp)[b5i][b3i];
	return( rv );
}

static	void	print_match( fp, locus, comp, n_descr, descr )
FILE	*fp;
char	locus[];
int	comp;
int	n_descr;
STREL_T	descr[];
{
	int	d;
	STREL_T	*stp;

	fprintf( fp, "%-12s %d", locus, comp );
	stp = descr; 
	fprintf( fp, " %4d %.*s", stp->s_matchoff + 1,
		stp->s_matchlen, &fm_sbuf[ stp->s_matchoff ] );

	for( ++stp, d = 1; d < n_descr; d++, stp++ ){
		if( stp->s_matchlen > 0 )
			fprintf( fp, " %.*s", stp->s_matchlen,
				&fm_sbuf[ stp->s_matchoff ] );
		else
			fprintf( fp, " ." );
	}
	fprintf( fp, "\n" );
}
