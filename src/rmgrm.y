%{

#include <stdio.h>
#include "rnamot.h"

extern	VALUE_T	rm_tokval;
extern	int	rm_context;

static	NODE_T	*np;

%}

%token	SYM_PARMS
%token	SYM_DESCR
%token	SYM_SITES
%token	SYM_SCORE

%token	SYM_SS
%token	SYM_H5
%token	SYM_H3
%token	SYM_P5
%token	SYM_P3
%token	SYM_T1
%token	SYM_T2
%token	SYM_T3
%token	SYM_Q1
%token	SYM_Q2
%token	SYM_Q3
%token	SYM_Q4

%token	SYM_ACCEPT
%token	SYM_BREAK
%token	SYM_CONTINUE
%token	SYM_ELSE
%token	SYM_FOR
%token	SYM_IF
%token	SYM_IN
%token	SYM_REJECT
%token	SYM_WHILE

%token	SYM_IDENT
%token	SYM_INT
%token	SYM_FLOAT
%token	SYM_STRING

%token	SYM_AND
%token	SYM_ASSIGN
%token	SYM_DOLLAR
%token	SYM_DONT_MATCH
%token	SYM_EQUAL
%token	SYM_GREATER
%token	SYM_GREATER_EQUAL
%token	SYM_LESS
%token	SYM_LESS_EQUAL
%token	SYM_MATCH
%token	SYM_MINUS
%token	SYM_MINUS_ASSIGN
%token	SYM_MINUS_MINUS
%token	SYM_NEGATE
%token	SYM_NOT
%token	SYM_NOT_EQUAL
%token	SYM_OR
%token	SYM_PERCENT
%token	SYM_PERCENT_ASSIGN
%token	SYM_PLUS
%token	SYM_PLUS_ASSIGN
%token	SYM_PLUS_PLUS
%token	SYM_STAR
%token	SYM_STAR_ASSIGN
%token	SYM_SLASH
%token	SYM_SLASH_ASSIGN

%token	SYM_LPAREN
%token	SYM_RPAREN
%token	SYM_LBRACK
%token	SYM_RBRACK
%token	SYM_LCURLY
%token	SYM_RCURLY
%token	SYM_COLON
%token	SYM_COMMA
%token	SYM_SEMICOLON

%token	SYM_CALL
%token	SYM_LIST
%token	SYM_STREF

%token	SYM_ERROR

%%
program		: parm_part descr_part site_part score_part ;

parm_part	: SYM_PARMS { rm_context = CTX_PARMS; } pd_list
		| ;
descr_part	: SYM_DESCR { rm_context = CTX_DESCR; } se_list ;
site_part	: SYM_SITES { rm_context = CTX_SITES; } site_list
		| ;
score_part	: SYM_SCORE { rm_context = CTX_SCORE; } rule_list
				{ SC_accept(); }
		| ;

pd_list		: pdef
		| pdef pd_list ;
pdef		: asgn SYM_SEMICOLON ;

se_list		: strel
		| strel se_list ;
strel		: strhdr	{ if( rm_context == CTX_DESCR )
					SE_close();
				  else if( rm_context == CTX_SITES )
					POS_close();
				}
		| stref ;
strhdr		: strtype	{ if( rm_context == CTX_DESCR )
					SE_open( $1 );
				  else if( rm_context == CTX_SITES )
					POS_open( $1 );
				  else
					$$ = node( $1, 0, 0, 0 );
				} ;
strtype		: SYM_SS	{ $$ = SYM_SS; }
		| SYM_H5	{ $$ = SYM_H5; }
		| SYM_H3	{ $$ = SYM_H3; }
		| SYM_P5	{ $$ = SYM_P5; }
		| SYM_P3	{ $$ = SYM_P3; }
		| SYM_T1	{ $$ = SYM_T1; }
		| SYM_T2	{ $$ = SYM_T2; }
		| SYM_T3	{ $$ = SYM_T3; }
		| SYM_Q1	{ $$ = SYM_Q1; }
		| SYM_Q2	{ $$ = SYM_Q2; }
		| SYM_Q3	{ $$ = SYM_Q3; }
		| SYM_Q4	{ $$ = SYM_Q4; } ;

site_list	: site
		| site_list site ;
site		: pairing SYM_IN expr 
				{ SI_close( $3 ); } ;

rule_list	: rule
		| rule rule_list ;
rule		: expr 		{ SC_action( $1 ); }
			action	{ SC_endaction(); } ;
		| action ;
action		: SYM_LCURLY stmt_list SYM_RCURLY ;
stmt_list	: stmt
		| stmt stmt_list ;
stmt		: accept_stmt
		| asgn_stmt
		| auto_stmt
		| break_stmt
		| call_stmt
		| cmpd_stmt
		| continue_stmt
		| empty_stmt
		| for_stmt
		| if_stmt
		| reject_stmt
		| while_stmt ;
accept_stmt	: SYM_ACCEPT SYM_SEMICOLON
				{ SC_accept(); } ;
asgn_stmt	: asgn SYM_SEMICOLON
				{ SC_mark();
				  SC_expr( 0, $1 );
				  SC_clear();
				} ;
auto_stmt	: auto_lval SYM_SEMICOLON
				{ SC_mark();
				  SC_expr( 0, $1 );
				  SC_clear();
				} ;
break_stmt	: SYM_BREAK SYM_SEMICOLON
				{ SC_break(); } ;
call_stmt	: fcall SYM_SEMICOLON
				{ SC_expr( 0, $1 );
				  SC_clear();
				} ;
cmpd_stmt	: SYM_LCURLY stmt_list SYM_RCURLY ;
continue_stmt	: SYM_CONTINUE SYM_SEMICOLON
				{ SC_continue(); } ;
empty_stmt	: empty SYM_SEMICOLON ;
for_stmt	: for_hdr stmt	{ SC_endfor(); } ;
if_stmt		: if_hdr stmt	{ SC_endif(); }
		| if_hdr stmt SYM_ELSE
				{ SC_else(); } stmt
				{ SC_endelse(); } ;
reject_stmt	: SYM_REJECT SYM_SEMICOLON
				{ SC_reject(); } ;
while_stmt	: SYM_WHILE SYM_LPAREN expr { SC_while( $3 ); }
			SYM_RPAREN stmt
				{ SC_endwhile(); } ;
if_hdr		: SYM_IF SYM_LPAREN expr { SC_if( $3 ); } SYM_RPAREN ;
for_hdr		: SYM_FOR SYM_LPAREN for_ctrl SYM_RPAREN ;
for_ctrl	: for_init	{ SC_forinit( $1 ); }
			 SYM_SEMICOLON for_test
				{ SC_fortest( $4 ); }
			SYM_SEMICOLON for_incr
				{ SC_forincr( $7 ); } ;
for_init	: asgn		{ $$ = $1; }
		| auto_lval	{ $$ = $1; }
		| empty 	{ $$ = $1; } ;
for_test	: asgn		{ $$ = $1; }
		| expr		{ $$ = $1; }
		| empty		{ $$ = $1; } ;
for_incr	: asgn		{ $$ = $1; }
		| auto_lval	{ $$ = $1; }
		| empty		{ $$ = $1; } ;

asgn		: lval asgn_op asgn
				{ $$ = node( $2, 0, $1, $3 );
				  if( rm_context == CTX_PARMS )
					PARM_add( $$ );
				  else if( rm_context == CTX_DESCR ||
					rm_context == CTX_SITES )
					SE_addval( $$ );
				}
		| lval asgn_op expr
				{ $$ = node( $2, 0, $1, $3 );
				  if( rm_context == CTX_PARMS )
					PARM_add( $$ );
				  else if( rm_context == CTX_DESCR ||
					rm_context == CTX_SITES )
					SE_addval( $$ );
				} ;
asgn_op		: SYM_ASSIGN	{ $$ = SYM_ASSIGN; }
		| SYM_MINUS_ASSIGN
				{ $$ = SYM_MINUS_ASSIGN; }
		| SYM_PLUS_ASSIGN
				{ $$ = SYM_PLUS_ASSIGN; }
		| SYM_PERCENT_ASSIGN
				{ $$ = SYM_PERCENT_ASSIGN; }
		| SYM_SLASH_ASSIGN
				{ $$ = SYM_SLASH_ASSIGN; }
		| SYM_STAR_ASSIGN
				{ $$ = SYM_STAR_ASSIGN; } ;
expr		: conj		{ $$ = $1; }
		| expr SYM_OR conj
				{ $$ = node( SYM_OR, 0, $1, $3 ); } ;
conj		: compare	{ $$ = $1; }
		| compare SYM_AND conj
				{ $$ = node( SYM_AND, 0, $1, $3 ); } ;
compare		: a_expr	{ $$ = $1; }
		| a_expr comp_op a_expr
				{ $$ = node( $2, 0, $1, $3 ); } ;
comp_op		: SYM_DONT_MATCH
				{ $$ = SYM_DONT_MATCH; }
		| SYM_EQUAL	{ $$ = SYM_EQUAL; }
		| SYM_GREATER	{ $$ = SYM_GREATER; }
		| SYM_GREATER_EQUAL
				{ $$ = SYM_GREATER_EQUAL; }
		| SYM_IN	{ $$ = SYM_IN; }
		| SYM_LESS	{ $$ = SYM_LESS; }
		| SYM_LESS_EQUAL
				{ $$ = SYM_LESS_EQUAL; }
		| SYM_MATCH	{ $$ = SYM_MATCH; }
		| SYM_NOT_EQUAL	{ $$ = SYM_NOT_EQUAL; } ;
a_expr		: term		{ $$ = $1; }
		| a_expr add_op term
				{ $$ = node( $2, 0, $1, $3 ); } ;
add_op		: SYM_PLUS	{ $$ = SYM_PLUS; }
		| SYM_MINUS 	{ $$ = SYM_MINUS; } ;
term		: factor	{ $$ = $1; }
		| term mul_op factor
				{ $$ = node( $2, 0, $1, $3 ); } ;
mul_op		: SYM_PERCENT	{ $$ = SYM_PERCENT; }
		| SYM_SLASH	{ $$ = SYM_SLASH; }
		| SYM_STAR	{ $$ = SYM_STAR; } ;
factor		: primary	{ $$ = $1; }
		| SYM_MINUS primary
				{ $$ = node( SYM_NEGATE, 0, 0, $2 ); }
		| SYM_NOT primary
				{ $$ = node( SYM_NOT, 0, 0, $2 ); }
		| pairing	{ $$ = $1; } ;
pairing 	: stref		{ if( rm_context == CTX_SCORE )
					$$ = $1;
				}
		| stref SYM_COLON pairing
				{ if( rm_context == CTX_SCORE )
					$$ = node( SYM_COLON, 0, $1, $3 );
				} ;
primary		: lval		{ $$ = $1; }
		| literal	{ $$ = $1; }
		| fcall		{ $$ = $1; }
		| SYM_LPAREN expr SYM_RPAREN
				{ $$ = $2; } ;
fcall		: ident SYM_LPAREN e_list SYM_RPAREN
				{ $$ = node( SYM_CALL, 0, $1, $3 ); } ;
stref		: strhdr SYM_LPAREN a_list SYM_RPAREN
				{ if( rm_context == CTX_DESCR )
					SE_close();
				  else if( rm_context == CTX_SITES )
					POS_close();
				  else if( rm_context == CTX_SCORE )
					$$ = node( SYM_STREF, 0, $1, $3 );
				} ;
lval		: ident		{ $$ = $1; }
		| auto_lval	{ $$ = $1; } ;
auto_lval	: incr_op ident	{ $$ = node( $1, 0, 0, $2 ); }
		| ident incr_op	{ $$ = node( $2, 0, $1, 0 ); } ;
literal		: SYM_INT	{ $$ = node( SYM_INT, &rm_tokval, 0, 0 ); }
		| SYM_FLOAT	{ $$ = node( SYM_FLOAT, &rm_tokval, 0, 0 ); }
		| SYM_STRING	{ $$ = node( SYM_STRING, &rm_tokval, 0, 0 ); } 
		| SYM_DOLLAR	{ $$ = node( SYM_DOLLAR, &rm_tokval, 0, 0 ); }
		| pairset	{ $$ = $1; } ;
ident		: SYM_IDENT 	{ $$ = node( SYM_IDENT, &rm_tokval, 0, 0 ); } ;
incr_op		: SYM_MINUS_MINUS
				{ $$ = SYM_MINUS_MINUS; }
		| SYM_PLUS_PLUS	{ $$ = SYM_PLUS_PLUS; } ;
e_list		: expr		{ if( rm_context != CTX_SCORE )
					PR_add( $1 );
				  else
					$$ = node( SYM_LIST, 0, $1, 0 );
				}
		| expr SYM_COMMA e_list
				{ if( rm_context != CTX_SCORE )
					PR_add( $1 );
				  else
					$$ = node( SYM_LIST, 0, $1, $3 );
				} ;
a_list		: asgn		{ if( rm_context == CTX_SCORE )
					$$ = node( SYM_LIST, 0, $1, 0 );
				}
		| asgn SYM_COMMA a_list
				{ if( rm_context == CTX_SCORE )
					$$ = node( SYM_LIST, 0, $1, $3 );
				} ;
pairset		: SYM_LCURLY 	{ if( rm_context != CTX_SCORE )
					PR_open();
				} e_list SYM_RCURLY
				{ if( rm_context != CTX_SCORE )
					$$ = PR_close();
				  else
					$$ = node( SYM_LCURLY, 0, 0, $2 );
				} ;
empty		: 		{ $$ = (  int )NULL; } ;
%%

#include "lex.yy.c"

int	yyerror( msg )
char	msg[];
{

	fprintf( stderr, "yyerror: %s\n", msg );
	return( 0 );
}
