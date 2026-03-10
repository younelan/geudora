/*
 * GTK4 Settings Dialog for gEudora
 * Multi-section preferences dialog similar to Mac Eudora
 */

#include "gtk_settings.h"
#include "gtk_prefs.h"
#include <stdlib.h>
#include <string.h>

/* Extended settings dialog structure with widget references */
typedef struct {
    /* Getting Started widgets */
    GtkWidget *gs_user_entry;
    GtkWidget *gs_server_entry;
    GtkWidget *gs_realname_entry;
    GtkWidget *gs_smtp_entry;
    GtkWidget *gs_email_entry;
    GtkWidget *gs_default_mailer;
    
    /* Checking Mail widgets */
    GtkWidget *cm_user_entry;
    GtkWidget *cm_server_entry;
    GtkWidget *cm_pop_radio;
    GtkWidget *cm_imap_radio;
    GtkWidget *cm_pwd_radio;
    GtkWidget *cm_kerb_radio;
    GtkWidget *cm_apop_radio;
    GtkWidget *cm_overlap;
    GtkWidget *cm_check_cb;
    GtkWidget *cm_interval_spin;
    GtkWidget *cm_battery;
    GtkWidget *cm_send_on_check;
    GtkWidget *cm_leave_cb;
    GtkWidget *cm_leave_spin;
    GtkWidget *cm_delete_trash;
    GtkWidget *cm_skip_cb;
    GtkWidget *cm_skip_spin;
    
    /* Sending Mail widgets */
    GtkWidget *sm_email_entry;
    GtkWidget *sm_domain_entry;
    GtkWidget *sm_smtp_entry;
    GtkWidget *sm_auto_send;
    GtkWidget *sm_keep_copy;
    GtkWidget *sm_wrap_text;
    GtkWidget *sm_include_sig;
    
    /* Display widgets */
    GtkWidget *disp_show_preview;
    GtkWidget *disp_show_toolbars;
    GtkWidget *disp_zoom_on_open;
    
    /* Advanced widgets */
    GtkWidget *adv_case_sensitive;
    GtkWidget *adv_offline_mode;
    GtkWidget *adv_expert_mode;
    
    /* Security widgets */
    GtkWidget *sec_save_password;
    GtkWidget *sec_use_ssl;
    
    /* Fonts widgets */
    GtkWidget *fonts_message_font;
    GtkWidget *fonts_message_size;
    GtkWidget *fonts_compose_font;
    GtkWidget *fonts_compose_size;
    
    /* Accounts widgets */
    GtkWidget *acct_name_entry;
    GtkWidget *acct_list;
    GtkWidget *acct_add_button;
    GtkWidget *acct_remove_button;
    
    /* Account storage */
    PrefsAccount accounts[10];
    int num_accounts;
    int current_account;  /* Currently selected account index */
} DialogWidgets;

/* Forward declarations */
static void on_section_selected(GtkListBox *box, GtkListBoxRow *row, gpointer user_data);
static GtkWidget* create_section_widget(SettingsSection section, SettingsDialog *dialog, DialogWidgets *widgets);
static void sync_widgets_to_settings(DialogWidgets *widgets, AppSettings *settings);
static void on_ok_clicked(GtkWidget *widget, gpointer user_data);
static void on_cancel_clicked(GtkWidget *widget, gpointer user_data);
static void on_add_account_clicked(GtkWidget *widget, gpointer user_data);
static void on_add_account_ok(GtkWidget *widget, gpointer user_data);
static void on_edit_account_clicked(GtkWidget *widget, gpointer user_data);
static void on_remove_account_clicked(GtkWidget *widget, gpointer user_data);
static void on_account_selected(GtkListBox *box, GtkListBoxRow *row, gpointer user_data);
static void on_edit_account_save(GtkWidget *widget, gpointer user_data);

/* Sync all widget values to settings structure */
static void sync_widgets_to_settings(DialogWidgets *widgets, AppSettings *settings)
{
    if (!widgets || !settings) return;
    
    /* Checking Mail is the source of truth for user/server */
    if (widgets->cm_user_entry) {
        strncpy(settings->pop_username, gtk_editable_get_text(GTK_EDITABLE(widgets->cm_user_entry)), 
                sizeof(settings->pop_username) - 1);
    }
    if (widgets->cm_server_entry) {
        strncpy(settings->pop_server, gtk_editable_get_text(GTK_EDITABLE(widgets->cm_server_entry)), 
                sizeof(settings->pop_server) - 1);
    }
    
    /* Sending Mail is the source of truth for email/smtp/domain */
    if (widgets->sm_email_entry) {
        strncpy(settings->email_address, gtk_editable_get_text(GTK_EDITABLE(widgets->sm_email_entry)), 
                sizeof(settings->email_address) - 1);
    }
    if (widgets->sm_smtp_entry) {
        strncpy(settings->smtp_server, gtk_editable_get_text(GTK_EDITABLE(widgets->sm_smtp_entry)), 
                sizeof(settings->smtp_server) - 1);
    }
    if (widgets->sm_domain_entry) {
        strncpy(settings->default_domain, gtk_editable_get_text(GTK_EDITABLE(widgets->sm_domain_entry)), 
                sizeof(settings->default_domain) - 1);
    }
    
    /* Real Name from Sending Mail */
    if (widgets->gs_realname_entry) {
        strncpy(settings->real_name, gtk_editable_get_text(GTK_EDITABLE(widgets->gs_realname_entry)), 
                sizeof(settings->real_name) - 1);
    }
    
    /* Checking Mail protocol and auth */
    if (widgets->cm_pop_radio) {
        settings->use_pop = gtk_check_button_get_active(GTK_CHECK_BUTTON(widgets->cm_pop_radio));
    }
    if (widgets->cm_imap_radio) {
        settings->use_imap = gtk_check_button_get_active(GTK_CHECK_BUTTON(widgets->cm_imap_radio));
    }
    if (widgets->cm_pwd_radio) {
        settings->use_passwords = gtk_check_button_get_active(GTK_CHECK_BUTTON(widgets->cm_pwd_radio));
    }
    if (widgets->cm_kerb_radio) {
        settings->use_kerberos = gtk_check_button_get_active(GTK_CHECK_BUTTON(widgets->cm_kerb_radio));
    }
    if (widgets->cm_apop_radio) {
        settings->use_apop = gtk_check_button_get_active(GTK_CHECK_BUTTON(widgets->cm_apop_radio));
    }
    if (widgets->cm_overlap) {
        settings->overlap_commands = gtk_check_button_get_active(GTK_CHECK_BUTTON(widgets->cm_overlap));
    }
    if (widgets->cm_interval_spin) {
        settings->check_interval = (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(widgets->cm_interval_spin));
    }
    if (widgets->cm_battery) {
        settings->check_battery = gtk_check_button_get_active(GTK_CHECK_BUTTON(widgets->cm_battery));
    }
    if (widgets->cm_send_on_check) {
        settings->send_on_check = gtk_check_button_get_active(GTK_CHECK_BUTTON(widgets->cm_send_on_check));
    }
    if (widgets->cm_leave_cb) {
        settings->leave_on_server = gtk_check_button_get_active(GTK_CHECK_BUTTON(widgets->cm_leave_cb));
    }
    if (widgets->cm_leave_spin) {
        settings->leave_days = (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(widgets->cm_leave_spin));
    }
    if (widgets->cm_delete_trash) {
        settings->delete_from_trash = gtk_check_button_get_active(GTK_CHECK_BUTTON(widgets->cm_delete_trash));
    }
    if (widgets->cm_skip_cb) {
        if (gtk_check_button_get_active(GTK_CHECK_BUTTON(widgets->cm_skip_cb))) {
            settings->skip_size_kb = (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(widgets->cm_skip_spin));
        } else {
            settings->skip_size_kb = 0;
        }
    }
    
    /* Sending Mail options */
    if (widgets->sm_auto_send) {
        settings->send_immediately = gtk_check_button_get_active(GTK_CHECK_BUTTON(widgets->sm_auto_send));
    }
    if (widgets->sm_keep_copy) {
        settings->keep_sent_copy = gtk_check_button_get_active(GTK_CHECK_BUTTON(widgets->sm_keep_copy));
    }
    if (widgets->sm_wrap_text) {
        settings->wrap_outgoing = gtk_check_button_get_active(GTK_CHECK_BUTTON(widgets->sm_wrap_text));
    }
    if (widgets->sm_include_sig) {
        settings->include_signature = gtk_check_button_get_active(GTK_CHECK_BUTTON(widgets->sm_include_sig));
    }
    
    /* Display */
    if (widgets->disp_show_preview) {
        settings->show_preview = gtk_check_button_get_active(GTK_CHECK_BUTTON(widgets->disp_show_preview));
    }
    if (widgets->disp_show_toolbars) {
        settings->show_toolbars = gtk_check_button_get_active(GTK_CHECK_BUTTON(widgets->disp_show_toolbars));
    }
    if (widgets->disp_zoom_on_open) {
        settings->zoom_on_open = gtk_check_button_get_active(GTK_CHECK_BUTTON(widgets->disp_zoom_on_open));
    }
    
    /* Advanced */
    if (widgets->adv_case_sensitive) {
        settings->case_sensitive_search = gtk_check_button_get_active(GTK_CHECK_BUTTON(widgets->adv_case_sensitive));
    }
    if (widgets->adv_offline_mode) {
        settings->offline_mode = gtk_check_button_get_active(GTK_CHECK_BUTTON(widgets->adv_offline_mode));
    }
    if (widgets->adv_expert_mode) {
        settings->expert_mode = gtk_check_button_get_active(GTK_CHECK_BUTTON(widgets->adv_expert_mode));
    }
    
    /* Security */
    if (widgets->sec_save_password) {
        settings->save_password = gtk_check_button_get_active(GTK_CHECK_BUTTON(widgets->sec_save_password));
    }
    if (widgets->sec_use_ssl) {
        settings->use_ssl = gtk_check_button_get_active(GTK_CHECK_BUTTON(widgets->sec_use_ssl));
    }
    
    /* Fonts */
    if (widgets->fonts_message_font) {
        PangoFontDescription *font_desc = gtk_font_dialog_button_get_font_desc(GTK_FONT_DIALOG_BUTTON(widgets->fonts_message_font));
        if (font_desc) {
            gchar *font_str = pango_font_description_to_string(font_desc);
            if (font_str) {
                strncpy(settings->message_font, font_str, sizeof(settings->message_font) - 1);
                g_free(font_str);
            }
        }
    }
    if (widgets->fonts_message_size) {
        settings->message_font_size = (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(widgets->fonts_message_size));
    }
    if (widgets->fonts_compose_font) {
        PangoFontDescription *font_desc = gtk_font_dialog_button_get_font_desc(GTK_FONT_DIALOG_BUTTON(widgets->fonts_compose_font));
        if (font_desc) {
            gchar *font_str = pango_font_description_to_string(font_desc);
            if (font_str) {
                strncpy(settings->compose_font, font_str, sizeof(settings->compose_font) - 1);
                g_free(font_str);
            }
        }
    }
    if (widgets->fonts_compose_size) {
        settings->compose_font_size = (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(widgets->fonts_compose_size));
    }
    
    /* Accounts */
    if (widgets->acct_name_entry) {
        const char *text = gtk_editable_get_text(GTK_EDITABLE(widgets->acct_name_entry));
        strncpy(settings->account_name, text, sizeof(settings->account_name) - 1);
    }
    /* Note: acct_list, acct_add_button, acct_remove_button are UI only, no sync needed */
}

/* Create a settings section widget */
static GtkWidget* create_section_widget(SettingsSection section, SettingsDialog *dialog, DialogWidgets *widgets)
{
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_set_margin_start(box, 20);
    gtk_widget_set_margin_end(box, 20);
    gtk_widget_set_margin_top(box, 20);
    gtk_widget_set_margin_bottom(box, 20);
    
    GtkWidget *title = NULL;
    AppSettings *settings = dialog->settings;
    
    switch (section) {
        case SETTINGS_GETTING_STARTED:
            title = gtk_label_new("Getting Started");
            gtk_box_append(GTK_BOX(box), title);
            
            GtkWidget *gs_check_title = gtk_label_new("Checking Mail");
            gtk_widget_add_css_class(gs_check_title, "subtitle");
            gtk_box_append(GTK_BOX(box), gs_check_title);
            
            GtkWidget *gs_user_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
            gtk_box_append(GTK_BOX(gs_user_box), gtk_label_new("User Name:"));
            widgets->gs_user_entry = gtk_entry_new();
            gtk_editable_set_text(GTK_EDITABLE(widgets->gs_user_entry), settings->pop_username);
            gtk_box_append(GTK_BOX(gs_user_box), widgets->gs_user_entry);
            gtk_box_append(GTK_BOX(box), gs_user_box);
            
            GtkWidget *gs_server_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
            gtk_box_append(GTK_BOX(gs_server_box), gtk_label_new("Mail Server:"));
            widgets->gs_server_entry = gtk_entry_new();
            gtk_editable_set_text(GTK_EDITABLE(widgets->gs_server_entry), settings->pop_server);
            gtk_box_append(GTK_BOX(gs_server_box), widgets->gs_server_entry);
            gtk_box_append(GTK_BOX(box), gs_server_box);
            
            GtkWidget *gs_send_title = gtk_label_new("Sending Mail");
            gtk_widget_add_css_class(gs_send_title, "subtitle");
            gtk_box_append(GTK_BOX(box), gs_send_title);
            
            GtkWidget *gs_realname_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
            gtk_box_append(GTK_BOX(gs_realname_box), gtk_label_new("Real Name:"));
            widgets->gs_realname_entry = gtk_entry_new();
            gtk_editable_set_text(GTK_EDITABLE(widgets->gs_realname_entry), settings->real_name);
            gtk_box_append(GTK_BOX(gs_realname_box), widgets->gs_realname_entry);
            gtk_box_append(GTK_BOX(box), gs_realname_box);
            
            GtkWidget *gs_smtp_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
            gtk_box_append(GTK_BOX(gs_smtp_box), gtk_label_new("SMTP Server:"));
            widgets->gs_smtp_entry = gtk_entry_new();
            gtk_editable_set_text(GTK_EDITABLE(widgets->gs_smtp_entry), settings->smtp_server);
            gtk_box_append(GTK_BOX(gs_smtp_box), widgets->gs_smtp_entry);
            gtk_box_append(GTK_BOX(box), gs_smtp_box);
            
            GtkWidget *gs_email_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
            gtk_box_append(GTK_BOX(gs_email_box), gtk_label_new("Email Address:"));
            widgets->gs_email_entry = gtk_entry_new();
            gtk_editable_set_text(GTK_EDITABLE(widgets->gs_email_entry), settings->email_address);
            gtk_box_append(GTK_BOX(gs_email_box), widgets->gs_email_entry);
            gtk_box_append(GTK_BOX(box), gs_email_box);
            
            widgets->gs_default_mailer = gtk_check_button_new_with_label("Make Eudora the default Mailer");
            gtk_check_button_set_active(GTK_CHECK_BUTTON(widgets->gs_default_mailer), settings->default_mailer);
            gtk_box_append(GTK_BOX(box), widgets->gs_default_mailer);
            break;
            
        case SETTINGS_CHECKING_MAIL:
            title = gtk_label_new("Checking Mail");
            gtk_box_append(GTK_BOX(box), title);
            
            GtkWidget *cm_acct_title = gtk_label_new("Account/Server Information");
            gtk_widget_add_css_class(cm_acct_title, "subtitle");
            gtk_box_append(GTK_BOX(box), cm_acct_title);
            
            GtkWidget *cm_user_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
            gtk_box_append(GTK_BOX(cm_user_box), gtk_label_new("User Name:"));
            widgets->cm_user_entry = gtk_entry_new();
            gtk_editable_set_text(GTK_EDITABLE(widgets->cm_user_entry), settings->pop_username);
            gtk_box_append(GTK_BOX(cm_user_box), widgets->cm_user_entry);
            gtk_box_append(GTK_BOX(box), cm_user_box);
            
            GtkWidget *cm_server_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
            gtk_box_append(GTK_BOX(cm_server_box), gtk_label_new("Mail Server:"));
            widgets->cm_server_entry = gtk_entry_new();
            gtk_editable_set_text(GTK_EDITABLE(widgets->cm_server_entry), settings->pop_server);
            gtk_box_append(GTK_BOX(cm_server_box), widgets->cm_server_entry);
            gtk_box_append(GTK_BOX(box), cm_server_box);
            
            GtkWidget *cm_protocol_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
            gtk_box_append(GTK_BOX(cm_protocol_box), gtk_label_new("Mail Protocol:"));
            widgets->cm_pop_radio = gtk_check_button_new_with_label("POP");
            widgets->cm_imap_radio = gtk_check_button_new_with_label("IMAP");
            gtk_check_button_set_group(GTK_CHECK_BUTTON(widgets->cm_imap_radio), GTK_CHECK_BUTTON(widgets->cm_pop_radio));
            gtk_check_button_set_active(GTK_CHECK_BUTTON(widgets->cm_pop_radio), settings->use_pop);
            gtk_check_button_set_active(GTK_CHECK_BUTTON(widgets->cm_imap_radio), settings->use_imap);
            /* Default to IMAP if neither is set */
            if (!settings->use_pop && !settings->use_imap) {
                gtk_check_button_set_active(GTK_CHECK_BUTTON(widgets->cm_imap_radio), TRUE);
            }
            gtk_box_append(GTK_BOX(cm_protocol_box), widgets->cm_pop_radio);
            gtk_box_append(GTK_BOX(cm_protocol_box), widgets->cm_imap_radio);
            gtk_box_append(GTK_BOX(box), cm_protocol_box);
            
            GtkWidget *cm_auth_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
            gtk_box_append(GTK_BOX(cm_auth_box), gtk_label_new("Authentication:"));
            widgets->cm_pwd_radio = gtk_check_button_new_with_label("Passwords");
            widgets->cm_kerb_radio = gtk_check_button_new_with_label("Kerberos");
            widgets->cm_apop_radio = gtk_check_button_new_with_label("APOP");
            gtk_check_button_set_group(GTK_CHECK_BUTTON(widgets->cm_kerb_radio), GTK_CHECK_BUTTON(widgets->cm_pwd_radio));
            gtk_check_button_set_group(GTK_CHECK_BUTTON(widgets->cm_apop_radio), GTK_CHECK_BUTTON(widgets->cm_pwd_radio));
            gtk_check_button_set_active(GTK_CHECK_BUTTON(widgets->cm_pwd_radio), settings->use_passwords);
            gtk_check_button_set_active(GTK_CHECK_BUTTON(widgets->cm_kerb_radio), settings->use_kerberos);
            gtk_check_button_set_active(GTK_CHECK_BUTTON(widgets->cm_apop_radio), settings->use_apop);
            gtk_box_append(GTK_BOX(cm_auth_box), widgets->cm_pwd_radio);
            gtk_box_append(GTK_BOX(cm_auth_box), widgets->cm_kerb_radio);
            gtk_box_append(GTK_BOX(cm_auth_box), widgets->cm_apop_radio);
            gtk_box_append(GTK_BOX(box), cm_auth_box);
            
            widgets->cm_overlap = gtk_check_button_new_with_label("Overlap commands");
            gtk_check_button_set_active(GTK_CHECK_BUTTON(widgets->cm_overlap), settings->overlap_commands);
            gtk_box_append(GTK_BOX(box), widgets->cm_overlap);
            
            GtkWidget *cm_conn_title = gtk_label_new("Connection");
            gtk_widget_add_css_class(cm_conn_title, "subtitle");
            gtk_box_append(GTK_BOX(box), cm_conn_title);
            
            GtkWidget *cm_check_interval = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
            widgets->cm_check_cb = gtk_check_button_new_with_label("Check for mail every");
            gtk_check_button_set_active(GTK_CHECK_BUTTON(widgets->cm_check_cb), TRUE);
            gtk_box_append(GTK_BOX(cm_check_interval), widgets->cm_check_cb);
            widgets->cm_interval_spin = gtk_spin_button_new_with_range(1, 999, 1);
            gtk_spin_button_set_value(GTK_SPIN_BUTTON(widgets->cm_interval_spin), settings->check_interval);
            gtk_box_append(GTK_BOX(cm_check_interval), widgets->cm_interval_spin);
            gtk_box_append(GTK_BOX(cm_check_interval), gtk_label_new("minutes"));
            gtk_box_append(GTK_BOX(box), cm_check_interval);
            
            widgets->cm_battery = gtk_check_button_new_with_label("Don't auto-check when using battery");
            gtk_check_button_set_active(GTK_CHECK_BUTTON(widgets->cm_battery), settings->check_battery);
            gtk_box_append(GTK_BOX(box), widgets->cm_battery);
            
            widgets->cm_send_on_check = gtk_check_button_new_with_label("Send on check");
            gtk_check_button_set_active(GTK_CHECK_BUTTON(widgets->cm_send_on_check), settings->send_on_check);
            gtk_box_append(GTK_BOX(box), widgets->cm_send_on_check);
            
            GtkWidget *cm_mgmt_title = gtk_label_new("Mail Management");
            gtk_widget_add_css_class(cm_mgmt_title, "subtitle");
            gtk_box_append(GTK_BOX(box), cm_mgmt_title);
            
            GtkWidget *cm_leave_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
            widgets->cm_leave_cb = gtk_check_button_new_with_label("Leave on server for");
            gtk_check_button_set_active(GTK_CHECK_BUTTON(widgets->cm_leave_cb), settings->leave_on_server);
            gtk_box_append(GTK_BOX(cm_leave_box), widgets->cm_leave_cb);
            widgets->cm_leave_spin = gtk_spin_button_new_with_range(0, 999, 1);
            gtk_spin_button_set_value(GTK_SPIN_BUTTON(widgets->cm_leave_spin), settings->leave_days);
            gtk_box_append(GTK_BOX(cm_leave_box), widgets->cm_leave_spin);
            gtk_box_append(GTK_BOX(cm_leave_box), gtk_label_new("days"));
            gtk_box_append(GTK_BOX(box), cm_leave_box);
            
            widgets->cm_delete_trash = gtk_check_button_new_with_label("Delete from server when emptied from trash");
            gtk_check_button_set_active(GTK_CHECK_BUTTON(widgets->cm_delete_trash), settings->delete_from_trash);
            gtk_box_append(GTK_BOX(box), widgets->cm_delete_trash);
            
            GtkWidget *cm_skip_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
            widgets->cm_skip_cb = gtk_check_button_new_with_label("Skip messages over");
            gtk_check_button_set_active(GTK_CHECK_BUTTON(widgets->cm_skip_cb), settings->skip_size_kb > 0);
            gtk_box_append(GTK_BOX(cm_skip_box), widgets->cm_skip_cb);
            widgets->cm_skip_spin = gtk_spin_button_new_with_range(0, 10000, 1);
            gtk_spin_button_set_value(GTK_SPIN_BUTTON(widgets->cm_skip_spin), settings->skip_size_kb);
            gtk_box_append(GTK_BOX(cm_skip_box), widgets->cm_skip_spin);
            gtk_box_append(GTK_BOX(cm_skip_box), gtk_label_new("K"));
            gtk_box_append(GTK_BOX(box), cm_skip_box);
            break;
            
        case SETTINGS_SENDING_MAIL:
            title = gtk_label_new("Sending Mail");
            gtk_box_append(GTK_BOX(box), title);
            
            GtkWidget *sm_srv_title = gtk_label_new("Server");
            gtk_widget_add_css_class(sm_srv_title, "subtitle");
            gtk_box_append(GTK_BOX(box), sm_srv_title);
            
            GtkWidget *sm_email_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
            gtk_box_append(GTK_BOX(sm_email_box), gtk_label_new("Email Address:"));
            widgets->sm_email_entry = gtk_entry_new();
            gtk_editable_set_text(GTK_EDITABLE(widgets->sm_email_entry), settings->email_address);
            gtk_box_append(GTK_BOX(sm_email_box), widgets->sm_email_entry);
            gtk_box_append(GTK_BOX(box), sm_email_box);
            
            GtkWidget *sm_email_note = gtk_label_new("(You only need to fill this in if it's different from Username@MailServer.)");
            gtk_label_set_wrap(GTK_LABEL(sm_email_note), TRUE);
            gtk_box_append(GTK_BOX(box), sm_email_note);
            
            GtkWidget *sm_domain_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
            gtk_box_append(GTK_BOX(sm_domain_box), gtk_label_new("Default Domain:"));
            widgets->sm_domain_entry = gtk_entry_new();
            gtk_editable_set_text(GTK_EDITABLE(widgets->sm_domain_entry), settings->default_domain);
            gtk_box_append(GTK_BOX(sm_domain_box), widgets->sm_domain_entry);
            gtk_box_append(GTK_BOX(box), sm_domain_box);
            
            GtkWidget *sm_domain_note = gtk_label_new("(This will be added to addresses you type that don't have domains on them.)");
            gtk_label_set_wrap(GTK_LABEL(sm_domain_note), TRUE);
            gtk_box_append(GTK_BOX(box), sm_domain_note);
            
            GtkWidget *sm_smtp_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
            gtk_box_append(GTK_BOX(sm_smtp_box), gtk_label_new("SMTP Server:"));
            widgets->sm_smtp_entry = gtk_entry_new();
            gtk_editable_set_text(GTK_EDITABLE(widgets->sm_smtp_entry), settings->smtp_server);
            gtk_box_append(GTK_BOX(sm_smtp_box), widgets->sm_smtp_entry);
            gtk_box_append(GTK_BOX(box), sm_smtp_box);
            
            GtkWidget *sm_smtp_note = gtk_label_new("(You only need to fill this in if it's different from your Mail Server.)");
            gtk_label_set_wrap(GTK_LABEL(sm_smtp_note), TRUE);
            gtk_box_append(GTK_BOX(box), sm_smtp_note);
            
            GtkWidget *sm_opt_title = gtk_label_new("Options");
            gtk_widget_add_css_class(sm_opt_title, "subtitle");
            gtk_box_append(GTK_BOX(box), sm_opt_title);
            
            widgets->sm_auto_send = gtk_check_button_new_with_label("Send immediately");
            gtk_check_button_set_active(GTK_CHECK_BUTTON(widgets->sm_auto_send), settings->send_immediately);
            gtk_box_append(GTK_BOX(box), widgets->sm_auto_send);
            
            widgets->sm_keep_copy = gtk_check_button_new_with_label("Keep copy of sent mail");
            gtk_check_button_set_active(GTK_CHECK_BUTTON(widgets->sm_keep_copy), settings->keep_sent_copy);
            gtk_box_append(GTK_BOX(box), widgets->sm_keep_copy);
            
            widgets->sm_wrap_text = gtk_check_button_new_with_label("Wrap outgoing text");
            gtk_check_button_set_active(GTK_CHECK_BUTTON(widgets->sm_wrap_text), settings->wrap_outgoing);
            gtk_box_append(GTK_BOX(box), widgets->sm_wrap_text);
            
            widgets->sm_include_sig = gtk_check_button_new_with_label("Include signature");
            gtk_check_button_set_active(GTK_CHECK_BUTTON(widgets->sm_include_sig), settings->include_signature);
            gtk_box_append(GTK_BOX(box), widgets->sm_include_sig);
            break;
            
        case SETTINGS_DISPLAY:
            title = gtk_label_new("Display");
            gtk_box_append(GTK_BOX(box), title);
            
            widgets->disp_show_preview = gtk_check_button_new_with_label("Show message preview");
            gtk_check_button_set_active(GTK_CHECK_BUTTON(widgets->disp_show_preview), settings->show_preview);
            gtk_box_append(GTK_BOX(box), widgets->disp_show_preview);
            
            widgets->disp_show_toolbars = gtk_check_button_new_with_label("Show toolbars");
            gtk_check_button_set_active(GTK_CHECK_BUTTON(widgets->disp_show_toolbars), settings->show_toolbars);
            gtk_box_append(GTK_BOX(box), widgets->disp_show_toolbars);
            
            widgets->disp_zoom_on_open = gtk_check_button_new_with_label("Zoom windows on open");
            gtk_check_button_set_active(GTK_CHECK_BUTTON(widgets->disp_zoom_on_open), settings->zoom_on_open);
            gtk_box_append(GTK_BOX(box), widgets->disp_zoom_on_open);
            break;
            
        case SETTINGS_ADVANCED:
            title = gtk_label_new("Advanced");
            gtk_box_append(GTK_BOX(box), title);
            
            widgets->adv_case_sensitive = gtk_check_button_new_with_label("Case sensitive search");
            gtk_check_button_set_active(GTK_CHECK_BUTTON(widgets->adv_case_sensitive), settings->case_sensitive_search);
            gtk_box_append(GTK_BOX(box), widgets->adv_case_sensitive);
            
            widgets->adv_offline_mode = gtk_check_button_new_with_label("Work offline");
            gtk_check_button_set_active(GTK_CHECK_BUTTON(widgets->adv_offline_mode), settings->offline_mode);
            gtk_box_append(GTK_BOX(box), widgets->adv_offline_mode);
            
            widgets->adv_expert_mode = gtk_check_button_new_with_label("Expert mode");
            gtk_check_button_set_active(GTK_CHECK_BUTTON(widgets->adv_expert_mode), settings->expert_mode);
            gtk_box_append(GTK_BOX(box), widgets->adv_expert_mode);
            break;
            
        case SETTINGS_SECURITY:
            title = gtk_label_new("Security");
            gtk_box_append(GTK_BOX(box), title);
            
            widgets->sec_save_password = gtk_check_button_new_with_label("Save password");
            gtk_check_button_set_active(GTK_CHECK_BUTTON(widgets->sec_save_password), settings->save_password);
            gtk_box_append(GTK_BOX(box), widgets->sec_save_password);
            
            widgets->sec_use_ssl = gtk_check_button_new_with_label("Use SSL/TLS");
            gtk_check_button_set_active(GTK_CHECK_BUTTON(widgets->sec_use_ssl), settings->use_ssl);
            gtk_box_append(GTK_BOX(box), widgets->sec_use_ssl);
            
            GtkWidget *change_password = gtk_button_new_with_label("Change Password...");
            gtk_box_append(GTK_BOX(box), change_password);
            break;
            
        case SETTINGS_FONTS:
            title = gtk_label_new("Fonts");
            gtk_box_append(GTK_BOX(box), title);
            
            GtkWidget *fonts_msg_title = gtk_label_new("Message Display");
            gtk_widget_add_css_class(fonts_msg_title, "subtitle");
            gtk_box_append(GTK_BOX(box), fonts_msg_title);
            
            GtkWidget *fonts_msg_font_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
            gtk_box_append(GTK_BOX(fonts_msg_font_box), gtk_label_new("Font:"));
            GtkFontDialog *font_dialog = gtk_font_dialog_new();
            widgets->fonts_message_font = gtk_font_dialog_button_new(font_dialog);
            if (settings->message_font[0]) {
                PangoFontDescription *font_desc = pango_font_description_from_string(settings->message_font);
                gtk_font_dialog_button_set_font_desc(GTK_FONT_DIALOG_BUTTON(widgets->fonts_message_font), font_desc);
                pango_font_description_free(font_desc);
            }
            gtk_box_append(GTK_BOX(fonts_msg_font_box), widgets->fonts_message_font);
            gtk_box_append(GTK_BOX(box), fonts_msg_font_box);
            
            GtkWidget *fonts_msg_size_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
            gtk_box_append(GTK_BOX(fonts_msg_size_box), gtk_label_new("Size:"));
            widgets->fonts_message_size = gtk_spin_button_new_with_range(8, 72, 1);
            gtk_spin_button_set_value(GTK_SPIN_BUTTON(widgets->fonts_message_size), settings->message_font_size > 0 ? settings->message_font_size : 12);
            gtk_box_append(GTK_BOX(fonts_msg_size_box), widgets->fonts_message_size);
            gtk_box_append(GTK_BOX(fonts_msg_size_box), gtk_label_new("pt"));
            gtk_box_append(GTK_BOX(box), fonts_msg_size_box);
            
            GtkWidget *fonts_comp_title = gtk_label_new("Compose");
            gtk_widget_add_css_class(fonts_comp_title, "subtitle");
            gtk_box_append(GTK_BOX(box), fonts_comp_title);
            
            GtkWidget *fonts_comp_font_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
            gtk_box_append(GTK_BOX(fonts_comp_font_box), gtk_label_new("Font:"));
            GtkFontDialog *font_dialog2 = gtk_font_dialog_new();
            widgets->fonts_compose_font = gtk_font_dialog_button_new(font_dialog2);
            if (settings->compose_font[0]) {
                PangoFontDescription *font_desc = pango_font_description_from_string(settings->compose_font);
                gtk_font_dialog_button_set_font_desc(GTK_FONT_DIALOG_BUTTON(widgets->fonts_compose_font), font_desc);
                pango_font_description_free(font_desc);
            }
            gtk_box_append(GTK_BOX(fonts_comp_font_box), widgets->fonts_compose_font);
            gtk_box_append(GTK_BOX(box), fonts_comp_font_box);
            
            GtkWidget *fonts_comp_size_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
            gtk_box_append(GTK_BOX(fonts_comp_size_box), gtk_label_new("Size:"));
            widgets->fonts_compose_size = gtk_spin_button_new_with_range(8, 72, 1);
            gtk_spin_button_set_value(GTK_SPIN_BUTTON(widgets->fonts_compose_size), settings->compose_font_size > 0 ? settings->compose_font_size : 12);
            gtk_box_append(GTK_BOX(fonts_comp_size_box), widgets->fonts_compose_size);
            gtk_box_append(GTK_BOX(fonts_comp_size_box), gtk_label_new("pt"));
            gtk_box_append(GTK_BOX(box), fonts_comp_size_box);
            break;
            
        case SETTINGS_ACCOUNTS:
            title = gtk_label_new("Accounts");
            gtk_box_append(GTK_BOX(box), title);
            
            GtkWidget *acct_list_title = gtk_label_new("Email Accounts");
            gtk_widget_add_css_class(acct_list_title, "subtitle");
            gtk_box_append(GTK_BOX(box), acct_list_title);
            
            /* Initialize account storage */
            widgets->num_accounts = prefs_load_accounts(widgets->accounts, 10);
            widgets->current_account = -1;
            
            /* Account list */
            GtkWidget *acct_scroll = gtk_scrolled_window_new();
            gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(acct_scroll), 150);
            widgets->acct_list = gtk_list_box_new();
            gtk_list_box_set_selection_mode(GTK_LIST_BOX(widgets->acct_list), GTK_SELECTION_SINGLE);
            gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(acct_scroll), widgets->acct_list);
            gtk_box_append(GTK_BOX(box), acct_scroll);
            gtk_widget_set_vexpand(acct_scroll, TRUE);
            
            /* Populate account list from loaded accounts */
            for (int i = 0; i < widgets->num_accounts; i++) {
                GtkWidget *row = gtk_list_box_row_new();
                GtkWidget *label = gtk_label_new(widgets->accounts[i].name);
                gtk_label_set_xalign(GTK_LABEL(label), 0);
                gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), label);
                gtk_list_box_append(GTK_LIST_BOX(widgets->acct_list), row);
                gtk_widget_set_visible(row, TRUE);
            }
            
            /* Add/Edit/Remove buttons */
            GtkWidget *acct_button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
            widgets->acct_add_button = gtk_button_new_with_label("Add Account");
            GtkWidget *acct_edit_button = gtk_button_new_with_label("Edit Account");
            widgets->acct_remove_button = gtk_button_new_with_label("Remove Account");
            gtk_box_append(GTK_BOX(acct_button_box), widgets->acct_add_button);
            gtk_box_append(GTK_BOX(acct_button_box), acct_edit_button);
            gtk_box_append(GTK_BOX(acct_button_box), widgets->acct_remove_button);
            gtk_box_append(GTK_BOX(box), acct_button_box);
            
            /* Initially disable Edit and Remove buttons */
            gtk_widget_set_sensitive(acct_edit_button, FALSE);
            gtk_widget_set_sensitive(widgets->acct_remove_button, FALSE);
            
            /* Connect button signals */
            g_signal_connect(widgets->acct_add_button, "clicked", G_CALLBACK(on_add_account_clicked), dialog);
            g_signal_connect(acct_edit_button, "clicked", G_CALLBACK(on_edit_account_clicked), dialog);
            g_signal_connect(widgets->acct_remove_button, "clicked", G_CALLBACK(on_remove_account_clicked), dialog);
            
            /* Connect selection changed to enable/disable Edit and Remove buttons */
            g_object_set_data(G_OBJECT(widgets->acct_list), "edit-button", acct_edit_button);
            g_object_set_data(G_OBJECT(widgets->acct_list), "remove-button", widgets->acct_remove_button);
            g_signal_connect(widgets->acct_list, "row-selected", G_CALLBACK(on_account_selected), dialog);
            
            break;
            
        case SETTINGS_ATTACHMENTS:
        default:
            title = gtk_label_new("Not Implemented");
            gtk_box_append(GTK_BOX(box), title);
            break;
    }
    
    if (title) {
        gtk_widget_add_css_class(title, "title-2");
    }
    
    return box;
}

/* Row selected callback - sync bidirectionally when switching sections */
static void on_section_selected(GtkListBox *box, GtkListBoxRow *row, gpointer user_data)
{
    (void)box;
    SettingsDialog *dialog = (SettingsDialog *)user_data;
    DialogWidgets *widgets = (DialogWidgets *)dialog->widgets;
    
    if (!row || !dialog) {
        return;
    }
    
    int index = gtk_list_box_row_get_index(row);
    
    /* Sync all widgets to settings before switching sections */
    sync_widgets_to_settings(widgets, dialog->settings);
    
    /* Bidirectional sync: Getting Started <-> Checking Mail <-> Sending Mail */
    
    /* Sync user/server between Getting Started and Checking Mail */
    if (widgets->cm_user_entry && widgets->gs_user_entry) {
        const char *cm_text = gtk_editable_get_text(GTK_EDITABLE(widgets->cm_user_entry));
        const char *gs_text = gtk_editable_get_text(GTK_EDITABLE(widgets->gs_user_entry));
        /* Use Checking Mail as source of truth */
        if (strlen(cm_text) > 0) {
            gtk_editable_set_text(GTK_EDITABLE(widgets->gs_user_entry), cm_text);
        } else if (strlen(gs_text) > 0) {
            gtk_editable_set_text(GTK_EDITABLE(widgets->cm_user_entry), gs_text);
        }
    }
    if (widgets->cm_server_entry && widgets->gs_server_entry) {
        const char *cm_text = gtk_editable_get_text(GTK_EDITABLE(widgets->cm_server_entry));
        const char *gs_text = gtk_editable_get_text(GTK_EDITABLE(widgets->gs_server_entry));
        if (strlen(cm_text) > 0) {
            gtk_editable_set_text(GTK_EDITABLE(widgets->gs_server_entry), cm_text);
        } else if (strlen(gs_text) > 0) {
            gtk_editable_set_text(GTK_EDITABLE(widgets->cm_server_entry), gs_text);
        }
    }
    
    /* Sync email/smtp between Getting Started and Sending Mail */
    if (widgets->sm_email_entry && widgets->gs_email_entry) {
        const char *sm_text = gtk_editable_get_text(GTK_EDITABLE(widgets->sm_email_entry));
        const char *gs_text = gtk_editable_get_text(GTK_EDITABLE(widgets->gs_email_entry));
        /* Use Sending Mail as source of truth */
        if (strlen(sm_text) > 0) {
            gtk_editable_set_text(GTK_EDITABLE(widgets->gs_email_entry), sm_text);
        } else if (strlen(gs_text) > 0) {
            gtk_editable_set_text(GTK_EDITABLE(widgets->sm_email_entry), gs_text);
        }
    }
    if (widgets->sm_smtp_entry && widgets->gs_smtp_entry) {
        const char *sm_text = gtk_editable_get_text(GTK_EDITABLE(widgets->sm_smtp_entry));
        const char *gs_text = gtk_editable_get_text(GTK_EDITABLE(widgets->gs_smtp_entry));
        if (strlen(sm_text) > 0) {
            gtk_editable_set_text(GTK_EDITABLE(widgets->gs_smtp_entry), sm_text);
        } else if (strlen(gs_text) > 0) {
            gtk_editable_set_text(GTK_EDITABLE(widgets->sm_smtp_entry), gs_text);
        }
    }
    
    /* Sync real name between Getting Started and Sending Mail */
    if (widgets->gs_realname_entry && widgets->sm_email_entry) {
        const char *gs_text = gtk_editable_get_text(GTK_EDITABLE(widgets->gs_realname_entry));
        /* Real name is stored in Sending Mail section, sync it */
        if (strlen(gs_text) > 0) {
            /* Getting Started has it, update settings */
            strncpy(dialog->settings->real_name, gs_text, sizeof(dialog->settings->real_name) - 1);
        }
    }
    
    /* Hide all sections */
    for (int i = 0; i < SETTINGS_COUNT; i++) {
        gtk_widget_set_visible(dialog->section_widgets[i], (i == index));
    }
}

/* Add account button clicked */
static void on_add_account_clicked(GtkWidget *widget, gpointer user_data)
{
    (void)widget;
    SettingsDialog *dialog = (SettingsDialog *)user_data;
    
    /* Create a simple dialog to add account */
    GtkWidget *add_dialog = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(add_dialog), "Add Email Account");
    gtk_window_set_modal(GTK_WINDOW(add_dialog), TRUE);
    gtk_window_set_transient_for(GTK_WINDOW(add_dialog), GTK_WINDOW(dialog->dialog));
    gtk_window_set_default_size(GTK_WINDOW(add_dialog), 400, 300);
    
    GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_set_margin_start(main_box, 12);
    gtk_widget_set_margin_end(main_box, 12);
    gtk_widget_set_margin_top(main_box, 12);
    gtk_widget_set_margin_bottom(main_box, 12);
    gtk_window_set_child(GTK_WINDOW(add_dialog), main_box);
    
    /* Account name */
    GtkWidget *name_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_box_append(GTK_BOX(name_box), gtk_label_new("Account Name:"));
    GtkWidget *name_entry = gtk_entry_new();
    gtk_box_append(GTK_BOX(name_box), name_entry);
    gtk_widget_set_hexpand(name_entry, TRUE);
    gtk_box_append(GTK_BOX(main_box), name_box);
    
    /* Email address */
    GtkWidget *email_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_box_append(GTK_BOX(email_box), gtk_label_new("Email Address:"));
    GtkWidget *email_entry = gtk_entry_new();
    gtk_box_append(GTK_BOX(email_box), email_entry);
    gtk_widget_set_hexpand(email_entry, TRUE);
    gtk_box_append(GTK_BOX(main_box), email_box);
    
    /* Account type */
    GtkWidget *type_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_box_append(GTK_BOX(type_box), gtk_label_new("Account Type:"));
    const char *types[] = { "IMAP", "POP", NULL };
    GtkWidget *type_combo = gtk_drop_down_new_from_strings(types);
    gtk_drop_down_set_selected(GTK_DROP_DOWN(type_combo), 0);
    gtk_box_append(GTK_BOX(type_box), type_combo);
    gtk_box_append(GTK_BOX(main_box), type_box);
    
    /* Mail server */
    GtkWidget *server_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_box_append(GTK_BOX(server_box), gtk_label_new("Mail Server:"));
    GtkWidget *server_entry = gtk_entry_new();
    gtk_box_append(GTK_BOX(server_box), server_entry);
    gtk_widget_set_hexpand(server_entry, TRUE);
    gtk_box_append(GTK_BOX(main_box), server_box);
    
    /* SMTP server */
    GtkWidget *smtp_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_box_append(GTK_BOX(smtp_box), gtk_label_new("SMTP Server:"));
    GtkWidget *smtp_entry = gtk_entry_new();
    gtk_box_append(GTK_BOX(smtp_box), smtp_entry);
    gtk_widget_set_hexpand(smtp_entry, TRUE);
    gtk_box_append(GTK_BOX(main_box), smtp_box);
    
    /* Username */
    GtkWidget *user_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_box_append(GTK_BOX(user_box), gtk_label_new("Username:"));
    GtkWidget *user_entry = gtk_entry_new();
    gtk_box_append(GTK_BOX(user_box), user_entry);
    gtk_widget_set_hexpand(user_entry, TRUE);
    gtk_box_append(GTK_BOX(main_box), user_box);
    
    /* Buttons */
    GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_set_margin_top(button_box, 12);
    GtkWidget *cancel_btn = gtk_button_new_with_label("Cancel");
    GtkWidget *add_btn = gtk_button_new_with_label("Add");
    gtk_box_append(GTK_BOX(button_box), cancel_btn);
    gtk_box_append(GTK_BOX(button_box), add_btn);
    gtk_box_append(GTK_BOX(main_box), button_box);
    
    /* Store data for callbacks */
    g_object_set_data(G_OBJECT(add_dialog), "name-entry", name_entry);
    g_object_set_data(G_OBJECT(add_dialog), "email-entry", email_entry);
    g_object_set_data(G_OBJECT(add_dialog), "type-combo", type_combo);
    g_object_set_data(G_OBJECT(add_dialog), "server-entry", server_entry);
    g_object_set_data(G_OBJECT(add_dialog), "smtp-entry", smtp_entry);
    g_object_set_data(G_OBJECT(add_dialog), "user-entry", user_entry);
    g_object_set_data(G_OBJECT(add_dialog), "settings-dialog", dialog);
    
    g_signal_connect_swapped(cancel_btn, "clicked", G_CALLBACK(gtk_window_close), add_dialog);
    g_signal_connect(add_btn, "clicked", G_CALLBACK(on_add_account_ok), add_dialog);
    
    gtk_window_present(GTK_WINDOW(add_dialog));
}

/* Add account OK button callback */
static void on_add_account_ok(GtkWidget *widget, gpointer user_data)
{
    (void)widget;
    GtkWidget *add_dialog = (GtkWidget *)user_data;
    
    GtkWidget *name_entry = (GtkWidget *)g_object_get_data(G_OBJECT(add_dialog), "name-entry");
    GtkWidget *email_entry = (GtkWidget *)g_object_get_data(G_OBJECT(add_dialog), "email-entry");
    GtkWidget *type_combo = (GtkWidget *)g_object_get_data(G_OBJECT(add_dialog), "type-combo");
    GtkWidget *server_entry = (GtkWidget *)g_object_get_data(G_OBJECT(add_dialog), "server-entry");
    GtkWidget *smtp_entry = (GtkWidget *)g_object_get_data(G_OBJECT(add_dialog), "smtp-entry");
    GtkWidget *user_entry = (GtkWidget *)g_object_get_data(G_OBJECT(add_dialog), "user-entry");
    SettingsDialog *dialog = (SettingsDialog *)g_object_get_data(G_OBJECT(add_dialog), "settings-dialog");
    DialogWidgets *widgets = (DialogWidgets *)dialog->widgets;
    
    const char *name = gtk_editable_get_text(GTK_EDITABLE(name_entry));
    const char *email = gtk_editable_get_text(GTK_EDITABLE(email_entry));
    
    GtkStringObject *type_item = GTK_STRING_OBJECT(gtk_drop_down_get_selected_item(GTK_DROP_DOWN(type_combo)));
    const char *type = gtk_string_object_get_string(type_item);
    
    const char *server = gtk_editable_get_text(GTK_EDITABLE(server_entry));
    const char *smtp = gtk_editable_get_text(GTK_EDITABLE(smtp_entry));
    const char *user = gtk_editable_get_text(GTK_EDITABLE(user_entry));
    
    if (!name || strlen(name) == 0) {
        g_print("Account name is required\n");
        return;
    }
    
    g_print("Adding account: %s (%s) - %s@%s\n", name, type, user, server);
    
    /* Add to accounts array */
    if (widgets->num_accounts < 10) {
        PrefsAccount *acct = &widgets->accounts[widgets->num_accounts];
        strncpy(acct->name, name, sizeof(acct->name) - 1);
        strncpy(acct->email, email, sizeof(acct->email) - 1);
        strncpy(acct->type, type ? type : "IMAP", sizeof(acct->type) - 1);
        strncpy(acct->server, server, sizeof(acct->server) - 1);
        strncpy(acct->smtp_server, smtp, sizeof(acct->smtp_server) - 1);
        strncpy(acct->username, user, sizeof(acct->username) - 1);
        acct->enabled = TRUE;
        
        /* Add to account list UI */
        GtkWidget *row = gtk_list_box_row_new();
        GtkWidget *label = gtk_label_new(name);
        gtk_label_set_xalign(GTK_LABEL(label), 0);
        gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), label);
        gtk_list_box_append(GTK_LIST_BOX(widgets->acct_list), row);
        gtk_widget_set_visible(row, TRUE);
        
        widgets->num_accounts++;
    }
    
    gtk_window_close(GTK_WINDOW(add_dialog));
}

/* Remove account button clicked */
static void on_remove_account_clicked(GtkWidget *widget, gpointer user_data)
{
    (void)widget;
    SettingsDialog *dialog = (SettingsDialog *)user_data;
    DialogWidgets *widgets = (DialogWidgets *)dialog->widgets;
    
    GtkListBoxRow *row = gtk_list_box_get_selected_row(GTK_LIST_BOX(widgets->acct_list));
    if (row) {
        int index = gtk_list_box_row_get_index(row);
        
        /* Remove from UI */
        gtk_list_box_remove(GTK_LIST_BOX(widgets->acct_list), GTK_WIDGET(row));
        
        /* Remove from accounts array */
        if (index >= 0 && index < widgets->num_accounts) {
            for (int i = index; i < widgets->num_accounts - 1; i++) {
                widgets->accounts[i] = widgets->accounts[i + 1];
            }
            widgets->num_accounts--;
            g_print("Account removed\n");
        }
    } else {
        g_print("No account selected to remove\n");
    }
}

/* Account row selected - enable/disable Edit and Remove buttons */
static void on_account_selected(GtkListBox *box, GtkListBoxRow *row, gpointer user_data)
{
    (void)box;
    (void)user_data;
    
    GtkWidget *edit_button = (GtkWidget *)g_object_get_data(G_OBJECT(box), "edit-button");
    GtkWidget *remove_button = (GtkWidget *)g_object_get_data(G_OBJECT(box), "remove-button");
    
    gboolean has_selection = (row != NULL);
    gtk_widget_set_sensitive(edit_button, has_selection);
    gtk_widget_set_sensitive(remove_button, has_selection);
}

/* Edit account button clicked */
static void on_edit_account_clicked(GtkWidget *widget, gpointer user_data)
{
    (void)widget;
    SettingsDialog *dialog = (SettingsDialog *)user_data;
    DialogWidgets *widgets = (DialogWidgets *)dialog->widgets;
    
    GtkListBoxRow *row = gtk_list_box_get_selected_row(GTK_LIST_BOX(widgets->acct_list));
    if (!row) return;
    
    int index = gtk_list_box_row_get_index(row);
    if (index < 0 || index >= widgets->num_accounts) return;
    
    PrefsAccount *acct = &widgets->accounts[index];
    widgets->current_account = index;
    
    /* Create edit dialog */
    GtkWidget *edit_dialog = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(edit_dialog), "Edit Email Account");
    gtk_window_set_modal(GTK_WINDOW(edit_dialog), TRUE);
    gtk_window_set_transient_for(GTK_WINDOW(edit_dialog), GTK_WINDOW(dialog->dialog));
    gtk_window_set_default_size(GTK_WINDOW(edit_dialog), 400, 300);
    
    GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_set_margin_start(main_box, 12);
    gtk_widget_set_margin_end(main_box, 12);
    gtk_widget_set_margin_top(main_box, 12);
    gtk_widget_set_margin_bottom(main_box, 12);
    gtk_window_set_child(GTK_WINDOW(edit_dialog), main_box);
    
    /* Account name */
    GtkWidget *name_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_box_append(GTK_BOX(name_box), gtk_label_new("Account Name:"));
    GtkWidget *name_entry = gtk_entry_new();
    gtk_editable_set_text(GTK_EDITABLE(name_entry), acct->name);
    gtk_box_append(GTK_BOX(name_box), name_entry);
    gtk_widget_set_hexpand(name_entry, TRUE);
    gtk_box_append(GTK_BOX(main_box), name_box);
    
    /* Email address */
    GtkWidget *email_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_box_append(GTK_BOX(email_box), gtk_label_new("Email Address:"));
    GtkWidget *email_entry = gtk_entry_new();
    gtk_editable_set_text(GTK_EDITABLE(email_entry), acct->email);
    gtk_box_append(GTK_BOX(email_box), email_entry);
    gtk_widget_set_hexpand(email_entry, TRUE);
    gtk_box_append(GTK_BOX(main_box), email_box);
    
    /* Account type */
    GtkWidget *type_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_box_append(GTK_BOX(type_box), gtk_label_new("Account Type:"));
    
    const char *types[] = { "IMAP", "POP", "SMTP", NULL };
    GtkWidget *type_combo = gtk_drop_down_new_from_strings(types);
    
    if (g_strcmp0(acct->type, "POP") == 0)
        gtk_drop_down_set_selected(GTK_DROP_DOWN(type_combo), 1);
    else if (g_strcmp0(acct->type, "SMTP") == 0)
        gtk_drop_down_set_selected(GTK_DROP_DOWN(type_combo), 2);
    else
        gtk_drop_down_set_selected(GTK_DROP_DOWN(type_combo), 0);
        
    gtk_box_append(GTK_BOX(type_box), type_combo);
    gtk_box_append(GTK_BOX(main_box), type_box);
    
    /* Mail server */
    GtkWidget *server_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_box_append(GTK_BOX(server_box), gtk_label_new("Mail Server:"));
    GtkWidget *server_entry = gtk_entry_new();
    gtk_editable_set_text(GTK_EDITABLE(server_entry), acct->server);
    gtk_box_append(GTK_BOX(server_box), server_entry);
    gtk_widget_set_hexpand(server_entry, TRUE);
    gtk_box_append(GTK_BOX(main_box), server_box);
    
    /* SMTP server */
    GtkWidget *smtp_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_box_append(GTK_BOX(smtp_box), gtk_label_new("SMTP Server:"));
    GtkWidget *smtp_entry = gtk_entry_new();
    gtk_editable_set_text(GTK_EDITABLE(smtp_entry), acct->smtp_server);
    gtk_box_append(GTK_BOX(smtp_box), smtp_entry);
    gtk_widget_set_hexpand(smtp_entry, TRUE);
    gtk_box_append(GTK_BOX(main_box), smtp_box);
    
    /* Username */
    GtkWidget *user_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_box_append(GTK_BOX(user_box), gtk_label_new("Username:"));
    GtkWidget *user_entry = gtk_entry_new();
    gtk_editable_set_text(GTK_EDITABLE(user_entry), acct->username);
    gtk_box_append(GTK_BOX(user_box), user_entry);
    gtk_widget_set_hexpand(user_entry, TRUE);
    gtk_box_append(GTK_BOX(main_box), user_box);
    
    /* Account status (Enabled/Disabled) */
    GtkWidget *status_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_box_append(GTK_BOX(status_box), gtk_label_new("Status:"));
    const char *statuses[] = { "Enabled", "Disabled", NULL };
    GtkWidget *status_combo = gtk_drop_down_new_from_strings(statuses);
    gtk_drop_down_set_selected(GTK_DROP_DOWN(status_combo), acct->enabled ? 0 : 1);
    gtk_box_append(GTK_BOX(status_box), status_combo);
    gtk_box_append(GTK_BOX(main_box), status_box);
    
    /* Buttons */
    GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_set_margin_top(button_box, 12);
    GtkWidget *cancel_btn = gtk_button_new_with_label("Cancel");
    GtkWidget *save_btn = gtk_button_new_with_label("Save");
    gtk_box_append(GTK_BOX(button_box), cancel_btn);
    gtk_box_append(GTK_BOX(button_box), save_btn);
    gtk_box_append(GTK_BOX(main_box), button_box);
    
    /* Store data for callbacks */
    g_object_set_data(G_OBJECT(edit_dialog), "name-entry", name_entry);
    g_object_set_data(G_OBJECT(edit_dialog), "email-entry", email_entry);
    g_object_set_data(G_OBJECT(edit_dialog), "type-combo", type_combo);
    g_object_set_data(G_OBJECT(edit_dialog), "server-entry", server_entry);
    g_object_set_data(G_OBJECT(edit_dialog), "smtp-entry", smtp_entry);
    g_object_set_data(G_OBJECT(edit_dialog), "user-entry", user_entry);
    g_object_set_data(G_OBJECT(edit_dialog), "status-combo", status_combo);
    g_object_set_data(G_OBJECT(edit_dialog), "settings-dialog", dialog);
    g_object_set_data(G_OBJECT(edit_dialog), "account-index", GINT_TO_POINTER(index));
    
    g_signal_connect_swapped(cancel_btn, "clicked", G_CALLBACK(gtk_window_close), edit_dialog);
    g_signal_connect(save_btn, "clicked", G_CALLBACK(on_edit_account_save), edit_dialog);
    
    gtk_window_present(GTK_WINDOW(edit_dialog));
}

/* Edit account save button callback */
static void on_edit_account_save(GtkWidget *widget, gpointer user_data)
{
    (void)widget;
    GtkWidget *edit_dialog = (GtkWidget *)user_data;
    
    GtkWidget *name_entry = (GtkWidget *)g_object_get_data(G_OBJECT(edit_dialog), "name-entry");
    GtkWidget *email_entry = (GtkWidget *)g_object_get_data(G_OBJECT(edit_dialog), "email-entry");
    GtkWidget *type_combo = (GtkWidget *)g_object_get_data(G_OBJECT(edit_dialog), "type-combo");
    GtkWidget *server_entry = (GtkWidget *)g_object_get_data(G_OBJECT(edit_dialog), "server-entry");
    GtkWidget *smtp_entry = (GtkWidget *)g_object_get_data(G_OBJECT(edit_dialog), "smtp-entry");
    GtkWidget *user_entry = (GtkWidget *)g_object_get_data(G_OBJECT(edit_dialog), "user-entry");
    GtkWidget *status_combo = (GtkWidget *)g_object_get_data(G_OBJECT(edit_dialog), "status-combo");
    SettingsDialog *dialog = (SettingsDialog *)g_object_get_data(G_OBJECT(edit_dialog), "settings-dialog");
    int index = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(edit_dialog), "account-index"));
    DialogWidgets *widgets = (DialogWidgets *)dialog->widgets;
    
    const char *name = gtk_editable_get_text(GTK_EDITABLE(name_entry));
    const char *email = gtk_editable_get_text(GTK_EDITABLE(email_entry));
    
    GtkStringObject *type_obj = GTK_STRING_OBJECT(gtk_drop_down_get_selected_item(GTK_DROP_DOWN(type_combo)));
    const char *type = gtk_string_object_get_string(type_obj);
    
    const char *server = gtk_editable_get_text(GTK_EDITABLE(server_entry));
    const char *smtp = gtk_editable_get_text(GTK_EDITABLE(smtp_entry));
    const char *user = gtk_editable_get_text(GTK_EDITABLE(user_entry));
    
    GtkStringObject *status_obj = GTK_STRING_OBJECT(gtk_drop_down_get_selected_item(GTK_DROP_DOWN(status_combo)));
    const char *status = gtk_string_object_get_string(status_obj);
    
    if (!name || strlen(name) == 0) {
        g_print("Account name is required\n");
        return;
    }
    
    g_print("Updating account %d: %s\n", index, name);
    
    /* Update account in array */
    PrefsAccount *acct = &widgets->accounts[index];
    strncpy(acct->name, name, sizeof(acct->name) - 1);
    strncpy(acct->email, email, sizeof(acct->email) - 1);
    strncpy(acct->type, type ? type : "IMAP", sizeof(acct->type) - 1);
    strncpy(acct->server, server, sizeof(acct->server) - 1);
    strncpy(acct->smtp_server, smtp, sizeof(acct->smtp_server) - 1);
    strncpy(acct->username, user, sizeof(acct->username) - 1);
    acct->enabled = (status && g_strcmp0(status, "enabled") == 0);
    
    /* Update list UI */
    GtkListBoxRow *row = gtk_list_box_get_row_at_index(GTK_LIST_BOX(widgets->acct_list), index);
    if (row) {
        GtkWidget *label = gtk_list_box_row_get_child(row);
        if (label) {
            gtk_label_set_text(GTK_LABEL(label), name);
        }
    }
    
    gtk_window_close(GTK_WINDOW(edit_dialog));
}

/* Create settings dialog */
SettingsDialog* create_settings_dialog(GtkWindow *parent, AppSettings *settings)
{
    SettingsDialog *dialog = g_new0(SettingsDialog, 1);
    DialogWidgets *widgets = g_new0(DialogWidgets, 1);
    
    dialog->settings = settings ? g_memdup2(settings, sizeof(AppSettings)) : g_new0(AppSettings, 1);
    dialog->widgets = widgets;
    
    /* Create main dialog window */
    dialog->dialog = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(dialog->dialog), "Settings");
    gtk_window_set_modal(GTK_WINDOW(dialog->dialog), TRUE);
    gtk_window_set_transient_for(GTK_WINDOW(dialog->dialog), parent);
    gtk_window_set_default_size(GTK_WINDOW(dialog->dialog), 700, 500);
    
    /* Create main vertical box for dialog content */
    GtkWidget *main_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_window_set_child(GTK_WINDOW(dialog->dialog), main_vbox);
    
    /* Create main horizontal box for content */
    GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_append(GTK_BOX(main_vbox), main_box);
    gtk_widget_set_hexpand(main_box, TRUE);
    gtk_widget_set_vexpand(main_box, TRUE);
    
    /* Create sidebar with section list */
    GtkWidget *sidebar = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_add_css_class(sidebar, "sidebar");
    gtk_widget_set_size_request(sidebar, 150, -1);
    gtk_widget_set_vexpand(sidebar, TRUE);
    gtk_widget_set_hexpand(sidebar, FALSE);
    
    dialog->section_list = GTK_LIST_BOX(gtk_list_box_new());
    gtk_list_box_set_selection_mode(dialog->section_list, GTK_SELECTION_SINGLE);
    gtk_widget_set_vexpand(GTK_WIDGET(dialog->section_list), TRUE);
    gtk_widget_set_hexpand(GTK_WIDGET(dialog->section_list), TRUE);
    
    const char *section_names[] = {
        "Getting Started",
        "Checking Mail",
        "Sending Mail",
        "Attachments",
        "Display",
        "Fonts",
        "Advanced",
        "Security",
        "Accounts"
    };
    
    for (int i = 0; i < SETTINGS_COUNT; i++) {
        GtkWidget *row = gtk_list_box_row_new();
        GtkWidget *label = gtk_label_new(section_names[i]);
        gtk_label_set_xalign(GTK_LABEL(label), 0);
        gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), label);
        gtk_list_box_append(dialog->section_list, row);
    }
    
    GtkWidget *scroll = gtk_scrolled_window_new();
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), GTK_WIDGET(dialog->section_list));
    gtk_box_append(GTK_BOX(sidebar), scroll);
    gtk_widget_set_vexpand(scroll, TRUE);
    gtk_box_append(GTK_BOX(main_box), sidebar);
    
    /* Create content area */
    dialog->content_area = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_add_css_class(dialog->content_area, "content-area");
    gtk_widget_set_hexpand(dialog->content_area, TRUE);
    gtk_widget_set_vexpand(dialog->content_area, TRUE);
    
    /* Create section widgets */
    for (int i = 0; i < SETTINGS_COUNT; i++) {
        dialog->section_widgets[i] = create_section_widget((SettingsSection)i, dialog, widgets);
        gtk_widget_set_visible(dialog->section_widgets[i], (i == 0));
        gtk_box_append(GTK_BOX(dialog->content_area), dialog->section_widgets[i]);
    }
    
    gtk_box_append(GTK_BOX(main_box), dialog->content_area);
    
    /* Connect section selection */
    g_signal_connect(dialog->section_list, "row-selected", 
                    G_CALLBACK(on_section_selected), dialog);
    
    /* Create button box */
    GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_set_margin_start(button_box, 12);
    gtk_widget_set_margin_end(button_box, 12);
    gtk_widget_set_margin_top(button_box, 12);
    gtk_widget_set_margin_bottom(button_box, 12);
    gtk_box_set_homogeneous(GTK_BOX(button_box), TRUE);
    
    GtkWidget *ok_button = gtk_button_new_with_label("OK");
    GtkWidget *cancel_button = gtk_button_new_with_label("Cancel");
    
    gtk_box_append(GTK_BOX(button_box), ok_button);
    gtk_box_append(GTK_BOX(button_box), cancel_button);
    gtk_box_append(GTK_BOX(main_vbox), button_box);
    
    /* Connect button signals */
    g_signal_connect(ok_button, "clicked", G_CALLBACK(on_ok_clicked), dialog);
    g_signal_connect(cancel_button, "clicked", G_CALLBACK(on_cancel_clicked), dialog);
    
    /* Store dialog pointer for later access */
    g_object_set_data(G_OBJECT(dialog->dialog), "settings-dialog", dialog);
    
    return dialog;
}

/* OK button clicked - sync and close */
static void on_ok_clicked(GtkWidget *widget, gpointer user_data)
{
    (void)widget;
    SettingsDialog *dialog = (SettingsDialog *)user_data;
    DialogWidgets *widgets = (DialogWidgets *)dialog->widgets;
    
    /* Sync all widgets to settings */
    sync_widgets_to_settings(widgets, dialog->settings);
    
    /* Save accounts to INI file */
    if (widgets->num_accounts > 0) {
        g_print("Saving %d accounts to INI file\n", widgets->num_accounts);
        prefs_save_accounts(widgets->accounts, widgets->num_accounts);
    }
    
    /* Save all settings to INI file */
    prefs_save(dialog->settings);
    
    /* Mark as OK response */
    g_object_set_data(G_OBJECT(dialog->dialog), "response-id", GINT_TO_POINTER(GTK_RESPONSE_OK));
    
    /* Close the window */
    gtk_window_close(GTK_WINDOW(dialog->dialog));
}

/* Cancel button clicked - close without saving */
static void on_cancel_clicked(GtkWidget *widget, gpointer user_data)
{
    (void)widget;
    SettingsDialog *dialog = (SettingsDialog *)user_data;
    
    /* Mark as CANCEL response */
    g_object_set_data(G_OBJECT(dialog->dialog), "response-id", GINT_TO_POINTER(GTK_RESPONSE_CANCEL));
    
    /* Close the window */
    gtk_window_close(GTK_WINDOW(dialog->dialog));
}

/* Show specific settings section */
void show_settings_section(SettingsDialog *dialog, SettingsSection section)
{
    if (!dialog) {
        return;
    }
    
    for (int i = 0; i < SETTINGS_COUNT; i++) {
        gtk_widget_set_visible(dialog->section_widgets[i], (i == section));
    }
}

/* Get settings dialog widget */
GtkWidget* get_settings_dialog_widget(SettingsDialog *dialog)
{
    if (!dialog) {
        return NULL;
    }
    
    return dialog->dialog;
}

/* Get settings from dialog - syncs all widget values to settings structure */
AppSettings* get_settings_from_dialog(SettingsDialog *dialog)
{
    if (!dialog || !dialog->settings || !dialog->widgets) {
        return NULL;
    }
    
    /* Sync all widget values to the settings structure */
    sync_widgets_to_settings((DialogWidgets *)dialog->widgets, dialog->settings);
    
    return dialog->settings;
}

/* Free settings dialog */
void free_settings_dialog(SettingsDialog *dialog)
{
    if (!dialog) {
        return;
    }
    
    if (dialog->widgets) {
        g_free(dialog->widgets);
    }
    
    g_free(dialog);
}
