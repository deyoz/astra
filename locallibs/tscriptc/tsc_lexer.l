%{
#include <sstream>
#include "tsc_ast.h"
#define YYLTYPE ast::LOC
#include "tsc_parser.tab.h"

#define yylval tsc_yylval
#define YY_USER_ACTION TrackCurrentPosition(yytext, yyleng);

static void UnputChar(char c);
static void UnputStr(const char* lexeme, size_t len);
static void Syncline(const char* lexeme, size_t len); /* handling M4 synclines */
static void TrackCurrentPosition(const char* lexeme, size_t len);
%}

%option noyywrap
%option stack
%option noyy_top_state

P_SYNC #line[ ][0-9]+([ ]\"[^"\n]+\")?\n
P_SKIP (([[:blank:]]+)|([[:blank:]]*#[^}\n]*))
P_MODULE_SEP %%[[:blank:]]*(#.*)?\n
P_BLOCK_BEG (!!)|(<<)|(>>)|("??")|(@@)

%s S_BLOCK S_DBLQ S_MASK

%%

^{P_SYNC} Syncline(yytext, yyleng); yy_set_bol(1); /* M4 syncline */
^{P_SKIP}$ yy_set_bol(1); /* skip comments and blanks */
{P_SKIP}$ /* skip comments and blanks at the right */
[\\]{P_SKIP}?\n /* concatenate */
<INITIAL>^{P_MODULE_SEP} return MODULE_SEP;

^~[[:blank:]]* { yy_push_state(S_MASK); return MASK_BEG; }
<S_MASK>\n { UnputChar('\n'); yy_pop_state(); return MASK_END; }
<S_MASK><<EOF>> { yy_pop_state(); return MASK_END; }

<INITIAL>^{P_BLOCK_BEG} { yy_push_state(S_BLOCK); yylval.str = new ast::Str(tsc_yylloc, yytext, yyleng); return BLOCK_BEG; }
<S_BLOCK>^({P_MODULE_SEP}|{P_BLOCK_BEG}) { UnputStr(yytext, yyleng); yy_set_bol(1); yy_pop_state(); return BLOCK_END; }
<S_BLOCK>^\n     { yy_pop_state(); return BLOCK_END; }
<S_BLOCK>;;\n    { yy_pop_state(); return BLOCK_END; }
<S_BLOCK><<EOF>> { yy_pop_state(); return BLOCK_EOF; }

"$(" return CALL;
^json[[:blank:]]+ { yylval.str = new ast::Str(tsc_yylloc, yytext, yyleng); return JSON; }
"\\{" yylval.symbol = '{'; return CHAR;
"\\}" yylval.symbol = '}'; return CHAR;
"\\\"" yylval.symbol = '"'; return CHAR;
[{}()\n] { yylval.symbol = *yytext; return *yytext; } /* special characters */
[[:blank:]]*= { yylval.str = new ast::Str(tsc_yylloc, yytext, yyleng); return ASSIGN; }
<INITIAL,S_BLOCK,S_MASK>[\"]/[^#{}\"\n]*\" { yy_push_state(S_DBLQ); yylval.symbol = '"'; return '{'; }
<S_DBLQ>[\"] { yy_pop_state(); yylval.symbol = '"'; return '}'; }
[[:blank:]] { yylval.symbol = *yytext; return BLANK; }
. { yylval.symbol = *yytext; return CHAR; }

%%

static void UnputChar(char c)
{
    unput(c);
    ast::Suite::LexerState& lexerState = ast::Suite::getCurrentInstance()->lexer;
    if (c == '\n' && !lexerState.posBeforeLf.empty()) {
        lexerState.pos.setPos(lexerState.posBeforeLf.top());
        lexerState.posBeforeLf.pop();

        tsc_yylloc.last_file = lexerState.pos.file();
        tsc_yylloc.last_line = lexerState.pos.line;
        tsc_yylloc.last_column = lexerState.pos.col;
    }
    yy_set_bol(c == '\n');
}

static void UnputStr(const char* lexeme, size_t len)
{
    for (size_t i = 0; i < len; ++i)
        UnputChar(lexeme[len - i - 1]);
}

static void TrackCurrentPosition(const char* lexeme, size_t len)
{
    ast::Suite::LexerState& lexerState = ast::Suite::getCurrentInstance()->lexer;
    tsc_yylloc.first_file = lexerState.pos.file();
    tsc_yylloc.first_line = lexerState.pos.line;
    tsc_yylloc.first_column = lexerState.pos.col;

    for (size_t i = 0; i < len; ++i)
        if (lexeme[i] == '\n') {
            lexerState.posBeforeLf.push(lexerState.pos.getPos()); /* save for unput */
            ++lexerState.pos.line;
            lexerState.pos.col = 1;
        } else
            ++lexerState.pos.col;

    tsc_yylloc.last_file = lexerState.pos.file();
    tsc_yylloc.last_line = lexerState.pos.line;
    tsc_yylloc.last_column = lexerState.pos.col;
}

/* 
 * lexeme must be
 * #line num\n
 * OR
 * #line num "file"\n
 */
static void Syncline(const char* lexeme, size_t len)
{
    if (ast::Suite::getCurrentInstance()->lexer.nosync)
        return;

    std::istringstream sync(std::string(lexeme, len));
    std::string tag, file;
    unsigned line;
    sync >> tag >> line;

    /* scan file name */
    char c;
    while (sync.get(c) && c != '"');
    while (sync.get(c) && c != '"')
        file += c;

    ast::Suite::LexerState& lexerState = ast::Suite::getCurrentInstance()->lexer;
    lexerState.pos.line = line;
    if (!file.empty())
        lexerState.pos.setFile(file);
}

