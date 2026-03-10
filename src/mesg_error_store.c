/* Simple JSON sidecar writer/reader for per-mailbox mesg errors. */
#include "../include/mesg_error_store.h"
#include "../include/StringUtil.h"
#include "../include/fileutil.h"
#include "../include/legacy_shim.h"
#include "../include/mailbox.h"
#include "../include/toc.h"
#include <errno.h>
#include <json-glib/json-glib.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

/* GLib/json-glib based JSON sidecar writer/reader for per-mailbox mesg errors.
 */
static void build_sidecar_path(const FSSpec *spec, char *out, size_t outlen) {
  if (!spec || !out)
    return;
  snprintf(out, outlen, "%s.mesg_errors.json", spec->path);
}

/* Minimal JSON escaping for strings */
static void json_escape(const char *in, char *out, size_t outlen) {
  size_t oi = 0;
  for (size_t i = 0; in && in[i] && oi + 4 < outlen; ++i) {
    unsigned char c = in[i];
    if (c == '"' || c == '\\') {
      out[oi++] = '\\';
      out[oi++] = c;
    } else if (c == '\n') {
      out[oi++] = '\\';
      out[oi++] = 'n';
    } else if (c == '\r') {
      out[oi++] = '\\';
      out[oi++] = 'r';
    } else {
      out[oi++] = c;
    }
  }
  out[oi] = '\0';
}

/* Minimal unescape for our format */
static void json_unescape(const char *in, char *out, size_t outlen) {
  size_t oi = 0;
  for (size_t i = 0; in && in[i] && oi + 1 < outlen; ++i) {
    if (in[i] == '\\' && in[i + 1]) {
      char n = in[++i];
      if (n == 'n')
        out[oi++] = '\n';
      else if (n == 'r')
        out[oi++] = '\r';
      else
        out[oi++] = n;
    } else
      out[oi++] = in[i];
  }
  out[oi] = '\0';
}

int mesg_error_store_save_all(TOCType * tocH) {
  if (!tocH)
    return -1;
  FSSpec spec = GetMailboxSpec(tocH, -1);
  char sidecar[PATH_MAX];
  build_sidecar_path(&spec, sidecar, sizeof(sidecar));

  JsonBuilder *builder = json_builder_new();
  json_builder_begin_array(builder);

  for (int i = 0; i < tocH->count; ++i) {
    mesgErrorHandle h = tocH->sums[i].mesgErrH;
    if (!h)
      continue;
    json_builder_begin_object(builder);
    char uidbuf[32];
    g_snprintf(uidbuf, sizeof(uidbuf), "%08lx", (unsigned long)(*h)->uidHash);
    json_builder_set_member_name(builder, "uid_hash");
    json_builder_add_string_value(builder, uidbuf);
    json_builder_set_member_name(builder, "error_code");
    json_builder_add_int_value(builder, (*h)->errorCode);
    json_builder_set_member_name(builder, "error_text");
    json_builder_add_string_value(builder, (const gchar *)(*h)->errorStr);
    json_builder_set_member_name(builder, "created_at");
    json_builder_add_int_value(builder, (gint64)time(NULL));
    json_builder_end_object(builder);
  }

  json_builder_end_array(builder);
  JsonGenerator *gen = json_generator_new();
  JsonNode *root = json_builder_get_root(builder);
  json_generator_set_root(gen, root);
  gchar *data = json_generator_to_data(gen, NULL);

  GError *gerr = NULL;
  GFile *file = g_file_new_for_path(sidecar);
  gboolean ok = g_file_replace_contents(file, data, strlen(data), NULL, FALSE,
                                        G_FILE_CREATE_NONE, NULL, NULL, &gerr);
  g_free(data);
  g_object_unref(gen);
  json_node_free(root);
  g_object_unref(builder);
  g_object_unref(file);
  if (!ok) {
    if (gerr)
      g_error_free(gerr);
    return -1;
  }
  return 0;
}

int mesg_error_store_load(TOCType * tocH) {
  if (!tocH)
    return -1;
  FSSpec spec = GetMailboxSpec(tocH, -1);
  char sidecar[PATH_MAX];
  build_sidecar_path(&spec, sidecar, sizeof(sidecar));

  FILE *f = fopen(sidecar, "r");
  if (!f)
    return 0; /* no file is not an error */

  char line[4096];
  while (fgets(line, sizeof(line), f)) {
    /* look for uid_hash */
    char *p = strstr(line, "\"uid_hash\":");
    if (!p)
      continue;
    p = strchr(p, '"');
    if (!p)
      continue; /* first quote */
    p = strchr(p + 1, '"');
    if (!p)
      continue; /* second quote */
    p++;
    char *q = strchr(p, '"');
    if (!q)
      continue;
    char uidbuf[32];
    size_t len = q - p;
    if (len >= sizeof(uidbuf))
      len = sizeof(uidbuf) - 1;
    memcpy(uidbuf, p, len);
    uidbuf[len] = 0;

    unsigned long uid = 0;
    if (sscanf(uidbuf, "%lx", &uid) != 1)
      continue;

    /* error_code */
    p = strstr(q, "\"error_code\":");
    if (!p)
      continue;
    int code = 0;
    if (sscanf(p + 13, "%d", &code) != 1)
      continue;

    /* error_text */
    p = strstr(p, "\"error_text\":");
    if (!p)
      continue;
    p = strchr(p, '"');
    if (!p)
      continue;
    p = strchr(p + 1, '"');
    if (!p)
      continue;
    p++;
    q = strchr(p, '"');
    if (!q)
      continue;
    char textbuf[1024];
    len = q - p;
    if (len >= sizeof(textbuf))
      len = sizeof(textbuf) - 1;
    memcpy(textbuf, p, len);
    textbuf[len] = 0;
    char unesc[1024];
    json_unescape(textbuf, unesc, sizeof(unesc));

    /* find matching summary and attach */
    short sum = FindSumByHash(tocH, (uLong)uid);
    if (sum != -1) {
      mesgErrorHandle h = (mesgErrorHandle)NuHandleClear(sizeof(MesgErrorType));
      if (h) {
        PCSTrim((*h)->errorStr, (PStr)unesc);
        (*h)->uidHash = (uLong)uid;
        (*h)->errorCode = code;
        tocH->sums[sum].mesgErrH = (void **)h;
      }
    }
  }
  fclose(f);
  return 0;
}
