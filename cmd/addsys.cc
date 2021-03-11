/*******************************************************************

    PROPRIETARY NOTICE

These coded instructions, statements, and computer programs contain
proprietary information of eComCloud Object Solutions, and they are
protected under copyright law. They may not be distributed, copied,
or used except under the provisions of the terms of the Source Code
Licence Agreement, in the file "LICENCE.md", which should have been
included with this software

    Copyright Notice

    (c) 2021 eComCloud Object Solutions.
        All rights reserved.
********************************************************************/
/**
 * addsys(8) imports a service bundle into the system. The layer into which the
 * bundle is to be imported must be specified with the -l argument. If there is
 * no `live` snapshot for the services specified in the bundle, the changes made
 * are reflected immediately in the configuration of the services; otherwise the
 * new configuration will not be made live until a refresh command is issued.
 */

#include <sys/socket.h>
#include <sys/un.h>
#include <cassert>
#include <memory>
#include <string>
#include <unistd.h>

#include "eci/Event.hh"
#include "eci/Logger.hh"
#include "eci/Platform.h"
#include "io.eComCloud.eci.IManager.hh"
#include "serviceBundleSchema.json.h"
#include "sqlite3.h"

struct Property
{
    std::string key;

    Property() = default;
    Property(std::string key) : key(std::move(key)){};
};

struct PropString : public Property
{
    std::string value;

    PropString() = default;
    PropString(std::string key, std::string value)
        : Property(std::move(key)), value(std::move(value)){};
};

struct PropPage : public Property
{
    std::list<std::unique_ptr<Property>> properties;

    PropPage() = default;
    PropPage(std::string key) : Property(std::move(key)){};
    PropPage(const PropPage &) = delete;
};

struct Instance
{
    std::string clsName;
    std::string name;
    std::list<std::unique_ptr<Property>> properties;

    Instance() = default;
    Instance(const Instance &) = delete;
};

struct Class
{
    std::string name;
    std::list<Instance> instances;
    std::list<std::unique_ptr<Property>> properties;

    Class() = default;
    Class(const Class &) = delete;
};

class AddSys : Logger, io_eComCloud_eci_IManagerDelegate, Handler
{
    sqlite3 *conn;
    struct ucl_parser *parser;
    ucl_object_t *schemaBundle;

    /* List of processed classes to be imported */
    std::list<Class> classes;

    void import(int layer, const char *bundlePath);
    void process(const ucl_object_t *obj);
    void processDeps(std::list<std::unique_ptr<Property>> &properties,
                     const ucl_object_t *obj, bool isDependents = false);
    void processInst(std::list<Instance> &nsts, const ucl_object_t *obj);
    /* Process a UCL page adding it to the properties list */
    void processPage(std::list<std::unique_ptr<Property>> &properties,
                     const ucl_object_t *obj, const char *type = NULL);
    void processProp(std::list<std::unique_ptr<Property>> &properties,
                     const ucl_object_t *obj);

  public:
    AddSys() : Logger("addsys"){};

    int main(int argc, char *argv[]);
};

void AddSys::import(int layer, const char *bundlePath)
{
    ucl_object_t *obj = NULL;
    ucl_schema_error errValidation;

    parser = ucl_parser_new(0);
    parser = ucl_parser_new(0);

    ucl_parser_add_file(parser, bundlePath);

    if (ucl_parser_get_error(parser))
    {

        log(kWarn, "  %s: %s\n", bundlePath, ucl_parser_get_error(parser));

        goto cleanup;
    }

    obj = ucl_parser_get_object(parser);

    if (!ucl_object_validate(schemaBundle, obj, &errValidation))
    {
        log(kWarn, "  %s: %s in:\n%s\n", bundlePath, errValidation.msg);
        printf("%s\n",
               ((char *)ucl_object_emit(errValidation.obj, UCL_EMIT_JSON)));
        goto cleanup;
    }

    process(obj);

cleanup:
    ucl_parser_free(parser);
    if (obj)
        ucl_obj_unref(obj);
}

#define UclIterate(top, iterName, objName, expand)                             \
    do                                                                         \
    {                                                                          \
        ucl_object_iter_t iterName = NULL;                                     \
        const ucl_object_t *objName;                                           \
        while ((objName = ucl_iterate_object(top, &iterName, expand)))

#define UclCloseIterate()                                                      \
    }                                                                          \
    while (0)

void AddSys::process(const ucl_object_t *uclCls)
{
    Class *cls;
    ucl_object_iter_t it = NULL;
    const ucl_object_t *obj;

    classes.emplace_back();
    cls = &classes.back();

    while ((obj = ucl_iterate_object(uclCls, &it, true)))
    {
        const char *key = ucl_object_key(obj);
        if (!strcmp(key, "name"))
            cls->name = ucl_object_tostring(obj);
        else if (!strcmp(key, "depends"))
            processDeps(cls->properties, obj);
        else if (!strcmp(key, "dependents"))
            processDeps(cls->properties, obj, true);
        else if (!strcmp(key, "instances"))
        {
            UclIterate(obj, instIt, inst, true)
                processInst(cls->instances, inst);
            UclCloseIterate();
        }
        else if (!strcmp(key, "methods"))
        {
            UclIterate(obj, depsIt, uclPage, true)
                processPage(cls->properties, uclPage, "method");
            UclCloseIterate();
        }
        else
            processProp(cls->properties, obj);
    }
}

void AddSys::processDeps(std::list<std::unique_ptr<Property>> &properties,
                         const ucl_object_t *obj, bool isDependents)
{
    UclIterate(obj, depsIt, uclArrDep, true)
    {
        const char *depType = ucl_object_key(uclArrDep);

        UclIterate(uclArrDep, depIt, uclDep, true)
        {
            PropPage *page = new PropPage;
            const char *nstName = ucl_object_tostring(uclDep);
            PropString *propNstPath = new PropString("instance", nstName);
            PropString *propDepType = new PropString(
                "type", isDependents ? "dependent" : "dependency");

            page->key.append("dependency_");
            page->key.append(depType);
            page->key.append(nstName);

            properties.emplace_back(page);
        }
        UclCloseIterate();
    }
    UclCloseIterate();
}

void AddSys::processInst(std::list<Instance> &nsts, const ucl_object_t *inst)
{
    Instance *nst;

    nsts.emplace_back();
    nst = &nsts.back();

    nst->name = ucl_object_key(inst);

    UclIterate(inst, objIt, obj, true)
    {
        const char *key = ucl_object_key(obj);
        if (!strcmp(key, "depends"))
            processDeps(nst->properties, obj);
        else if (!strcmp(key, "dependents"))
            processDeps(nst->properties, obj, true);
        else if (!strcmp(key, "methods"))
        {
            UclIterate(obj, depsIt, uclPage, true)
                processPage(nst->properties, uclPage, "method");
            UclCloseIterate();
        }
        else

            printf("extra key: \"%s\"\n", key);
    }
    UclCloseIterate();
}

void AddSys::processPage(std::list<std::unique_ptr<Property>> &properties,
                         const ucl_object_t *obj, const char *type)
{
    PropPage *page = new PropPage;

    assert(ucl_object_type(obj) == UCL_OBJECT);

    page->key = ucl_object_key(obj);

    if (!type)
        page->properties.emplace_back(new PropString("type", type));

    UclIterate(obj, propsIt, member, true)
    {
        if (ucl_object_type(member) == UCL_STRING)
            page->properties.emplace_back(new PropString(
                ucl_object_key(member), ucl_object_tostring(member)));
        else
            processPage(page->properties, member);
    }
    UclCloseIterate();

    properties.emplace_back(std::move(page));
}

void AddSys::processProp(std::list<std::unique_ptr<Property>> &properties,
                         const ucl_object_t *obj)
{
    if (ucl_object_type(obj) == UCL_STRING)
        properties.emplace_back(
            new PropString(ucl_object_key(obj), ucl_object_tostring(obj)));
    else
        processPage(properties, obj);
}

int AddSys::main(int argc, char *argv[])
{
    char c;
    int res;
    int layer = 0;
    const char *pathDb = NULL;

    parser = ucl_parser_new(0);
    ucl_parser_add_string(parser, kserviceBundleSchema_json, 0);

    if (ucl_parser_get_error(parser))
    {
        /* ought to not happen */
        die("Internal error while parsing schema: %s\n",
            ucl_parser_get_error(parser));
    }

    schemaBundle = ucl_parser_get_object(parser);
    ucl_parser_free(parser);

    /*
     * -l {1,2,3,4}: layer into which to import
     * -r <path>: path to the repository database
     * <path>: path of the service bundle
     */
    while ((c = getopt(argc, argv, "l:r:")) != -1)
        switch (c)
        {
        case 'l':
#define CASE(i)                                                                \
    if (!strcmp(optarg, #i))                                                   \
    layer = i
            CASE(1);
            else CASE(2);
            else CASE(3);
            else CASE(4);
            else die("Invalid argument: %s is not a layer", optarg);
            break;

        case 'r':
            pathDb = optarg;
            break;
        }

    if (!layer)
        die("Invalid argument: no layer specified\n");
    else if (!pathDb)
        die("Invalid argument: no repository path specified\n");
    if (argc - optind != 1)
        die("Invalid argument: only one service bundle path is allowed\n");

    res = sqlite3_open_v2(pathDb, &conn,
                          SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX, NULL);
    if (res != SQLITE_OK)
        die("Failed to open repository: %s\n", sqlite3_errmsg(conn));

    import(layer, argv[optind]);

    return 0;
}

int main(int argc, char *argv[])
{
    AddSys addsys;
    return addsys.main(argc, argv);
}