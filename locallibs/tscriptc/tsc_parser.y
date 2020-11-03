%{
#include <stdio.h>
#include <iostream>
#include <stdexcept>
#include <sstream>
#include "tsc_ast.h"

#define YYERROR_VERBOSE
#define YYDEBUG 1
#define YYLTYPE ast::LOC
#define YYINITDEPTH 10000

int yylex();
int tsc_yylex_destroy();
void yyerror(const char* s);

/* copy-paste from bison manual + file processing */
# define YYLLOC_DEFAULT(Current, Rhs, N) \
 do                                                                  \
   if (N)                                                            \
     {                                                               \
       (Current).first_file   = YYRHSLOC(Rhs, 1).first_file;         \
       (Current).first_line   = YYRHSLOC(Rhs, 1).first_line;         \
       (Current).first_column = YYRHSLOC(Rhs, 1).first_column;       \
       (Current).last_file    = YYRHSLOC(Rhs, N).last_file;          \
       (Current).last_line    = YYRHSLOC(Rhs, N).last_line;          \
       (Current).last_column  = YYRHSLOC(Rhs, N).last_column;        \
     }                                                               \
   else                                                              \
     {                                                               \
       (Current).first_file   = (Current).last_file  =               \
         YYRHSLOC(Rhs, 0).last_file;                                 \
       (Current).first_line   = (Current).last_line   =              \
         YYRHSLOC(Rhs, 0).last_line;                                 \
       (Current).first_column = (Current).last_column =              \
         YYRHSLOC(Rhs, 0).last_column;                               \
     }                                                               \
 while (0)

%}

%locations /* enable locations tracking */

%union {
    char symbol;
    ast::Suite* suite;
    ast::Seq* seq;
    ast::Str* str;
    ast::Quote* quote;
    ast::Call* call;
    ast::Params* params;
    ast::Param* param;
}

%token MODULE_SEP
%token MASK_BEG
%token MASK_END
%token <str> JSON
%token <str> BLOCK_BEG
%token BLOCK_END
%token BLOCK_EOF
%token <symbol> CHAR
%token <symbol> BLANK
%token <str> ASSIGN
%token CALL

%token <symbol> '\n'
%token <symbol> '{'
%token <symbol> '}'
%token <symbol> '('
%token <symbol> ')'

%type <suite> suite
%type <seq> module
%type <seq> nonempty_line_quote
%type <call> block
%type <call> json_block
%type <params> block_params
%type <seq> json_body
%type <seq> block_body
%type <seq> block_body_
%type <seq> block_end
%type <symbol> line_char
%type <symbol> json_char
%type <quote> quote
%type <seq> json_quote
%type <call> call
%type <params> call_params
%type <param> param
%type <param> named_param
%type <param> unnamed_param
%type <seq> val
%type <seq> balanced_val

%%

suite
    : module { $$ = ast::Suite::getCurrentInstance(); $$->modules.push_back($1); }
    | suite MODULE_SEP module { $$->modules.push_back($3); }
    ;
module
    : module block { $$->append($2); }
    | module nonempty_line_quote '\n' { $$->append($2); $$->append(new ast::Str(@3, $3)); }
    | module '\n' { $$->append(new ast::Str(@2, $2)); }
    | %empty { $$ = new ast::Seq; }
    ;
json_body
    : json_body json_char { $$->append(new ast::Str(@2, $2)); }
    | json_body json_quote { $$->append($2); }
    | json_body ASSIGN { $$->append($2); }
    | json_body call { $$->append($2); }
    | %empty { $$ = new ast::Seq; }
    ;
nonempty_line_quote
    : line_char { $$ = new ast::Seq(@1, $1); }
    | quote { $$ = new ast::Seq($1); }
    | call { $$ = new ast::Seq($1); }
    | ASSIGN { $$ = new ast::Seq($1); }
    | json_block { $$ = new ast::Seq($1); }
    | nonempty_line_quote line_char { $$->append(@2, $2); }
    | nonempty_line_quote quote { $$->append($2); }
    | nonempty_line_quote call { $$->append($2); }
    | nonempty_line_quote ASSIGN { $$->append($2); }
    | nonempty_line_quote json_block { $$->append($2); }
    ;
json_block
    : JSON json_quote
    {
        ast::Params* params = new ast::Params;
        params->params.push_back(new ast::Param(NULL, $2));
        $$ = new ast::Call(false, @$, new ast::Seq(new ast::Str(@1, "json")), params);
    }
    ;
block
    : BLOCK_BEG opt_blanks block_params block_end
        {
            $$ = new ast::Call(true, @$, new ast::Seq($1), $3);
        }
    | BLOCK_BEG opt_blanks block_params '\n' block_body
        {
            $$ = new ast::Call(true, @$, new ast::Seq($1), $3);
            $$->params.push_back(new ast::Param(NULL, new ast::Seq(new ast::Quote(@5, $5))));
        }
    ;
block_body
    : block_body_ block_end { $$->append($2); }
    ;
block_body_
    : nonempty_line_quote
    | block_body_ '\n' nonempty_line_quote { $$->append(@2, $2); $$->append($3); }
    ;
block_end
    : BLOCK_EOF { $$ = new ast::Seq(new ast::Str(@$, '\n')); }
    | '\n' BLOCK_EOF { $$ = new ast::Seq(new ast::Str(@1, '\n')); }
    | BLOCK_END { $$ = new ast::Seq; }
    | '\n' BLOCK_END { $$ = new ast::Seq(new ast::Str(@1, '\n')); }
    ;
block_params
    : param { $$ = new ast::Params; $$->params.push_back($1); }
    | param blanks block_params { $$ = $3; $$->params.insert($$->params.begin(), $1); }
    | %empty { $$ = new ast::Params; }
    ;
json_char
    : line_char
    | '\n'
    ;
line_char
    : CHAR
    | BLANK
    | '('
    | ')'
    ;
quote
    : '{' module '}' { $$ = new ast::Quote(@$, $2); }
    | '{' module nonempty_line_quote '}' { $2->append($3); $$ = new ast::Quote(@$, $2); }
    ;
json_quote
    : '{' json_body '}'
    {
        $$ = new ast::Seq(new ast::Str(@1, $1));
        $$->append($2);
        $$->append(new ast::Str(@3, $3));
    }
    ;
call
    : CALL ')' { $$ = new ast::Call(false, @$, new ast::Seq, new ast::Params); }
    | CALL val call_params opt_whitespace ')' { $$ = new ast::Call(false, @$, $2, $3); }
    | MASK_BEG block_params MASK_END { $$ = new ast::Call(false, @$, new ast::Seq(new ast::Str(@1, "mask")), $2); }
    ;
call_params
    : call_params whitespace param { $$->params.push_back($3); }
    | %empty { $$ = new ast::Params; }
    ;
param
    : named_param
    | unnamed_param
    ;
named_param
    : val ASSIGN opt_whitespace val {  $$ = new ast::Param($1, $4); }
    ;
unnamed_param
    : val { $$ = new ast::Param(NULL, $1); }
    ;
val
    : CHAR { $$ = new ast::Seq(@1, $1); }
    | quote { $$ = new ast::Seq($1); }
    | call { $$ = new ast::Seq($1); }
    | balanced_val
    | val CHAR { $$->append(@2, $2); }
    | val quote { $$->append($2); }
    | val call { $$->append($2); }
    | val balanced_val { $$->append($2); }
    ;
balanced_val
    : '(' val ')' { $$ = new ast::Seq(@1, $1); $$->append($2); $$->append(@3, $3); }
    ;
opt_whitespace
    : whitespace
    | %empty
    ;
whitespace
    : BLANK
    | '\n'
    | whitespace BLANK
    | whitespace '\n'
    ;
opt_blanks
    : blanks
    | %empty
    ;
blanks
    : BLANK
    | blanks BLANK
    ;

%%

void yyerror(const char* err)
{
    const ast::Pos pos = ast::Suite::getCurrentInstance()->lexer.pos.getPos();
    std::ostringstream out;
    if (!pos.file.empty())
        out << pos.file << ' ';
    out << "line " << pos.line << ", col " << pos.col << " : " << err;
    throw std::runtime_error(out.str());
}

void Parse(FILE* in, ast::Suite& suite)
{
    extern FILE* tsc_yyin; /* lexer input */
    tsc_yyin = in;
    yyparse();
    tsc_yylex_destroy();
}

