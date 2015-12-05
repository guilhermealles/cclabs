#ifndef SCANNER_DEFINITIONS
#define SCANNER_DEFINITIONS

void parseScannerDefinitions(FILE *file);
void initializeScannerDefinitions();
intSet parseDefinition(char *buffer, unsigned int buffer_size);
void printScannerDefinitions();

#endif
