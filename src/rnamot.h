#ifndef	__RNAMOT__
#define	__RNAMOT__

#define	VERSION	"v1.4.0 2000-dec-17"

#define	U_MSG_S	\
"usage: %s [ options ] -descr descr-file [ -dtype dtype ] [ seq-file ]\n\n\
options:\n\
\t-c\n\
\t-d\n\
\t-h\n\
\t-s\n\
\t-v\n\
\t-Did=expr\n\
\t-Idir\n\
\t-help\n\
\n\
-descr descr-file\n\
\n\
-dtype { genbank | fastn }\n"

#define	UNDEF	(-1)
#define	UNBOUNDED	0x7fffffff
#define	EFN_INFINITY	16000	/* ? */

#define	MIN(a,b)	((a)<(b)?(a):(b))
#define	MAX(a,b)	((a)>(b)?(a):(b))

#define	DT_FASTN	0
#define	DT_GENBANK	1

#define	T_UNDEF		0
#define	T_INT		1
#define	T_FLOAT		2
#define	T_STRING	3
#define	T_PAIRSET	4
#define	T_POS		5
#define	T_IDENT		6
#define	N_TYPE		7
#define	T_IJ(i,j)	((i)*N_TYPE+(j))

#define	C_UNDEF		0
#define	C_LIT		1
#define	C_VAR		2
#define	C_EXPR		3

#define	S_UNDEF		0
#define	S_GLOBAL	1
#define	S_STREL		2
#define	S_SITE		3

	/* context for the parser/lexer	*/
#define	CTX_START	0
#define	CTX_PARMS	1
#define	CTX_DESCR	2
#define	CTX_SITES	3
#define	CTX_SCORE	4

typedef	struct	value_t	{
	int	v_type;
	union	{
		int	v_ival;
		double	v_dval;
		void	*v_pval;
	} v_value;
} VALUE_T;

typedef	struct	ident_t	{
	struct	ident_t	*i_left;
	struct	ident_t	*i_right;
	char	*i_name;
	int	i_type;
	int	i_class;
	int	i_scope;
	int	i_reinit;
	VALUE_T	i_val;
} IDENT_T;

#define	BCODE_A		0
#define	BCODE_C		1
#define	BCODE_G		2
#define	BCODE_T		3
#define	BCODE_N		4
#define N_BCODES	5

typedef	int	BP_MAT_T[ N_BCODES ][ N_BCODES ];
typedef	int	BT_MAT_T[ N_BCODES ][ N_BCODES ][ N_BCODES ];
typedef	int	BQ_MAT_T[ N_BCODES ][ N_BCODES ][ N_BCODES ][ N_BCODES ];

typedef	struct	pair_t	{
	int	p_n_bases;
	char	p_bases[ 4 ];
} PAIR_T;

typedef	struct	pairset_t	{
	int	ps_n_pairs;
	PAIR_T	*ps_pairs;
	void	*ps_mat[ 2 ];	/* 1 for duplex, 2 for 3-plex, 4-plex */
} PAIRSET_T;

typedef	struct	pairlist_t	{
	struct	pairlist_t	*pl_next;
	PAIRSET_T	*pl_pset;
} PAIRLIST_T;

typedef	struct	node_t	{
	int	n_sym;
	int	n_type;
	int	n_class;
	int	n_lineno;
	char	*n_filename;
	VALUE_T	n_val;
	struct	node_t	*n_left;
	struct	node_t	*n_right;
} NODE_T;

typedef	struct	addr_t	{
	int	a_l2r;
	int	a_offset;
} ADDR_T;

typedef	struct	pos_t	{
	int	p_type;
	int	p_lineno;
	char	*p_tag;
	int	p_dindex;
	ADDR_T	p_addr;
} POS_T;

typedef	struct	site_t	{
	struct	site_t	*s_next;
	POS_T	*s_pos;
	int	s_n_pos;
	PAIRSET_T	*s_pairset;
} SITE_T;

#define	SA_PROPER	001
#define	SA_5PAIRED	002
#define	SA_3PAIRED	004
#define	SA_N_ATTR	3

typedef	struct	strel_t	{
	int	s_checked;	/* used during linking		*/
	int	s_type;
	int	s_attr;		/* holds set of SA_*		*/
	char	s_index;	/* index into descr array	*/
	int	s_lineno;
	int	s_searchno;	/* index into searches[]	*/
	int	s_matchoff;	/* matched string starts here	*/
	int	s_matchlen;	/* matched string is this long	*/
	int	s_n_mismatches;	/* number of mismatches		*/
	int	s_n_mispairs;	/* number of mismatches		*/
	char	*s_tag;
	struct	strel_t	*s_next;
	struct	strel_t	*s_prev;
	struct	strel_t	*s_inner;
	struct	strel_t	*s_outer;
	struct	strel_t	**s_mates;
	int	s_n_mates;
	struct	strel_t	**s_scopes;
	int	s_n_scopes;
	int	s_scope;
	int	s_minlen;
	int	s_maxlen;
	int	s_minglen;
	int	s_maxglen;
	int	s_minilen;
	int	s_maxilen;
	ADDR_T	s_start;
	ADDR_T	s_stop;
	char	*s_seq;
	char	*s_expbuf;
	char	*s_e_expbuf;
	int	s_mismatch;
	double	s_matchfrac;
	int	s_mispair;
	double	s_pairfrac;
	PAIRSET_T	*s_pairset;
} STREL_T;

typedef	struct	search_t	{
	STREL_T	*s_descr;
	STREL_T	*s_forward;
	STREL_T	*s_backup;
	int	s_zero;
	int	s_dollar;
} SEARCH_T;

typedef	struct	incdir_t	{
	struct	incdir_t	*i_next;
	char	*i_name;
} INCDIR_T;

#endif
