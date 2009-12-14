/* vim: set sw=4 sts=4 : */

int RemoveFileCallback(void *args, int numCols, char **results, char **columnNames);
int SearchPkgPrintCallback(void *args, int numCols, char **results, char **columnNames);
int SearchFilePrintCallback(void *args, int numCols, char **results, char **columnNames);
int ReturnSilentDataFromDB(void *args, int numCols, char **results, char **columnNames);
int SaveListOfLinks(void *args, int numCols, char **results, char **columnNames);
int GetFieldCallback(void *args, int numCols, char **results, char **columnNames);
int SavePackageListCallback(void *args, int numCols, char **results, char **columnNames);
