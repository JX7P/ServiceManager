#include <algorithm>

#include "eci/CxxUtil.hh"
#include "wsrpcgen.hh"

static std::string quote(std::string str)
{
    return "\"" + str + "\"";
}

bool TypeRef::isVoid()
{
    return type == "void";
}

std::string TypeRef::makeArg(std::string id)
{
    return makeDecl(id, byRef ? "&" : "");
}

std::string TypeRef::makeSvcArg(std::string id)
{
    return makeDecl(id, svcByRRef ? "&&" : "");
}

std::string TypeRef::makeDecl(std::string id, std::string declaratorPrefix)
{
    std::string res;
    if (list)
        res.append("std::list<");

    if (type == "string")
        res += "std::string";
    else
        res += type;

    if (list)
        res += ">";

    return res + " " + declaratorPrefix + id;
}

std::string TypeRef::canonicalName()
{
    std::string canon = type;
    std::replace(canon.begin(), canon.end(), ':', '_');
    return canon;
}

std::string TypeRef::makeDeserialiseCall(std::string uclObj, std::string outObj)
{
    return "wsRPCDeserialise" + canonicalName() + (list ? "List" : "") + "(" +
           uclObj + ", " + outObj + ")";
}

std::string TypeRef::makeSerialiseCall(std::string inObj)
{
    return "wsRPCSerialise" + canonicalName() + (list ? "List" : "") + "(" +
           inObj + ")";
}

void TypeRef::genSerialiseInto(OutStream &os, std::string in, std::string out)
{
    std::string elS;

    if (!def)
        elS = "wsRPCSerialise" + canonicalName() + "(&" + in + ")";
    else
        elS = in + ".serialise()";

    if (list)
    {
        os.add(out + " = ucl_object_typed_new(UCL_ARRAY);\n");
        os.add("for (auto &" + in + " : " + in + ")\n");
        os.add("  ucl_array_append(" + out + ", " + elS + ");\n");
    }
    else
        return os.add(out + " = " + elS + ";\n");
}

void TypeRef::genDeserialiseInto(OutStream &os, std::string suc,
                                 std::string uIn, std::string out,
                                 bool alreadyPointer)
{
    std::string elS;

    if (!def)
        elS = "wsRPCDeserialise" + canonicalName() + "(" + uIn + ", " +
              (alreadyPointer ? "" : "&") + out + ")";
    else
        elS = def->fullyQualifiedPrefix() + "deserialise(" + uIn + ", " +
              (alreadyPointer ? "" : "&") + out + ")";

    if (list)
    {
        os.add("ucl_object_iter_t it = NULL;\n");
        os.add("const ucl_object_t *uEl;\n");

        // os.add("for (auto " + in + " : " + in + ") {\n");
        os.add("while ((uEl = ucl_iterate_object(" + uIn +
               ", &it, true))) {\n");
        os.depth += 2;
        if (def)
        {
            os.add(type + " outEl;\n");
            os.add("if (!" + type + "::deserialise(uEl, &outEl)) {\n");
            os.add("  " + suc + " = false;\n");
            os.add("  break;\n");
            os.add("}\n");
        }
        else
        {
            /* TODO: make a 'makeElDecl()' or something that ignores lists (and
             * maybe in the future optionals ) */
            os.add(canonicalName() + " outEl" + ";\n");
            os.add("if (!wsRPCDeserialise" + canonicalName() +
                   "(uEl, &outEl)) {\n");
            os.add("  suc = false;\n");
            os.add("  break;\n");
            os.add("}\n");
        }
        os.add("(" + out + ")" + (alreadyPointer ? "->" : ".") +
               "emplace_back(std::move(outEl));\n");
        os.depth -= 2;
        os.add("}\n");
    }
    else
        return os.add(suc + " = " + elS + ";\n");
}

/**
 * @section Type de/serialisation
 */

void SerialisableDef::genSerDecl(OutStream &os, bool qualified,
                                 std::string declspec)
{
    os.add(declspec + "ucl_object_t * " +
           (qualified ? fullyQualifiedPrefix() : "") + "serialise()");
}

void SerialisableDef::genDeserDecl(OutStream &os, bool qualified,
                                   std::string declspec)
{
    os.add(declspec + "bool " + (qualified ? fullyQualifiedPrefix() : "") +
           "deserialise (const ucl_object_t *obj," + name + " *out)");
}

void SerialisableDef::genSerialise(OutStream &os)
{
    genSerImpl(os);
    genDeserImpl(os);
}

void StructDef::genDef(OutStream &os)
{
    os.add("struct " + name + " {\n");

    os.depth += 2;

    /* struct member types */
    for (auto type : types)
        type->genDef(os);

    /* struct members */
    for (auto decl : decls)
        os.add(decl->type->makeDecl(decl->id) + ";\n");
    os.cadd("\n");

    /* serialisation function */
    genSerDecl(os);
    os.cadd(";\n");

    /* deserialisation function */
    genDeserDecl(os, false, "static ");
    os.cadd(";\n");

    os.depth -= 2;
    os.add("};\n");
}

void StructDef::genSerImpl(OutStream &os)
{
    for (auto type : types)
        /* always true, structs grammatically can't contain otherwise */
        dynamic_cast<SerialisableDef *>(type)->genSerImpl(os);

    genSerDecl(os, true);
    os.cadd("\n");
    os.add("{\n");
    os.depth += 2;

    os.add("ucl_object_t * out = ucl_object_typed_new(UCL_OBJECT);\n");
    for (auto decl : decls)
        os.add("ucl_object_t * u" + decl->id + ";\n");
    for (auto decl : decls)
        decl->type->genSerialiseInto(os, decl->id, "u" + decl->id);
    for (auto decl : decls)
        os.add("ucl_object_insert_key(out, u" + decl->id + ", " +
               quote(decl->id) + ", 0, false);\n");
    os.add("return out;\n");

    os.depth -= 2;
    os.add("}\n");
}

void StructDef::genDeserImpl(OutStream &os)
{
    for (auto type : types)
    {
        os.add("/* for sub-member " + type->name + " */\n");
        dynamic_cast<SerialisableDef *>(type)->genDeserImpl(os);
    }

    genDeserDecl(os, true);
    os.cadd("\n");
    os.add("{\n");
    os.depth += 2;

    os.add("bool suc = true;\n");

    /* ucl object member declarations */
    for (auto decl : decls)
        os.add("const ucl_object_t * u" + decl->id + ";\n");

    os.add("if (!(ucl_object_type(obj) == UCL_OBJECT))\n");
    os.add("  return false;\n");

    /* assign ucl object members by looking them up; if unfound, return */
    for (auto decl : decls)
        os.add("if (!(u" + decl->id + " = ucl_object_lookup(obj, " +
               quote(decl->id) + "))) return false;\n");

    os.add("/* member deserialisation */\n");
    for (auto decl : decls)
    {
        os.add("/* deserialise member " + decl->id + " */\n");
        decl->type->genDeserialiseInto(os, "suc", "u" + decl->id,
                                       "out->" + decl->id);
        os.add("if (!suc) return false;\n\n");
    }

    os.cadd("\n");

    os.add("return true;\n");

    os.depth -= 2;
    os.add("}\n");
}

void UnionDef::genDef(OutStream &os)
{
    os.add("struct " + name + " {\n");

    os.depth += 2;

    os.add(enumType->makeDecl("type") + ";\n");

    /* union cases */
    os.add("union {\n");
    for (auto cas : cases)
    {
        os.add("struct " + cas.first + "{\n");
        os.depth += 2;
        for (auto decl : cas.second)
            os.add(decl->type->makeDecl(decl->id) + ";\n");
        os.depth -= 2;
        os.add("};\n\n");
    }
    os.add("} value;\n\n");

    /* serialisation function */
    genSerDecl(os);
    os.cadd(";\n");

    /* deserialisation function */
    genDeserDecl(os, false, "static ");
    os.cadd(";\n");

    os.depth -= 2;
    os.add("};\n");
}

void UnionDef::genSerImpl(OutStream &os)
{
    for (auto type : types)
        /* always true, structs grammatically can't contain otherwise */
        dynamic_cast<SerialisableDef *>(type)->genSerImpl(os);

    genSerDecl(os, true);
    os.cadd("\n");
    os.add("{\n");
    os.depth += 2;

    /*    os.add("ucl_object_t * out = ucl_object_typed_new(UCL_OBJECT);\n");
        for (auto decl : decls)
            os.add("ucl_object_t * u" + decl->id + ";\n");
        for (auto decl : decls)
            decl->type->genSerialiseInto(os, decl->id, "u" + decl->id);
        for (auto decl : decls)
            os.add("ucl_object_insert_key(out, u" + decl->id + ", " +
                   quote(decl->id) + ", 0, false);\n");
        os.add("return out;\n");*/

    os.depth -= 2;
    os.add("}\n");
}

void UnionDef::genDeserImpl(OutStream &os)
{
    for (auto type : types)
    {
        os.add("/* for sub-member " + type->name + " */\n");
        dynamic_cast<SerialisableDef *>(type)->genDeserImpl(os);
    }

    genDeserDecl(os, true);
    os.cadd("\n");
    os.add("{\n");
    os.depth += 2;

    /*    os.add("bool suc = true;\n");

        /* ucl object member declarations
        for (auto decl : decls)
            os.add("const ucl_object_t * u" + decl->id + ";\n");

        os.add("if (!(ucl_object_type(obj) == UCL_OBJECT))\n");
        os.add("  return false;\n");

        /* assign ucl object members by looking them up; if unfound, return
        for (auto decl : decls)
            os.add("if (!(u" + decl->id + " = ucl_object_lookup(obj, " +
                   quote(decl->id) + "))) return false;\n");

        os.add("/* member deserialisation \n");
        for (auto decl : decls)
        {
            os.add("/* deserialise member " + decl->id + " \n");
        decl->type->genDeserialiseInto(os, "suc", "u" + decl->id,
                                       "out->" + decl->id);
    os.add("if (!suc) return false;\n\n");
}

os.cadd("\n");
*/ os.add("return true;\n");

    os.depth -= 2;
    os.add("}\n");
}

void EnumDef::genDef(OutStream &os)
{
    os.add("struct " + name + "{\n");
    os.depth += 2;

    /* the actual enum - it's a member */
    os.add("enum _E" + name + " {\n");
    os.depth += 2;
    for (auto entry : entries)
        os.add(entry.id + (entry.iVal.size() ? " = " + entry.iVal : "") +
               ",\n");
    os.add("kMax,\n");
    os.depth -= 2;
    os.add("};\n\n");

    /* string values */
    for (auto entry : entries)
        os.add("static const char * " + entry.id + "PSz;\n");

    os.cadd("\n");

    /* actual value */
    os.add("_E" + name + " value;\n\n");

    os.add(name + "() : value(kMax) {};\n");
    os.add(name + "(const " + "_E" + name + "&value) : value(value) {};\n");
    os.add("operator _E" + name + "() { return value; }\n");

    /* to-string function */
    os.add("const char * toPSz() const;\n");

    /* comparisons */
    // os.add("bool operator==(const " + name + "&) const;\n");

    /* serialisation function */
    genSerDecl(os);
    os.cadd(";\n\n");

    os.add("static " + name + " fromPSz(const char * pSz);\n");

    /* deserialisation function */
    genDeserDecl(os, false, "static ");
    os.cadd(";\n");

    os.depth -= 2;
    os.add("};\n");
}

void EnumDef::genSerImpl(OutStream &os)
{
    bool firstCase = true;

    /* strings */
    for (auto entry : entries)
        os.add("const char * " + fullyQualifiedPrefix() + entry.id + "PSz = " +
               (entry.str.size() ? entry.str : "\"" + entry.id + "\"") + ";\n");
    os.cadd("\n");

    /* to-string function */
    os.add("const char * " + fullyQualifiedPrefix() + "toPSz() const\n");
    os.cadd("{\n");
    os.depth += 2;
    for (auto entry : entries)
    {
        if (!firstCase)
            os.add("else if");
        else
            os.add("if");
        os.add("(value ==" + entry.id + ")\n");
        os.add("  return " + entry.id + "PSz;\n");
        firstCase = false;
    }
    os.add("else assert(\"bad object\");\n");
    os.add("return NULL;\n");
    os.depth -= 2;
    os.add("}\n\n");

    /* comparison */
    /* os.add("bool " + fullyQualifiedPrefix() + "operator==(const " + name +
            "&other) const\n");
     os.cadd("{\n");
     os.add("return value == other.value;\n");
     os.cadd("}\n\n");*/

    genSerDecl(os, true);
    os.cadd("\n");
    os.add("{\n");
    os.depth += 2;
    os.add("return ucl_object_fromstring(toPSz());\n");
    os.depth -= 2;
    os.add("}\n");
}

void EnumDef::genDeserImpl(OutStream &os)
{
    bool firstCase = true;
    /* from-string function */
    os.add(fullyQualifiedName() + " " + fullyQualifiedPrefix() +
           "fromPSz(const char * pSz)\n");
    os.cadd("{\n");
    os.depth += 2;
    for (auto entry : entries)
    {
        if (!firstCase)
            os.add("else if");
        else
            os.add("if");
        os.add("(!strcmp(pSz, " + entry.id + "PSz))\n");
        os.add("  return " + entry.id + ";\n");
        firstCase = false;
    }
    os.add("else return kMax;\n");
    os.depth -= 2;
    os.add("}\n\n");

    /* actual deserialisation function */
    genDeserDecl(os, true);
    os.cadd("\n");
    os.add("{\n");
    os.depth += 2;
    os.add("const char * txt;\n");
    os.add("_E" + name + " value;\n");

    os.add("if (!ucl_object_tostring_safe(obj, &txt)) return false;\n");
    os.add("if ((*out = fromPSz(txt)).value == kMax) return false;\n");
    os.add("return true;\n");

    os.depth -= 2;
    os.add("}\n");
}

std::string Method::implFunName(int ver)
{
    return name + "_v" + toStr(ver);
}

void Method::genClientCallDecl(OutStream &os, int ver, std::string prefix)
{
    os.add("WSRPCError " + prefix + implFunName(ver) + "(");

    os.cadd("WSRPCTransport * xprt");
    if (!retType->isVoid())
        os.cadd(", " + retType->makeDecl("*rval"));

    for (auto arg : args)
    {
        os.cadd(", ");
        os.cadd(arg->type->makeArg(arg->id));
    }
    os.cadd(")");
}

void Method::genClientCallAsynchDecl(OutStream &os, int ver,
                                     std::string className, std::string prefix)
{
    os.add("WSRPCCompletion * " + prefix + implFunName(ver) + "_async(");
    os.cadd("WSRPCTransport * xprt, ");
    os.cadd(className + "Delegate* delegate");

    for (auto arg : args)
    {
        os.cadd(", ");
        os.cadd(arg->type->makeArg(arg->id));
    }
    os.cadd(")");
}

void Method::genClientCallCommonPartImpl(OutStream &os, int ver,
                                         std::string prefix)
{
    os.add("ucl_object_t * params = ucl_object_typed_new(UCL_OBJECT);\n");

    /* serialise arguments */
    for (auto arg : args)
        os.add("ucl_object_t * u" + arg->id + ";\n");
    for (auto arg : args)
        arg->type->genSerialiseInto(os, arg->id, "u" + arg->id);
    for (auto arg : args)
        os.add("ucl_object_insert_key(params, u" + arg->id + ", " +
               quote(arg->id) + ", 0, false);\n");
}

void Method::genClientCallImpl(OutStream &os, int ver, std::string prefix)
{
    genClientCallDecl(os, ver, prefix);
    os.cadd("\n");
    os.add("{\n");
    os.depth += 2;
    os.add("WSRPCError werr;\n");
    os.add("WSRPCCompletion * comp;\n");

    genClientCallCommonPartImpl(os, ver, prefix);

    os.add("comp = ");
    os.cadd("xprt->sendMessage(" + quote(implFunName(ver)) +
            ", params, NULL, NULL);\n");

    os.add("if (comp->wait()) {\n");
    os.depth += 2;
    if (!retType->isVoid())
    {
        os.add("if(comp->result && rval) {\n");
        os.depth += 2;

        os.add("bool suc;\n");
        retType->genDeserialiseInto(os, "suc", "comp->result", "rval", true);

        os.add("if (!suc) {\n");
        os.depth += 2;
        os.add("werr.errcode = WSRPCError::kLocalDeserialisationFailure;\n");
        os.add("werr.errmsg = \"Local deserialisation failure.\";\n");
        os.depth -= 2;
        os.add("}\n");

        os.depth -= 2;
        os.add("}\n");
    }
    os.depth -= 2;
    os.add("}\n");

    os.add("werr = comp->err;\n");
    os.add("delete comp;\n");
    os.add("return werr;\n");

    os.depth -= 2;
    os.add("}\n");
}

void Method::genClientCallAsynchImpl(OutStream &os, int ver, std::string prefix)
{
    std::string className(prefix.substr(0, prefix.size() - 6));

    genClientCallAsynchDecl(os, ver, className, prefix);
    os.cadd("\n");
    os.add("{\n");
    os.depth += 2;
    os.add("WSRPCCompletion * comp;\n");

    genClientCallCommonPartImpl(os, ver, prefix);

    os.add("comp = ");
    os.cadd("xprt->sendMessage(" + quote(implFunName(ver)) +
            ", params, delegate, " + className +
            "Delegate::" + implFunName(ver) + "_didReplyDispatch);\n ");

    os.add("return comp;\n");

    os.depth -= 2;
    os.add("}\n");
}

void Method::genClientDelegateDecl(OutStream &os, int ver, std::string prefix)
{
    os.add("void " + prefix + implFunName(ver) + "_didReply(WSRPCError * err");
    if (!retType->isVoid())
        os.cadd(", " + retType->makeArg("retVal"));
    os.cadd(")");
}

void Method::genClientDelegateDispatcherDecl(OutStream &os, int ver,
                                             std::string prefix)
{
    os.add("void " + prefix + implFunName(ver) +
           "_didReplyDispatch(void * _delegate, WSRPCCompletion * comp)");
}

void Method::genClientDelegateImpl(OutStream &os, int ver, std::string prefix)
{
    genClientDelegateDecl(os, ver, prefix);
    os.add("\n");
    os.add("{\n");
    os.depth += 2;
    os.add("fprintf(stderr, " +
           quote("Unhandled reply to method <" + name + ">\\n") + ");\n");
    os.depth -= 2;
    os.add("}\n");
}

void Method::genClientDelegateDispatcherImpl(OutStream &os, int ver,
                                             std::string prefix)
{
    std::string clsName(prefix.substr(0, prefix.size() - 2)); /* fix this! */
    genClientDelegateDispatcherDecl(os, ver, prefix);
    os.add("{\n");
    os.depth += 2;
    os.add(clsName + " *delegate = (" + clsName + "*) _delegate;\n");

    if (!retType->isVoid())
    {
        os.add(retType->makeDecl("rval") + ";\n\n");
        os.add("if(comp->result) {\n");
        os.depth += 2;

        os.add("bool suc;\n");
        retType->genDeserialiseInto(os, "suc", "comp->result", "rval", false);

        os.add("if (!suc) {\n");
        os.depth += 2;
        os.add(
            "comp->err.errcode = WSRPCError::kLocalDeserialisationFailure;\n");
        os.add("comp->err.errmsg = \"Local deserialisation failure.\";\n");
        os.depth -= 2;
        os.add("}\n");

        os.depth -= 2;
        os.add("}\n");
    }

    os.add("delegate->" + implFunName(ver) + "_didReply(&comp->err");
    if (!retType->isVoid())
        os.cadd(", rval");
    os.cadd(");\n");

    os.depth -= 2;
    os.add("}\n");
}

void Method::genServerImplDecl(OutStream &os, int ver)
{
    bool firstArg = true;
    os.add("virtual bool " + implFunName(ver) + "(");
    os.cadd("WSRPCReq * req");
    if (!retType->isVoid())
        os.cadd(", " + retType->makeDecl("*rval"));
    for (auto arg : args)
    {
        os.cadd(", ");
        os.cadd(arg->type->makeSvcArg(arg->id));
    }
    os.cadd(") = 0;\n");
}

void Method::genServerImplCase(OutStream &os, int ver)
{
    bool firstArg = true;
    os.add("if (req->method_name ==  \"" + implFunName(ver) + "\") {\n");
    os.depth += 2;
    /* arg declarations */
    for (auto arg : args)
    {
        os.add("const ucl_object_t * " + arg->id + "_obj;\n");
        os.add(arg->type->makeDecl(arg->id) + ";\n");
    }
    if (!retType->isVoid())
        os.add(retType->makeDecl("rval") + ";\n");

    /* arg deserialisation */
    for (auto arg : args)
    {
        os.add("if (!(" + arg->id + "_obj = ucl_object_lookup(req->params, \"" +
               arg->id + "\"))) {\n");
        os.add("  req->err = WSRPCError::invalidParams();\n");
        os.add("  goto end;\n");
        os.add("}\n");

        arg->type->genDeserialiseInto(os, "succeeded", arg->id + "_obj",
                                      "&" + arg->id, true);

        os.add("if (!succeeded) {\n");
        os.add("  req->err = WSRPCError::invalidParams();\n");
        os.add("  goto end;\n");
        os.add("}\n\n");
    }

    /* call impl */
    os.add("succeeded = vt->" + implFunName(ver) + "(");
    os.cadd("req");
    if (!retType->isVoid())
        os.cadd(", &rval");
    for (auto arg : args)
    {
        os.cadd(", std::move(" + arg->id + ")");
        firstArg = false;
    }
    os.cadd(");\n");

    if (!retType->isVoid())
    {
        os.add("if (succeeded && !(req->result)) {\n");
        // os.add("  req->result = " + retType->makeSerialiseCall("&rval") +
        //       ";\n");
        retType->genSerialiseInto(os, "rval", "req->result");
        os.add("}\n");
    }
    os.add("goto end;\n");
    os.depth -= 2;
    os.add("}\n\n");
}

void Version::genClientCallDecls(OutStream &os, std::string className)
{
    for (auto meth : methods)
    {
        os.cadd("static");
        meth->genClientCallDecl(os, num);
        os.cadd(";\n");
        os.cadd("static");
        meth->genClientCallAsynchDecl(os, num, className);
        os.cadd(";\n\n");
    }
}

void Version::genClientCallImpls(OutStream &os, std::string prefix)
{
    for (auto meth : methods)
    {
        meth->genClientCallImpl(os, num, prefix);
        meth->genClientCallAsynchImpl(os, num, prefix);
    }
}

void Version::genClientDelegateDecls(OutStream &os)
{
    for (auto meth : methods)
    {
        os.cadd("virtual");
        meth->genClientDelegateDecl(os, num);
        os.cadd(";\n");
        os.cadd("static");
        meth->genClientDelegateDispatcherDecl(os, num);
        os.cadd(";\n");
    }
}

void Version::genClientDelegateImpls(OutStream &os, std::string prefix)
{
    for (auto meth : methods)
    {
        meth->genClientDelegateImpl(os, num, prefix);
        meth->genClientDelegateDispatcherImpl(os, num, prefix);
    }
}

void Version::genServerImplDecls(OutStream &os)
{
    for (auto meth : methods)
        meth->genServerImplDecl(os, num);
}

void Version::genServerImplCases(OutStream &os, std::string prefix)
{
    for (auto meth : methods)
        meth->genServerImplCase(os, num);
    os.add("else {\n");
    os.add("return -1;\n");
    os.add("}\n");
}

std::string Program::className()
{
    std::string canon = name;
    std::replace(canon.begin(), canon.end(), '.', '_');
    return canon;
}

void Program::genDef(OutStream &os)
{
    os.add("#pragma once\n");

    /* server */

    os.add("struct " + className() + "VTable : public WSRPCVTable {\n");
    os.depth += 2;
    for (auto def : types)
        def->genDef(os);
    os.add("static int handleReq(WSRPCReq * req, WSRPCVTable * baseVt);\n");
    for (auto v : versions)
        v->genServerImplDecls(os);

    for (auto def : types)
    {
        /* programs grammatically can't contain subprograms */
        dynamic_cast<SerialisableDef *>(def)->genDeserDecl(os);
        os.cadd(";\n");
    }

    os.depth -= 2;
    os.add("};\n\n");

    os.add("class " + className() + "Delegate\n");
    os.add("{\n");
    os.add(" public:\n");
    os.depth += 2;
    for (auto v : versions)
    {
        v->genClientDelegateDecls(os);
    }
    os.depth -= 2;
    os.add("};\n\n");

    /* client */
    os.add("class " + className() + "Clnt \n");
    os.add("{\n");
    os.add(" public:\n");
    os.depth += 2;
    for (auto v : versions)
    {
        v->genClientCallDecls(os, className());
    }
    os.depth -= 2;
    os.add("};\n\n");
}

void Program::genSerialise(OutStream &os)
{
    for (auto type : types)
        type->genSerialise(os);
}

void Program::genServer(OutStream &os)
{
    os.add("int " + className() +
           "VTable::handleReq(WSRPCReq * req, WSRPCVTable * baseVt)\n{\n");
    os.depth += 2;
    os.add("bool succeeded = false;\n");
    os.add(className() + "VTable * vt = static_cast<" + className() +
           "VTable *>(baseVt);\n");
    for (auto v : versions)
    {
        v->genServerImplCases(os, className() + "VTable::");
    }
    os.add("end:\n");
    os.add("if (!succeeded)\n");
    os.add("  return 1;\n");
    os.add("return 0;");
    os.depth -= 2;
    os.add("}\n\n");
}

void Program::genClient(OutStream &os)
{
    for (auto v : versions)
        v->genClientCallImpls(os, className() + "Clnt::");

    for (auto v : versions)
        v->genClientDelegateImpls(os, className() + "Delegate::");
}

std::string TrlUnit::hdrName()
{
    size_t lSlashPos = name.find_last_of('/');
    std::string shortOutName;
    if (lSlashPos == std::string::npos)
        shortOutName = name;
    else
        shortOutName = name.substr(lSlashPos + 1);
    return shortOutName.substr(0, shortOutName.size() - 1) + "hh";
}

void TrlUnit::genHeader(OutStream &os)
{
    os.add("#pragma once\n");
    os.add(code);
    os.add("#include \"eci/WSRPC.hh\"\n");

    for (auto import : imports)
        os.add("#include \"" + import->hdrName() + "\"\n");
    for (auto type : types)
        type->genDef(os);
    for (auto program : programs)
        program->genDef(os);
}

void TrlUnit::genSerialise(OutStream &os)
{
    for (auto import : imports)
        os.add("#include \"" + import->hdrName() + "\"\n");
    for (auto type : types)
        type->genSerialise(os);
    for (auto program : programs)
        program->genSerialise(os);
}

void TrlUnit::genServer(OutStream &os)
{
    for (auto import : imports)
        os.add("#include \"" + import->hdrName() + "\"\n");
    for (auto program : programs)
        program->genServer(os);
}

void TrlUnit::genClient(OutStream &os)
{
    for (auto import : imports)
        os.add("#include \"" + import->hdrName() + "\"\n");
    for (auto program : programs)
        program->genClient(os);
}