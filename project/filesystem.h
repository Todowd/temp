#include<unordered_map>
#include<vector>


#ifndef FILESYSTEM_H
#define FILESYSTEM_H

struct open_file{
  int descriptor;
  int address;
  int rwPtr;
  char mode;
  char* fileName;
};

class FileSystem {
  DiskManager *DM;
  PartitionManager *PM;
  char fileSystemName;
  int fileSystemSize;
  /* declare other private members here */
  int descCounter;
  std::vector<open_file> open_files;

  public:
    FileSystem(DiskManager *dm, char name);
    int createFile(char *filename, int fLen);
    int createDirectory(char *dirname, int dLen);
    int lockFile(char *filename, int fLen);
    int unlockFile(char *filename, int fLen, int lockId);
    int deleteFile(char *filename, int fLen);
    int deleteDirectory(char *dirname, int dLen);
    int openFile(char *filename, int fLen, char mode, int lockId);
    int closeFile(int fileDesc);
    int readFile(int fileDesc, char *data, int len);
    int writeFile(int fileDesc, char *data, int len);
    int appendFile(int fileDesc, char *data, int len);
    int seekFile(int fileDesc, int offset, int flag);
    int truncFile(int fileDesc, int offset, int flag);
    int renameFile(char *filename1, int fLen1, char *filename2, int fLen2);
    int renameDirectory(char *dirname1, int dLen1, char *dirname2, int dLen2);
    int getAttribute(char *filename, int fLen, char type);
    int setAttribute(char *buffer, char type);

    /* declare other public members here */
    static bool validFilename(char* file, int len);

  protected:
    char* reset;
    int find(int start, char* name, int nLen, char type);
    static bool validFile(char* name, int len);
    int shiftFixFolder(int folder=1);
    int expandFolder(int loc);
    int findFolderEnd(int start);
    //int allocate(int dirAddress, char* dirName, int dLen);

};
#endif
