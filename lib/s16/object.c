#include <stdlib.h>
#include <string.h>
#include "s16.h"
#include "s16db.h"

#define gen_find_id_wrapper(name, type)                                        \
    type * name##_find_id (name##_list box, unsigned long key)                 \
    {                                                                          \
        name##_list_iterator i;                                                \
        for (i = name##_list_begin (box); i != NULL;                           \
             name##_list_iterator_next (&i))                                   \
            if (i->val->id == key)                                             \
                return i->val;                                                 \
        return 0;                                                              \
    }

#define gen_find_name_wrapper(Name, type)                                      \
    type * Name##_find_name (Name##_list box, char const * nam)                \
    {                                                                          \
        Name##_list_iterator i;                                                \
        for (i = Name##_list_begin (box); i != NULL;                           \
             Name##_list_iterator_next (&i))                                   \
            if (!strcmp (i->val->name, nam))                                   \
                return i->val;                                                 \
        return 0;                                                              \
    }

gen_find_id_wrapper (svc, svc_t);
gen_find_name_wrapper (svc, svc_t);

gen_find_id_wrapper (prop, property_t);
gen_find_name_wrapper (prop, property_t);

svc_t * s16_svc_new ()
{
    svc_t * new_svc = calloc (1, sizeof (svc_t));
    new_svc->properties = List_new ();
    new_svc->instances = List_new ();
    return new_svc;
}

svc_instance_t * s16_inst_new (const char * name)
{
    svc_instance_t * new_inst = calloc (1, sizeof (svc_instance_t));
    new_inst->name = strdup (name);

    new_inst->properties = List_new ();
    return new_inst;
}

const char * _object_get_property_string (prop_list box, const char * key)
{
    property_t * prop = prop_find_name (box, key);
    if (!prop)
        return 0;
    else
        return prop->value.pval_u.s;
}

long * _object_get_property_int (prop_list box, const char * key)
{
    property_t * prop = prop_find_name (box, key);
    if (!prop)
        return 0;
    else
        return &prop->value.pval_u.i;
}

void _object_set_property_string (prop_list box, const char * key,
                                  const char * value)
{
    if ((!key) | (!box))
        return;
    DestroyPropIfExists (box, key);
    svc_id_t rnum;
    property_t * newProp = calloc (1, sizeof (property_t));

    newProp->name = strdup (key);
    while (prop_find_id (box, rnum))
        rnum = rand ();
    newProp->id = rnum;
    newProp->value.type = STRING;
    newProp->value.pval_u.s = strdup (value);

    prop_list_add (box, newProp);
}

void _object_set_property_int (prop_list box, const char * key, long value)
{
    if ((!key) | (!box))
        return;
    DestroyPropIfExists (box, key);
    svc_id_t rnum;
    property_t * newProp = calloc (1, sizeof (property_t));

    newProp->name = strdup (key);
    while (prop_find_id (box, rnum))
        rnum = rand ();
    newProp->id = rnum;
    newProp->value.type = NUMBER;
    newProp->value.pval_u.i = value;

    prop_list_add (box, newProp);
}

const char * svc_object_get_property_string (svc_t * Svc, const char * key)
{
    return _object_get_property_string (Svc->properties, key);
}

long * svc_object_get_property_int (svc_t * Svc, const char * key)
{
    return _object_get_property_int (Svc->properties, key);
}

void svc_object_set_property_string (svc_t * Svc, const char * key,
                                     const char * value)
{
    _object_set_property_string (Svc->properties, key, value);
}

void svc_object_set_property_int (svc_t * Svc, const char * key, long value)
{
    _object_set_property_int (Svc->properties, key, value);
}

void destroy_property (property_t * delProperty)
{
    if (delProperty->value.type == STRING)
        free (delProperty->value.pval_u.s);
    free (delProperty);
}

void destroy_properties_list (prop_list box)
{
    for (prop_list_iterator it = prop_list_begin (box); it != NULL;
         prop_list_iterator_next (&it))
    {
        destroy_property (it->val);
    }
    List_destroy (box);
}

void destroy_instance (svc_instance_t * delInstance)
{
    destroy_properties_list (delInstance->properties);
    free (delInstance->name);
    free (delInstance);
}

void destroy_svc (svc_t * delSvc)
{
    for (inst_list_iterator it = inst_list_begin (delSvc->instances);
         it != NULL; inst_list_iterator_next (&it))
    {
        destroy_instance (it->val);
    }
    List_destroy (delSvc->instances);
    destroy_properties_list (delSvc->properties);
    free (delSvc->name);
    free (delSvc);
}

void destroy_svcs_list (svc_list box)
{
    for (svc_list_iterator it = svc_list_begin (box); it != NULL;
         svc_list_iterator_next (&it))
    {
        destroy_svc (it->val);
    }
    List_destroy (box);
}