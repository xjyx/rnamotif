%{

#include <stdio.h>
#include "rnamot.h"

extern	VALUE_T	rmval;

%}

%token	SYM_PARM
%token	SYM_DESCR
%token	SYM_SITE

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

%token	SYM_IDENT
%token	SYM_INT
%token	SYM_STRING

%token	SYM_EQUAL
%token	SYM_PLUS_EQUAL
%token	SYM_MINUS_EQUAL
%token	SYM_PLUS
%token	SYM_MINUS
%token	SYM_DOLLAR
%token	SYM_LPAREN
%token	SYM_RPAREN
%token	SYM_LCURLY
%token	SYM_RCURLY
%token	SYM_PERIOD
%token	SYM_COMMA
%token	SYM_COLON
%token	SYM_ERROR

%%
program		: parm_part descr_part site_part ;

parm_part	: SYM_PARM { set_context( SYM_PARM ); } assign_list
		| ;
assign_list	: assign
		| assign assign_list ;
assign		: ident assign_op expr
				{ $$ = node( $2, 0, $1, $3 ); } ;
assign_op	: SYM_EQUAL	{ $$ = SYM_EQUAL; }
		| SYM_PLUS_EQUAL
				{ $$ = SYM_PLUS_EQUAL; }
		| SYM_MINUS_EQUAL
				{ $$ = SYM_MINUS_EQUAL; } ;
expr		: val 		{ $$ = $1; }
		| val add_op expr
				{ $$ = node( $2, 0, $1, $3 ); } ;
add_op		: SYM_PLUS	{ $$ = SYM_PLUS; }
		| SYM_MINUS 	{ $$ = SYM_MINUS; } ;
val		: ident		{ $$ = $1; }
		| intval	{ $$ = $1; }
		| lastval	{ $$ = $1; }
		| strval	{ $$ = $1; }
		| pairval 	{ $$ = $1; } ;

ident		: SYM_IDENT 	{ $$ = node( SYM_IDENT, &rmval, 0, 0 ); } ;
intval		: SYM_INT	{ $$ = node( SYM_INT, &rmval, 0, 0 ); } ;
lastval		: SYM_DOLLAR 	{ $$ = node( SYM_DOLLAR, &rmval, 0, 0 ); } ;
strval		: SYM_STRING 	{ $$ = node( SYM_STRING, &rmval, 0, 0 ); } ;
pairval		: SYM_LCURLY pair_list SYM_RCURLY ;
pair_list	: pair
		| pair SYM_COMMA pair_list ;
pair		: SYM_STRING ;

descr_part	: SYM_DESCR { set_context( SYM_DESCR ); } strel_list ;
strel_list	: strel
		| strel strel_list ;
strel		: strtype SYM_LPAREN strparm_list SYM_RPAREN ;
strtype		: SYM_SS	{ SE_new( SYM_SS ); }
		| SYM_H5	{ SE_new( SYM_H5 ); }
		| SYM_H3	{ SE_new( SYM_H3 ); }
		| SYM_P5	{ SE_new( SYM_H5 ); }
		| SYM_P3	{ SE_new( SYM_P3 ); }
		| SYM_T1	{ SE_new( SYM_T1 ); }
		| SYM_T2	{ SE_new( SYM_T2 ); }
		| SYM_T3	{ SE_new( SYM_T3 ); }
		| SYM_Q1	{ SE_new( SYM_Q1 ); }
		| SYM_Q2	{ SE_new( SYM_Q2 ); }
		| SYM_Q3	{ SE_new( SYM_Q3 ); }
		| SYM_Q4	{ SE_new( SYM_Q4 ); } ;
strparm_list	: strparm
		| strparm SYM_COMMA strparm_list ;
strparm		: assign 	{ SE_addval( $1); } ;

site_part	: SYM_SITE { set_context( SYM_SITE ); } site_list
		| ;
site_list	: site
		| site site_list ;
site		: siteaddr_list SYM_EQUAL pairval ;
siteaddr_list	: strel
		| strel SYM_COLON siteaddr_list ;
%%

#include "lex.yy.c"

int	yyerror( msg )
char	msg[];
{

	fprintf( stderr, "yyerror: %s\n", msg );
	return( 0 );
}
