#include<iostream>
#include<fstream>

#include "disk.h"
#include "diskmanager.h"
#include "bitvector.h"

using namespace std;

//Super Block Offset
#define SBO 13

DiskPartition::DiskPartition(char nm, int sz, int s, int e):
partitionName(nm), partitionSize(sz), partitionStart(s), partitionEnd(e) {}

DiskManager::DiskManager(Disk *d, int partcount, DiskPartition *dp)
{
  myDisk = d;
  partCount= partcount;
  int r = myDisk->initDisk();
  //char buffer[64];

  /* If needed, initialize the disk to keep partition information */
  /* else  read back the partition information from the DISK1 */
  char temp[d->blkSize];
  d->readDiskBlock(0, temp);
  if(r==1)
  {
    //initialize
    diskP = dp;
    int start = 1;

    for(int i = 0; i < partCount; i++) //Initing superblock
    {
      int partSize = dp[i].partitionSize;
      char partName = dp[i].partitionName;
      int end = (start + partSize);

      //Writes
      writeName(temp, partName, i * SBO); //Pos 0 is name
      writeNumber(temp, partSize, (i*SBO)+1); //Pos 1 is the size of the partition
      writeNumber(temp, start, (i*SBO)+5); //Pos 5 is the start block
      writeNumber(temp, end, (i*SBO)+9);
      dp[i].partitionStart=start;
      dp[i].partitionEnd=end;

      start += partSize; //Conitinuous allocation model?
    }
    d->writeDiskBlock(0, temp);
  }
  else
  {
  	//if partCount exists as zero
  	//diskP=new DiskPartition[4];
    diskP=new DiskPartition[partCount];
    //read the data and set the parameters
    //if partCount is zero because array is empty just check all the spots in the
    //super block. This may happen outside of the given drivers
    //for(int i=0; temp[i*SBO]!='#'; i++) {
    for(int i=0; i<partCount; i++) {
      //ignore the params passed, as the disk is always correct if already set
      diskP[i].partitionName=readName(temp, i*SBO);
      diskP[i].partitionSize=readNumber(temp, (i*SBO)+1);
      diskP[i].partitionStart=readNumber(temp, (i*SBO)+5);
      diskP[i].partitionEnd=readNumber(temp, (i*SBO)+9);
    }
  }
}

/*
 *   returns:
 *   0, if the block is successfully read;
 *  -1, if disk can't be opened; (same as disk)
 *  -2, if blknum is out of bounds; (same as disk)
 *  -3 if partition doesn't exist
 */
int DiskManager::readDiskBlock(char partitionname, int blknum, char *blkdata)
{
  /* write the code for reading a disk block from a partition */
    // blknum OOB check
    if ((blknum < 0) || (blknum >= myDisk->blkCount)) return(-2);
    // Partition check
//    bool found = false;
    for(int i = 0; i < partCount; i++)
    {
      if (diskP[i].partitionName == partitionname)
      {
//        found = true;
        //checking bounds within partition location within DISK
        if(((blknum+diskP[i].partitionStart)>diskP[i].partitionEnd)) {
          return -2;
        }
        //returning status value from Disk::readDiskBlock
        return myDisk->readDiskBlock(diskP[i].partitionStart+blknum, blkdata);
      }
    }
//    if (!found) return(-3);
    return -3;
}

/*
 *   returns:
 *   0, if the block is successfully written;
 *  -1, if disk can't be opened; (same as disk)
 *  -2, if blknum is out of bounds;  (same as disk)
 *  -3 if partition doesn't exist
 */
 //blknum is relative to the partition in the disk (blknum needs offset to write to DISK)
int DiskManager::writeDiskBlock(char partitionname, int blknum, char *blkdata) {
  /* write the code for writing a disk block to a partition */
  //check bounds relative to max blk count, and is within lower partition bounds
  if((blknum<0)||(blknum>=myDisk->blkCount)) {
    return -2;
  }
  for(int i=0; i<partCount; i++) {
    if(diskP[i].partitionName==partitionname) {
      //check bounds, doing here because blknum done by relation to partition
      if(((blknum+diskP[i].partitionStart)>diskP[i].partitionEnd)) {
        return -2;
      }
      //partition start +1 because cannot
      return myDisk->writeDiskBlock(diskP[i].partitionStart+blknum, blkdata);
    }
  }
  return -3; //place holder so there is no warnings when compiling.
}

/*
 * return size of partition
 * -1 if partition doesn't exist.
 */
int DiskManager::getPartitionSize(char partitionname)
{
  /* write the code for returning partition size */
  for(int i = 0; i < partCount; i++)
  {
    if (diskP[i].partitionName == partitionname)
    {
      return diskP[i].partitionSize;
    }
  }
  return -1; //place holder so there is no warnings when compiling.
}

//static
void DiskManager::writeNumber(char* buffer, int num, int pos)
{
    for(int i=0; i<4; i++) {
        int tmp=1;
        for(int k=0; k<=i; k++) {
            tmp*=10;
        }
        int place=tmp;
        tmp=(num%tmp);
        num-=tmp;
        tmp/=place/10;
        buffer[(4-i)+pos-1]=(char)(tmp+48);
    }
}

//static
void DiskManager::writeName(char* buffer, char name, int pos)
{
  buffer[pos] = name;
}

//static
void DiskManager::writeChars(char* buffer, char* toCpy, int pos, int len) {
  for(int i=pos; i<len; i++) {
    buffer[pos]=toCpy[i-pos];
  }
  return;
}

//static
int DiskManager::readNumber(char* buffer, int pos) {
    int result=0;
    for(int i=0; i<4; i++) {
        int tmp=((int)buffer[pos+i])-48;
        for(int k=0; k<(3-i); k++) {
            tmp*=10;
        }
        result+=tmp;
    }
    return result;
}

//static
char DiskManager::readName(char* buffer, int pos) {
	return buffer[pos];
}
