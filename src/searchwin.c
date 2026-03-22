/* Copyright (c) 2017, Computer History Museum
All rights reserved.
Redistribution and use in source and binary forms, with or without modification,
are permitted (subject to the limitations in the disclaimer below) provided that
the following conditions are met:
 * Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.
 * Neither the name of Computer History Museum nor the names of its contributors
may be used to endorse or promote products derived from this software without
specific prior written permission.
NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY THIS
LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. */

/*
 * searchwin.c - Search window implementation (GTK4 port)
 *
 * All text searching is delegated to macmbx_search().
 * Numeric criteria (status, priority, junk, size, date, age) are compared
 * locally from MacmbxMsgSum fields.
 */

#include "searchwin.h"
#include "message.h"
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>

/* macmbx API */
#include "macmbx.h"

/* External: get the global mailbox store */
extern MacmbxStore *gtk_mailbox_get_store(void);

/* External: open a message in the UI */
extern void eudora_open_message(MacmbxTOC *toc, int index);

/* ------------------------------------------------------------------ */
/* Globals                                                            */
/* ------------------------------------------------------------------ */
static int gSearchWinCount;

/* ------------------------------------------------------------------ */
/* Category dropdown labels and values                                */
/* ------------------------------------------------------------------ */
typedef struct {
  const char *label;
  int value;
} CategoryEntry;

static const CategoryEntry kCategories[] = {
  { "Anywhere",       SC_ANYWHERE },
  { "Headers",        SC_HEADERS },
  { "Body",           SC_BODY },
  { "From",           SC_FROM },
  { "To",             SC_TO },
  { "Subject",        SC_SUBJECT },
  { "Cc",             SC_CC },
  { "Bcc",            SC_BCC },
  { "Any Recipient",  SC_ANY_RECIPIENT },
  { "Status",         SC_STATUS },
  { "Priority",       SC_PRIORITY },
  { "Junk Score",     SC_JUNK_SCORE },
  { "Size (KB)",      SC_SIZE },
  { "Date",           SC_DATE },
  { "Age",            SC_AGE },
  { "Summary",        SC_SUMMARY },
};
#define N_CATEGORIES (int)(sizeof(kCategories)/sizeof(kCategories[0]))

static const char *kTextRelations[] = {
  "contains", "contains word", "doesn't contain",
  "is", "is not", "starts with", "ends with", NULL
};
/* Indices map to SR_CONTAINS=1 .. SR_ENDS=7 */

static const char *kNumRelations[] = {
  "is", "is not", "greater than", "less than", NULL
};
/* Indices map to SR_EQUAL=1 .. SR_LESS=4 */

/* ------------------------------------------------------------------ */
/* Result list store columns                                          */
/* ------------------------------------------------------------------ */
enum {
  RCOL_FROM = 0,
  RCOL_SUBJECT,
  RCOL_DATE,
  RCOL_MAILBOX,
  RCOL_TOC_PTR,    /* pointer to MacmbxTOC (stored as G_TYPE_POINTER) */
  RCOL_MSG_INDEX,  /* int index into toc->msgs[] */
  RCOL_COUNT
};

/* ------------------------------------------------------------------ */
/* Internal helpers                                                   */
/* ------------------------------------------------------------------ */

/* Map SearchCriterion.category to macmbx_search field string.
   Returns NULL if the category is numeric (handled locally). */
static const char *category_to_field(int cat) {
  switch (cat) {
    case SC_FROM:          return "from";
    case SC_SUBJECT:       return "subject";
    case SC_BODY:          return "body";
    case SC_ANYWHERE:      return "all";
    case SC_HEADERS:       return "headers";
    case SC_TO:            return "header:To";
    case SC_CC:            return "header:Cc";
    case SC_BCC:           return "header:Bcc";
    case SC_ANY_RECIPIENT: return "header:To";  /* search To, then Cc */
    case SC_SUMMARY:       return "all";  /* search everything */
    default:               return NULL;   /* numeric criteria */
  }
}

/* Check if a category is numeric (compared locally from summary) */
static bool is_numeric_category(int cat) {
  switch (cat) {
    case SC_STATUS:
    case SC_PRIORITY:
    case SC_JUNK_SCORE:
    case SC_SIZE:
    case SC_DATE:
    case SC_AGE:
      return true;
    default:
      return false;
  }
}

/* Compare two ints, return -1/0/1 */
static int int_compare(int a, int b) {
  if (a == b) return 0;
  return a < b ? -1 : 1;
}

/* Test one numeric criterion against a message summary.
   Returns true if matched. */
static bool match_numeric(MacmbxMsgSum *sum, SearchCriterion *crit) {
  int result;
  switch (crit->category) {
    case SC_STATUS: {
      int val = sum->state;
      return crit->specifier == val ? crit->relation == SR_EQUAL
                                    : crit->relation == SR_NOT_EQUAL;
    }
    case SC_PRIORITY: {
      int val = sum->priority;
      if (!val) val = 3;
      result = int_compare((int)crit->specifier, val);
      break;
    }
    case SC_SIZE: {
      long sizeK = (sum->length + 1023) / 1024;
      result = int_compare((int)sizeK, (int)crit->specifier);
      break;
    }
    case SC_JUNK_SCORE: {
      int val = sum->spam_score;
      result = int_compare(val, (int)crit->specifier);
      break;
    }
    case SC_DATE: {
      time_t msgTime = (time_t)sum->seconds;
      struct tm *tm = gmtime(&msgTime);
      if (!tm) return false;
      result = int_compare(tm->tm_year + 1900, (int)crit->specifier);
      break;
    }
    case SC_AGE: {
      time_t now = time(NULL);
      time_t msgTime = (time_t)sum->seconds;
      long diffSecs = (long)(now - msgTime);
      long ageValue;
      switch (crit->ageUnits) {
        case AGE_DAYS:   ageValue = diffSecs / 86400; break;
        case AGE_WEEKS:  ageValue = diffSecs / (86400 * 7); break;
        case AGE_MONTHS: ageValue = diffSecs / (86400 * 30); break;
        case AGE_YEARS:  ageValue = diffSecs / (86400 * 365); break;
        default:         ageValue = diffSecs / 86400; break;
      }
      result = int_compare((int)ageValue, (int)crit->specifier);
      break;
    }
    default:
      return false;
  }
  switch (crit->relation) {
    case SR_EQUAL:     return result == 0;
    case SR_NOT_EQUAL: return result != 0;
    case SR_GREATER:   return result > 0;
    case SR_LESS:      return result < 0;
  }
  return false;
}

/* Recursively collect all mailbox node paths into a GPtrArray */
static void collect_mailbox_paths(MacmbxNode *node, GPtrArray *paths) {
  for (MacmbxNode *n = node; n; n = n->next) {
    if (n->type == MACMBX_NODE_MAILBOX) {
      g_ptr_array_add(paths, g_strdup(n->path));
    }
    if (n->children) {
      collect_mailbox_paths(n->children, paths);
    }
  }
}

/* Format a time_t as a date string */
static void format_date(uint32_t seconds, char *buf, size_t buflen) {
  time_t t = (time_t)seconds;
  struct tm *tm = localtime(&t);
  if (tm) {
    strftime(buf, buflen, "%Y-%m-%d %H:%M", tm);
  } else {
    g_strlcpy(buf, "?", buflen);
  }
}

/* Extract mailbox display name from path */
static const char *path_to_mbname(const char *path) {
  const char *slash = strrchr(path, '/');
  return slash ? slash + 1 : path;
}

/* ------------------------------------------------------------------ */
/* Search execution                                                   */
/* ------------------------------------------------------------------ */

/* Search a single TOC with the given criteria using macmbx_search for
   text criteria and local comparison for numeric criteria.
   Adds matching results to the GtkListStore. */
static void search_one_toc(MacmbxTOC *toc, const char *mbname,
                            SearchInfo *si, GtkListStore *store) {
  if (!toc || toc->count == 0) return;

  /* For each criterion, build a set of matching indices */
  /* Start with all messages or none, depending on AND/OR mode */
  bool *matches = g_malloc0(sizeof(bool) * (size_t)toc->count);

  /* Initialize: AND mode = all true, OR mode = all false */
  if (!si->matchAny) {
    for (int i = 0; i < toc->count; i++) matches[i] = true;
  }

  bool any_criteria = false;

  for (int c = 0; c < si->criteriaCount; c++) {
    SearchCriterion *crit = &si->criteria[c];
    if (crit->text[0] == '\0' && !is_numeric_category(crit->category))
      continue;  /* skip blank text criteria */

    any_criteria = true;

    if (is_numeric_category(crit->category)) {
      /* Local numeric comparison on each summary */
      for (int m = 0; m < toc->count; m++) {
        bool hit = match_numeric(&toc->msgs[m], crit);
        if (si->matchAny) {
          matches[m] = matches[m] || hit;
        } else {
          matches[m] = matches[m] && hit;
        }
      }
    } else {
      /* Text search via macmbx_search */
      const char *field = category_to_field(crit->category);
      if (!field) continue;

      int *results = NULL;
      int count = macmbx_search(toc, field, crit->text, &results);

      if (si->matchAny) {
        /* OR: mark hits as true */
        for (int r = 0; r < count; r++) {
          if (results[r] >= 0 && results[r] < toc->count)
            matches[results[r]] = true;
        }
      } else {
        /* AND: build a hit set, then intersect */
        bool *hit_set = g_malloc0(sizeof(bool) * (size_t)toc->count);
        for (int r = 0; r < count; r++) {
          if (results[r] >= 0 && results[r] < toc->count)
            hit_set[results[r]] = true;
        }
        for (int m = 0; m < toc->count; m++) {
          matches[m] = matches[m] && hit_set[m];
        }
        g_free(hit_set);
      }

      free(results);

      /* For SC_ANY_RECIPIENT, also search Cc */
      if (crit->category == SC_ANY_RECIPIENT) {
        results = NULL;
        count = macmbx_search(toc, "header:Cc", crit->text, &results);
        if (si->matchAny) {
          for (int r = 0; r < count; r++) {
            if (results[r] >= 0 && results[r] < toc->count)
              matches[results[r]] = true;
          }
        } else {
          /* For AND + ANY_RECIPIENT: OR the two sub-results, then AND with matches */
          /* Actually, ANY_RECIPIENT means "To OR Cc", so we expand the hit set */
          for (int r = 0; r < count; r++) {
            if (results[r] >= 0 && results[r] < toc->count)
              matches[results[r]] = true; /* already intersected above, re-enable Cc hits */
          }
        }
        free(results);
      }
    }
  }

  if (!any_criteria) {
    g_free(matches);
    return;
  }

  /* Add matching messages to the result store */
  char datebuf[32];
  GtkTreeIter iter;
  for (int m = 0; m < toc->count; m++) {
    if (!matches[m]) continue;
    MacmbxMsgSum *sum = &toc->msgs[m];
    format_date(sum->seconds, datebuf, sizeof(datebuf));

    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter,
      RCOL_FROM,      sum->from,
      RCOL_SUBJECT,   sum->subject,
      RCOL_DATE,      datebuf,
      RCOL_MAILBOX,   mbname,
      RCOL_TOC_PTR,   toc,
      RCOL_MSG_INDEX, m,
      -1);
    si->nHits++;
  }

  g_free(matches);
}

/* ------------------------------------------------------------------ */
/* GTK UI: Criteria row management                                    */
/* ------------------------------------------------------------------ */

static GtkStringList *make_string_list(const char **items) {
  GtkStringList *sl = gtk_string_list_new(NULL);
  for (int i = 0; items[i]; i++)
    gtk_string_list_append(sl, items[i]);
  return sl;
}

static GtkWidget *make_category_dropdown(void) {
  const char *labels[N_CATEGORIES + 1];
  for (int i = 0; i < N_CATEGORIES; i++)
    labels[i] = kCategories[i].label;
  labels[N_CATEGORIES] = NULL;
  GtkStringList *sl = make_string_list(labels);
  GtkWidget *dd = gtk_drop_down_new(G_LIST_MODEL(sl), NULL);
  gtk_widget_set_size_request(dd, 130, -1);
  return dd;
}

static GtkWidget *make_relation_dropdown(bool numeric) {
  const char **items = numeric ? kNumRelations : kTextRelations;
  GtkStringList *sl = make_string_list(items);
  GtkWidget *dd = gtk_drop_down_new(G_LIST_MODEL(sl), NULL);
  gtk_widget_set_size_request(dd, 130, -1);
  return dd;
}

/* Callback: when category changes, swap the relation dropdown */
static void on_category_changed(GtkDropDown *dd, GParamSpec *pspec, gpointer data) {
  (void)pspec;
  SearchInfo *si = (SearchInfo *)data;
  /* Find which criterion row this is */
  GtkWidget *row = gtk_widget_get_parent(GTK_WIDGET(dd));
  for (int i = 0; i < si->criteriaCount; i++) {
    if (si->criteriaWidgets[i].catCombo == GTK_WIDGET(dd)) {
      guint sel = gtk_drop_down_get_selected(dd);
      if (sel < (guint)N_CATEGORIES) {
        int cat = kCategories[sel].value;
        bool numeric = is_numeric_category(cat);
        /* Replace relation dropdown */
        GtkWidget *old_rel = si->criteriaWidgets[i].relCombo;
        GtkWidget *new_rel = make_relation_dropdown(numeric);
        if (old_rel && row) {
          /* Insert new after catCombo, remove old */
          GtkWidget *sibling = gtk_widget_get_next_sibling(si->criteriaWidgets[i].catCombo);
          gtk_box_insert_child_after(GTK_BOX(row), new_rel, si->criteriaWidgets[i].catCombo);
          gtk_box_remove(GTK_BOX(row), old_rel);
        }
        si->criteriaWidgets[i].relCombo = new_rel;
      }
      break;
    }
  }
}

static GtkWidget *create_criteria_row(SearchInfo *si, int index) {
  GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_widget_set_margin_start(row, 4);
  gtk_widget_set_margin_end(row, 4);
  gtk_widget_set_margin_top(row, 2);
  gtk_widget_set_margin_bottom(row, 2);

  GtkWidget *cat = make_category_dropdown();
  GtkWidget *rel = make_relation_dropdown(false);
  GtkWidget *entry = gtk_entry_new();
  gtk_widget_set_hexpand(entry, TRUE);
  gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "Search text...");

  gtk_box_append(GTK_BOX(row), cat);
  gtk_box_append(GTK_BOX(row), rel);
  gtk_box_append(GTK_BOX(row), entry);

  si->criteriaWidgets[index].row = row;
  si->criteriaWidgets[index].catCombo = cat;
  si->criteriaWidgets[index].relCombo = rel;
  si->criteriaWidgets[index].entry = entry;
  si->criteriaWidgets[index].specCombo = NULL;

  g_signal_connect(cat, "notify::selected", G_CALLBACK(on_category_changed), si);

  return row;
}

/* Read criteria from UI widgets into SearchInfo */
static void read_criteria_from_ui(SearchInfo *si) {
  for (int i = 0; i < si->criteriaCount; i++) {
    GtkWidget *catDD = si->criteriaWidgets[i].catCombo;
    GtkWidget *relDD = si->criteriaWidgets[i].relCombo;
    GtkWidget *entry = si->criteriaWidgets[i].entry;

    guint catSel = gtk_drop_down_get_selected(GTK_DROP_DOWN(catDD));
    guint relSel = gtk_drop_down_get_selected(GTK_DROP_DOWN(relDD));

    if (catSel < (guint)N_CATEGORIES) {
      si->criteria[i].category = kCategories[catSel].value;
    }

    bool numeric = is_numeric_category(si->criteria[i].category);
    if (numeric) {
      si->criteria[i].relation = (int)relSel + 1; /* SR_EQUAL=1 */
    } else {
      si->criteria[i].relation = (int)relSel + 1; /* SR_CONTAINS=1 */
    }

    if (entry) {
      const char *text = gtk_editable_get_text(GTK_EDITABLE(entry));
      g_strlcpy(si->criteria[i].text, text ? text : "", sizeof(si->criteria[i].text));
      /* For numeric criteria, parse text as number for specifier */
      if (numeric) {
        si->criteria[i].specifier = atol(si->criteria[i].text);
      }
    }
  }
}

/* ------------------------------------------------------------------ */
/* GTK UI: Callbacks                                                  */
/* ------------------------------------------------------------------ */

/* Forward declarations */
static void on_search_clicked(GtkButton *btn, gpointer data);
static void on_more_criteria(GtkButton *btn, gpointer data);
static void on_fewer_criteria(GtkButton *btn, gpointer data);
static void on_result_activated(GtkTreeView *tv, GtkTreePath *path,
                                 GtkTreeViewColumn *col, gpointer data);

/* Search button callback */
static void on_search_clicked(GtkButton *btn, gpointer data) {
  (void)btn;
  SearchInfo *si = (SearchInfo *)data;
  if (!si) return;

  /* If searching, stop */
  if (si->searching) {
    si->searching = false;
    gtk_button_set_label(GTK_BUTTON(si->searchBtn), "Search");
    gtk_label_set_text(GTK_LABEL(si->statusLabel), "Search stopped.");
    return;
  }

  /* Read criteria from UI */
  read_criteria_from_ui(si);

  /* Get match mode */
  guint matchSel = gtk_drop_down_get_selected(GTK_DROP_DOWN(si->matchCombo));
  si->matchAny = (matchSel == 1); /* 0=All, 1=Any */

  /* Clear previous results */
  GtkListStore *result_store = GTK_LIST_STORE(
    gtk_tree_view_get_model(GTK_TREE_VIEW(si->resultView)));
  gtk_list_store_clear(result_store);
  si->nHits = 0;

  /* Free old mailbox paths */
  if (si->mailboxPaths) {
    g_ptr_array_free(si->mailboxPaths, TRUE);
    si->mailboxPaths = NULL;
  }

  /* Collect mailbox paths from the store */
  MacmbxStore *store = gtk_mailbox_get_store();
  if (!store) {
    gtk_label_set_text(GTK_LABEL(si->statusLabel), "No mailbox store available.");
    return;
  }

  si->mailboxPaths = g_ptr_array_new_with_free_func(g_free);
  MacmbxNode *root = macmbx_store_root(store);
  collect_mailbox_paths(root, si->mailboxPaths);

  if (si->mailboxPaths->len == 0) {
    gtk_label_set_text(GTK_LABEL(si->statusLabel), "No mailboxes found.");
    return;
  }

  si->searching = true;
  si->didSearch = true;
  gtk_button_set_label(GTK_BUTTON(si->searchBtn), "Stop");

  char status[256];
  /* Search each mailbox */
  for (guint i = 0; i < si->mailboxPaths->len; i++) {
    if (!si->searching) break;

    const char *path = g_ptr_array_index(si->mailboxPaths, i);
    const char *mbname = path_to_mbname(path);

    snprintf(status, sizeof(status), "Searching %s... (%u/%u)",
             mbname, i + 1, si->mailboxPaths->len);
    gtk_label_set_text(GTK_LABEL(si->statusLabel), status);

    /* Process pending GTK events to keep UI responsive */
    while (g_main_context_iteration(NULL, FALSE)) {}

    MacmbxTOC *toc = macmbx_toc_open(path);
    if (!toc) continue;

    search_one_toc(toc, mbname, si, result_store);
    /* Note: we do NOT close the TOC here because result rows hold pointers to it.
       The TOC registry in macmbx keeps them alive. */
  }

  si->searching = false;
  gtk_button_set_label(GTK_BUTTON(si->searchBtn), "Search");
  snprintf(status, sizeof(status), "Found %ld messages in %u mailboxes.",
           si->nHits, si->mailboxPaths->len);
  gtk_label_set_text(GTK_LABEL(si->statusLabel), status);
}

/* More Criteria button */
static void on_more_criteria(GtkButton *btn, gpointer data) {
  (void)btn;
  SearchInfo *si = (SearchInfo *)data;
  if (si->criteriaCount >= SEARCH_MAX_CRITERIA) return;
  GtkWidget *row = create_criteria_row(si, si->criteriaCount);
  gtk_box_append(GTK_BOX(si->criteriaBox), row);
  si->criteriaCount++;
  if (si->fewerBtn)
    gtk_widget_set_sensitive(si->fewerBtn, si->criteriaCount > 1);
}

/* Fewer Criteria button */
static void on_fewer_criteria(GtkButton *btn, gpointer data) {
  (void)btn;
  SearchInfo *si = (SearchInfo *)data;
  if (si->criteriaCount <= 1) return;
  si->criteriaCount--;
  GtkWidget *row = si->criteriaWidgets[si->criteriaCount].row;
  if (row) {
    gtk_box_remove(GTK_BOX(si->criteriaBox), row);
    si->criteriaWidgets[si->criteriaCount].row = NULL;
    si->criteriaWidgets[si->criteriaCount].catCombo = NULL;
    si->criteriaWidgets[si->criteriaCount].relCombo = NULL;
    si->criteriaWidgets[si->criteriaCount].entry = NULL;
  }
  gtk_widget_set_sensitive(si->fewerBtn, si->criteriaCount > 1);
}

/* Double-click on result row: open the message */
static void on_result_activated(GtkTreeView *tv, GtkTreePath *path,
                                 GtkTreeViewColumn *col, gpointer data) {
  (void)col; (void)data;
  GtkTreeModel *model = gtk_tree_view_get_model(tv);
  GtkTreeIter iter;
  if (!gtk_tree_model_get_iter(model, &iter, path)) return;

  MacmbxTOC *toc = NULL;
  int idx = -1;
  gtk_tree_model_get(model, &iter,
    RCOL_TOC_PTR, &toc,
    RCOL_MSG_INDEX, &idx,
    -1);

  if (toc && idx >= 0) {
    eudora_open_message(toc, idx);
  }
}

/* ------------------------------------------------------------------ */
/* Build the search panel widget                                      */
/* ------------------------------------------------------------------ */

static GtkWidget *build_search_panel(SearchInfo *si) {
  GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
  gtk_widget_set_margin_start(vbox, 6);
  gtk_widget_set_margin_end(vbox, 6);
  gtk_widget_set_margin_top(vbox, 6);
  gtk_widget_set_margin_bottom(vbox, 6);

  /* ---- Criteria area ---- */
  si->criteriaBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
  gtk_box_append(GTK_BOX(vbox), si->criteriaBox);

  /* First criterion row */
  si->criteriaCount = 1;
  GtkWidget *row0 = create_criteria_row(si, 0);
  gtk_box_append(GTK_BOX(si->criteriaBox), row0);

  /* ---- Control bar: Match mode + More/Fewer + Search ---- */
  GtkWidget *ctrl = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  gtk_widget_set_margin_top(ctrl, 4);
  gtk_widget_set_margin_bottom(ctrl, 4);
  gtk_box_append(GTK_BOX(vbox), ctrl);

  /* Match All / Match Any dropdown */
  const char *match_items[] = { "Match All", "Match Any", NULL };
  GtkStringList *match_sl = make_string_list(match_items);
  si->matchCombo = gtk_drop_down_new(G_LIST_MODEL(match_sl), NULL);
  gtk_widget_set_size_request(si->matchCombo, 120, -1);
  gtk_box_append(GTK_BOX(ctrl), si->matchCombo);

  /* More Criteria */
  si->moreBtn = gtk_button_new_with_label("More Criteria");
  g_signal_connect(si->moreBtn, "clicked", G_CALLBACK(on_more_criteria), si);
  gtk_box_append(GTK_BOX(ctrl), si->moreBtn);

  /* Fewer Criteria */
  si->fewerBtn = gtk_button_new_with_label("Fewer Criteria");
  gtk_widget_set_sensitive(si->fewerBtn, FALSE);
  g_signal_connect(si->fewerBtn, "clicked", G_CALLBACK(on_fewer_criteria), si);
  gtk_box_append(GTK_BOX(ctrl), si->fewerBtn);

  /* Spacer */
  GtkWidget *spacer = gtk_label_new("");
  gtk_widget_set_hexpand(spacer, TRUE);
  gtk_box_append(GTK_BOX(ctrl), spacer);

  /* Search button */
  si->searchBtn = gtk_button_new_with_label("Search");
  gtk_widget_add_css_class(si->searchBtn, "suggested-action");
  g_signal_connect(si->searchBtn, "clicked", G_CALLBACK(on_search_clicked), si);
  gtk_box_append(GTK_BOX(ctrl), si->searchBtn);

  /* ---- Separator ---- */
  GtkWidget *sep = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
  gtk_box_append(GTK_BOX(vbox), sep);

  /* ---- Results area: GtkTreeView ---- */
  GtkListStore *result_store = gtk_list_store_new(RCOL_COUNT,
    G_TYPE_STRING,   /* From */
    G_TYPE_STRING,   /* Subject */
    G_TYPE_STRING,   /* Date */
    G_TYPE_STRING,   /* Mailbox */
    G_TYPE_POINTER,  /* TOC ptr */
    G_TYPE_INT       /* msg index */
  );
  si->resultStore = GTK_WIDGET(result_store); /* keep ref for convenience */

  GtkWidget *tv = gtk_tree_view_new_with_model(GTK_TREE_MODEL(result_store));
  g_object_unref(result_store); /* tree view holds ref */

  /* Columns */
  GtkCellRenderer *rend = gtk_cell_renderer_text_new();

  GtkTreeViewColumn *col_from = gtk_tree_view_column_new_with_attributes(
    "From", rend, "text", RCOL_FROM, NULL);
  gtk_tree_view_column_set_resizable(col_from, TRUE);
  gtk_tree_view_column_set_min_width(col_from, 120);
  gtk_tree_view_column_set_expand(col_from, TRUE);
  gtk_tree_view_append_column(GTK_TREE_VIEW(tv), col_from);

  GtkTreeViewColumn *col_subj = gtk_tree_view_column_new_with_attributes(
    "Subject", rend, "text", RCOL_SUBJECT, NULL);
  gtk_tree_view_column_set_resizable(col_subj, TRUE);
  gtk_tree_view_column_set_min_width(col_subj, 180);
  gtk_tree_view_column_set_expand(col_subj, TRUE);
  gtk_tree_view_append_column(GTK_TREE_VIEW(tv), col_subj);

  GtkTreeViewColumn *col_date = gtk_tree_view_column_new_with_attributes(
    "Date", rend, "text", RCOL_DATE, NULL);
  gtk_tree_view_column_set_resizable(col_date, TRUE);
  gtk_tree_view_column_set_min_width(col_date, 130);
  gtk_tree_view_append_column(GTK_TREE_VIEW(tv), col_date);

  GtkTreeViewColumn *col_mb = gtk_tree_view_column_new_with_attributes(
    "Mailbox", rend, "text", RCOL_MAILBOX, NULL);
  gtk_tree_view_column_set_resizable(col_mb, TRUE);
  gtk_tree_view_column_set_min_width(col_mb, 100);
  gtk_tree_view_append_column(GTK_TREE_VIEW(tv), col_mb);

  si->resultView = tv;

  /* Double-click opens message */
  g_signal_connect(tv, "row-activated", G_CALLBACK(on_result_activated), si);

  /* Scrolled window for results */
  GtkWidget *sw = gtk_scrolled_window_new();
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
    GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_widget_set_vexpand(sw, TRUE);
  gtk_widget_set_hexpand(sw, TRUE);
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(sw), tv);
  gtk_box_append(GTK_BOX(vbox), sw);

  /* ---- Status label ---- */
  si->statusLabel = gtk_label_new("Ready.");
  gtk_label_set_xalign(GTK_LABEL(si->statusLabel), 0.0f);
  gtk_widget_set_margin_top(si->statusLabel, 2);
  gtk_box_append(GTK_BOX(vbox), si->statusLabel);

  return vbox;
}

/* ------------------------------------------------------------------ */
/* Public API: Panel builder for notebook tab                         */
/* ------------------------------------------------------------------ */

GtkWidget *CreateSearchPanel(void) {
  SearchInfo *si = g_new0(SearchInfo, 1);
  GtkWidget *panel = build_search_panel(si);
  /* Store si as object data on the panel so we can retrieve it later */
  g_object_set_data_full(G_OBJECT(panel), "search-info", si, g_free);
  gSearchWinCount++;
  return panel;
}

/* ------------------------------------------------------------------ */
/* Public API: Search window lifecycle                                */
/* ------------------------------------------------------------------ */

MyWindowPtr SearchOpen(int searchMode) {
  (void)searchMode;
  /* Create a standalone GTK window containing the search panel */
  GtkWidget *win = gtk_window_new();
  gtk_window_set_title(GTK_WINDOW(win), "Search");
  gtk_window_set_default_size(GTK_WINDOW(win), 700, 500);

  SearchInfo *si = g_new0(SearchInfo, 1);
  GtkWidget *panel = build_search_panel(si);
  gtk_window_set_child(GTK_WINDOW(win), panel);

  /* Store si on the window */
  g_object_set_data_full(G_OBJECT(win), "search-info", si, g_free);

  gtk_window_present(GTK_WINDOW(win));
  gSearchWinCount++;

  /* Return as MyWindowPtr — callers that use this can cast */
  return (MyWindowPtr)NULL; /* legacy compat; real window is GTK-managed */
}

void SearchClose_cb(MyWindowPtr win) {
  (void)win;
  if (gSearchWinCount > 0) gSearchWinCount--;
}

/* ------------------------------------------------------------------ */
/* Public API: Query functions                                        */
/* ------------------------------------------------------------------ */

bool IsSearchWindow(void *winWP) {
  if (!winWP) return false;
  /* Check if this GtkWidget has our search-info data */
  if (!G_IS_OBJECT(winWP)) return false;
  return g_object_get_data(G_OBJECT(winWP), "search-info") != NULL;
}

MacmbxTOC *GetTOCFromSearchWin(char *spec) {
  (void)spec;
  return NULL;
}

void GetSearchTOC(MyWindowPtr win, MacmbxTOC **ptoc) {
  if (!ptoc) return;
  *ptoc = NULL;
}

bool SearchViewIsMailbox(MacmbxTOC *tocH) {
  (void)tocH;
  return false;
}

bool GetSearchWinSpec(void *winWP, char *spec) {
  (void)winWP; (void)spec;
  return false;
}

/* ------------------------------------------------------------------ */
/* Public API: Summary copy / update                                  */
/* ------------------------------------------------------------------ */

void CopySum(MacmbxMsgSum *sumFrom, MacmbxMsgSum *sumTo, short virtualMBIdx) {
  if (!sumFrom || !sumTo) return;
  (void)virtualMBIdx;
  *sumTo = *sumFrom;
}

void SearchUpdateSum(MacmbxTOC *tocH, short sumNum,
                      MacmbxTOC *fromTocH, long serialNum,
                      bool transfer, bool nuke) {
  (void)tocH; (void)sumNum; (void)fromTocH;
  (void)serialNum; (void)transfer; (void)nuke;
  if (!gSearchWinCount) return;
}

/* ------------------------------------------------------------------ */
/* Public API: Incremental search / mailbox tracking                  */
/* ------------------------------------------------------------------ */

bool SearchIncremental(MyWindowPtr win, MacmbxTOC *tocH, int sumNum) {
  (void)win; (void)tocH; (void)sumNum;
  return false;
}

void SearchInvalTocBox(MacmbxTOC *tocH, short sumNum, int boxCol) {
  (void)tocH; (void)sumNum; (void)boxCol;
}

void TellSearchMBRename(char *oldSpec, char *newSpec) {
  (void)oldSpec; (void)newSpec;
}

bool SearchBoxesInclude(MyWindowPtr win, MacmbxTOC *tocH) {
  (void)win; (void)tocH;
  return false;
}

/* ------------------------------------------------------------------ */
/* Public API: Idle and update                                        */
/* ------------------------------------------------------------------ */

void SearchAllIdle(void) {
  if (!gSearchWinCount) return;
}

void SearchMBUpdate(void) {
  if (!gSearchWinCount) return;
}

/* ------------------------------------------------------------------ */
/* Public API: Saved searches                                         */
/* ------------------------------------------------------------------ */

void SearchSave(MyWindowPtr win, bool saveAs) {
  (void)win; (void)saveAs;
}

void OpenSearchFile(char *spec) {
  (void)spec;
}

void OpenSearchFileAndStart(char *spec) {
  (void)spec;
}

/* ------------------------------------------------------------------ */
/* Public API: Search menu                                            */
/* ------------------------------------------------------------------ */

void BuildSearchMenu(void) { }

void OpenSearchMenu(short item) {
  (void)item;
}

/* ------------------------------------------------------------------ */
/* Public API: Utility                                                */
/* ------------------------------------------------------------------ */

void SearchNewFindStringLo(const char *str, bool withPrejudice) {
  (void)str; (void)withPrejudice;
}

void SearchFixUnread(MacmbxTOC *tocH, bool unread) {
  (void)tocH; (void)unread;
}

void SearchSetWTitle(MyWindowPtr win) {
  (void)win;
}

void AddCriteriaText(SearchInfo *si, char *buf, int bufSize) {
  if (!si || !buf || bufSize <= 0) return;
  buf[0] = '\0';
  int pos = 0;
  for (int i = 0; i < si->criteriaCount && pos < bufSize - 1; i++) {
    if (i > 0 && pos < bufSize - 3) {
      buf[pos++] = ',';
      buf[pos++] = ' ';
    }
    int written = snprintf(buf + pos, (size_t)(bufSize - pos), "%s",
                            si->criteria[i].text);
    if (written > 0) pos += written;
  }
}
