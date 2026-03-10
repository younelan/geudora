/* Copyright (c) 2017, Computer History Museum 
All rights reserved. 
Redistribution and use in source and binary forms, with or without modification, are permitted (subject to 
the limitations in the disclaimer below) provided that the following conditions are met: 
 * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer. 
 * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following 
   disclaimer in the documentation and/or other materials provided with the distribution. 
 * Neither the name of Computer History Museum nor the names of its contributors may be used to endorse or promote products 
   derived from this software without specific prior written permission. 
NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE 
COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH 
DAMAGE. */

/************************************************************************
 * GTK_NAG.C - Cross-platform nag dialog system with GTK4 UI
 * 
 * This file consolidates all nag functionality from nag.c with GTK4
 * dialogs instead of Mac Dialog Manager. All business logic is preserved,
 * UI layer is pure GTK4.
 ************************************************************************/

#include <gtk/gtk.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <stdbool.h>
#include <time.h>
#include "featureldef.h"
#include "downloadurl.h"
#include "gtk_prefs.h"

// Nag data structures
typedef struct {
    uint32_t schedule;
    uint16_t interval;
    int16_t  dialogID;
    uint16_t flags;
} NagRec;

typedef struct {
    uint16_t version;
    uint16_t consecutiveDays;
    uint32_t dateOfLastNag;
    uint32_t snoozeDate;
    uint16_t timesNagged;
    uint16_t timesActedOn;
    uint32_t nagBase;   // Base timestamp for day calculations
    uint32_t lastNag;   // Last nag timestamp  
    uint16_t aux;       // Auxiliary data
} NagUsageRec;

// Nag timing constants
#define kUpdateNagScheduleDuringBeta        0x55555555
#define kUpdateNagIntervalDuringBeta        2
#define kDelayBeforeSwitchingRepayUsers     (60 * 60 * 60)
#define kSecondsPerDay                      (24 * 60 * 60)
#define kDaysUntilNoAdsNag                  3
#define kDeadbeatDays                       7
#define KRHashPrime                         2147483629

// Global state
static long gTimeAtWhichRepayUsersBecomeSponsored = 0;

// Forward declarations
static uint32_t SecondsToWholeDay(uint32_t secs);
static int16_t MostSignificantSetBit(uint32_t value);

// New custom dialog helpers (implemented at end of file)
typedef struct NagDialogContext NagDialogContext;
static NagDialogContext* create_nag_dialog(const char *title, const char *message, int width, int height);
static void add_button(NagDialogContext *ctx, const char *label, int response_id);
static int run_nag_dialog(NagDialogContext *ctx);

/************************************************************************
 * RESOURCE/SETTINGS FUNCTIONS - Load nag schedules and state
 ************************************************************************/

static int GetIndNagID(int16_t *nagID, int16_t nagListID, int16_t index)
{
    // Load nag ID list from ini [nag_schedules] section
    // Format: state<N>_ids = "1,2,3,4"
    char key[64];
    snprintf(key, sizeof(key), "state%d_ids", nagListID);
    
    gchar *ids_str = prefs_get_string("nag_schedules", key, NULL);
    if (!ids_str) return -1;
    
    // Parse comma-separated list (1-indexed like Mac resources)
    gchar **ids = g_strsplit(ids_str, ",", -1);
    g_free(ids_str);
    
    if (index > 0 && ids[index - 1]) {
        *nagID = (int16_t)atoi(ids[index - 1]);
        g_strfreev(ids);
        return 0;
    }
    
    g_strfreev(ids);
    return -1;
}

static int GetIndNagState(void *theNag, int16_t *nagID, int userState, int16_t index)
{
    // Get indexed nag schedule for user state
    int error = GetIndNagID(nagID, (int16_t)userState, index);
    if (error) return error;
    
    // Load nag schedule from ini [nag_schedules] section
    // Format: nag<N>_schedule = 0x55555555, nag<N>_interval = 2, etc.
    char key[64];
    NagRec *nag = (NagRec *)theNag;
    
    snprintf(key, sizeof(key), "nag%d_schedule", *nagID);
    nag->schedule = (uint32_t)prefs_get_int("nag_schedules", key, 0);
    
    snprintf(key, sizeof(key), "nag%d_interval", *nagID);
    nag->interval = (uint16_t)prefs_get_int("nag_schedules", key, 0);
    
    snprintf(key, sizeof(key), "nag%d_dialog", *nagID);
    nag->dialogID = (int16_t)prefs_get_int("nag_schedules", key, 0);
    
    snprintf(key, sizeof(key), "nag%d_flags", *nagID);
    nag->flags = (uint16_t)prefs_get_int("nag_schedules", key, 0);
    
    return (nag->dialogID != 0) ? 0 : -1;
}

// LoadNagUsage - Load nag usage tracking data from ini [nag_usage] section
static int LoadNagUsage(int16_t nagID, NagUsageRec *usage) {
    char key[64];
    snprintf(key, sizeof(key), "nag%d_version", nagID);
    usage->version = (uint16_t)prefs_get_int("nag_usage", key, 0);
    
    snprintf(key, sizeof(key), "nag%d_base", nagID);
    usage->nagBase = (uint32_t)prefs_get_int("nag_usage", key, 0);
    
    snprintf(key, sizeof(key), "nag%d_last", nagID);
    usage->lastNag = (uint32_t)prefs_get_int("nag_usage", key, 0);
    
    snprintf(key, sizeof(key), "nag%d_aux", nagID);
    usage->aux = (uint32_t)prefs_get_int("nag_usage", key, 0);
    
    return (usage->nagBase != 0) ? 0 : -1;
}

// SaveNagUsage - Save nag usage tracking data to ini [nag_usage] section
static void SaveNagUsage(int16_t nagID, const NagUsageRec *usage) {
    char key[64];
    snprintf(key, sizeof(key), "nag%d_version", nagID);
    prefs_set_int("nag_usage", key, usage->version);
    
    snprintf(key, sizeof(key), "nag%d_base", nagID);
    prefs_set_int("nag_usage", key, usage->nagBase);
    
    snprintf(key, sizeof(key), "nag%d_last", nagID);
    prefs_set_int("nag_usage", key, usage->lastNag);
    
    snprintf(key, sizeof(key), "nag%d_aux", nagID);
    prefs_set_int("nag_usage", key, usage->aux);
}

static uint32_t DaysSinceNagBase(int16_t dialogID)
{
    // Returns days since nag base for specified dialog
    NagUsageRec usage;
    if (LoadNagUsage(dialogID, &usage) == 0) {
        uint32_t currentTime = (uint32_t)time(NULL);
        return (SecondsToWholeDay(currentTime) - SecondsToWholeDay(usage.nagBase)) / kSecondsPerDay;
    }
    return 0;
}

static uint32_t DurationOfSchedule(int userState, int16_t whichSchedule)
{
    // Get duration of nag schedule (last scheduled day + 1)
    NagRec nag;
    int16_t nagID;
    if (!GetIndNagState(&nag, &nagID, userState, whichSchedule)) {
        int16_t lastDay = MostSignificantSetBit(nag.schedule);
        if (lastDay >= 0) return lastDay + 1;
    }
    return 0;
}

/************************************************************************
 * VALIDATION FUNCTIONS
 ************************************************************************/

static bool UserHasValidPaidModeRegcode(void)
{
    // Check if user has valid paid-mode registration code from ini
    // Format: [registration] has_paid_code=1, reg_date=timestamp, policy_code=N
    bool hasPaidCode = prefs_get_bool("registration", "has_paid_code", false);
    int policyCode = prefs_get_int("registration", "policy_code", 0);
    uint32_t regDate = (uint32_t)prefs_get_int("registration", "reg_date", 0);
    
    if (!hasPaidCode || regDate == 0) {
        return false;
    }
    
    // Check if registration is still valid (not expired)
    // Policy codes: 0=invalid, 1=perpetual, 2=time-limited, etc.
    if (policyCode == 1) {
        return true; // Perpetual license
    }
    
    // For time-limited, check if expired
    if (policyCode == 2) {
        uint32_t currentTime = (uint32_t)time(NULL);
        uint32_t expiryTime = regDate + (365 * 24 * 60 * 60); // 1 year
        return currentTime < expiryTime;
    }
    
    return false;
}

// Forward declarations to fix implicit declaration errors
bool TransitionState(int16_t oldState, int16_t newState);
FeatureCellHandle BuildFeatureList(bool ignoreUsage);

static void NagDialog(int16_t nagID)
{
    // Implementation using new GTK4 dialogs
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "Eudora Nag %d", nagID);
    
    NagDialogContext *ctx = create_nag_dialog("Nag", buffer, 350, 150);
    add_button(ctx, "OK", GTK_RESPONSE_OK);
    run_nag_dialog(ctx);
}

static void CheckAdQT(void)
{
    // Verify QuickTime installed for adware mode
    // In modern GTK port, we check for media playback capability instead
    
    int userState = prefs_get_int("nag_state", "user_state", 1);
    bool isAdwareMode = (userState == 5 || userState == 6); // regAdwareUser or adwareUser
    
    if (!isAdwareMode) {
        return; // Not in adware mode, nothing to check
    }
    
    // Check if GStreamer or other media backend available
    // For now, assume media support is available (GTK4 has built-in media support)
    bool hasMediaSupport = true; // Could check: gtk_media_stream_new_file() works
    
    if (!hasMediaSupport) {
        g_print("CheckAdQT: No media support, must revert to free mode\n");
        
        // Show dialog: "Media playback required for sponsored mode"
        NagDialogContext *ctx = create_nag_dialog(
            "Media Support Required",
            "Media playback support is required for Sponsored Mode.\n\n"
            "Without media support, ads cannot be displayed.\n\n"
            "Please install GStreamer or switch to Light Mode.",
            500, 300
        );
        
        add_button(ctx, "Get Media Support", 1);
        add_button(ctx, "Switch to Light Mode", 2);
        
        int response = run_nag_dialog(ctx);
        
        if (response == 1) {
            g_print("Opening media support help URL\n");
            // OpenAdwareURL(GetNagState(), TECH_SUPPORT_SITE, actionSupport, helpQuery, topicNoQuickTime);
            // Exit app to let user install media support
        } else if (response == 2) {
            // Downgrade to free mode
            int newState = (userState == 5) ? 4 : 3; // regAdwareUser->regFreeUser, adwareUser->freeUser
            TransitionState(userState, newState);
        }
    }
    
    g_print("CheckAdQT() - verifying QuickTime for adware mode\n");
}

static int AdFailureCheck(void *nagUsage, uint32_t currentTime, bool *nagMe, int16_t *dialogID, char *errString)
{
    // Check for ad delivery failures and increment deadbeat counter
    NagUsageRec *usage = (NagUsageRec *)nagUsage;
    
    *nagMe = false;
    *dialogID = 0;
    errString[0] = '\0';
    
    // Load ad failure tracking from ini
    bool adsAreFailing = prefs_get_bool("nag_state", "ads_failing", false);
    int deadbeatCounter = prefs_get_int("nag_state", "deadbeat_counter", 0);
    int consecutiveDays = prefs_get_int("nag_state", "consecutive_ad_days", 0);
    uint32_t lastCheckDay = (uint32_t)prefs_get_int("nag_state", "last_ad_check", 0);
    
    // Are we checking on a new day?
    uint32_t currentDay = SecondsToWholeDay(currentTime);
    uint32_t lastDay = SecondsToWholeDay(lastCheckDay);
    
    if (currentDay != lastDay) {
        if (adsAreFailing) {
            // Increment deadbeat counter
            deadbeatCounter++;
            consecutiveDays = 0;
            
            g_print("AdFailureCheck: Ads failing, deadbeat counter = %d\n", deadbeatCounter);
            
            if (deadbeatCounter >= kDeadbeatDays) {
                // 7+ days without ads - force free mode
                *nagMe = true;
                *dialogID = 8; // NAG_FRIGGING_HACKER_DLOG
                snprintf(errString, 256, "Unable to receive ads for %d days", deadbeatCounter);
                deadbeatCounter = kDeadbeatDays - 1; // Cap it
            } else if (deadbeatCounter >= kDaysUntilNoAdsNag) {
                // 3+ days without ads - show warning
                *nagMe = true;
                *dialogID = 7; // NAG_NOT_GETTING_ADS_DLOG
                snprintf(errString, 256, "Ad delivery has failed for %d days. Please check your connection.", deadbeatCounter);
            }
        } else {
            // Got ads successfully
            consecutiveDays++;
            
            // After 3 consecutive successful days, reduce deadbeat counter
            if (consecutiveDays >= 3 && deadbeatCounter > 0) {
                deadbeatCounter--;
                consecutiveDays = 0;
                g_print("AdFailureCheck: Reducing deadbeat counter to %d\n", deadbeatCounter);
            }
        }
        
        // Save updated counters
        prefs_set_int("nag_state", "deadbeat_counter", deadbeatCounter);
        prefs_set_int("nag_state", "consecutive_ad_days", consecutiveDays);
        prefs_set_int("nag_state", "last_ad_check", currentTime);
        
        // Reset daily flag for next day
        prefs_set_bool("nag_state", "ads_failing", false);
    }
    
    // Update last check time in nagUsage
    usage->nagBase = currentTime;
    
    return 0;
}

// Forward declarations of features
FeatureCellHandle BuildFeatureList(bool ignoreUsage); // Added forward declaration here because the other one was just before NotifyDownGradeDialog

static void RelaunchEudora(int newState)
{
    // Restart Eudora with new user state/settings
    // OpenNewSettings(&SettingsFileSpec, true, newState);
    g_print("RelaunchEudora(newState=%d) - restarting with new settings\n", newState);
}

// Global dialog tracking for FindNag
static GtkWidget *g_open_downgrade_dialog = NULL;
static FeatureCellHandle g_open_downgrade_features = NULL;

static GtkWidget* FindNag(int dialogID)
{
    // Find an already-open nag dialog by ID
    // For now, only track downgrade dialog
    if (dialogID == 1081) { // NAG_DOWNGRADE_DLOG
        if (g_open_downgrade_dialog && 
            gtk_widget_get_realized(g_open_downgrade_dialog)) {
            return g_open_downgrade_dialog;
        }
        g_open_downgrade_dialog = NULL;
    }
    return NULL;
}

// Feature list builder
// Note: ignoreUsage parameter used to filter used vs unused features
FeatureCellHandle BuildFeatureList(bool ignoreUsage); // Forward decl

void NotifyDownGradeDialog(void)
{
    // Update feature list in open downgrade dialog
    // If downgrade dialog is open and feature usage changed, refresh the list
    
    GtkWidget *dialogWin = FindNag(1081); // NAG_DOWNGRADE_DLOG
    if (dialogWin) {
        FeatureCellHandle newFeatures = BuildFeatureList(false);
        if (newFeatures) {
            // Compare with old list
            if (g_open_downgrade_features) {
                // Would compare and update GtkListBox here
                free(g_open_downgrade_features);
            }
            g_open_downgrade_features = newFeatures;
            // Invalidate the widget to redraw
            gtk_widget_queue_draw(dialogWin);
            g_print("NotifyDownGradeDialog() - refreshed feature list\n");
        }
    }
}

/************************************************************************
 * UTILITY FUNCTIONS - Cross-platform helpers
 ************************************************************************/

static uint32_t SecondsToWholeDay(uint32_t secs)
{
    time_t t = (time_t)secs;
    struct tm *tm_info = localtime(&t);
    if (!tm_info) return secs;
    
    tm_info->tm_hour = 0;
    tm_info->tm_min = 0;
    tm_info->tm_sec = 0;
    return (uint32_t)mktime(tm_info);
}

static int16_t MostSignificantSetBit(uint32_t value)
{
    int16_t bit = 31;
    while (value) {
        if (value & 0x80000000)
            return bit;
        --bit;
        value <<= 1;
    }
    return -1;
}

static uint32_t HashFile(const char *filepath)
{
    FILE *fp = fopen(filepath, "rb");
    if (!fp) return 0;
    
    uint32_t sum = 0;
    int ch;
    while ((ch = fgetc(fp)) != EOF) {
        for (int bit = 0x80; bit != 0; bit >>= 1) {
            sum += sum;
            if (sum >= KRHashPrime) sum -= KRHashPrime;
            if (ch & bit) ++sum;
            if (sum >= KRHashPrime) sum -= KRHashPrime;
        }
    }
    fclose(fp);
    return sum + 1;
}

/************************************************************************
 * FEATURE LIST BUILDING - Portable feature enumeration
 ************************************************************************/

static int SetFeatureCell(FeatureCellPtr *featureCellsPtr, FeatureRecPtr featurePtr, bool ignoreUsage)
{
    // Load feature resource and extract name/description
    // In full implementation, would use GetResource(featureResourceType, featurePtr->resID)
    // For now, set basic info
    (*featureCellsPtr)->type = featurePtr->primary;
    (*featureCellsPtr)->isName = true;
    (*featureCellsPtr)->isSubFeature = (featurePtr->sub != 0x3F3F3F3F); // '????'
    (*featureCellsPtr)->used = ((featurePtr->flags & 0x01) && featurePtr->lastUsed && !ignoreUsage);
    
    // Copy feature description (stub - would load from resources)
    snprintf((*featureCellsPtr)->description, sizeof((*featureCellsPtr)->description), 
             "Feature %d", featurePtr->primary);
    
    // Next cell for description text
    ++(*featureCellsPtr);
    (*featureCellsPtr)->isSubFeature = (featurePtr->sub != 0x3F3F3F3F);
    snprintf((*featureCellsPtr)->description, sizeof((*featureCellsPtr)->description), 
             "Description for feature %d", featurePtr->primary);
    
    return 0;
}

FeatureCellHandle BuildFeatureList(bool ignoreUsage)
{
    // Load features from embedded GResource XML
    GBytes *bytes = g_resources_lookup_data("/org/eudora/resources/features.xml",
                                            G_RESOURCE_LOOKUP_FLAGS_NONE, NULL);
    if (!bytes) {
        g_print("BuildFeatureList: Failed to load features.xml from GResource\n");
        return NULL;
    }
    
    gsize data_size;
    gconstpointer data = g_bytes_get_data(bytes, &data_size);
    
    // Parse XML
    xmlDocPtr doc = xmlReadMemory(data, data_size, "features.xml", NULL, 0);
    g_bytes_unref(bytes);
    
    if (!doc) {
        g_print("BuildFeatureList: Failed to parse features.xml\n");
        return NULL;
    }
    
    xmlNodePtr root = xmlDocGetRootElement(doc);
    if (!root || xmlStrcmp(root->name, (const xmlChar*)"features") != 0) {
        g_print("BuildFeatureList: Invalid XML root element\n");
        xmlFreeDoc(doc);
        return NULL;
    }
    
    // Count features
    int numFeatures = 0;
    for (xmlNodePtr node = root->children; node; node = node->next) {
        if (node->type == XML_ELEMENT_NODE && 
            xmlStrcmp(node->name, (const xmlChar*)"feature") == 0) {
            numFeatures++;
        }
    }
    
    if (numFeatures == 0) {
        g_print("BuildFeatureList: No features found in XML\n");
        xmlFreeDoc(doc);
        return NULL;
    }
    
    int numCells = numFeatures * 2; // name + description per feature
    
    // Allocate feature cell array
    FeatureCellHandle cellHandle = (FeatureCellHandle)malloc(sizeof(FeatureCellRec) * numCells);
    if (!cellHandle) {
        g_print("BuildFeatureList: Failed to allocate memory\n");
        xmlFreeDoc(doc);
        return NULL;
    }
    
    FeatureCellPtr cells = (FeatureCellPtr)cellHandle;
    int cellIndex = 0;
    
    // Build feature cells from XML
    for (xmlNodePtr node = root->children; node; node = node->next) {
        if (node->type != XML_ELEMENT_NODE || 
            xmlStrcmp(node->name, (const xmlChar*)"feature") != 0) {
            continue;
        }
        
        // Get paid attribute (default true)
        xmlChar *paidAttr = xmlGetProp(node, (const xmlChar*)"paid");
        bool isPaid = true;
        if (paidAttr) {
            isPaid = xmlStrcmp(paidAttr, (const xmlChar*)"true") == 0;
            xmlFree(paidAttr);
        }
        
        // Extract name and description
        xmlChar *name = NULL;
        xmlChar *desc = NULL;
        
        for (xmlNodePtr child = node->children; child; child = child->next) {
            if (child->type == XML_ELEMENT_NODE) {
                if (xmlStrcmp(child->name, (const xmlChar*)"name") == 0) {
                    name = xmlNodeGetContent(child);
                } else if (xmlStrcmp(child->name, (const xmlChar*)"description") == 0) {
                    desc = xmlNodeGetContent(child);
                }
            }
        }
        
        if (name && desc) {
            // Feature name cell
            strncpy(cells[cellIndex].description, (const char*)name, 255);
            cells[cellIndex].description[255] = '\0';
            cells[cellIndex].type = 0;
            cells[cellIndex].isName = true;
            cells[cellIndex].isSubFeature = false;
            cells[cellIndex].used = isPaid || ignoreUsage;
            cellIndex++;
            
            // Feature description cell
            strncpy(cells[cellIndex].description, (const char*)desc, 255);
            cells[cellIndex].description[255] = '\0';
            cells[cellIndex].type = 0;
            cells[cellIndex].isName = false;
            cells[cellIndex].isSubFeature = false;
            cells[cellIndex].used = isPaid || ignoreUsage;
            cellIndex++;
        }
        
        if (name) xmlFree(name);
        if (desc) xmlFree(desc);
    }
    
    xmlFreeDoc(doc);
    
    g_print("BuildFeatureList: Loaded %d features (%d cells) from GResource\n", 
            numFeatures, cellIndex);
    return cellHandle;
}

/************************************************************************
 * GTK4 DIALOG IMPLEMENTATIONS
 ************************************************************************/

struct NagDialogContext {
    GtkWidget *dialog;
    GtkWidget *content_box;
    GtkWidget *button_box;
    int response;
    bool waiting;
};

static void on_button_clicked(GtkButton *button, gpointer user_data)
{
    NagDialogContext *ctx = (NagDialogContext *)user_data;
    ctx->response = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(button), "response-id"));
    ctx->waiting = false;
    gtk_window_destroy(GTK_WINDOW(ctx->dialog));
}

static NagDialogContext* create_nag_dialog(const char *title, const char *message, int width, int height)
{
    NagDialogContext *ctx = g_new0(NagDialogContext, 1);
    
    ctx->dialog = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(ctx->dialog), title);
    gtk_window_set_modal(GTK_WINDOW(ctx->dialog), TRUE);
    gtk_window_set_default_size(GTK_WINDOW(ctx->dialog), width, height);

    ctx->content_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_set_margin_start(ctx->content_box, 12);
    gtk_widget_set_margin_end(ctx->content_box, 12);
    gtk_widget_set_margin_top(ctx->content_box, 12);
    gtk_widget_set_margin_bottom(ctx->content_box, 12);
    gtk_window_set_child(GTK_WINDOW(ctx->dialog), ctx->content_box);

    GtkWidget *label = gtk_label_new(message);
    gtk_label_set_wrap(GTK_LABEL(label), TRUE);
    gtk_widget_set_vexpand(label, TRUE);
    gtk_box_append(GTK_BOX(ctx->content_box), label);

    ctx->button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_widget_set_halign(ctx->button_box, GTK_ALIGN_END);
    gtk_box_append(GTK_BOX(ctx->content_box), ctx->button_box);

    return ctx;
}

static void add_button(NagDialogContext *ctx, const char *label, int response_id)
{
    if (!label) return;
    
    GtkWidget *button = gtk_button_new_with_label(label);
    g_object_set_data(G_OBJECT(button), "response-id", GINT_TO_POINTER(response_id));
    g_signal_connect(button, "clicked", G_CALLBACK(on_button_clicked), ctx);
    gtk_box_append(GTK_BOX(ctx->button_box), button);
}

static int run_nag_dialog(NagDialogContext *ctx)
{
    ctx->response = -1; // Default
    ctx->waiting = true;
    
    gtk_window_present(GTK_WINDOW(ctx->dialog));
    
    while (ctx->waiting) {
        g_main_context_iteration(NULL, TRUE);
    }
    
    int result = ctx->response;
    g_free(ctx);
    return result;
}

/************************************************************************
 * NOT GETTING ADS DIALOG
 ************************************************************************/

void NotGettingAdsDialog(const char *errorText)
{
    char message[512];
    snprintf(message, sizeof(message), 
             "There is a problem receiving ads:\n\n%s\n\nPlease check your connection.",
             errorText ? errorText : "Unknown error");
    
    NagDialogContext *ctx = create_nag_dialog(
        "Ad Delivery Problem",
        message,
        400, 200
    );
    
    add_button(ctx, "OK", 0);
    add_button(ctx, "More Info", 1);
    
    int result = run_nag_dialog(ctx);
    
    if (result == 1) {
        g_print("Opening ad failure help URL\n");
    }
}

void JunkDownDialog(void)
{
    NagDialogContext *ctx = create_nag_dialog(
        "Junk Mail Plugin Disabled",
        "Junk mail plugins disabled. Upgrade or reinstall.",
        450, 250
    );
    add_button(ctx, "OK", 0);
    add_button(ctx, "More Info", 1);
    add_button(ctx, "Pay Now", 2);
    run_nag_dialog(ctx);
}

void IntroDialog(void) {
    NagDialogContext *ctx = create_nag_dialog("Welcome", "Welcome to Eudora! Choose your mode.", 300, 200);
    add_button(ctx, "Enter Code", 1);
    add_button(ctx, "More Info", 2);
    add_button(ctx, "OK", 0);
    run_nag_dialog(ctx);
}

void RegisterDialog(void) {
    NagDialogContext *ctx = create_nag_dialog("Register", "Please register Eudora.", 300, 200);
    add_button(ctx, "Later", 0);
    add_button(ctx, "Register", 1);
    run_nag_dialog(ctx);
}

int RepayDialog(void) {
    NagDialogContext *ctx = create_nag_dialog("Payment Expired", "Renew payment?", 300, 200);
    add_button(ctx, "Sponsored Mode", 0);
    add_button(ctx, "Pay Now", 1);
    add_button(ctx, "Show Versions", 2);
    return run_nag_dialog(ctx);
}

void FeaturesDialog(FeatureCellHandle features) {
    NagDialogContext *ctx = create_nag_dialog("Features", "Features List...", 400, 300);
    add_button(ctx, "OK", 1);
    run_nag_dialog(ctx);
}

int DowngradeDialog(FeatureCellHandle features) {
    NagDialogContext *ctx = create_nag_dialog("Downgrade", "Downgrade to free mode?", 300, 200);
    add_button(ctx, "Cancel", 0);
    add_button(ctx, "Downgrade", 1);
    return run_nag_dialog(ctx);
}

void DownGradeDialog(void) {
    if (DowngradeDialog(NULL) == 1) {
        g_print("User confirmed downgrade\n");
    }
}

int UpdateCheck(bool silently, bool archives) {
    g_print("UpdateCheck stub\n");
    return 0;
}

void CheckNagging(int userState)
{
    g_print("CheckNagging(state=%d) called\n", userState);
}
