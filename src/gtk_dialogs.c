/* GTK4 Dialog System for gEudora */

#include "mailbox.h"
#include "prefdefs.h"
#include "gtk_dialogs.h"
#include <gtk/gtk.h>
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>

/* Alert button return values */
#define kAlertStdAlertOKButton 1
#define kAlertStdAlertCancelButton 2
#define kAlertStdAlertOtherButton 3

/* Progress dialog state */
static GtkWidget *progress_window = NULL;
static GtkWidget *progress_label = NULL;
static GtkWidget *progress_bar = NULL;

/* String resource IDs to actual strings mapping */
static const char *get_string_resource(int stringId) {
  /* This loads string resources - expandable based on needs */
  switch (stringId) {
  case 1000:
    return "%d days"; /* JUNK_MAILBOX_EMPTY_DAYS */
  case 1001:
    return "Threshold: %d%%"; /* JUNK_MAILBOX_EMPTY_THRESH */
  case 1002:
    return "Junk"; /* FILE_ALIAS_JUNK */
  case 1003:
    return "The %s mailbox contains %d messages from more than %d %s "
           "ago.\n\nWould you like to empty it now?"; /* JUNK_EMPTY_WARNING */
  case 1004:
    return "%s mailbox already exists"; /* JUNK_PREEXISTING_WARNING */
  case 1005:
    return "%s folder already exists"; /* JUNK_PREEXISTING_FOLDER_WARNING */
  case 1006:
    return "Junk"; /* JUNK */
  case 1007:
    return "You cannot use the %s mailbox to trim the %s mailbox!"; /* JUNK_JUNK_IS_BAD_TRIM_DEST
                                                                     */
  case 1008:
    return "Trash"; /* FILE_ALIAS_TRASH */
  case 1009:
    return "Could not archive messages from %s.\n\nThe %s mailbox could not be "
           "moved to %s."; /* JUNK_CANT_ARCHIVE */
  default:
    return "";
  }
}

/* Compose formatted alert message with resource string substitution */
static char *compose_alert_message(int msgResId, ...) {
  va_list args;
  char buffer[1024];
  const char *format = get_string_resource(msgResId);

  va_start(args, msgResId);
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);

  return g_strdup(buffer);
}

/* ComposeStdAlert - Show standard alert dialog with formatted message */
short ComposeStdAlert(AlertType alertType, int msgResId, ...) {
  const char *title;
  char *message;
  va_list args;

  /* Map alert type to GTK message type */
  switch (alertType) {
  case Note:
    // msg_type = GTK_MESSAGE_INFO; // Removed unused variable
    title = "Information";
    break;
  case Caution:
    // msg_type = GTK_MESSAGE_WARNING; // Removed unused variable
    title = "Warning";
    break;
  case Stop:
    // msg_type = GTK_MESSAGE_ERROR; // Removed unused variable
    title = "Error";
    break;
  default:
    // msg_type = GTK_MESSAGE_INFO; // Removed unused variable
    title = "Notice";
  }

  /* Build message with variable arguments */
  va_start(args, msgResId);
  char buffer[1024];
  const char *format = get_string_resource(msgResId);

  /* Simple message formatting - expand as needed */
  if (format && *format) {
    vsnprintf(buffer, sizeof(buffer), format, args);
    message = g_strdup(buffer);
  } else {
    message = g_strdup_printf("Alert (Resource ID: %d)", msgResId);
  }
  va_end(args);

  /* For now, just print to console and return OK - full GTK4 async dialog needs
   * main loop */
  g_print("%s: %s\n", title, message);
  g_free(message);

  /* Simple synchronous behavior for compatibility - return OK */
  return kAlertStdAlertOKButton;
}

/* AlertStr is implemented in nickmng.c with full GTK4 dialog */

/* Progress functions (OpenProgress, ProgressMessage, ProgressMessageR,
   CloseProgress) are implemented in progress.c */

/* String resource functions */
/* GetRString is provided by util.c — do not redefine here */

/* ComposeRString and EqualStrRes are implemented in stringutil.c */

/* Preference functions - simple GKeyFile-based implementation */
static GKeyFile *prefs_keyfile = NULL;
static char *prefs_file_path = NULL;

static void ensure_prefs_loaded(void) {
  if (prefs_keyfile)
    return;

  prefs_keyfile = g_key_file_new();
  prefs_file_path = g_build_filename(g_get_user_config_dir(), "eudora",
                                     "preferences.ini", NULL);

  /* Try to load existing prefs */
  g_key_file_load_from_file(prefs_keyfile, prefs_file_path,
                            G_KEY_FILE_KEEP_COMMENTS, NULL);
}

long GetPrefLong(short prefId) {
  ensure_prefs_loaded();

  char key[64];
  snprintf(key, sizeof(key), "pref_%d", prefId);

  return g_key_file_get_integer(prefs_keyfile, "preferences", key, NULL);
}

void SetPrefLong(short prefId, long value) {
  ensure_prefs_loaded();

  char key[64];
  snprintf(key, sizeof(key), "pref_%d", prefId);

  g_key_file_set_integer(prefs_keyfile, "preferences", key, value);

  /* Save to file */
  gchar *data = g_key_file_to_data(prefs_keyfile, NULL, NULL);
  if (data) {
    g_mkdir_with_parents(g_path_get_dirname(prefs_file_path), 0755);
    g_file_set_contents(prefs_file_path, data, -1, NULL);
    g_free(data);
  }
}

void SetPref(int prefId, const char *value) {
  ensure_prefs_loaded();

  char key[64];
  snprintf(key, sizeof(key), "pref_%d", prefId);

  g_key_file_set_string(prefs_keyfile, "preferences", key, value);

  /* Save to file */
  gchar *data = g_key_file_to_data(prefs_keyfile, NULL, NULL);
  if (data) {
    g_mkdir_with_parents(g_path_get_dirname(prefs_file_path), 0755);
    g_file_set_contents(prefs_file_path, data, -1, NULL);
    g_free(data);
  }
}

/* GetPref - fill dest with preference string, return dest */
unsigned char *GetPref(unsigned char *dest, short prefId) {
  ensure_prefs_loaded();

  char key[64];
  snprintf(key, sizeof(key), "pref_%d", prefId);

  gchar *val = g_key_file_get_string(prefs_keyfile, "preferences", key, NULL);
  if (dest) {
    if (val) {
      size_t len = strlen(val);
      if (len > 255) len = 255;
      /* Store as Pascal string (length byte + data) */
      dest[0] = (unsigned char)len;
      memcpy(dest + 1, val, len);
      g_free(val);
    } else {
      dest[0] = 0;
    }
  }
  return dest;
}

/* PrefIsSet - check if boolean preference is set */
bool PrefIsSet(short prefId) {
  ensure_prefs_loaded();

  char key[64];
  snprintf(key, sizeof(key), "pref_%d", prefId);

  GError *err = NULL;
  gboolean val = g_key_file_get_boolean(prefs_keyfile, "preferences", key, &err);
  if (err) {
    g_error_free(err);
    return false;
  }
  return (bool)val;
}

/* Nag - stub for nag/registration dialogs */
int Nag(int id, void *p, void *proc, void *filter, bool b, ...) {
  return 0;
}

/* Additional UI constants */
#define kpTitle 0 /* Progress title flag */
