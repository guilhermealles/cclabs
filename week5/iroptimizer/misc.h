#ifndef MISC_H
#define MISC_H

/* set of miscellaneous helper functions */
void abortMessage(char *format, ...);
void *safeMalloc(int nbytes);
void *safeRealloc(void *ptr, int nbytes);
char *readFile(char *fnm);
char *stringDuplicate(char *str);
int areEqualStrings(char *s1, char *s2);

#endif
