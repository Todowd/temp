#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cstring>
#include "disk.h"

using namespace std;

/*
    super control block format (block 0 of the entire disk file):
    number of blocks (unsigned int), block size (unsigned int), number of partitions (unsigned short), partition start locations (up to 4, unsigned int)
    minimum usage of block 0: 10=4+4+2 bytes
    maximum usage of block 0: 26=4+4+2+(4*4) bytes
*/
#define INITIAL 10

Disk::Disk(int numblocks, int blksz, char *fname)
{
  blkCount = numblocks;
  diskSize = numblocks * blksz;
  blkSize = blksz;
  //diskFilename = strdup(fname);
  diskFilename=fname;
}

Disk::~Disk()
{
}

int Disk::initDisk()
{
  fstream f(diskFilename, ios::in);
  if (!f) {
    f.open(diskFilename, ios::out);
    if (!f) {
      cerr << "Error: Cannot create disk file" << endl;
      return(-1);
    }
    for (int i = 0; i < diskSize; i++) f.put('#');
    f.close();
    return(1);
  }
  f.close();
  return 0 ;
}

int Disk::readDiskBlock(int blknum, char *blkdata)
/*
  returns -1, if disk can't be opened;
  returns -2, if blknum is out of bounds;
  returns 0, if the block is successfully read;
*/
{
  if ((blknum < 0) || (blknum >= blkCount)) return(-2);
  ifstream f(diskFilename, ios::in);
  if (!f) return(-1);
  f.seekg(blknum * blkSize);
  f.read(blkdata, blkSize);
  f.close();
  return(0);
}

int Disk::writeDiskBlock(int blknum, char *blkdata)
/*
  returns -1, if DISK can't be opened;
  returns -2, if blknum is out of bounds;
  returns 0, if the block is successfully read;
*/
{
  if ((blknum < 0) || (blknum >= blkCount)) return(-2);
  fstream f(diskFilename, ios::in|ios::out);
  if (!f) return(-1);
  f.seekg(blknum * blkSize);
  f.write(blkdata, blkSize);
  f.close();
  return(0);
}
