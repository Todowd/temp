#include "disk.h"

#ifndef DISK_MANAGER_H
#define DISK_MANAGER_H

using namespace std;

class DiskPartition {
  public:
  	DiskPartition(char nm='\0', int sz=0, int s=0, int e=0);
    char partitionName;
    int partitionSize;
    //add variables as needed to the data structure here.
    int partitionStart;
    int partitionEnd;
};

class DiskManager {
  Disk *myDisk;
  int partCount;
  DiskPartition *diskP;

  /* declare other private members here */

  public:
    DiskManager(Disk *d, int partCount, DiskPartition *dp);
    ~DiskManager();
    int readDiskBlock(char partitionname, int blknum, char *blkdata);
    int writeDiskBlock(char partitionname, int blknum, char *blkdata);
    int getBlockSize() {return myDisk->getBlockSize();};
    int getPartitionSize(char partitionname);
    /* declare other public members here  mine*/
    static void writeNumber(char* buffer, int num, int pos);
    static void writeName(char* buffer, char name, int pos);
    static void writeChars(char* buffer, char* toCpy, int pos, int len);
    static char readName(char* buffer, int pos);
    static int readNumber(char* buffer, int pos);

};
#endif
