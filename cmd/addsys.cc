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
#include <memory>
#include <string>
#include <unistd.h>

#include "eci/Event.hh"
#include "eci/Logger.hh"
#include "eci/Platform.h"
#include "io.eComCloud.eci.IManager.hh"
#include "serviceBundleSchema.json.h"
#include "sqlite3.h"

class Property
{
};

struct PropString : public Property
{
    std::string value;
};

struct PropPage : public Property
{
    std::list<std::unique_ptr<Property>> properties;

    PropPage() = default;
    PropPage(const PropPage &) = delete;
};

struct Instance
{
    std::string clsName;
    std::string name;

    Instance() = default;
    Instance(const Instance &) = delete;
};

struct Class
{
    std::string name;
    std::list<Instance> instances;

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

void AddSys::process(const ucl_object_t *obj)
{
    Class *cls;

    classes.emplace_back();
    cls = &classes.back();

    printf("Out: %s\n", ucl_object_emit(obj, UCL_EMIT_JSON));
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