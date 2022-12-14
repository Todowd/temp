#include <iostream>

#include "disk.h"
#include "diskmanager.h"
#include "partitionmanager.h"
#include "bitvector.h"
using namespace std;

/*
    partition control block format (block 0 of the partition):
    #of blocks  size of blocks  free block count    free block pointers
*/


PartitionManager::PartitionManager(DiskManager *dm, char partitionname, int partitionsize) {
  DM = dm;
  partitionName = partitionname;
  partitionSize = DM->getPartitionSize(partitionName);

  /* If needed, initialize bit vector to keep track of free and allocted
     blocks in this partition */
  char buffer[getBlockSize()];
  DM->readDiskBlock(partitionName, 0, buffer);
  bv = new BitVector(partitionSize); //Added the bitvector
  bool exists=false;
  for(int i=0; i<getBlockSize(); i++) {
    //partition was created, grab contents of BV info
    if(buffer[i]!='#') {
      //get partition info from disk
      bv->setBitVector((unsigned int*)buffer);
      exists=true;
      break;
    }
  }
  if(!exists) {//bit Vector was not created buffer full of # (empty DISK char)
    //partition doesnt exist create it
    //BV initialize to all zero, BV for all new partitions are the same
    //0th block of partition used by BV
    bv->setBit(0);
    //1st block of partition used by root dir
    bv->setBit(1);
    writeBV();
    //root dir starts empty so no need to add anything
  }
}

PartitionManager::~PartitionManager() {
  delete bv;
}

/*
 * return blocknum, -1 otherwise
 */
int PartitionManager::getFreeDiskBlock() {
  /* write the code for allocating a partition block */
  for(int i=2; i<partitionSize; i++) {
    //block isnt used
    if(bv->testBit(i)==0) {
      bv->setBit(i);
      if(writeBV()!=0) {
        return -1;
      }
      return i;
    }
  }
  return -1;
}

/*
 * return 0 for sucess, -1 otherwise
 */
int PartitionManager::returnDiskBlock(int blknum) {
  /* write the code for deallocating a partition block */
  //CHECK IF FILE OPEN???
  char buffer[getBlockSize()];
  for(int i=0; i<getBlockSize(); buffer[i++]='#') {}
  if(DM->writeDiskBlock(partitionName, blknum, buffer)==0) {
    bv->resetBit(blknum);
    return writeBV();
  }
  return -1;
}


int PartitionManager::readDiskBlock(int blknum, char *blkdata)
{
  return DM->readDiskBlock(partitionName, blknum, blkdata);
}

int PartitionManager::writeDiskBlock(int blknum, char *blkdata)
{
  return DM->writeDiskBlock(partitionName, blknum, blkdata);
}

int PartitionManager::getBlockSize()
{
  return DM->getBlockSize();
}

int PartitionManager::writeBV() {
  char buffer[getBlockSize()];
  for(int i=0; i<getBlockSize(); i++) {
      buffer[i]='#';
  }
  bv->getBitVector((unsigned int*)buffer);
  return DM->writeDiskBlock(partitionName, 0, buffer);
}
