#pragma once

#include <list>
#include <map>
#include <memory>
#include <stack>
#include <string>
#include <vector>

#include "lemon_base.h"

struct Program;
struct Arg;
struct TypeDef;
struct TrlUnit;

/* Details of the position of some source code. */
class Position
{
    size_t m_oldLine, m_oldCol, m_oldPos;
    size_t m_line, m_col, m_pos;

  public:
    Position(size_t oldLine, size_t oldCol, size_t oldPos, size_t line,
             size_t col, size_t pos)
        : m_oldLine(oldLine), m_oldCol(oldCol), m_oldPos(oldPos), m_line(line),
          m_col(col), m_pos(pos)
    {
    }

    /* Get line number */
    size_t line() const;
    /* Get column number*/
    size_t col() const;
    /* Get absolute position in source-file */
    size_t pos() const;
};

inline size_t Position::line() const
{
    return m_line;
}

inline size_t Position::col() const
{
    return m_col;
}

inline size_t Position::pos() const
{
    return m_pos;
}

struct Token
{
    Token() = default;
    Token(const Token &) = default;
    Token(Token &&) = default;

    Token(double f) : floatValue(f)
    {
    }
    Token(long i) : intValue(i)
    {
    }
    Token(const std::string &s) : stringValue(s)
    {
    }
    Token(std::string &&s) : stringValue(std::move(s))
    {
    }

    Token &operator=(const Token &) = default;
    Token &operator=(Token &&) = default;

    operator std::string() const
    {
        return stringValue;
    }
    operator const char *() const
    {
        return stringValue.c_str();
    }
    operator double() const
    {
        return floatValue;
    }

    double floatValue = 0.0;
    long intValue = 0;
    std::string stringValue;
};

struct ProgramNode;
struct MethodNode;

class MVST_Parser : public lemon_base<Token>
{
  protected:
    std::string fName;
    std::string &fText;
    std::list<std::string> includeDirs;
    int m_line = 0, m_col = 0, m_pos = 0;
    int m_oldLine = 0, m_oldCol = 0, m_oldPos = 0;

  public:
    using lemon_base::parse;
    TrlUnit *trlunit;

    int line() const
    {
        return m_line;
    }
    int col() const
    {
        return m_col;
    }
    int pos1() const
    {
        return m_pos;
    }

    static TrlUnit *parseFile(std::string fName,
                              std::list<std::string> includeDirs);
    static MVST_Parser *create(std::string fName, std::string &fText);

    MVST_Parser(std::string f, std::string &ft)
        : fName(f), fText(ft) //, program(nullptr)
    {
    }

    /* parsing */
    void parse(int major)
    {
        parse(major, Token{});
    }

    template <class T> void parse(int major, T &&t)
    {
        parse(major, Token(std::forward<T>(t)));
    }

    virtual void trace(FILE *, const char *)
    {
    }

    /* line tracking */
    Position pos();

    void recOldPos()
    {
        m_oldPos = m_pos;
        m_oldLine = m_line;
        m_oldCol = m_col;
    }

    void cr()
    {
        m_pos += m_col + 1;
        m_line++;
        m_col = 0;
    }
    void incCol()
    {
        m_col++;
    }
};

struct OutStream
{
    std::string txt;

    int depth = 0;

    void add(std::string str)
    {
        txt.append(std::string(depth, ' '));
        txt.append(str);
    }

    void cadd(std::string str)
    {
        txt.append(str);
    }

    void writeTo(std::string fileName);
};

struct Scope;

struct Scoped
{
    Scope *parentScope;
    virtual void synthInScope(Scope *aScope);
    /* lookup an identifier - strictly not one with ::, do that by recursing. */
    virtual TypeDef *lookupIdentifier(std::string id);
};

struct Scope : public Scoped
{
    std::string name;
    std::list<TypeDef *> types;

    std::string fullyQualifiedName();
    virtual std::string fullyQualifiedPrefix();

    virtual void synthInScope(Scope *aScope);
    /* lookup an identifier - strictly not one with ::, do that by recursing. */
    virtual TypeDef *lookupIdentifier(std::string id);
};

struct TrlUnit : public Scope
{
    std::string code;
    std::list<TrlUnit *> imports;
    std::list<Program *> programs;

    std::string hdrName();
    std::string fullyQualifiedPrefix();

    void synthInScope(Scope *aScope);
    TypeDef *lookupIdentifier(std::string id);

    void genHeader(OutStream &os);
    void genSerialise(OutStream &os);
    void genServer(OutStream &os);
    void genClient(OutStream &os);
};

struct TypeRef
{
    /* is it a list? */
    bool list = false;
    /* should client call wrapper take a by-reference argument? */
    bool byRef = false;
    /* should svc function use an RValue reference? */
    bool svcByRRef = false;

    /* if this is a type defined in the IDL, its type definition */
    TypeDef *def = NULL;

    std::string type;

    bool isVoid();
    bool isBuiltinOrExt();

    /**
     * lookup in parent scope to see if this type has been defined. If not, we
     * assume it's user-defined and expect existing serialisation functions
     * provided for it.
     */
    void synthInScope(Scope *aScope);

    std::string makeDecl(std::string id, std::string declaratorPrefix = "");
    std::string makeArg(std::string id); /* as above but might be ref */
    std::string makeSvcArg(std::string id);
    std::string makeDeserialiseCall(std::string uclObj, std::string outObj);
    std::string makeSerialiseCall(std::string inObj);

    /* generates code to serialise variable @in to ucl object named @out */
    void genSerialiseInto(OutStream &os, std::string in, std::string out);

    /* generates code to deserialise ucl object @uin to variable named @out,
     * storing success value into @suc */
    void genDeserialiseInto(OutStream &os, std::string suc, std::string uIn,
                            std::string out, bool alreadyPointer = false);

    std::string canonicalName();
};

struct Decl /* typedecl */
{
    TypeRef *type;
    std::string id;
};

struct TypeDef : public Scope
{
    /* generates the type defintion, maybe nestedly */
    virtual void genDef(OutStream &os) = 0;

    /* generalise the de/serialise source file contents */
    virtual void genSerialise(OutStream &os) = 0;
};

struct SerialisableDef : public TypeDef
{
    virtual void genSerDecl(OutStream &os, bool qualified = false,
                            std::string declspec = "");
    virtual void genSerImpl(OutStream &os) = 0;
    virtual void genDeserDecl(OutStream &os, bool qualified = false,
                              std::string declspec = "");
    virtual void genDeserImpl(OutStream &os) = 0;

    void genSerialise(OutStream &os);
};

struct StructDef : public SerialisableDef
{
    std::list<Decl *> decls;

    void synthInScope(Scope *aScope);

    void genDef(OutStream &os);
    void genSerImpl(OutStream &os);
    void genDeserImpl(OutStream &os);
};

struct UnionDef : public SerialisableDef
{
    /* enum by which the type is identified */
    TypeRef *enumType;
    /* each pair string must be "default" or a possible value of enumType */
    std::list<std::pair<std::string, std::list<Decl *>>> cases;

    void synthInScope(Scope *aScope);

    void genDef(OutStream &os);
    void genSerImpl(OutStream &os);
    void genDeserImpl(OutStream &os);
};

struct EnumDef : public SerialisableDef
{
    struct Entry
    {
        std::string id;
        std::string str;
        std::string iVal;
    };

    std::list<Entry> entries;

    void genDef(OutStream &os);
    void genSerImpl(OutStream &os);
    void genDeserImpl(OutStream &os);
};

struct Method
{
  private:
    std::string implFunName(int ver);

  public:
    TypeRef *retType;
    std::string name;
    std::list<Decl *> args;

    void synthInScope(Scope *aScope);

    /* generates the declaration part of the client call - without a
     * terminating semicolon, so can also be used to head a definition */
    void genClientCallDecl(OutStream &os, int ver, std::string prefix = "");
    void genClientCallAsynchDecl(OutStream &os, int ver, std::string className,
                                 std::string prefix = "");
    void genClientCallCommonPartImpl(OutStream &os, int ver,
                                     std::string prefix);
    void genClientCallImpl(OutStream &os, int ver, std::string prefix);
    void genClientCallAsynchImpl(OutStream &os, int ver, std::string prefix);

    void genClientDelegateDecl(OutStream &os, int ver, std::string prefix = "");
    void genClientDelegateDispatcherDecl(OutStream &os, int ver,
                                         std::string prefix = "");

    void genClientDelegateImpl(OutStream &os, int ver, std::string prefix = "");
    void genClientDelegateDispatcherImpl(OutStream &os, int ver,
                                         std::string prefix = "");

    /* generates implementation declaration */
    void genServerImplDecl(OutStream &os, int ver);
    /* generates case for switch of RPC method name */
    void genServerImplCase(OutStream &os, int ver);
};

struct Version
{
    unsigned long num;
    std::string name;
    std::list<Method *> methods;

    void synthInScope(Scope *aScope);

    /* generate the client-call declarations */
    void genClientCallDecls(OutStream &os, std::string className);
    void genClientCallImpls(OutStream &os, std::string prefix);

    void genClientDelegateDecls(OutStream &os);
    void genClientDelegateImpls(OutStream &os, std::string prefix);

    void genServerImplDecls(OutStream &os);
    void genServerImplHandlerDecl(OutStream &os, std::string prefix = "");
    void genServerImplCases(OutStream &os, std::string prefix);

    void genSerialise(OutStream &os, std::string prefix);
};

struct Program : public TypeDef
{
    unsigned long num;
    std::list<Version *> versions;

    std::string className();

    void synthInScope(Scope *aScope);

    void genDef(OutStream &os);
    void genSerialise(OutStream &os);
    void genServer(OutStream &os);
    void genClient(OutStream &os);
};