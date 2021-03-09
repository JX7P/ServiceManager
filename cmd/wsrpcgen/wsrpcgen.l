%{
#include "wsrpcgen.hh"
#include "wsrpcgen.tab.h"
%}

%option noyywrap
%option stack
%option yylineno
%option reentrant
%option extra-type = "MVST_Parser *"
%option prefix = "mvst"
%option caseless

%{

#define YY_USER_ACTION \
    yyextra->recOldPos(); \
    for(int i = 0; yytext[i] != '\0'; i++) \
    { \
        if(yytext[i] == '\n') \
        { \
            yyextra->cr(); \
        } \
        else \
            yyextra->incCol(); \
    }

#define p(x) yyextra->parse(TOK_##x)
#define ps(x) yyextra->parse(TOK_##x, yytext)
#define pi(x) yyextra->parse(TOK_##x, atol(yytext))
#define pf(x) yyextra->parse(TOK_##x, atof(yytext))
#define ph(x) yyextra->parse(TOK_##x, strtol(yytext, NULL, 16))

%}

DIGIT ([0-9])
LTR ([a-zA-Z_\xA8\xAA\xAD\xAF\xB2\xB5\xB7\xB8\xB9\xBA\xBC\xBD\xBE]|[\xC0-\xFF][\x80-\xBF]*|\\u([0-9a-fA-F]{4}))
IDENT (:?{LTR})(:?{LTR}|{DIGIT}|::)*
NEWLINE ((\n)|(\r)|(\r\n))

%x C_COMMENT

%%

^"%".*                          {
        yyextra->trlunit->code.append(
          std::string(yytext).substr(1, std::string::npos) + "\n"); 
    }
"//".*

^"#import ".*                   {
        std::string name = std::string(yytext).substr(9);
        name.pop_back();
        yyextra->parse(TOK_IMPORT, name); 
    }

"program"                       { p(PROGRAM); }
"version"                       { p(VERSION); }
"byref"                         { p(BYREF); }
"svc_byrref"                    { p(SVC_BYRREF); }
"struct"                        { p(STRUCT); }
"enum"                          { p(ENUM); }
"union"                         { p(UNION); }
"case"                          { p(CASE); }
"type"                          { p(TYPE); }

"-"?"0"[xX][0-9A-F]+                                { ph(INTEGER); }
-[0-9]+                                             { pi(INTEGER); }
[0-9]+                                              { pi(INTEGER); }
(:?{LTR})(:?{LTR}|{DIGIT}|\.|::)*                   { ps(IDENT); }
L?\"(\\.|[^\\"])*\"	                                { ps(STRLIT); }


":"                             { p(COLON); }
";"                             { p(SEMI); }
"{"                             { p(LBRACE); }
"}"                             { p(RBRACE); }
"};"                            { p(RBRACE); p(SEMI); }
"("                             { p(LBRACKET); }
")"                             { p(RBRACKET); } 
","                             { p(COMMA); }
"="                             { p(EQUALS); }
"<"                             { p(LCARET); }
">"                             { p(RCARET); }

"/*"            { BEGIN(C_COMMENT); }
<C_COMMENT>"*/" { BEGIN(INITIAL); }
<C_COMMENT>\n   { }
<C_COMMENT>.    { }

[ \t\n]
