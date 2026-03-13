/* gEudora Theme Engine
 * Unified CSS theming for all panels: Welcome, Statistics, Filters,
 * Address Book, message view, and application chrome. */

#include "theme.h"
#include "gtk_prefs.h"
#include "../gEditCtrl/geditctrl.h"
#include <string.h>

#define PREFS_GROUP_THEME "theme"
#define PREFS_KEY_THEME   "active_theme"

static GeudoraTheme current_theme = THEME_LIGHT;
static GtkWidget *root_window = NULL;
static GtkCssProvider *theme_provider = NULL;

static const char *theme_names[THEME_COUNT] = {
  "Light", "Dark", "Nord", "Solarized", "Monokai"
};

const char *theme_get_name(GeudoraTheme t) {
  if (t >= 0 && t < THEME_COUNT) return theme_names[t];
  return "Light";
}

GeudoraTheme theme_get_current(void) { return current_theme; }
GtkWidget *theme_get_root(void) { return root_window; }

/* ═══════════════════════════════════════════════════════════════════
 * Theme color definitions — each theme defines a palette that gets
 * injected into a single CSS string using named classes.
 *
 * Colors: bg, surface, surface2, border, text, text2, text3,
 *         accent, accent2, hero_from, hero_to, hero_text,
 *         success, warning, danger, info
 * ═══════════════════════════════════════════════════════════════════ */

typedef struct {
  const char *bg, *surface, *surface2, *border;
  const char *text, *text2, *text3;
  const char *accent, *accent2;
  const char *hero_from, *hero_to, *hero_text, *hero_sub;
  const char *success, *warning, *danger, *info, *purple;
  const char *bar_bg;
  const char *input_bg, *input_border, *input_text;
  const char *tab_bg, *tab_active;
} ThemePalette;

static const ThemePalette palettes[THEME_COUNT] = {
  /* LIGHT */
  { "#f0f2f5", "#ffffff", "#f8fafc", "#e2e6ec",
    "#1e293b", "#475569", "#94a3b8",
    "#2563eb", "#1d4ed8",
    "#1e3a5f", "#3a7bb8", "#ffffff", "rgba(255,255,255,0.7)",
    "#16a34a", "#d97706", "#dc2626", "#0891b2", "#7c3aed",
    "#e2e8f0",
    "#ffffff", "#cbd5e1", "#1e293b",
    "#f1f5f9", "#2563eb" },

  /* DARK */
  { "#0f1419", "#1a1f2e", "#242937", "#2d3548",
    "#e2e8f0", "#94a3b8", "#64748b",
    "#60a5fa", "#3b82f6",
    "#1a1f2e", "#2d3a5c", "#e2e8f0", "rgba(226,232,240,0.6)",
    "#4ade80", "#fbbf24", "#f87171", "#22d3ee", "#a78bfa",
    "#2d3548",
    "#242937", "#3d4560", "#e2e8f0",
    "#1a1f2e", "#60a5fa" },

  /* NORD */
  { "#2e3440", "#3b4252", "#434c5e", "#4c566a",
    "#eceff4", "#d8dee9", "#a4b0c3",
    "#88c0d0", "#5e81ac",
    "#3b4252", "#4c566a", "#eceff4", "rgba(216,222,233,0.7)",
    "#a3be8c", "#ebcb8b", "#bf616a", "#88c0d0", "#b48ead",
    "#434c5e",
    "#3b4252", "#4c566a", "#eceff4",
    "#3b4252", "#88c0d0" },

  /* SOLARIZED */
  { "#fdf6e3", "#eee8d5", "#f5efdc", "#d3cbb7",
    "#073642", "#586e75", "#93a1a1",
    "#268bd2", "#2aa198",
    "#073642", "#0a4f5c", "#eee8d5", "rgba(238,232,213,0.75)",
    "#859900", "#b58900", "#dc322f", "#2aa198", "#6c71c4",
    "#d3cbb7",
    "#eee8d5", "#c9c0a5", "#073642",
    "#eee8d5", "#268bd2" },

  /* MONOKAI */
  { "#1e1f1c", "#272822", "#3e3d32", "#49483e",
    "#f8f8f2", "#a6a68a", "#75715e",
    "#66d9ef", "#a6e22e",
    "#272822", "#3e3d32", "#f8f8f2", "rgba(248,248,242,0.6)",
    "#a6e22e", "#e6db74", "#f92672", "#66d9ef", "#ae81ff",
    "#3e3d32",
    "#272822", "#49483e", "#f8f8f2",
    "#272822", "#66d9ef" },
};

/* ═══════════════════════════════════════════════════════════════════
 * Generate the full CSS for a palette.
 * Every panel's classes are defined here using palette colors.
 * ═══════════════════════════════════════════════════════════════════ */

static char *generate_css(const ThemePalette *p) {
  GString *s = g_string_new(NULL);

  /* ── Global / Window chrome ── */
  g_string_append_printf(s,
    /* Window and base text */
    "window, .main-bg { background: %s; color: %s; }"
    "label { color: %s; }"
    "separator { background: %s; }"
    "image { color: %s; -gtk-icon-style: symbolic; }"

    /* Notebook tabs */
    "notebook > header { background: %s; border-bottom: 1px solid %s; }"
    "notebook > header > tabs > tab { background: %s; color: %s;"
    "  border-radius: 6px 6px 0 0; padding: 4px 12px; margin: 0 1px; }"
    "notebook > header > tabs > tab:checked {"
    "  background: %s; color: %s; font-weight: 600;"
    "  border-bottom: 2px solid %s; }"

    /* Paned and scrollbar */
    "paned > separator { background: %s; min-width: 2px; min-height: 2px; }"
    "scrollbar trough { background: %s; }"
    "scrollbar slider { background: %s; border-radius: 99px; min-width: 6px; }"
    "scrollbar slider:hover { background: %s; }",

    /* window */ p->bg, p->text,
    /* label */ p->text,
    /* separator */ p->border,
    /* image */ p->text2,
    /* nb header */ p->surface, p->border,
    /* tab */ p->tab_bg, p->text2,
    /* tab:checked */ p->surface, p->text, p->accent,
    /* paned sep */ p->border,
    /* scrollbar */ p->bg, p->border, p->text3
  );

  /* ── Buttons (normal + toolbar + flat) ── */
  g_string_append_printf(s,
    "button { background: %s; color: %s; border: 1px solid %s;"
    "  border-radius: 6px; padding: 4px 12px; }"
    "button:hover { background: %s; border-color: %s; }"
    "button:active { background: %s; }"
    "button image { color: %s; }"

    /* Flat / frameless buttons (toolbar, close buttons) */
    "button.flat, button.image-button {"
    "  background: transparent; border-color: transparent; }"
    "button.flat:hover, button.image-button:hover {"
    "  background: alpha(%s, 0.12); border-color: transparent; }"

    /* Toolbar area and toolbar buttons */
    ".toolbar-area { background: %s; border-bottom: 1px solid %s; }"
    ".dockable-toolbar { background: %s; }"
    ".toolbar { background: %s; }"
    ".toolbar button, .dockable-toolbar button {"
    "  background: transparent; border: none; border-radius: 6px;"
    "  padding: 4px 6px; min-width: 28px; min-height: 28px; }"
    ".toolbar button:hover, .dockable-toolbar button:hover {"
    "  background: alpha(%s, 0.15); }"
    ".toolbar button:active, .dockable-toolbar button:active {"
    "  background: alpha(%s, 0.25); }"
    ".toolbar button image, .dockable-toolbar button image { color: %s; }"

    /* Suggested/destructive action buttons */
    "button.suggested-action {"
    "  background: %s; color: %s; border-color: %s; }"
    "button.suggested-action:hover {"
    "  background: %s; }"
    "button.destructive-action {"
    "  background: %s; color: %s; border-color: %s; }",

    /* button */ p->surface, p->text, p->border,
    /* hover */ p->surface2, p->accent,
    /* active */ p->border,
    /* button image */ p->text2,
    /* flat hover */ p->text,
    /* toolbar-area */ p->surface, p->border,
    /* dockable-toolbar */ p->surface,
    /* .toolbar */ p->surface,
    /* toolbar btn hover */ p->text,
    /* toolbar btn active */ p->text,
    /* toolbar btn image */ p->text2,
    /* suggested */ p->accent, p->hero_text, p->accent,
    /* suggested hover */ p->accent2,
    /* destructive */ p->danger, p->hero_text, p->danger
  );

  /* ── Inputs, entries, text views ── */
  g_string_append_printf(s,
    "entry { background: %s; color: %s; border: 1px solid %s;"
    "  border-radius: 6px; caret-color: %s; }"
    "entry:focus { border-color: %s; outline-color: %s; }"
    "entry > text { color: %s; }"
    "entry text { color: %s; }"
    "entry placeholder { color: %s; }"

    "textview, textview text {"
    "  background: %s; color: %s; }"

    /* Spin button */
    "spinbutton { background: %s; color: %s; border: 1px solid %s;"
    "  border-radius: 6px; }"
    "spinbutton > text { color: %s; }"
    "spinbutton > button { background: transparent; border: none;"
    "  color: %s; }",

    /* entry */ p->input_bg, p->input_text, p->input_border, p->text,
    /* focus */ p->accent, p->accent,
    /* entry text */ p->input_text,
    /* entry text2 */ p->input_text,
    /* placeholder */ p->text3,
    /* textview */ p->input_bg, p->input_text,
    /* spin */ p->input_bg, p->input_text, p->input_border,
    /* spin text */ p->input_text,
    /* spin btn */ p->text2
  );

  /* ── Frame, check, dropdown, popover ── */
  g_string_append_printf(s,
    "frame > border { border-color: %s; border-radius: 8px; }"
    "frame > label { color: %s; font-weight: 600; font-size: 0.88em; }"

    "checkbutton { color: %s; }"
    "checkbutton indicator { border-color: %s; background: %s; }"
    "checkbutton indicator:checked { background: %s; border-color: %s; }"

    "switch { background: %s; }"
    "switch:checked { background: %s; }"
    "switch slider { background: %s; }"

    "dropdown { background: %s; color: %s; border: 1px solid %s;"
    "  border-radius: 6px; }"
    "dropdown > button { background: transparent; color: %s;"
    "  border: none; }"
    "dropdown > button > box > label { color: %s; }"

    "popover, popover.menu {"
    "  background: %s; color: %s; border: 1px solid %s;"
    "  border-radius: 8px; }"
    "popover modelbutton { color: %s; }"
    "popover modelbutton:hover { background: %s; }"
    "popover label { color: %s; }"

    /* GTK4 GtkDropDown popup list items */
    "popover listview { background: %s; color: %s; }"
    "popover listview row { color: %s; padding: 4px 8px; }"
    "popover listview row:selected { background: %s; color: %s; }"
    "popover listview row:hover { background: %s; color: %s; }"
    "popover listview row:hover label { color: %s; }"
    "popover listview row label { color: inherit; }"
    "popover listview row cell { color: %s; }"
    "popover list row { color: %s; }"
    "popover list row:selected { background: %s; color: %s; }"
    "popover list row:hover { background: %s; color: %s; }"
    "popover list row:hover label { color: %s; }"
    "popover list row label { color: inherit; }",

    /* frame */ p->border, p->text2,
    /* check */ p->text, p->border, p->input_bg,
    /* check:checked */ p->accent, p->accent,
    /* switch */ p->border, p->accent, p->surface,
    /* dropdown */ p->input_bg, p->text, p->input_border,
    /* dropdown btn */ p->text, p->text,
    /* popover */ p->surface, p->text, p->border,
    /* popover model */ p->text, p->surface2, p->text,
    /* popover listview */ p->surface, p->text,
    /* listview row */ p->text,
    /* listview row:selected */ p->accent, "#ffffff",
    /* listview row:hover */ p->surface2, p->text, p->text,
    /* listview row cell */ p->text,
    /* popover list row */ p->text,
    /* list row:selected */ p->accent, "#ffffff",
    /* list row:hover */ p->surface2, p->text, p->text
  );

  /* ── Listbox and navigation sidebar ── */
  g_string_append_printf(s,
    "list { background: %s; color: %s; }"
    "list > row { color: %s; border-bottom: 1px solid %s; padding: 2px 0; }"
    "list > row:selected { background: %s; color: %s; }"
    "list > row:selected label { color: %s; }"
    "list > row:selected image { color: %s; }"
    "list > row:hover { background: %s; color: %s; }"
    "list > row:hover label { color: %s; }"
    "list > row label { color: %s; }"
    "list > row image { color: %s; }"

    /* Navigation sidebar (settings dialog) */
    ".navigation-sidebar { background: %s; }"
    ".navigation-sidebar > row { color: %s; background: %s; }"
    ".navigation-sidebar > row:selected {"
    "  background: alpha(%s, 0.15); color: %s; }"
    ".navigation-sidebar > row:selected label { color: %s; }"
    ".navigation-sidebar > row:selected image { color: %s; }"
    ".navigation-sidebar > row:hover { background: %s; color: %s; }"
    ".navigation-sidebar > row:hover label { color: %s; }"
    ".navigation-sidebar > row label { color: %s; }"
    ".navigation-sidebar > row image { color: %s; }"

    /* Treeview — message list */
    "treeview { background: %s; color: %s; }"
    "treeview header button { background: %s; color: %s;"
    "  border-bottom: 1px solid %s; }"
    "treeview > row { background: transparent; color: %s; }"
    "treeview > row:nth-child(even) { background: alpha(%s, 0.3); }"
    "treeview > row:selected { background: alpha(%s, 0.2); color: %s; }"
    "treeview > row:selected:focus { background: alpha(%s, 0.3); color: %s; }"
    "treeview > row:hover { background: alpha(%s, 0.1); }",

    /* list */ p->surface, p->text,
    /* row */ p->text, p->border,
    /* row:selected */ p->accent, p->hero_text,
    /* sel label/img */ p->hero_text, p->hero_text,
    /* row:hover */ p->surface2, p->text, p->text,
    /* row label/img */ p->text, p->text2,
    /* nav sidebar */ p->surface,
    /* nav row */ p->text, p->surface,
    /* nav row:sel */ p->accent, p->accent,
    /* nav sel label/img */ p->accent, p->accent,
    /* nav row:hover */ p->surface2, p->text, p->text,
    /* nav row label/img */ p->text, p->text2,
    /* treeview bg */ p->surface, p->text,
    /* tree header */ p->surface2, p->text2, p->border,
    /* row */ p->text,
    /* row:even */ p->border,
    /* row:selected */ p->accent, p->text,
    /* row:selected:focus */ p->accent, p->text,
    /* row:hover */ p->accent
  );

  /* ── Mailbox sidebar ── */
  g_string_append_printf(s,
    ".mb-sidebar { background: %s; }"
    ".mb-sidebar > row { background: transparent; border: none;"
    "  border-radius: 6px; margin: 1px 4px; padding: 0; }"
    ".mb-sidebar > row:selected { background: alpha(%s, 0.15); }"
    ".mb-sidebar > row:selected .mb-name,"
    ".mb-sidebar > row:selected .mb-unread,"
    ".mb-sidebar > row:selected .mb-icon { color: %s; }"

    ".mb-icon { color: %s; }"
    ".mb-name { color: %s; font-size: 0.88em; }"
    ".mb-unread { color: %s; font-weight: 700; font-size: 0.88em; }"
    ".mb-folder { color: %s; font-size: 0.78em; font-weight: 700;"
    "  letter-spacing: 0.04em; text-transform: uppercase; }"

    ".mb-pill {"
    "  background: %s; color: %s; border-radius: 10px;"
    "  padding: 0 6px; font-size: 0.72em; font-weight: 700;"
    "  min-height: 18px;"
    "}"
    ".mb-drop-target {"
    "  background: alpha(%s, 0.2);"
    "  outline: 2px dashed %s; outline-offset: -2px;"
    "  border-radius: 6px;"
    "}",
    /* sidebar bg */ p->surface,
    /* row:selected */ p->accent,
    /* sel text/icon */ p->accent,
    /* icon */ p->text2,
    /* name */ p->text,
    /* unread name */ p->text,
    /* folder */ p->text3,
    /* pill bg */ p->accent, /* pill text */ p->hero_text,
    /* drop-target bg */ p->accent, /* drop-target outline */ p->accent
  );

  /* ── Wazoo titlebar ── */
  g_string_append_printf(s,
    ".wazoo-titlebar {"
    "  background: %s; color: %s; padding: 2px 4px; }"
    ".wazoo-titlebar label { color: %s; }"
    ".wazoo-titlebar image { color: %s; }"
    ".wazoo-titlebar button { background: transparent; border: none; }"
    ".wazoo-titlebar button:hover { background: alpha(%s, 0.12); }",
    p->surface2, p->text, p->text, p->text2, p->text
  );

  /* ── Welcome page ── */
  g_string_append_printf(s,
    ".welcome-bg { background: %s; }"
    ".welcome-hero {"
    "  background: linear-gradient(135deg, %s 0%%, %s 100%%);"
    "  padding: 28px 36px 24px; border-radius: 0 0 16px 16px;"
    "}"
    ".welcome-greeting { font-size: 1.6em; font-weight: 700; color: %s; }"
    ".welcome-account-line { font-size: 0.88em; color: %s; margin-top: 2px; }"

    ".stat-card {"
    "  background: %s; border-radius: 10px; padding: 14px 18px;"
    "  border: 1px solid %s;"
    "}"
    ".stat-number { font-size: 1.7em; font-weight: 800; }"
    ".stat-label { font-size: 0.80em; color: %s; font-weight: 500; }"
    ".stat-accent-blue  .stat-number { color: %s; }"
    ".stat-accent-green .stat-number { color: %s; }"
    ".stat-accent-amber .stat-number { color: %s; }"
    ".stat-accent-red   .stat-number { color: %s; }"

    ".wc-section-title {"
    "  font-size: 0.72em; font-weight: 700; color: %s;"
    "  letter-spacing: 0.1em;"
    "}"

    ".action-card {"
    "  background: %s; border-radius: 10px; padding: 12px 16px;"
    "  border: 1px solid %s;"
    "}"
    ".action-card:hover { background: %s; border-color: %s; }"
    ".action-icon {"
    "  border-radius: 8px; padding: 7px;"
    "  background: alpha(%s, 0.12);"
    "}"
    ".action-icon-green  { background: alpha(%s, 0.12); }"
    ".action-icon-amber  { background: alpha(%s, 0.12); }"
    ".action-icon-purple { background: alpha(%s, 0.12); }"
    ".action-icon-rose   { background: alpha(%s, 0.12); }"
    ".action-icon-cyan   { background: alpha(%s, 0.12); }"
    ".action-title { font-size: 0.92em; font-weight: 600; color: %s; }"
    ".action-desc { font-size: 0.80em; color: %s; }"

    ".shortcut-pill {"
    "  background: %s; border-radius: 8px; padding: 8px 14px;"
    "  border: 1px solid %s;"
    "}"
    ".shortcut-pill:hover { background: %s; border-color: %s; }"
    ".shortcut-key {"
    "  font-size: 0.76em; font-weight: 600; font-family: monospace;"
    "  color: %s; background: %s; border-radius: 4px; padding: 2px 6px;"
    "}"
    ".shortcut-label { font-size: 0.82em; color: %s; font-weight: 500; }"

    ".tip-bar {"
    "  background: alpha(%s, 0.10); border-radius: 10px;"
    "  padding: 12px 18px; border: 1px solid alpha(%s, 0.25);"
    "}"
    ".tip-text { font-size: 0.85em; color: %s; }",

    /* welcome-bg */ p->bg,
    /* hero grad */ p->hero_from, p->hero_to,
    /* greeting */ p->hero_text,
    /* acct line */ p->hero_sub,
    /* stat-card */ p->surface, p->border,
    /* stat-label */ p->text3,
    /* accents */ p->info, p->success, p->warning, p->danger,
    /* section */ p->text3,
    /* action-card */ p->surface, p->border,
    /* hover */ p->surface2, p->accent,
    /* icons */ p->accent, p->success, p->warning, p->purple, p->danger, p->info,
    /* title/desc */ p->text, p->text2,
    /* shortcut */ p->surface, p->border,
    /* sc hover */ p->surface2, p->accent,
    /* sc key */ p->text2, p->bg, p->text,
    /* tip */ p->warning, p->warning, p->text2
  );

  /* ── Statistics panel ── */
  g_string_append_printf(s,
    ".stat-bg { background: %s; }"
    ".stat-hero {"
    "  background: linear-gradient(135deg, %s 0%%, %s 100%%);"
    "  padding: 18px 24px 14px; border-radius: 0 0 12px 12px;"
    "}"
    ".stat-hero-title { font-size: 1.2em; font-weight: 700; color: %s; }"
    ".stat-hero-sub { font-size: 0.80em; color: %s; }"

    ".s-pill {"
    "  background: %s; border-radius: 10px; padding: 12px 16px;"
    "  border: 1px solid %s;"
    "}"
    ".s-pill-num { font-size: 1.4em; font-weight: 800; }"
    ".s-pill-lbl { font-size: 0.74em; color: %s; font-weight: 500; }"
    ".s-pill-sub { font-size: 0.68em; color: %s; }"
    ".s-blue   .s-pill-num { color: %s; }"
    ".s-green  .s-pill-num { color: %s; }"
    ".s-amber  .s-pill-num { color: %s; }"
    ".s-red    .s-pill-num { color: %s; }"
    ".s-purple .s-pill-num { color: %s; }"
    ".s-cyan   .s-pill-num { color: %s; }"
    ".s-rose   .s-pill-num { color: %s; }"

    ".s-sect { font-size: 0.74em; font-weight: 700; color: %s;"
    "  letter-spacing: 0.08em; }"

    ".s-bar-bg { background: %s; border-radius: 4px; min-height: 7px; }"
    ".s-bar-fill { background: %s; border-radius: 4px; min-height: 7px; }"
    ".s-bar-green  { background: %s; }"
    ".s-bar-amber  { background: %s; }"
    ".s-bar-red    { background: %s; }"
    ".s-bar-purple { background: %s; }"
    ".s-bar-lbl { font-size: 0.78em; color: %s; }"
    ".s-bar-val { font-size: 0.78em; color: %s; font-weight: 600; }"

    ".s-card {"
    "  background: %s; border-radius: 10px; padding: 14px 18px;"
    "  border: 1px solid %s;"
    "}"
    ".s-card-title { font-size: 0.86em; font-weight: 600; color: %s; }"

    ".s-period-on {"
    "  background: %s; color: %s; border-radius: 6px;"
    "  padding: 3px 10px; font-weight: 600; font-size: 0.78em;"
    "}"
    ".s-period-off {"
    "  background: %s; color: %s; border-radius: 6px;"
    "  padding: 3px 10px; font-size: 0.78em;"
    "}"
    ".s-junk-good { color: %s; font-weight: 700; }"
    ".s-junk-bad  { color: %s; font-weight: 700; }",

    /* stat-bg */ p->bg,
    /* hero */ p->hero_from, p->hero_to, p->hero_text, p->hero_sub,
    /* pill */ p->surface, p->border,
    /* pill lbl/sub */ p->text2, p->text3,
    /* pill colors */ p->info, p->success, p->warning, p->danger,
      p->purple, p->info, p->danger,
    /* sect */ p->text3,
    /* bar */ p->bar_bg, p->accent, p->success, p->warning, p->danger, p->purple,
    /* bar lbl/val */ p->text2, p->text,
    /* card */ p->surface, p->border, p->text,
    /* period on */ p->accent, p->hero_text,
    /* period off */ p->bar_bg, p->text2,
    /* junk */ p->success, p->danger
  );

  /* ── Filters panel ── */
  g_string_append_printf(s,
    ".filt-panel { background: %s; }"
    ".filt-hero {"
    "  background: linear-gradient(135deg, %s 0%%, %s 100%%);"
    "  padding: 14px 20px; border-radius: 0 0 10px 10px;"
    "}"
    ".filt-hero-title { font-size: 1.1em; font-weight: 700; color: %s; }"
    ".filt-hero-sub { font-size: 0.78em; color: %s; }"
    ".filt-count-pill {"
    "  background: alpha(%s, 0.20); color: %s;"
    "  border-radius: 12px; padding: 2px 10px;"
    "  font-size: 0.78em; font-weight: 600;"
    "}"
    ".filt-badge-in {"
    "  background: %s; color: %s; border-radius: 4px;"
    "  padding: 1px 6px; font-size: 0.73em; font-weight: 700;"
    "}"
    ".filt-badge-out {"
    "  background: %s; color: %s; border-radius: 4px;"
    "  padding: 1px 6px; font-size: 0.73em; font-weight: 700;"
    "}"
    ".filt-badge-man {"
    "  background: %s; color: %s; border-radius: 4px;"
    "  padding: 1px 6px; font-size: 0.73em; font-weight: 700;"
    "}"
    "list > row.filt-drop-target {"
    "  border-top: 2px solid %s;"
    "}",
    p->bg,
    p->hero_from, p->hero_to,
    p->hero_text, p->hero_sub,
    p->hero_text, p->hero_text,
    p->info, p->hero_text,
    p->warning, p->hero_text,
    p->purple, p->hero_text,
    p->accent
  );

  /* ── Address Book panel ── */
  g_string_append_printf(s,
    ".ab-panel { background: %s; }"
    ".ab-hero {"
    "  background: linear-gradient(135deg, %s 0%%, %s 100%%);"
    "  padding: 14px 20px; border-radius: 0 0 10px 10px;"
    "}"
    ".ab-hero-title { font-size: 1.1em; font-weight: 700; color: %s; }"
    ".ab-hero-sub { font-size: 0.78em; color: %s; }"
    ".ab-count-pill {"
    "  background: alpha(%s, 0.20); color: %s;"
    "  border-radius: 12px; padding: 2px 10px;"
    "  font-size: 0.78em; font-weight: 600;"
    "}"
    ".ab-entry-card {"
    "  background: %s; border-radius: 8px; padding: 8px 12px;"
    "  border: 1px solid %s;"
    "}"
    ".ab-entry-card:selected { background: %s; }"
    ".ab-name { font-weight: 600; font-size: 0.92em; color: %s; }"
    ".ab-addr { font-size: 0.80em; color: %s; }"
    ".ab-initial {"
    "  font-weight: 700; font-size: 1.0em; color: %s;"
    "  background: alpha(%s, 0.15); border-radius: 50%%;"
    "  min-width: 32px; min-height: 32px;"
    "}"
    ".ab-section-title {"
    "  font-size: 0.72em; font-weight: 700; color: %s;"
    "  letter-spacing: 0.08em; padding: 8px 12px 4px;"
    "}"

    /* Address book tab content areas */
    ".ab-tab-content { background: %s; color: %s; }"
    ".ab-tab-content label { color: %s; }"
    ".ab-tab-content entry { background: %s; color: %s;"
    "  border: 1px solid %s; }"
    ".ab-tab-content entry > text { color: %s; }"
    ".ab-tab-content textview, .ab-tab-content textview text {"
    "  background: %s; color: %s; }"
    ".ab-tab-content frame > border { border-color: %s; }",
    p->bg,
    p->hero_from, p->hero_to, p->hero_text, p->hero_sub,
    p->hero_text, p->hero_text,
    p->surface, p->border,
    p->accent,
    p->text, p->text2,
    p->accent, p->accent,
    p->text3,
    /* ab-tab-content */ p->surface, p->text,
    /* labels */ p->text,
    /* entry */ p->input_bg, p->input_text, p->input_border,
    /* entry text */ p->input_text,
    /* textview */ p->input_bg, p->input_text,
    /* frame */ p->border
  );

  /* ── Message view ── */
  g_string_append_printf(s,
    ".msg-header-box { padding: 8px 12px; background: %s; }"
    ".msg-header-name { font-weight: bold; font-size: 0.88em; color: %s;"
    "  min-width: 70px; }"
    ".msg-header-value { font-size: 0.88em; color: %s; }"
    ".msg-header-subject .msg-header-value {"
    "  font-weight: bold; font-size: 0.96em; color: %s; }"
    ".msg-separator { min-height: 1px; background: %s; margin: 4px 0; }"
    ".msg-body-view { padding: 8px 12px; background: %s; color: %s; }"
    ".msg-quote-1 { color: %s; }"
    ".msg-quote-2 { color: %s; }"
    ".msg-quote-3 { color: %s; }",
    p->surface2, p->text2, p->text, p->text, p->border,
    p->surface, p->text,
    p->info, p->success, p->purple
  );

  /* ── Settings dialog specifics ── */
  g_string_append_printf(s,
    /* dim-label used in settings form rows and group headers */
    ".dim-label { color: %s; }"

    /* boxed-list-separate used for settings group frames */
    ".boxed-list-separate { background: %s; border: 1px solid %s;"
    "  border-radius: 8px; }"
    ".boxed-list-separate > box { background: %s; }"
    ".boxed-list-separate > box > separator { background: %s; }"
    ".boxed-list-separate > box label { color: %s; }"

    /* Color buttons in labels settings */
    "colorbutton { border: 1px solid %s; border-radius: 4px; }"

    /* Headerbar if used */
    "headerbar { background: %s; color: %s;"
    "  border-bottom: 1px solid %s; }"
    "headerbar label { color: %s; }"
    "headerbar button { color: %s; }",

    /* dim-label */ p->text2,
    /* boxed-list */ p->surface, p->border,
    /* inner box */ p->surface,
    /* separator */ p->border,
    /* labels */ p->text,
    /* colorbutton */ p->border,
    /* headerbar */ p->surface, p->text, p->border, p->text, p->text2
  );

  /* ── Additional GTK widget classes ── */
  g_string_append_printf(s,
    /* boxed-list (accounts page) */
    ".boxed-list { background: %s; border: 1px solid %s; border-radius: 8px; }"
    ".boxed-list row { background: %s; color: %s; }"
    ".boxed-list row label { color: %s; }"

    /* Dockable panel */
    ".dockable-panel { background: %s; }"

    /* Viewport and scrolled windows */
    "scrolledwindow { background: %s; }"
    "viewport { background: %s; color: %s; }"

    /* Scale/slider */
    "scale trough { background: %s; }"
    "scale highlight { background: %s; }"
    "scale slider { background: %s; }"

    /* Tooltip */
    "tooltip { background: %s; color: %s;"
    "  border: 1px solid %s; border-radius: 6px; }"
    "tooltip label { color: %s; }"

    /* Link button */
    "linkbutton { color: %s; }"
    "linkbutton:hover { color: %s; }",

    /* boxed-list */ p->surface, p->border,
    /* bl row */ p->surface, p->text, p->text,
    /* dockable-panel */ p->bg,
    /* scrolledwindow */ p->bg,
    /* viewport */ p->bg, p->text,
    /* scale */ p->bar_bg, p->accent, p->surface,
    /* tooltip */ p->surface2, p->text, p->border, p->text,
    /* link */ p->accent, p->accent2
  );

  /* ── Theme toggle button ── */
  g_string_append_printf(s,
    ".theme-toggle {"
    "  border-radius: 50%%; min-width: 28px; min-height: 28px;"
    "  padding: 4px; background: alpha(%s, 0.12);"
    "  border: 1px solid %s;"
    "}"
    ".theme-toggle:hover { background: alpha(%s, 0.22); }"
    ".theme-toggle image { color: %s; }",
    p->accent, p->border, p->accent, p->accent
  );

  char *result = g_string_free(s, FALSE);
  return result;
}

/* ── Tracked editor list for live theme updates ── */
static GPtrArray *tracked_editors = NULL;

static void on_editor_destroyed(gpointer data, GObject *where_the_object_was) {
  (void)data;
  if (tracked_editors)
    g_ptr_array_remove_fast(tracked_editors, where_the_object_was);
}

static void reapply_all_editors(void) {
  if (!tracked_editors) return;
  for (guint i = 0; i < tracked_editors->len; i++)
    theme_apply_to_editor(g_ptr_array_index(tracked_editors, i));
}

/* ═══════════════════════════════════════════════════════════════════ */

void theme_init(GtkWidget *rw) {
  tracked_editors = g_ptr_array_new();
  root_window = rw;
  theme_provider = gtk_css_provider_new();
  gtk_style_context_add_provider_for_display(
      gdk_display_get_default(), GTK_STYLE_PROVIDER(theme_provider),
      GTK_STYLE_PROVIDER_PRIORITY_APPLICATION + 1);

  /* Load saved theme from prefs */
  int saved = prefs_get_int(PREFS_GROUP_THEME, PREFS_KEY_THEME, 0);
  if (saved < 0 || saved >= THEME_COUNT) saved = 0;
  theme_apply((GeudoraTheme)saved);
}

void theme_apply(GeudoraTheme theme) {
  if (theme < 0 || theme >= THEME_COUNT) theme = THEME_LIGHT;
  current_theme = theme;

  char *css = generate_css(&palettes[theme]);
  gtk_css_provider_load_from_string(theme_provider, css);
  g_free(css);

  /* Persist to prefs */
  prefs_set_int(PREFS_GROUP_THEME, PREFS_KEY_THEME, (int)theme);

  /* Set GTK dark preference for Dark/Nord/Monokai */
  GtkSettings *settings = gtk_settings_get_default();
  gboolean dark = (theme == THEME_DARK || theme == THEME_NORD || theme == THEME_MONOKAI);
  g_object_set(settings, "gtk-application-prefer-dark-theme", dark, NULL);

  /* Re-apply theme colors to all tracked gEditCtrl widgets */
  reapply_all_editors();
}

void theme_cycle(void) {
  GeudoraTheme next = (current_theme + 1) % THEME_COUNT;
  theme_apply(next);
}

static gboolean parse_color(const char *spec, GdkRGBA *out) {
  return gdk_rgba_parse(out, spec);
}

void theme_apply_to_editor(GtkWidget *editor_ctrl) {
  if (!editor_ctrl) return;
  /* Track this editor for live theme updates */
  if (tracked_editors) {
    gboolean found = FALSE;
    for (guint i = 0; i < tracked_editors->len; i++) {
      if (g_ptr_array_index(tracked_editors, i) == editor_ctrl) {
        found = TRUE;
        break;
      }
    }
    if (!found) {
      g_ptr_array_add(tracked_editors, editor_ctrl);
      g_object_weak_ref(G_OBJECT(editor_ctrl), on_editor_destroyed, NULL);
    }
  }
  const ThemePalette *p = &palettes[current_theme];
  GdkRGBA bg, text, caret, sel_bg;
  if (!parse_color(p->surface, &bg)) bg = (GdkRGBA){1, 1, 1, 1};
  if (!parse_color(p->text, &text)) text = (GdkRGBA){0, 0, 0, 1};
  caret = text; /* caret same as text color */
  if (!parse_color(p->accent, &sel_bg)) sel_bg = (GdkRGBA){0.3, 0.4, 0.6, 1};
  geditctrl_set_theme_colors(editor_ctrl, &bg, &text, &caret, &sel_bg);
}
