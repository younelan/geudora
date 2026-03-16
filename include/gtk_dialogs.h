/* GTK4 Dialog System for gEudora - Header */

#ifndef GTK_DIALOGS_H
#define GTK_DIALOGS_H

#include <stdbool.h>

/* Alert dialog types (Mac compatibility) */
/* Note, Caution, Stop are often defined as macros elsewhere; use them as-is */
#ifndef Note
#define Note 0
#endif
#ifndef Caution
#define Caution 1
#endif
#ifndef Stop
#define Stop 2
#endif
typedef int AlertType;

/* Alert button return values */
#define kAlertStdAlertOKButton 1
#define kAlertStdAlertCancelButton 2
#define kAlertStdAlertOtherButton 3

/* Progress message flags - Use enum from progress.h instead of macros */

/* Show standard alert dialog */
short ComposeStdAlert(AlertType alertType, int msgResId, ...);
void AlertStr(short alertID, short type, const char *message);

/* Progress dialog functions - signatures aligned with progress.h to avoid
  conflicting declarations when both headers are included. */
int OpenProgress(void);
void ProgressMessage(short which, const char *message);
int Nag(int id, void *p, void *proc, void *filter, bool b, ...);
void ProgressMessageR(short which, short messageId);
void CloseProgress(void);

/* String resource functions */
char *GetRString(char *dest, short id);
// unsigned char* ComposeRString(unsigned char *dest, int resId, ...);
// bool EqualStrRes(const unsigned char *str, short resId);

/* Preference functions */
char *GetPref(char *dest, short prefId);
long GetPrefLong(short prefId);
void SetPrefLong(short prefId, long value);
void SetPref(int prefId, const char *value);
#undef PrefIsSet
bool PrefIsSet(short prefId);

#endif /* GTK_DIALOGS_H */
