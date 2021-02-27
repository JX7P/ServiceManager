#include "wsrpcgen.hh"

void Scoped::synthInScope(Scope *aScope)
{
    parentScope = aScope;
}

TypeDef *Scoped::lookupIdentifier(std::string id)
{
    return parentScope->lookupIdentifier(id);
}

void Scope::synthInScope(Scope *aScope)
{
    Scoped::synthInScope(aScope);
    for (auto type : types)
        type->synthInScope(this);
}

/* lookup an identifier - strictly not one with ::, do that by recursing. */
TypeDef *Scope::lookupIdentifier(std::string id)
{
    for (auto type : types)
        if (type->name == id)
            return type;
    return parentScope ? parentScope->lookupIdentifier(id) : NULL;
}

std::string Scope::fullyQualifiedPrefix()
{
    return (parentScope ? parentScope->fullyQualifiedPrefix() : "") + name +
           "::";
}

std::string Scope::fullyQualifiedName()
{
    return (parentScope ? parentScope->fullyQualifiedPrefix() : "") + name;
}

std::string TrlUnit::fullyQualifiedPrefix()
{
    return "";
}

TypeDef *TrlUnit::lookupIdentifier(std::string id)
{
    TypeDef *cand;
    for (auto type : types)
        if (type->name == id)
            return type;
    for (auto unit : imports)
        if ((cand = unit->lookupIdentifier(id)))
            return cand;
    return NULL;
}

void TrlUnit::synthInScope(Scope *aScope)
{
    Scoped::synthInScope(aScope);
    for (auto import : imports)
        import->synthInScope(NULL);
    for (auto type : types)
        type->synthInScope(this);
    for (auto program : programs)
        program->synthInScope(this);
}

void StructDef::synthInScope(Scope *aScope)
{
    Scope::synthInScope(aScope);

    for (auto decl : decls)
        decl->type->synthInScope(this);
}

void TypeRef::synthInScope(Scope *aScope)
{
    std::vector<std::string> parts;
    size_t start;
    size_t end = 0;
    bool first = true;

    while ((start = type.find_first_not_of("::", end)) != std::string::npos)
    {
        end = type.find("::", start);
        parts.push_back(type.substr(start, end - start));
    }

    for (auto part : parts)
    {
        if (first)
            def = aScope->lookupIdentifier(part);
        else
            def = def->lookupIdentifier(part);
        first = false;
        if (!def)
            break;
    }
}

void Method::synthInScope(Scope *aScope)
{
    retType->synthInScope(aScope);
    for (auto arg : args)
        arg->type->synthInScope(aScope);
}

void Version::synthInScope(Scope *aScope)
{
    for (auto method : methods)
        method->synthInScope(aScope);
}

void Program::synthInScope(Scope *aScope)
{
    TypeDef::synthInScope(aScope);
    for (auto version : versions)
        version->synthInScope(this);
}
