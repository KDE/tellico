#ifndef tokens_h
#define tokens_h
/* tokens.h -- List of labelled tokens and stuff
 *
 * Generated from: bibtex.g
 *
 * Terence Parr, Will Cohen, and Hank Dietz: 1989-1994
 * Purdue University Electrical Engineering
 * ANTLR Version 1.33
 */
#define zzEOF_TOKEN 1
#define AT 2
#define COMMENT 4
#define NUMBER 9
#define NAME 10
#define LBRACE 11
#define RBRACE 12
#define ENTRY_OPEN 13
#define ENTRY_CLOSE 14
#define EQUALS 15
#define HASH 16
#define COMMA 17
#define STRING 25

void bibfile(AST**_root);
void entry(AST**_root);
void body(AST**_root, bt_metatype metatype );
void contents(AST**_root, bt_metatype metatype );
void fields(AST**_root);
void field(AST**_root);
void value(AST**_root);
void simple_value(AST**_root);

#endif
extern SetWordType zzerr1[];
extern SetWordType zzerr2[];
extern SetWordType zzerr3[];
extern SetWordType zzerr4[];
extern SetWordType setwd1[];
extern SetWordType zzerr5[];
extern SetWordType setwd2[];
