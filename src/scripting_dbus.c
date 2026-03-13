/* scripting_dbus.c — D-Bus scripting backend for Eudora (Linux)
 *
 * Exposes Eudora's scripting API over D-Bus so external tools and scripts
 * can automate filter management (and later: mailboxes, messages, etc.).
 *
 * D-Bus interface: org.geudora.Scripting
 * Object path:     /org/geudora/Filters
 *
 * Methods mirror the ScriptXxx() functions in scripting.h:
 *   CountFilters()                          -> i
 *   CreateFilter(position: i)               -> x  (returns new ID)
 *   DeleteFilter(id: x, byId: b)           -> b
 *   GetFilterProperty(id: x, byId: b, prop: i) -> v
 *   SetFilterProperty(id: x, byId: b, prop: i, val: v) -> b
 *   GetTermProperty(id: x, byId: b, term: i, prop: i) -> v
 *   SetTermProperty(id: x, byId: b, term: i, prop: i, val: v) -> b
 *
 * Build: link with gio-2.0 (GDBus). GLib/GIO are already GTK dependencies.
 *
 * Copyright (c) 2017, Computer History Museum. All rights reserved.
 * See LICENSE for terms.
 */

#include "scripting.h"
#include <gio/gio.h>
#include <string.h>

/* D-Bus interface XML introspection data */
static const char *sIntrospectionXML =
  "<node>"
  "  <interface name='org.geudora.Scripting'>"
  "    <method name='CountFilters'>"
  "      <arg direction='out' type='i' name='count'/>"
  "    </method>"
  "    <method name='CreateFilter'>"
  "      <arg direction='in'  type='i' name='position'/>"
  "      <arg direction='out' type='x' name='newId'/>"
  "    </method>"
  "    <method name='DeleteFilter'>"
  "      <arg direction='in'  type='x' name='idOrIndex'/>"
  "      <arg direction='in'  type='b' name='byId'/>"
  "      <arg direction='out' type='b' name='success'/>"
  "    </method>"
  "    <method name='GetFilterProperty'>"
  "      <arg direction='in'  type='x' name='idOrIndex'/>"
  "      <arg direction='in'  type='b' name='byId'/>"
  "      <arg direction='in'  type='i' name='property'/>"
  "      <arg direction='out' type='v' name='value'/>"
  "    </method>"
  "    <method name='SetFilterProperty'>"
  "      <arg direction='in'  type='x' name='idOrIndex'/>"
  "      <arg direction='in'  type='b' name='byId'/>"
  "      <arg direction='in'  type='i' name='property'/>"
  "      <arg direction='in'  type='v' name='value'/>"
  "      <arg direction='out' type='b' name='success'/>"
  "    </method>"
  "    <method name='GetTermProperty'>"
  "      <arg direction='in'  type='x' name='idOrIndex'/>"
  "      <arg direction='in'  type='b' name='byId'/>"
  "      <arg direction='in'  type='i' name='termIndex'/>"
  "      <arg direction='in'  type='i' name='property'/>"
  "      <arg direction='out' type='v' name='value'/>"
  "    </method>"
  "    <method name='SetTermProperty'>"
  "      <arg direction='in'  type='x' name='idOrIndex'/>"
  "      <arg direction='in'  type='b' name='byId'/>"
  "      <arg direction='in'  type='i' name='termIndex'/>"
  "      <arg direction='in'  type='i' name='property'/>"
  "      <arg direction='in'  type='v' name='value'/>"
  "      <arg direction='out' type='b' name='success'/>"
  "    </method>"
  "  </interface>"
  "</node>";

static GDBusNodeInfo *sNodeInfo = NULL;
static guint sOwnerId = 0;
static guint sRegistrationId = 0;

/*----------------------------------------------------------------------
 * Convert ScriptValue to GVariant (for D-Bus return values)
 *--------------------------------------------------------------------*/
static GVariant *script_value_to_variant(const ScriptValue *v)
{
  switch (v->type) {
  case kScriptValString:
    return g_variant_new_variant(g_variant_new_string(v->u.str));
  case kScriptValLong:
    return g_variant_new_variant(g_variant_new_int64(v->u.num));
  case kScriptValBool:
    return g_variant_new_variant(g_variant_new_boolean(v->u.flag));
  default:
    return g_variant_new_variant(g_variant_new_string(""));
  }
}

/*----------------------------------------------------------------------
 * Convert GVariant to ScriptValue (for D-Bus input values)
 *--------------------------------------------------------------------*/
static ScriptValue variant_to_script_value(GVariant *variant)
{
  ScriptValue v;
  memset(&v, 0, sizeof(v));

  /* Unwrap outer variant container */
  GVariant *inner = g_variant_get_variant(variant);
  const GVariantType *t = g_variant_get_type(inner);

  if (g_variant_type_equal(t, G_VARIANT_TYPE_STRING)) {
    v.type = kScriptValString;
    const char *s = g_variant_get_string(inner, NULL);
    strncpy(v.u.str, s, 255);
    v.u.str[255] = '\0';
  } else if (g_variant_type_equal(t, G_VARIANT_TYPE_INT64)) {
    v.type = kScriptValLong;
    v.u.num = (long)g_variant_get_int64(inner);
  } else if (g_variant_type_equal(t, G_VARIANT_TYPE_BOOLEAN)) {
    v.type = kScriptValBool;
    v.u.flag = g_variant_get_boolean(inner);
  } else if (g_variant_type_equal(t, G_VARIANT_TYPE_INT32)) {
    v.type = kScriptValLong;
    v.u.num = (long)g_variant_get_int32(inner);
  }

  g_variant_unref(inner);
  return v;
}

/*----------------------------------------------------------------------
 * D-Bus method call handler
 *--------------------------------------------------------------------*/
static void handle_method_call(GDBusConnection *conn,
                               const gchar *sender,
                               const gchar *objectPath,
                               const gchar *interfaceName,
                               const gchar *methodName,
                               GVariant *params,
                               GDBusMethodInvocation *invocation,
                               gpointer userData)
{
  (void)conn; (void)sender; (void)objectPath;
  (void)interfaceName; (void)userData;

  if (g_strcmp0(methodName, "CountFilters") == 0) {
    long count = 0;
    ScriptCountFilters(&count);
    g_dbus_method_invocation_return_value(invocation,
      g_variant_new("(i)", (gint32)count));

  } else if (g_strcmp0(methodName, "CreateFilter") == 0) {
    gint32 position;
    long newId = 0;
    g_variant_get(params, "(i)", &position);
    int err = ScriptCreateFilter(position, &newId);
    if (err)
      g_dbus_method_invocation_return_error(invocation, G_DBUS_ERROR,
        G_DBUS_ERROR_FAILED, "CreateFilter failed: %d", err);
    else
      g_dbus_method_invocation_return_value(invocation,
        g_variant_new("(x)", (gint64)newId));

  } else if (g_strcmp0(methodName, "DeleteFilter") == 0) {
    gint64 idOrIndex;
    gboolean byId;
    g_variant_get(params, "(xb)", &idOrIndex, &byId);
    int err = ScriptDeleteFilter((long)idOrIndex, byId);
    g_dbus_method_invocation_return_value(invocation,
      g_variant_new("(b)", err == 0));

  } else if (g_strcmp0(methodName, "GetFilterProperty") == 0) {
    gint64 idOrIndex;
    gboolean byId;
    gint32 prop;
    g_variant_get(params, "(xbi)", &idOrIndex, &byId, &prop);
    ScriptValue out;
    memset(&out, 0, sizeof(out));
    int err = ScriptGetFilterProperty((long)idOrIndex, byId,
                                      (ScriptPropertyID)prop, &out);
    if (err)
      g_dbus_method_invocation_return_error(invocation, G_DBUS_ERROR,
        G_DBUS_ERROR_FAILED, "GetFilterProperty failed: %d", err);
    else
      g_dbus_method_invocation_return_value(invocation,
        g_variant_new("(v)", script_value_to_variant(&out)));

  } else if (g_strcmp0(methodName, "SetFilterProperty") == 0) {
    gint64 idOrIndex;
    gboolean byId;
    gint32 prop;
    GVariant *valVariant;
    g_variant_get(params, "(xbiv)", &idOrIndex, &byId, &prop, &valVariant);
    ScriptValue in = variant_to_script_value(valVariant);
    g_variant_unref(valVariant);
    int err = ScriptSetFilterProperty((long)idOrIndex, byId,
                                      (ScriptPropertyID)prop, &in);
    g_dbus_method_invocation_return_value(invocation,
      g_variant_new("(b)", err == 0));

  } else if (g_strcmp0(methodName, "GetTermProperty") == 0) {
    gint64 idOrIndex;
    gboolean byId;
    gint32 termIndex, prop;
    g_variant_get(params, "(xbii)", &idOrIndex, &byId, &termIndex, &prop);
    ScriptValue out;
    memset(&out, 0, sizeof(out));
    int err = ScriptGetTermProperty((long)idOrIndex, byId, termIndex,
                                    (ScriptPropertyID)prop, &out);
    if (err)
      g_dbus_method_invocation_return_error(invocation, G_DBUS_ERROR,
        G_DBUS_ERROR_FAILED, "GetTermProperty failed: %d", err);
    else
      g_dbus_method_invocation_return_value(invocation,
        g_variant_new("(v)", script_value_to_variant(&out)));

  } else if (g_strcmp0(methodName, "SetTermProperty") == 0) {
    gint64 idOrIndex;
    gboolean byId;
    gint32 termIndex, prop;
    GVariant *valVariant;
    g_variant_get(params, "(xbiiv)", &idOrIndex, &byId, &termIndex,
                  &prop, &valVariant);
    ScriptValue in = variant_to_script_value(valVariant);
    g_variant_unref(valVariant);
    int err = ScriptSetTermProperty((long)idOrIndex, byId, termIndex,
                                    (ScriptPropertyID)prop, &in);
    g_dbus_method_invocation_return_value(invocation,
      g_variant_new("(b)", err == 0));
  }
}

static const GDBusInterfaceVTable sVTable = {
  handle_method_call,
  NULL,  /* get_property */
  NULL,  /* set_property */
  {0}
};

/*----------------------------------------------------------------------
 * Bus acquired callback — register our object
 *--------------------------------------------------------------------*/
static void on_bus_acquired(GDBusConnection *conn, const gchar *name,
                            gpointer userData)
{
  (void)name; (void)userData;
  GError *err = NULL;

  sRegistrationId = g_dbus_connection_register_object(
    conn,
    "/org/geudora/Filters",
    sNodeInfo->interfaces[0],
    &sVTable,
    NULL,  /* user_data */
    NULL,  /* user_data_free_func */
    &err);

  if (err) {
    g_warning("Failed to register D-Bus object: %s", err->message);
    g_error_free(err);
  }
}

/*======================================================================
 * ScriptingInit / ScriptingShutdown — D-Bus backend lifecycle
 *====================================================================*/

int ScriptingInit(void)
{
  GError *err = NULL;

  sNodeInfo = g_dbus_node_info_new_for_xml(sIntrospectionXML, &err);
  if (!sNodeInfo) {
    g_warning("Failed to parse D-Bus introspection XML: %s",
              err ? err->message : "unknown");
    if (err) g_error_free(err);
    return -1;
  }

  sOwnerId = g_bus_own_name(G_BUS_TYPE_SESSION,
    "org.geudora.Scripting",
    G_BUS_NAME_OWNER_FLAGS_NONE,
    on_bus_acquired,
    NULL,  /* name_acquired */
    NULL,  /* name_lost */
    NULL,  /* user_data */
    NULL); /* user_data_free_func */

  return 0;
}

void ScriptingShutdown(void)
{
  if (sOwnerId) {
    g_bus_unown_name(sOwnerId);
    sOwnerId = 0;
  }
  if (sNodeInfo) {
    g_dbus_node_info_unref(sNodeInfo);
    sNodeInfo = NULL;
  }
}
