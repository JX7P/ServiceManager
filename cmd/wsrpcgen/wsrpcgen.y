/*
 * Copyright 2020 David MacKay.  All rights reserved.
 * Use is subject to license terms.
 */

%include {
	#include <cstdlib>
	#include <iostream>
	#include <fstream>
	#include <list>

	#include "eci/CxxUtil.hh"
	#include "wsrpcgen.hh"
	#include "wsrpcgen.tab.h"
	#include "wsrpcgen.l.h"

	#define LEMON_SUPER MVST_Parser
}

%token_type    {Token}
%token_prefix  TOK_

%code {

TrlUnit * MVST_Parser::parseFile (std::string fName,
	std::list<std::string> includeDirs)
{
    MVST_Parser *parser;
    std::string src;
    std::ifstream f;
    yyscan_t scanner;
    YY_BUFFER_STATE yyb;

    f.exceptions(std::ios::failbit | std::ios::badbit);

        f.open(fName);

        f.seekg(0, std::ios::end);
        src.reserve(f.tellg());
        f.seekg(0, std::ios::beg);

        src.assign((std::istreambuf_iterator<char>(f)),
                   std::istreambuf_iterator<char>());

    parser = MVST_Parser::create(fName, src);
	parser->includeDirs = includeDirs;
	
    if (false)
        parser->trace(stdout, "<parser>: ");

	parser->trlunit = new TrlUnit;
	parser->trlunit->name = fName;

    mvstlex_init_extra(parser, &scanner);
    /* Now we need to scan our string into the buffer. */
    yyb = mvst_scan_string(src.c_str(), scanner);

    while (mvstlex(scanner))
        ;
    parser->parse(0);

	return parser->trlunit;
}

MVST_Parser * MVST_Parser::create(std::string fName, std::string & fText)
{
	return new yypParser(fName, fText);
}

Position MVST_Parser::pos()
{
	yypParser *self = (yypParser *)this;
	return Position(self->m_oldLine, self->m_oldCol, self->m_oldPos, self->m_line, self->m_col, self->m_pos);
}

}

%syntax_error {
	const YYACTIONTYPE stateno = yytos->stateno;
	size_t eolPos = fText.find("\n", m_pos);
	std::string eLine = fText.substr(m_pos, eolPos - m_pos);
	size_t i;

	std::cerr << "WSRPCGen: " << fName << "(" << toStr(m_line) + "," 
			  << toStr(m_col) << "): "
			  << "Syntax error: unexpected " 
			  << yyTokenName[yymajor] << "\n";

	std::cerr << "+ " << eLine << "\n";
	std::cerr << "+ ";
	for (i = 0; i < m_oldCol; i++)
		std::cerr << " ";
	for (; i < m_col; i++)
		std::cerr << "^";

	std::cerr << "\n\texpected one of: \n";

	for (unsigned i = 0; i < YYNTOKEN; ++i)
	{
		int yyact = yy_find_shift_action(i, stateno);
		if (yyact != YY_ERROR_ACTION && yyact != YY_NO_ACTION)
			std::cerr << "\t" << yyTokenName[i] << "\n";
	}
}

file ::= trlunit.

%type trlunit { TrlUnit * }

trlunit ::= optimports(i) optdefs(t) optprograms(d).
	{
		trlunit->imports = i;
		trlunit->types  = t;
		trlunit->programs = d;
	}

%type optimports { std::list<TrlUnit *> }
%type imports { std::list<TrlUnit *> }
%type import { TrlUnit * }
%type optprograms { std::list<Program *> }
%type programs { std::list<Program *> }
%type program { Program * }
%type versions { std::list<Version *> }
%type version { Version * }
%type methods { std::list<Method *> }
%type method { Method * }
%type decls_semis { std::list<Decl *>}

optimports ::= .
optimports(L) ::= imports(l). { L = l; }

imports(L) ::= import(d). { L.push_back(d); }
imports(L) ::= imports(l) import(d).
	{
		L = std::move(l); L.push_back(d);
	}

import(I) ::= IMPORT(s).
	{
		bool succeeded = false;
		try {
			I = MVST_Parser::parseFile(s.stringValue, includeDirs);
			succeeded = true;
		} catch (std::ios_base::failure &e)
		{
			for (auto dir: includeDirs)
			{
				try {
					I = MVST_Parser::parseFile(dir + s.stringValue, includeDirs);
					succeeded = true;
				} catch (std::ios_base::failure &e) {
				}
			}
		}
		if (!succeeded)
		{
			printf("Error: Importing %s failed.\n", s.stringValue.c_str());
			abort();
		}
	}

optprograms ::= .
optprograms(L) ::= programs(l). { L = l; }

programs(L) ::= program(d) SEMI. { L.push_back(d); }
programs(L) ::= programs(l) program(d) SEMI.
	{
		L = std::move(l); L.push_back(d);
	}

program(D) ::= PROGRAM IDENT(name) LBRACE optdefs(defs) versions(v) RBRACE 
	EQUALS INTEGER(num).
	{
		D = new Program;
		D->name = name.stringValue;
		D->types = defs;
		D->versions = v;
		D->num = num;
	}

%type optdefs { std::list<TypeDef *> }
%type defs { std::list<TypeDef *> }
%type def { TypeDef * }

optdefs ::= .
optdefs(A) ::= defs(a). { A = a; }

defs(L) ::= def(d) SEMI. { L.push_back(d); }
defs(L) ::= defs(l) def(d) SEMI. { L = std::move(l); L.push_back(d);  }

def(D) ::= STRUCT IDENT(id) LBRACE optdefs(defs) decls_semis(d) RBRACE.
        {
			StructDef * S = new StructDef;
			S->name = id.stringValue;
			S->types = defs;
			S->decls = d;
			D = S;
        }
def(D) ::= ENUM IDENT(id) LBRACE enum_entries(e) optcomma RBRACE.
	{
		EnumDef * E = new EnumDef;
		E->name = id.stringValue;
		E->entries = e;
		D = E;
	}

optcomma ::= COMMA.
optcomma ::= .

decls_semis(L) ::= decl(d) SEMI . { L.push_back(d); }
decls_semis(L) ::= decls_semis(l) decl(d) SEMI. { L = std::move(l); L.push_back(d); }

%type enum_entries { std::list<EnumDef::Entry> }
%type enum_entry { EnumDef::Entry }

enum_entries(L) ::= enum_entry(d) . { L.push_back(d); }
enum_entries(L) ::= enum_entries(l) COMMA enum_entry(d). { L = std::move(l); L.push_back(d); }

enum_entry(E) ::= IDENT(id) . { E.id = id.stringValue; }
enum_entry(E) ::= IDENT(id) LBRACKET STRLIT(s) RBRACKET. {
	E.id = id.stringValue;
	E.str = s.stringValue;
}
enum_entry(E) ::= IDENT(id) EQUALS INTEGER(i). {
	E.id = id.stringValue;
	E.iVal = toStr(i.intValue); 
}
enum_entry(E) ::= IDENT(id) LBRACKET STRLIT(s) RBRACKET EQUALS INTEGER(i). {
	E.id = id.stringValue;
	E.str = s.stringValue;
	E.iVal = toStr(i.intValue); 
}

versions(L) ::= version(d) SEMI. { L.push_back(d); }
versions(L) ::= versions(l) version(d) SEMI. { L = std::move(l); L.push_back(d);  }

version(V) ::= VERSION IDENT(name) LBRACE methods(m) RBRACE EQUALS INTEGER(num).
	{
		V = new Version;
		V->num = num.intValue;
		V->name = name.stringValue;
		V->methods = m;
	}

methods(L) ::= method(d) SEMI. { L.push_back(d); }
methods(L) ::= methods(l) method(d) SEMI. { L = std::move(l); L.push_back(d);  }

method(M) ::= type(ret) IDENT(name) LBRACKET optargs(args) RBRACKET optmethnum.
	{
		M = new Method;
		M->retType = ret;
		M->name = name.stringValue;
		M->args = args;
	}

optmethnum ::= .
optmethnum ::= EQUALS INTEGER.

%type decl { Decl * }
%type args { std::list<Decl*> }
%type optargs { std::list<Decl*> }

optargs ::= .
optargs(A) ::= args(a). { A = a; }

decl(D) ::= type(type) IDENT(id) .
	{
		D = new Decl;
		D->id = id.stringValue;
		D->type = type;
	}

args(L) ::= decl(d). { L.push_back(d); }
args(L) ::= args(l) COMMA decl(d). { L = l; L.push_back(d); }

%type type { TypeRef * }
%type basic_type { TypeRef * }

type(T) ::= BYREF type(t).
	{
		(T= t)->byRef = true;
	}
type(T) ::= SVC_BYRREF type(t).
	{
		(T= t)->svcByRRef = true;
	}
type(T) ::= basic_type(t). { T = t; }

basic_type(T) ::= IDENT(id) opt_list(l).
	{
		T = new TypeRef;
		T->type = id.stringValue;
		T->list = l;
	}

%type opt_list { bool }

opt_list(l) ::= . { l = false; }
opt_list(l) ::= LCARET RCARET. { l = true; }