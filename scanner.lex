%{
    #include "Token.h"
    #include <memory>
    #define YYSTYPE Node*
    #include "parser.tab.hpp"
    #include "hw3_output.hpp"

    using namespace output;

%}

%option yylineno
%option noyywrap

ileagal         [^\x20-\x7E \r\t\n]
whitespace		[\t\n\r ]
printable       [\x20-\x7E]
octal           [0-7]
hexadecimal     [0-9A-F]
three_octal     {octal}{octal}{octal}
two_hex         {hexadecimal}{hexadecimal}
four_hex        {two_hex}{two_hex}
eight_hex       {four_hex}{four_hex}
escape          ([\"nrt0]|(x({hexadecimal}{0,2})))
backslash       \\
bs_esc          \\([\"nrt0\\]|(x({hexadecimal}{0,2})))
digits          ([1-9]([0-9]*))|"0"
caps_letter     [A-Z]
letters         [a-zA-Z]
quotation_marks \"


%%

void                                                yylval = new Void(); return VOID;
int                                                 yylval = new Type(TN_INT); return INT;
byte                                                yylval = new Type(TN_BYTE); return BYTE;
"b"                                                 yylval = new B(); return B_TOKEN;
bool                                                yylval = new Type(TN_BOOL); return BOOL;
and                                                 yylval = new And(); return AND;
or                                                  yylval = new Or(); return OR;
not                                                 yylval = new Not(); return NOT;
true                                                yylval = new Boolean(true); return TRUE;
false                                               yylval = new Boolean(false); return FALSE;
return                                              yylval = new Return(); return RETURN;
if                                                  yylval = new If(); return IF;
else                                                yylval = new Else(); return ELSE;
while                                               yylval = new While(); return WHILE;
break                                               yylval = new Break(); return BREAK;
continue                                            yylval = new Continue(); return CONTINUE;
,                                                   yylval = new Comma(); return COMMA;
;                                                   yylval = new Sc(); return SC;
\(                                                  yylval = new Lparen(); return LPAREN;
\)                                                  yylval = new Rparen(); return RPAREN;
\{                                                  yylval = new Lbrace(); return LBRACE;
\}                                                  yylval = new Rbrace(); return RBRACE;
"=="|"!="                                           yylval = new Relop(yytext); return RELOP;
"<"|">"|"<="|">="                                   yylval = new Relop(yytext); return RELOP2;
=                                                   yylval = new Assign(); return ASSIGN;
"+"|"-"|"*"|"/"                                     yylval = new Binop(*yytext); return BINOP;
[a-zA-Z]([a-zA-Z0-9]*)                              yylval = new Id(yytext); return ID;
{digits}                                            yylval = new Num(std::stoi(yytext)); return NUM;
\"([^\n\r\"\\]|\\[rnt\"\\])+\"                      yylval = new String(yytext); return STRING;
\/\/[^\r\n]*[\r|\n|\r\n]?                           ;
[\n\r\t\s ]                                         ;
.                                                   errorLex(yylineno); exit(0);

%%