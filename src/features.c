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

#include "features.h"
#include "gtk_nag.h"
#include "mydefs.h"
#include "mailbox.h"
#include <gtk/gtk.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <assert.h>
#include <time.h>

#define FILE_NUM 121

// Global feature list - standard C
FeatureRec* gFeatureList = NULL;
int gFeatureCount = 0;

// Helper functions
static bool IsFreeMode(void) {
    return !UserHasValidPaidModeRegcode();
}

static uint32_t GMTDateTime(void) {
    return (uint32_t)time(NULL);
}

#ifndef DEATH_BUILD

// Port to GTK: Load features from GResource XML instead of Mac resource fork
void RegisterProFeatures(void)
{
    // Load from embedded GResource
    GBytes *bytes = g_resources_lookup_data("/org/eudora/resources/features.xml",
                                            G_RESOURCE_LOOKUP_FLAGS_NONE, NULL);
    if (!bytes) {
        g_printerr("RegisterProFeatures: Failed to load features.xml from GResource\n");
        return;
    }
    
    gsize data_size;
    gconstpointer data = g_bytes_get_data(bytes, &data_size);
    
    xmlDocPtr doc = xmlReadMemory(data, data_size, "features.xml", NULL, 0);
    g_bytes_unref(bytes);
    
    if (!doc) {
        g_printerr("RegisterProFeatures: Failed to parse features.xml\n");
        return;
    }
    
    xmlNodePtr root = xmlDocGetRootElement(doc);
    if (!root || xmlStrcmp(root->name, (const xmlChar*)"features") != 0) {
        g_printerr("RegisterProFeatures: Invalid XML root\n");
        xmlFreeDoc(doc);
        return;
    }
    
    // Register dummy features
    RegisterFeature(featureNone, featureNone, featNoResource, false);
    RegisterFeature(featureEudoraPro, featureMatchAny, featNoResource, false);
    
    // Load features from XML
    short index = 0;
    for (xmlNodePtr node = root->children; node; node = node->next) {
        if (node->type != XML_ELEMENT_NODE || 
            xmlStrcmp(node->name, (const xmlChar*)"feature") != 0) {
            continue;
        }
        
        // Get 4-char ID
        xmlChar *idAttr = xmlGetProp(node, (const xmlChar*)"id");
        if (!idAttr) continue;
        
        uint32_t featureID = 0;
        if (xmlStrlen(idAttr) == 4) {
            featureID = ((uint32_t)idAttr[0] << 24) |
                       ((uint32_t)idAttr[1] << 16) |
                       ((uint32_t)idAttr[2] << 8) |
                        (uint32_t)idAttr[3];
        }
        xmlFree(idAttr);
        
        if (featureID) {
            RegisterFeature(featureID, featureMatchAny, 1000 + index, false);
            index++;
        }
    }
    
    xmlFreeDoc(doc);
    
    // Disable unsupported features
    DisableFeature(featureSimultaneousDirService);
    DisableFeature(featureClip2Phone);
}



//
//	RegisterFeature
//
//		Registers a feature and also enables it for use.  If you'd like Eudora to know about
//		a certain feature, but you do not want this feature to be used, you'll need to call
//		DisableFeature to turn it off.
//

void RegisterFeature (FeatureType primary, FeatureType sub, short resID, bool hasSubFeatures)
{
	FeatureRec		newFeature;
	
	// Registered features are present and enabled to start.
	newFeature.primary				= primary;
	newFeature.sub						= sub;
	newFeature.flags					= featurePresent | featureEnabled;
	newFeature.lastUsed				= 0;
	newFeature.resID					= resID;
	newFeature.hasSubFeatures	= hasSubFeatures;
	
	// Grow feature list with realloc
	FeatureRec* newList = realloc(gFeatureList, (gFeatureCount + 1) * sizeof(FeatureRec));
	if (newList) {
		gFeatureList = newList;
		gFeatureList[gFeatureCount] = newFeature;
		gFeatureCount++;
	}
}


//
//	UpdateFeatureUsage
//
//		- Read in the 'F U ' resource
//		- For each record...
//				...mark it used.  Simple!
//

void UpdateFeatureUsage (FeatureRec* featureSet)

{
	// Feature usage persistence is handled by GTK settings
	// This function is a no-op in the GTK port
}


//
//	SaveFeaturesWithExtemeProfanity
//
//		Create and save a 'F U ' resource to the Settings File
//

void SaveFeaturesWithExtemeProfanity (FeatureRec* featureSet)

{
	// Feature usage persistence is handled by GTK settings
	// This function is a no-op in the GTK port
}



//
//	HasFeature
//
//		Test to see whether or not a particular feature is present and enabled
//

Boolean	HasFeature (FeatureID feature)

{
#ifdef LIGHT
	return (false);
#else
	FeatureRecPtr	featurePtr;
	Boolean				hasFeature;

	hasFeature = false;
	// Simple right now since we don't downgrade on the fly...  Eventually we'll
	// really want to look at the featureEnabled bit (it's always on now).
	if (!IsFreeMode ())
		if ((featurePtr = FindFeatureID (gFeatureList, feature)))
			hasFeature = featurePtr->flags & featureEnabled ? true : false;
	return (hasFeature);
#endif
}


//
//	DisableFeature
//
//		Called to turn off a particular feature.
//

void DisableFeature (FeatureID feature)

{
	FeatureRecPtr	featurePtr;

	if (gFeatureList)
		if ((featurePtr = FindFeatureID (gFeatureList, feature)))
			featurePtr->flags &= ~featureEnabled;
}


//
//	EnableFeature
//
//		Called to turn on a particular feature.
//

void EnableFeature (FeatureID feature)

{
	FeatureRecPtr	featurePtr;

	if (gFeatureList)
		if ((featurePtr = FindFeatureID (gFeatureList, feature)))
			featurePtr->flags |= featureEnabled;
}


//
//	UseFeature
//
//		This routine should be called each time a feature is used to timestamp
//		the filter's usage information in memory.  Be careful, though... we don't
//		want to call this too frequently to the point of impacting performance.
//

void UseFeature (FeatureID feature)

{
	FeatureRecPtr	featurePtr;

	if (gFeatureList)
		if ((featurePtr = FindFeatureID (gFeatureList, feature))) {
			featurePtr->lastUsed = GMTDateTime ();
			NotifyDownGradeDialog ();
		}
}


//
//	UseFeatureType
//
//		Like UseFeature, only much less efficient!
//

void UseFeatureType (FeatureType feature)

{
	FeatureRecPtr	featurePtr;

	if (gFeatureList)
		if ((featurePtr = FindFeatureType (gFeatureList, feature, feature))) {
			featurePtr->lastUsed = GMTDateTime ();
			NotifyDownGradeDialog ();
		}
}

//
//	FindFeatureType
//
//		Find a particular feature record in a feature set, by passing in a feature type.
//		This is the slow and laborious way to search for a given feature.  Upon closer
//		examination, it's linear.  This routine is meant to only be used by the feature
//		manager, and even then probably shouldn't ever really be used.
//

FeatureRecPtr FindFeatureType (FeatureRec* featureSet, FeatureType primary, FeatureType sub)

{
	FeatureRecPtr	featurePtr;
	int					count;

	count = gFeatureCount;
	featurePtr = featureSet;
	while (count--) {
		if (primary == sub) {
			if (featurePtr->primary == primary || featurePtr->sub == primary)
				return (featurePtr);
		}
		else
			if (!primary || featurePtr->primary == primary)
				if (!sub || featurePtr->sub == sub)
					return (featurePtr);
		++featurePtr;
	}
	return (NULL);
}


//
//	FindFeatureID
//
//		Find a particular feature record in a feature set, by passing in an index.
//		This is a much faster way to search for a given feature.  This routine is
//		meant to only be used by the feature manager.
//

FeatureRecPtr FindFeatureID (FeatureRec* featureSet, FeatureID feature)

{
	return (feature < gFeatureCount ? featureSet + feature : NULL);
}

#define	SUBSTITUTE_ICON

void DisableMenuItemWithProOnlyIcon (GtkWidget* menu, short item)

{
	// GTK4 port: Menu item styling handled through CSS
	// This function is a no-op - menu features are controlled by HasFeature checks
}

//	Pass in enable:  -1 = do nothing  0 = disable  1 = enable

void SetMenuItemBasedOnFeature (GtkWidget* menu, short item, FeatureType feature, short enable)

{
	// GTK4 port: Use gtk_widget_set_sensitive() for menu items
	// enable: -1 = do nothing, 0 = disable, 1 = enable
	if (HasFeature(feature)) {
		if (enable >= 0) {
			// In GTK4, menu items are GtkWidgets - sensitivity controlled per item
			// Caller should use: gtk_widget_set_sensitive(menu_item, enable != 0)
		}
	}
	// Feature not available - item should be disabled by caller
}
#endif
