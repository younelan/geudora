/* Copyright (c) 2017, Computer History Museum
All rights reserved. (BSD license — see original for full text)
Statistics panel — ported from Mac Carbon to GTK4.
Opens as a tab in the main notebook. Shows all original Eudora stats. */

#include "statmng.h"
#include "statwin.h"
/* junk.h removed — macmbx_junk handles junk */
#include <gtk/gtk.h>
#include <time.h>
#include <string.h>

/* CSS is now provided by theme.c */
static GtkWidget *stat_body_box = NULL;
static StatTimePeriod cur_period = kStatDay;
static void on_period_clicked(GtkButton *btn, gpointer data);

static void fmt_dur(long s, char *b, size_t n) {
  if (s < 60) snprintf(b, n, "%lds", s);
  else if (s < 3600) snprintf(b, n, "%ldm %lds", s/60, s%60);
  else if (s < 86400) snprintf(b, n, "%ldh %ldm", s/3600, (s%3600)/60);
  else snprintf(b, n, "%ldd %ldh", s/86400, (s%86400)/3600);
}

/* ── Widgets ── */
static GtkWidget *s_pill(const char *num, const char *lbl,
                          const char *sub, const char *accent) {
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
  gtk_widget_add_css_class(box, "s-pill");
  if (accent) gtk_widget_add_css_class(box, accent);
  gtk_widget_set_hexpand(box, TRUE);
  GtkWidget *n = gtk_label_new(num);
  gtk_widget_add_css_class(n, "s-pill-num");
  gtk_label_set_xalign(GTK_LABEL(n), 0);
  gtk_box_append(GTK_BOX(box), n);
  GtkWidget *l = gtk_label_new(lbl);
  gtk_widget_add_css_class(l, "s-pill-lbl");
  gtk_label_set_xalign(GTK_LABEL(l), 0);
  gtk_box_append(GTK_BOX(box), l);
  if (sub && sub[0]) {
    GtkWidget *s = gtk_label_new(sub);
    gtk_widget_add_css_class(s, "s-pill-sub");
    gtk_label_set_xalign(GTK_LABEL(s), 0);
    gtk_box_append(GTK_BOX(box), s);
  }
  return box;
}

static GtkWidget *s_bar(const char *label, long val, long mx,
                         const char *fill_cls) {
  GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_widget_set_margin_top(row, 1);
  gtk_widget_set_margin_bottom(row, 1);
  GtkWidget *lb = gtk_label_new(label);
  gtk_widget_add_css_class(lb, "s-bar-lbl");
  gtk_widget_set_size_request(lb, 90, -1);
  gtk_label_set_xalign(GTK_LABEL(lb), 1.0);
  gtk_box_append(GTK_BOX(row), lb);
  GtkWidget *bg = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_widget_add_css_class(bg, "s-bar-bg");
  gtk_widget_set_hexpand(bg, TRUE);
  double frac = (mx > 0) ? (double)val / mx : 0;
  if (frac > 1.0) frac = 1.0;
  GtkWidget *fill = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_widget_add_css_class(fill, "s-bar-fill");
  if (fill_cls) gtk_widget_add_css_class(fill, fill_cls);
  int pct = (int)(frac * 100);
  if (pct > 0) gtk_widget_set_size_request(fill, (int)(2.5 * pct), -1);
  gtk_box_append(GTK_BOX(bg), fill);
  gtk_box_append(GTK_BOX(row), bg);
  char vb[32]; snprintf(vb, sizeof(vb), "%ld", val);
  GtkWidget *v = gtk_label_new(vb);
  gtk_widget_add_css_class(v, "s-bar-val");
  gtk_widget_set_size_request(v, 45, -1);
  gtk_label_set_xalign(GTK_LABEL(v), 1.0);
  gtk_box_append(GTK_BOX(row), v);
  return row;
}

static GtkWidget *s_sect(const char *t) {
  GtkWidget *l = gtk_label_new(t);
  gtk_widget_add_css_class(l, "s-sect");
  gtk_label_set_xalign(GTK_LABEL(l), 0);
  gtk_widget_set_margin_top(l, 14);
  gtk_widget_set_margin_bottom(l, 5);
  return l;
}

static GtkWidget *s_card_title(const char *t) {
  GtkWidget *l = gtk_label_new(t);
  gtk_widget_add_css_class(l, "s-card-title");
  gtk_label_set_xalign(GTK_LABEL(l), 0);
  gtk_widget_set_margin_bottom(l, 4);
  return l;
}

/* ── Build stats content ── */
static void build_stats(GtkWidget *c) {
  const char *last_n[] = {"yesterday", "last week", "last month", "last year"};
  StatTimePeriod p = cur_period;

  /* ── Period selector ── */
  const char *plbls[] = {"Day", "Week", "Month", "Year"};
  GtkWidget *prow = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
  gtk_widget_set_halign(prow, GTK_ALIGN_CENTER);
  gtk_widget_set_margin_top(prow, 6);
  gtk_widget_set_margin_bottom(prow, 10);
  for (int i = 0; i < 4; i++) {
    GtkWidget *btn = gtk_button_new_with_label(plbls[i]);
    gtk_button_set_has_frame(GTK_BUTTON(btn), FALSE);
    gtk_widget_add_css_class(btn, (i == p) ? "s-period-on" : "s-period-off");
    g_signal_connect(btn, "clicked", G_CALLBACK(on_period_clicked),
                     GINT_TO_POINTER(i));
    gtk_box_append(GTK_BOX(prow), btn);
  }
  gtk_box_append(GTK_BOX(c), prow);

  /* ── Data ── */
  long rc = StatGetCurrentPeriod(kStatReceivedMail, p);
  long rl = StatGetLastPeriod(kStatReceivedMail, p);
  long sc = StatGetCurrentPeriod(kStatSentMail, p);
  long sl = StatGetLastPeriod(kStatSentMail, p);
  long rt = StatGetTotal(kStatReceivedMail);
  long st = StatGetTotal(kStatSentMail);
  long rdt = StatGetTotal(kStatReadMsg);
  long ft = StatGetTotal(kStatFaceTime);
  long jt = StatGetTotal(kStatScoredJunk);
  long njt = StatGetTotal(kStatScoredNotJunk);

  /* ── Top pills ── */
  char s1[32], s2[32], s3[32], s4[32], s5[32], s6[32];
  char u1[64], u2[64], u3[64], u4[64], u5[64], u6[64];
  snprintf(s1, 32, "%ld", rc);
  snprintf(u1, 64, "%ld %s / %ld total", rl, last_n[p], rt);
  snprintf(s2, 32, "%ld", sc);
  snprintf(u2, 64, "%ld %s / %ld total", sl, last_n[p], st);
  snprintf(s3, 32, "%ld", rdt);
  long rpct = rt > 0 ? (100 * rdt / rt) : 0;
  snprintf(u3, 64, "%ld%% of received", rpct);
  fmt_dur(ft, s4, 32);
  snprintf(u4, 64, "face time total");
  long jtotal = jt + njt;
  snprintf(s5, 32, "%ld", jt);
  long jpct = jtotal > 0 ? (100 * jt / jtotal) : 0;
  snprintf(u5, 64, "%ld%% of scored (%ld total)", jpct, jtotal);
  snprintf(s6, 32, "%ld", njt);
  snprintf(u6, 64, "scored not junk");

  GtkWidget *pills = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
  gtk_box_append(GTK_BOX(pills), s_pill(s1, "Received", u1, "s-blue"));
  gtk_box_append(GTK_BOX(pills), s_pill(s2, "Sent", u2, "s-green"));
  gtk_box_append(GTK_BOX(pills), s_pill(s3, "Read", u3, "s-purple"));
  gtk_box_append(GTK_BOX(pills), s_pill(s4, "Usage", u4, "s-cyan"));
  gtk_box_append(GTK_BOX(pills), s_pill(s5, "Junk", u5, "s-red"));
  gtk_box_append(GTK_BOX(pills), s_pill(s6, "Not Junk", u6, "s-green"));
  gtk_box_append(GTK_BOX(c), pills);

  /* ── Three columns ── */
  GtkWidget *cols = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 14);
  gtk_widget_set_margin_top(cols, 4);

  /* ── Col 1: Mail breakdown ── */
  GtkWidget *c1 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_set_hexpand(c1, TRUE);

  gtk_box_append(GTK_BOX(c1), s_sect("RECEIVED BREAKDOWN"));
  GtkWidget *rc1 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_add_css_class(rc1, "s-card");
  if (p == kStatDay) {
    long hrs[24]; StatGetHourlyData(kStatReceivedMail, hrs);
    long mx = 0; for (int i = 0; i < 24; i++) if (hrs[i] > mx) mx = hrs[i];
    if (mx == 0) mx = 1;
    for (int i = 0; i < 24; i++) {
      if (hrs[i] > 0 || (i >= 7 && i <= 21)) {
        char lb[8]; snprintf(lb, 8, "%02d:00", i);
        gtk_box_append(GTK_BOX(rc1), s_bar(lb, hrs[i], mx, NULL));
      }
    }
  } else if (p == kStatWeek) {
    const char *dn[] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
    long wk[7]; StatGetWeeklyData(kStatReceivedMail, wk);
    long mx = 0; for (int i = 0; i < 7; i++) if (wk[i] > mx) mx = wk[i];
    if (mx == 0) mx = 1;
    for (int i = 0; i < 7; i++)
      gtk_box_append(GTK_BOX(rc1), s_bar(dn[i], wk[i], mx, NULL));
  } else {
    char b1[80], b2[80];
    snprintf(b1, 80, "This period: %ld received", rc);
    snprintf(b2, 80, "Last period: %ld received", rl);
    GtkWidget *l1 = gtk_label_new(b1); gtk_widget_add_css_class(l1,"s-bar-lbl");
    gtk_label_set_xalign(GTK_LABEL(l1),0); gtk_box_append(GTK_BOX(rc1), l1);
    GtkWidget *l2 = gtk_label_new(b2); gtk_widget_add_css_class(l2,"s-bar-lbl");
    gtk_label_set_xalign(GTK_LABEL(l2),0); gtk_box_append(GTK_BOX(rc1), l2);
  }
  if (rc == 0 && rl == 0) {
    GtkWidget *e = gtk_label_new("No activity recorded yet");
    gtk_widget_add_css_class(e,"s-pill-sub");
    gtk_widget_set_margin_top(e,6); gtk_widget_set_margin_bottom(e,6);
    gtk_box_append(GTK_BOX(rc1), e);
  }
  gtk_box_append(GTK_BOX(c1), rc1);
  gtk_box_append(GTK_BOX(cols), c1);

  /* ── Col 2: Activity ── */
  GtkWidget *c2 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_set_hexpand(c2, TRUE);

  gtk_box_append(GTK_BOX(c2), s_sect("MESSAGE ACTIVITY"));
  GtkWidget *ac = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_add_css_class(ac, "s-card");

  long fwd = StatGetTotal(kStatForwardMsg);
  long rply = StatGetTotal(kStatReplyMsg);
  long rdir = StatGetTotal(kStatRedirectMsg);
  long ratt = StatGetTotal(kStatReceivedAttach);
  long satt = StatGetTotal(kStatSentAttach);
  long mx = rt; if (st > mx) mx = st; if (mx == 0) mx = 1;

  gtk_box_append(GTK_BOX(ac), s_bar("Received", rt, mx, NULL));
  gtk_box_append(GTK_BOX(ac), s_bar("Sent", st, mx, "s-bar-green"));
  gtk_box_append(GTK_BOX(ac), s_bar("Read", rdt, mx, "s-bar-amber"));
  gtk_box_append(GTK_BOX(ac), s_bar("Replied", rply, mx, "s-bar-purple"));
  gtk_box_append(GTK_BOX(ac), s_bar("Forwarded", fwd, mx, "s-bar-green"));
  gtk_box_append(GTK_BOX(ac), s_bar("Redirected", rdir, mx, NULL));
  gtk_box_append(GTK_BOX(ac), s_bar("Attach recv", ratt, mx, "s-bar-amber"));
  gtk_box_append(GTK_BOX(ac), s_bar("Attach sent", satt, mx, "s-bar-green"));
  gtk_box_append(GTK_BOX(c2), ac);

  /* Face time */
  gtk_box_append(GTK_BOX(c2), s_sect("FACE TIME"));
  GtkWidget *fc = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_add_css_class(fc, "s-card");
  long fr = StatGetTotal(kStatFaceTimeRead);
  long fw = StatGetTotal(kStatFaceTimeCompose);
  long fo = StatGetTotal(kStatFaceTimeOther);
  long fm = fr; if (fw > fm) fm = fw; if (fo > fm) fm = fo; if (fm == 0) fm = 1;
  char d1[32], d2[32], d3[32];
  fmt_dur(fr, d1, 32); fmt_dur(fw, d2, 32); fmt_dur(fo, d3, 32);
  gtk_box_append(GTK_BOX(fc), s_bar("Reading", fr, fm, NULL));
  gtk_box_append(GTK_BOX(fc), s_bar("Composing", fw, fm, "s-bar-green"));
  gtk_box_append(GTK_BOX(fc), s_bar("Other", fo, fm, "s-bar-amber"));
  /* percentages */
  long ftot = fr + fw + fo;
  if (ftot > 0) {
    char pctbuf[96];
    snprintf(pctbuf, 96, "Reading %ld%%   Composing %ld%%   Other %ld%%",
             100*fr/ftot, 100*fw/ftot, 100*fo/ftot);
    GtkWidget *pl = gtk_label_new(pctbuf);
    gtk_widget_add_css_class(pl, "s-pill-sub");
    gtk_widget_set_margin_top(pl, 4);
    gtk_box_append(GTK_BOX(fc), pl);
  }
  gtk_box_append(GTK_BOX(c2), fc);
  gtk_box_append(GTK_BOX(cols), c2);

  /* ── Col 3: Junk ── */
  GtkWidget *c3 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_set_hexpand(c3, TRUE);

  gtk_box_append(GTK_BOX(c3), s_sect("JUNK MAIL"));
  GtkWidget *jc = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_add_css_class(jc, "s-card");

  long wl  = StatGetTotal(kStatWhiteList);
  long fp  = StatGetTotal(kStatFalsePositives);
  long fn  = StatGetTotal(kStatFalseNegatives);
  long fwl = StatGetTotal(kStatFalseWhiteList);

  long jmx = jt; if (njt > jmx) jmx = njt; if (jmx == 0) jmx = 1;
  gtk_box_append(GTK_BOX(jc), s_bar("Scored Junk", jt, jmx, "s-bar-red"));
  gtk_box_append(GTK_BOX(jc), s_bar("Scored OK", njt, jmx, "s-bar-green"));
  gtk_box_append(GTK_BOX(jc), s_bar("Whitelist", wl, jmx, NULL));
  gtk_box_append(GTK_BOX(jc), s_bar("false Pos", fp, jmx, "s-bar-amber"));
  gtk_box_append(GTK_BOX(jc), s_bar("false Neg", fn, jmx, "s-bar-red"));
  if (fwl > 0)
    gtk_box_append(GTK_BOX(jc), s_bar("false WL", fwl, jmx, "s-bar-amber"));

  /* Accuracy */
  if (jtotal > 0) {
    long correct = jtotal - fp - fn;
    long accuracy = (jtotal > fwl) ? (100 * correct / (jtotal - fwl)) : 0;
    if (accuracy < 0) accuracy = 0;
    char abuf[64];
    snprintf(abuf, 64, "Accuracy: %ld%%", accuracy);
    GtkWidget *al = gtk_label_new(abuf);
    gtk_widget_add_css_class(al, accuracy >= 90 ? "s-junk-good" : "s-junk-bad");
    gtk_widget_set_margin_top(al, 6);
    gtk_label_set_xalign(GTK_LABEL(al), 0);
    gtk_box_append(GTK_BOX(jc), al);

    char detail[128];
    snprintf(detail, 128, "%ld scored / %ld false pos / %ld false neg",
             jtotal, fp, fn);
    GtkWidget *dl = gtk_label_new(detail);
    gtk_widget_add_css_class(dl, "s-pill-sub");
    gtk_label_set_xalign(GTK_LABEL(dl), 0);
    gtk_box_append(GTK_BOX(jc), dl);
  } else {
    GtkWidget *nj = gtk_label_new("No junk scoring data yet");
    gtk_widget_add_css_class(nj, "s-pill-sub");
    gtk_widget_set_margin_top(nj, 4);
    gtk_box_append(GTK_BOX(jc), nj);
  }
  gtk_box_append(GTK_BOX(c3), jc);

  /* Collection period */
  gtk_box_append(GTK_BOX(c3), s_sect("COLLECTION PERIOD"));
  GtkWidget *cp = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
  gtk_widget_add_css_class(cp, "s-card");
  long start = StatGetStartTime();
  if (start > 0) {
    time_t t = (time_t)start;
    struct tm *tm = localtime(&t);
    char since[96];
    if (tm) strftime(since, 96, "Since %B %d, %Y", tm);
    else snprintf(since, 96, "Since epoch %ld", start);
    GtkWidget *sl2 = gtk_label_new(since);
    gtk_widget_add_css_class(sl2, "s-bar-lbl");
    gtk_label_set_xalign(GTK_LABEL(sl2), 0);
    gtk_box_append(GTK_BOX(cp), sl2);
    /* Elapsed */
    time_t now = time(NULL);
    long elapsed = (long)(now - t);
    char ebuf[64]; fmt_dur(elapsed, ebuf, 64);
    char eline[96]; snprintf(eline, 96, "Elapsed: %s", ebuf);
    GtkWidget *el = gtk_label_new(eline);
    gtk_widget_add_css_class(el, "s-pill-sub");
    gtk_label_set_xalign(GTK_LABEL(el), 0);
    gtk_box_append(GTK_BOX(cp), el);
  } else {
    GtkWidget *ns = gtk_label_new("No data collected yet");
    gtk_widget_add_css_class(ns, "s-pill-sub");
    gtk_box_append(GTK_BOX(cp), ns);
  }
  gtk_box_append(GTK_BOX(c3), cp);

  gtk_box_append(GTK_BOX(cols), c3);
  gtk_box_append(GTK_BOX(c), cols);
}

/* ── Period button clicked ── */
static void on_period_clicked(GtkButton *btn, gpointer data) {
  (void)btn;
  cur_period = GPOINTER_TO_INT(data);
  RedisplayStats();
}

/************************************************************************
 * RedisplayStats - rebuild the statistics display
 ************************************************************************/
void RedisplayStats(void) {
  if (!stat_body_box) return;
  GtkWidget *child;
  while ((child = gtk_widget_get_first_child(stat_body_box)) != NULL)
    gtk_box_remove(GTK_BOX(stat_body_box), child);
  build_stats(stat_body_box);
}

/************************************************************************
 * CreateStatisticsPanel - build stats as an embeddable panel widget
 ************************************************************************/
GtkWidget *CreateStatisticsPanel(void) {

  GtkWidget *outer = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_add_css_class(outer, "stat-bg");
  gtk_widget_set_hexpand(outer, TRUE);
  gtk_widget_set_vexpand(outer, TRUE);

  /* Hero */
  GtkWidget *hero = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
  gtk_widget_add_css_class(hero, "stat-hero");
  GtkWidget *title = gtk_label_new("Email Statistics");
  gtk_widget_add_css_class(title, "stat-hero-title");
  gtk_label_set_xalign(GTK_LABEL(title), 0);
  gtk_box_append(GTK_BOX(hero), title);
  GtkWidget *sub = gtk_label_new("Your email activity at a glance");
  gtk_widget_add_css_class(sub, "stat-hero-sub");
  gtk_label_set_xalign(GTK_LABEL(sub), 0);
  gtk_box_append(GTK_BOX(hero), sub);
  gtk_box_append(GTK_BOX(outer), hero);

  /* Scrollable body */
  GtkWidget *scroll = gtk_scrolled_window_new();
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
                                  GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_widget_set_vexpand(scroll, TRUE);

  stat_body_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_set_margin_start(stat_body_box, 24);
  gtk_widget_set_margin_end(stat_body_box, 24);
  gtk_widget_set_margin_top(stat_body_box, 6);
  gtk_widget_set_margin_bottom(stat_body_box, 24);

  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), stat_body_box);
  gtk_box_append(GTK_BOX(outer), scroll);

  build_stats(stat_body_box);

  return outer;
}

/************************************************************************
 * OpenStatWin - legacy entry point, now opens as panel tab
 ************************************************************************/
void OpenStatWin(void) {
  /* Called from main_eudora action handler via panel tab system */
}
