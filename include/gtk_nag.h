/* Copyright (c) 2017, Computer History Museum
All rights reserved. */

#ifndef GTK_NAG_H
#define GTK_NAG_H

#include "featureldef.h"
#include <gtk/gtk.h>
#include <stdbool.h>

// Feature list builder - loads from GResource XML
FeatureCellHandle BuildFeatureList(bool ignoreUsage);

// Dialog API
void NotGettingAdsDialog(const char *errorText);
void JunkDownDialog(void);
int DowngradeDialog(FeatureCellHandle features);
void DownGradeDialog(void); // Simple wrapper
void FeaturesDialog(FeatureCellHandle features);
int RepayDialog(void);
void IntroDialog(void);
void RegisterDialog(void);

// Nag system API
void CheckNagging(int userState);
int UpdateCheck(bool silently, bool archives);
void FinishedUpdateCheck(long silently, int theError, void *info);
void TransitionState(int newState);
int InitNagging(void);
void DoPendingNagDialog(int pendingNagResult);

// Notification functions
void NotifyDownGradeDialog(void);

// Validation functions
bool UserHasValidPaidModeRegcode(void);
void CheckAdQT(void);
int AdFailureCheck(void *nagUsage, uint32_t currentTime, bool *nagMe,
                   int *dialogID, char *errString);

#endif
