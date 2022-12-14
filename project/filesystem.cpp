#include <time.h>
#include <cstdlib>
#include <iostream>
#include <string>
#include <unistd.h>

#include "disk.h"
#include "diskmanager.h"
#include "partitionmanager.h"
#include "filesystem.h"

#define OFFSET 4          // address size
#define lockSetAddr 28    // Position in inode where lock is stored, if 1 file is locked else file is unlocked
#define lockIdAddr 29     // Position where the 2 digit random number that is the lockId required to unlock the file is stored in the inode
#define createdAddr 33    //Position where the created time string is stored
#define modifyAddr 47     //Position where the modified time string is stored

using namespace std;

// Buildtools was the github windows desktop application default name, but really was Tyler
// HELPER FUNCTIONS

/*
buffer[i]=filename
buffer[i+1:i+4]=ptr
buffer[i+5]=type
buffer[61:64]=continuation
*/

//This function may seem insignificant to you...but it means a lot to me and therefore it will live in my heart and in this file by green
//It is never called, but let me tell you...it was at one point. And when it was this project didnt work. So...yeah. Anyways, I loved it so
//look at it, realize that I did something I never had to do in the first place, laugh at me, and then call me stupid. But it is here for your
//enjoyment, professor and group members alike. Thank you.
// int FileSystem::allocate(int dirAddress, char* dirName, int dLen)
// {
//   /*
//   Params:
//     dirAddress: address of the parent inode where we need to write the newly allocated block to
//     dirName : name of the child directory we are allocating a block for
//     dLen : lenght of the dirName
//   */
//   //Reading in the parent inode
//   char parentInode[DM->getBlockSize()];
//   PM->readDiskBlock(dirAddress, parentInode);
//   //cout << "Made it here" << endl;
//   //Find the position in the parent where we need to write the allocated block
//   int position;
//   for(int i = 0; i < DM->getBlockSize(); i+=6)
//   {
//     //cout << "Made it here " << endl;
//     if(i >= 60)
//     {

//       //Read the address of the continuation block
//       dirAddress = DiskManager::readNumber(parentInode, 60);
//       //Read in the continuation block to the parentInode buffer
//       PM->readDiskBlock(dirAddress, parentInode);

//       //Reset the i var so we keep looping
//       i = 0;
//     }
//     //cout << "Made it here" << endl;
//     if(parentInode[i] == dirName[dLen - 1] && parentInode[i+5] == 'D')
//     {
//       //We have found the directory that we need to allocated a block for and the pos
//       position = i + 1; //+ 1 because we need the pos after the name

//       break;
//     }
//   }

//   //Now that we have the pos we can get the new disk block
//   int newBlock = PM->getFreeDiskBlock();
//   if(newBlock < 0)
//   {
//     PM->returnDiskBlock(newBlock);
//     return -1;
//   }

//   //Write the new address to the parent inode
//   DiskManager::writeNumber(parentInode, newBlock, position);

//   //Write out the inode to the disk
//   PM->writeDiskBlock(dirAddress, parentInode);

//   //Return the address of the newblock we have allocated
//   return newBlock;
// }

bool FileSystem::validFilename(char *file, int len)
{
  for (int i = 0; i < len; i++)
  {
    // if even char, will be filename/dir
    // starting at 0%2==0 start char is '/'
    if (i % 2)
    {
      // check if valid filename
      if (!(((file[i] >= 65) && (file[i] <= 90)) || ((file[i] >= 97) && (file[i] <= 122))))
      {
        return false;
      }
    }
    else
    {
      if (file[i] != '/')
      {
        return false;
      }
    }
  }
  return true;
}

int FileSystem::find(int start, char *name, int nLen, char type)
{
  // check file name
  if(nLen%2==1) {
    return -1;
  }
  if ((!(((name[1] >= 65) && (name[1] <= 90)) || ((name[1] >= 97) && (name[1] <= 122)))) || (name[0] != '/'))
  {
    return -1;
  }

  char buffer[DM->getBlockSize()];
  int status = PM->readDiskBlock(start, buffer);
  if (status < 0)
  {
    return -3;
  }
  // looking for the final file/folder
  if (nLen <= 2)
  {
    for (int i = 0; i < DM->getBlockSize(); i += 6)
    {
      // found it
      if ((name[1] == buffer[i]) && buffer[i + 5] == type)
      {
        return DiskManager::readNumber(buffer, i + 1);
      }
      // checking continuation block
      if ((i == DM->getBlockSize() - 4) && (buffer[i] != '#'))
      {
        // enter the continuation block
        start = DiskManager::readNumber(buffer, i);
        status = PM->readDiskBlock(start, buffer);
        if (status < 0)
        {
          return -3;
        }
        i = -6;
      }
      // not in last spot, no continuation block, doesnt exist
      if (i == -6)
      {
        if (buffer[0] == '#')
        {
          return -2;
        }
      }
      else if (buffer[i] == '#')
      {
        return -2;
      }
    }
    // couldnt find it
    return -2;
  }
  // need to climb through directories;
  for (int i = 0; i < DM->getBlockSize(); i += 6)
  {
    // found it, climb into that sub-dir
    if ((name[1] == buffer[i]) && buffer[i + 5] == 'D')
    {
      start = DiskManager::readNumber(buffer, i + 1);
      //      return find(start, name+2, nLen-2, type);
      // name=name+2;
      name++;
      name++;
      int tmp= find(start, name, nLen - 2, type);
      return tmp;
    }
    // checking continuation block
    if ((i == DM->getBlockSize() - 4) && (buffer[i + 1] != '#'))
    {
      // enter the continuation block
      status = PM->readDiskBlock(DiskManager::readNumber(buffer, i), buffer);
      if (status < 0)
      {
        return -3;
      }
      i = -6;
    }
    // not in last spot, no continuation block, doesnt exist
    if (i == -6)
    {
      if (buffer[0] == '#')
      {
        return -5;
      }
    }
    else if (buffer[i] == '#')
    {
      return -5;
    }
  }
  // couldnt find it
  return -2;
};

int FileSystem::shiftFixFolder(int folder) {
  if(folder<1) {
    return -2;
  }
  int next=folder;
  int blkSize=DM->getBlockSize();
  char buffer[blkSize];
  char ahead[blkSize];
  int status=PM->readDiskBlock(folder, buffer);
  char reset[blkSize];
  for(int i=0; i<blkSize; reset[i++]='#') {}
  if(status<0) {
    return -1;
  }
  //shift the inode info
  for(int i=0; i<blkSize; i+=6) {
    //the inode before the next block (do we need to look ahead to get the next inode?
    if(i==(blkSize-((2*OFFSET)+2))) {
      //check if continuation block exists
      if(buffer[blkSize-OFFSET]=='#') {
        //no continuation block, so doesnt matter if we have a node here or not
        break;
      }
      //there is a continuation block grab its info
      next=DiskManager::readNumber(buffer, blkSize-OFFSET);
      if(next<0) {
        return -1;
      }
      status=PM->readDiskBlock(next, ahead);
      //check if we need to shift
      if(buffer[i]=='#') {
        //check if there is data we can pull from next continuation block
        if(ahead[0]!='#') {
          for(int j=i; j<(i+OFFSET+2); j++) {
            buffer[j]=ahead[j-i];
            ahead[j-i]='#';
          }
        }
        //no more nodes to shift from the next node (will return unused memory)
        if(ahead[2+OFFSET]=='#') {
          //youve yee'd your last haw bucko

          //get rid of the continuation block ptr
          for(int j=blkSize-OFFSET; j<blkSize; j++) {
            buffer[j]='#';
          }
          //make sure the next node is empty upon delete
          status=PM->writeDiskBlock(next, reset);
          if(status<0) {
            return -1;
          }
          //give memory back
          status=PM->returnDiskBlock(next);
          if(status<0) {
            return -1;
          }
          //were done
          break;
        }
      }
      //whether or not we shifted data from the next node, into the current buffer, we move to the next node
      status=PM->writeDiskBlock(folder, buffer);
      if(status<0) {
        return -1;
      }
      for(int j=0; j<blkSize; j++) {
        buffer[j]=ahead[j];
      }
      folder=next;
      //need to account for the for loop hitting the 'end' of the block but we moved to the next block so reset this
      //on the end of the for loop it will add +6 to make zero, and start at the beginning of the new current (former next node)
      i=-6;
    }
    //somewhere in the dir inode, need to shift
    else if(buffer[i]=='#') {
      //check if next spot has inode we can shift into place
      if(buffer[i+OFFSET+2]=='#') {
        //there is not
        break;
      }
      for(int j=i; j<(i+OFFSET+2); j++) {
        buffer[j]=buffer[j+OFFSET+2];
        buffer[j+OFFSET+2]='#';
      }
    }
    //dont need an else, because if there is an inode there we dont care, we move on to try and shift others
  }
  //write out the buffer
  status=PM->writeDiskBlock(folder, buffer);
  if(status<0) {
    return -1;
  }
  return 0;
}

// will expand if folder NEEDS TO be expaneded to add more
// returns 0 upon success, <0 otherwise
int FileSystem::expandFolder(int loc)
{
  loc = findFolderEnd(loc);
  char buffer[DM->getBlockSize()];
  int nn = PM->readDiskBlock(loc, buffer);
  if (nn < 0)
  {
    return nn;
  }
  if (buffer[DM->getBlockSize() - 5] != '#')
  {
    int n = PM->getFreeDiskBlock();
    if (n < 0)
    {
      PM->returnDiskBlock(n);
      return n;
    }
    DiskManager::writeNumber(buffer, n, DM->getBlockSize() - 4);
    nn = PM->writeDiskBlock(loc, buffer);
    if (nn < 0)
    {
      PM->returnDiskBlock(n);
      return nn;
    }
  }
  return 0;
}

// return block at the end of this inodes continuation, less than 0 otherwise
int FileSystem::findFolderEnd(int start)
{
  char buffer[DM->getBlockSize()];
  int s = PM->readDiskBlock(start, buffer);
  if (s < 0)
  {
    return s;
  }
  while (buffer[DM->getBlockSize() - 4] != '#')
  {
    start = DiskManager::readNumber(buffer, DM->getBlockSize() - 4);
    s = PM->readDiskBlock(start, buffer);
    if (s < 0)
    {
      return s;
    }
  }
  return start;
}

FileSystem::FileSystem(DiskManager *dm, char name)
{
  DM = dm;
  fileSystemName = name;
  fileSystemSize = DM->getPartitionSize(name);
  PM = new PartitionManager(DM, name, fileSystemSize);
  descCounter = 1; // Used to have unique positive integers for file descriptors
}

int FileSystem::createFile(char *filename, int fLen)
{
  int status;
  // check if file already made
  int folder = find(1, filename, fLen, 'F');
  // invalid name
  if (folder == -1)
  {
    return -3;
  }
  else if(folder==-5) {
    return -4;
  }
  // already exists
  else if (folder >= 1)
  {
    return -1;
  }
  char buffer[DM->getBlockSize()];
  // add file to folder
  // putting file in subdir
  folder = 1;
  // if not root dir
  if (fLen > 2)
  {
    // find folder we will put file in
    folder = find(1, filename, fLen - 2, 'D'); // I know could optimize by reading this above and looking for file above, then already have just later problems
  }

  if (expandFolder(folder) != 0)
  {
    return -4;
  }

  folder = findFolderEnd(folder);
  status = PM->readDiskBlock(folder, buffer);
  if (status < 0)
  {
    return -4;
  }
  // find spot for inode
  int INodeLoc = PM->getFreeDiskBlock();
  if (INodeLoc < 0)
  {
    PM->returnDiskBlock(INodeLoc);
    return -2;
  }
  for (int i = 0; i < DM->getBlockSize(); i += 6)
  {
    //check if its a formerly exists inode spot that got deleted
    //yo! thats free realestate
    if(buffer[i]=='#') {
      buffer[i]=filename[fLen-1];
      DiskManager::writeNumber(buffer, INodeLoc, i+1); // We dont need an inode yet
      buffer[i+5] ='F';
      break;
    }
  }
  status = PM->writeDiskBlock(folder, buffer);
  if (status < 0)
  {
    PM->returnDiskBlock(INodeLoc);
    return -4;
  }

  //We are done
  // create inode
  // prevent garbage from entering array
  status = PM->readDiskBlock(INodeLoc, buffer);
  if (status < 0)
  {
    // failed file creation return memory
    PM->returnDiskBlock(INodeLoc);
    return -4;
  }
  //write inode to disk
  int ptr = 0;
  int size = 1; // inode and first block for file
  DiskManager::writeName(buffer, filename[fLen - 1], ptr++);
  DiskManager::writeName(buffer, 'F', ptr++);
  // file size in blocks not includeing inode??? (initially given one block for data, writing past that will be handled in an append or write to file)
  DiskManager::writeNumber(buffer, size, ptr);
  ptr += OFFSET;
  // use BV as nullptr, just check if ever writing to our nullptr
  // direct addr 1
  DiskManager::writeNumber(buffer, 0, ptr);
  ptr += OFFSET;
  // direct addr 2
  DiskManager::writeNumber(buffer, 0, ptr);
  ptr += OFFSET;
  // direct addr 3
  DiskManager::writeNumber(buffer, 0, ptr);
  ptr += OFFSET;
  // indect addr
  DiskManager::writeNumber(buffer, 0, ptr);
  ptr += OFFSET;

  // Locked
  DiskManager::writeNumber(buffer, 0, ptr++);
  ptr += OFFSET;

  int code = setAttribute(buffer, 'C');
  if(code < 0) //Set attribute failed
  {
    PM->returnDiskBlock(INodeLoc);
    return -4;
  }
  status = PM->writeDiskBlock(INodeLoc, buffer);

  if (status < 0)
  {
    // failed file creation return memory
    PM->returnDiskBlock(INodeLoc);
    return -4;
  }
  return 0;
}

/*
buffer[i]=filename
buffer[i+1:i+4]=ptr
buffer[i+5]=type
buffer[61:64]=continuation
*/
int FileSystem::createDirectory(char *dirname, int dLen)
{
 // check if already exists
  int status = 0;
  int s = find(1, dirname, dLen, 'D');
  // invalid name
  if (s == -1)
  {
    return -3;
  }
  else if(s==-5) {
    return -4;
  }
  // already exists
  else if (s >= 1)
  {
    return -1;
  }
  char buffer[DM->getBlockSize()];
  // in root dir
  int loc = 1;
  // grab block of the dir above location (if not in root)
  if (dLen > 2)
  {
    loc = find(1, dirname, dLen - 2, 'D');
    if (loc < 0)
    {
      return -3;
    }
  }
  s = PM->readDiskBlock(loc, buffer);
  if (s < 0)
  {
    return -4;
  }
  // NEED TO CHECK IF LAST inode POS IS TAKEN, IF SO EXPAND FOLDER (USE HELPER TO EXPAND)
  status = expandFolder(loc);
  if (status < 0)
  {
    return -4;
  }
  // s is now outdated
  status = findFolderEnd(loc);
  if (status < 0)
  {
    return -4;
  }
  // NOW HAVE FOLDER, ADD THE int inode FROM BELOW TO THAT FOLDER THEN SHOULD BE DONE

  int inode = PM->getFreeDiskBlock();
  // not enough space in partition
  if (inode < 0)
  {
    return -2;
  }
  // grab inode for end of that folder that we are adding to
  PM->readDiskBlock(status, buffer);
  // find first open spot in that dir to add new inode
  for (int i = 0; i < DM->getBlockSize(); i += 6)
  {
    if(buffer[i]=='#') {
      buffer[i] = dirname[dLen - 1];                  // folder name
      DiskManager::writeNumber(buffer, inode, i + 1); // block loc of new folder inode
      buffer[i + 5] = 'D';
      loc=status;
      break;
    }
  }
  // update the dir to now point to our new folder
  s = PM->writeDiskBlock(loc, buffer);
  if (s < 0)
  {
    PM->returnDiskBlock(inode);
    return -4;
  }
  // dont need to touch the inode loc because the folder starts empty;
  return 0;
}

int FileSystem::lockFile(char *filename, int fLen)
{
  int status = find(1, filename, fLen, 'F');
  if (status == -2) // File does not exist
  {
    return -2;
  }
  else if (status == -1) // File name is invalid
  {
    return -4;
  }
  else if (status == -3) // Unknown error
  {
    return -4;
  }
  // Reading the inode out of memory
  char inode[DM->getBlockSize()];
  PM->readDiskBlock(status, inode);

  if (inode[lockSetAddr] == '1') // File is already locked
  {
    return -1;
  }

  for(unsigned int i=0; i<open_files.size(); i++) {
    if (open_files[i].fileName == filename) //The file is open and therefore cannot be locked
    {
      return -3;
    }
  }

  int lockId = rand() % 99;
  inode[lockSetAddr] = '1';
  DiskManager::writeNumber(inode, lockId, lockIdAddr); // Writing the lockId to the inode, problem with this line, whole number not writing to the inode

  int s = PM->writeDiskBlock(status, inode); // writing to the inode to actually lock, uncomment when the structure of the inode is figured out
  if (s < 0)
  {
    return -4;
  }

  return lockId;
}

int FileSystem::unlockFile(char *filename, int fLen, int lockId)
{
  int status = find(1, filename, fLen, 'F');
  if (status == -2) // File does not exist
  {
    return -2;
  }
  else if (status == -1) // File name is invalid
  {
    return -2;
  }
  else if (status == -3) // Unknown error
  {
    return -2;
  }

  // Getting the inode
  char inode[DM->getBlockSize()];
  PM->readDiskBlock(status, inode);

  // Reading the inode to determine if the lockId is correct
  int realLockId = DiskManager::readNumber(inode, lockIdAddr);
  if (lockId != realLockId) // Invalid lockIDs
  {
    return -1;
  }

  DiskManager::writeNumber(inode, 0, lockSetAddr); // Unlock the file
  int s = PM->writeDiskBlock(status, inode);
  if (s < 0)
  {
    return -4;
  }
  return 0;
}

int FileSystem::deleteFile(char *filename, int fLen)
{
  int status = find(1, filename, fLen, 'F');
  if (status < 0)
  {
    return status == -2 ? -1 : -3;
  }
  int loc = status;
  // if opened
  for (unsigned int i = 0; i < open_files.size(); i++)
  {
    if (open_files[i].address == loc)
    {
      return -2;
    }
  }
  /*
  buffer[0]=name
  buffer[1]=type
  buffer[2:5]=filesize
  buffer[6:9]=direct addr1
  buffer[10:13]=direct addr2
  buffer[14:17]=direct addr3
  buffer[18:21]=indirect addr
  */
  char buffer[DM->getBlockSize()];

  status = PM->readDiskBlock(loc, buffer);
  // file is locked return -2;
  if(buffer[lockSetAddr] == '1')
  {
    return -2;
  }
  int pos = 2;
  int size = DiskManager::readNumber(buffer, pos);
  int addr = 0;
  // get rid of direct address 1
  pos += OFFSET;
  //file size must always be atleast 2: first block of data, and inode
  if (size >= 2)
  {
    addr = DiskManager::readNumber(buffer, pos);
    // do not erase the BV
    if (addr != 0)
    {
      status = PM->returnDiskBlock(addr);
      if (status < 0)
      {
        return -3;
      }
    }
  }
  pos += OFFSET;
  // get rid of direct address 2
  if (size >= 3)
  {
    addr = DiskManager::readNumber(buffer, pos);
    // do not erase the BV
    if (addr != 0)
    {
      status = PM->returnDiskBlock(addr);
      if (status < 0)
      {
        return -3;
      }
    }
  }
  pos += OFFSET;
  // get rid of direct address 3
  if (size >= 4)
  {
    addr = DiskManager::readNumber(buffer, pos);
    // do not erase the BV
    if (addr != 0)
    {
      status = PM->returnDiskBlock(addr);
      if (status < 0)
      {
        return -3;
      }
    }
  }
  pos += OFFSET;

  // get rid of indirect addresses
  if (size >= 5)
  {
    // get addr of file continuation block and read it in
    addr = DiskManager::readNumber(buffer, pos);
    // do not erase the BV
    if (addr != 0)
    {
      status = PM->readDiskBlock(addr, buffer);
      if (status < 0)
      {
        return -3;
      }
      // now we have that data, get rid of the indirect inode
      status = PM->returnDiskBlock(addr);
      if (status < 0)
      {
        return -3;
      }
      // go through each addr pointer if exists and delete
      for (int i = 0; i < DM->getBlockSize(); i += OFFSET)
      {
        // done like this incase of fragmentation in the indirect inode
        if (buffer[i] == '#')
        {
          // nothing in this spot
          continue;
        }
        addr = DiskManager::readNumber(buffer, i);
        // do not erase the BV
        if (addr == 0)
        {
          continue;
        }
        status = PM->returnDiskBlock(addr);
        if (status < 0)
        {
          return -3;
        }
      }
    }
  }
  // get rid of file inode
  status = PM->returnDiskBlock(loc);
  if (status < 0)
  {
    return -3;
  }
  if(fLen == 2) //we are in the root directory
  {
    int address = 1;
    PM->readDiskBlock(address, buffer);
    bool end = false;
    pos = 0;
    while(!end)
    {
      if(buffer[pos] == filename[fLen - 1] && buffer[pos + 5] == 'F') //we have found it
      {
        for(int i = pos; i < pos + 6; i++)
        {
          buffer[i] = '#';
        }

        PM->writeDiskBlock(address, buffer);
        break;
        end=true;
      }

      pos += 6;
      if(pos > 60) //We have reached the end of the dirInode
      {
        pos = 0;
        if(DiskManager::readNumber(buffer, DM->getBlockSize()-OFFSET) == 0) //No continuation block
        {
          return -3;
        }

        address = DiskManager::readNumber(buffer, DM->getBlockSize()-OFFSET);
        PM->readDiskBlock(address, buffer);
      }
    }
    int tmp=shiftFixFolder(1);
  }
  else
  {
    status = find(1, filename, fLen - 2, 'D');
    if(status < 0) //DirInode cant be found
    {
      return -3;
    }
    int head=status;
    PM->readDiskBlock(status, buffer);
    bool end = false;
    pos = 0;
    while(!end)
    {
      if(buffer[pos] == filename[fLen - 1] && buffer[pos + 5] == 'F') //we have found it
      {
        for(int i = pos; i < pos + 6; i++)
        {
          buffer[i] = '#';
        }
        break;
        end=true;
      }

      pos += 6;
      if(pos > DM->getBlockSize()-OFFSET) //We have reached the end of the dirInode
      {
        pos = 0;
        if(DiskManager::readNumber(buffer, DM->getBlockSize()-OFFSET) == 0) //No continuation block
        {
          return -3;
        }

        status = DiskManager::readNumber(buffer, DM->getBlockSize()-OFFSET);
        PM->readDiskBlock(status, buffer);
        int tmp=shiftFixFolder(head);
      }
    }
    PM->writeDiskBlock(status, buffer);
    int tmp=shiftFixFolder(head);
  }
  return 0;
}

int FileSystem::deleteDirectory(char *dirname, int dLen)
{
  // you cant delete the root directory you fool
  if (dLen == 1)
  {
    return -3;
  }
  int status = find(1, dirname, dLen, 'D');
  if (status < 0 && status != 9999)
  {
    return status == -2 ? -1 : status==-1?-3:-4;
  }
  int loc = status;
  int baseloc = loc;

  // NEEDS IMPLEMENTATION
  // if opened or locked -2
  char buffer[DM->getBlockSize()];
  // check if empty
  if(status != 9999)
  {

    status = PM->readDiskBlock(loc, buffer);
    if (status < 0)
    {
      return -3;
    }
    for (int i = 0; i < DM->getBlockSize(); i += 6)
    {
      if ((DM->getBlockSize() - 4 == i) && (buffer[i] != '#'))
      {
        // grab next continuation block
        i = 0;
        loc = DiskManager::readNumber(buffer, DM->getBlockSize() - 4);
        // do NOT overrite the BV or the root directory
        if (loc <= 1)
        {
          // if youre here, you got problems kid, go home
          return -3;
        }
        status = PM->readDiskBlock(loc, buffer);
        if (status < 0)
        {
          return -3;
        }
      }
      else if (buffer[i] != '#')
      {
        // there is a filename there
        return -2;
      }
    }
    // folder is empty, we can finally delete it
    char reset[DM->getBlockSize()];
    for (int i = 0; i < DM->getBlockSize(); reset[i++]='#')
    {
  //    reset[i] = '#';
    }
    loc = baseloc;
    status = PM->readDiskBlock(loc, buffer);
    if (status < 0)
    {
      return -3;
    }
    while (buffer[DM->getBlockSize() - 4] != '#') // only need to look at the last 4 chars (continuation block addr) because we already know its empty
    {
      status = PM->writeDiskBlock(loc, reset);
      if (status < 0)
      {
        return -3;
      }
      status = PM->returnDiskBlock(loc);
      if (status < 0)
      {
        return -3;
      }
      loc = DiskManager::readNumber(buffer, DM->getBlockSize() - 4);
      // do NOT overrite the BV or the root directory
      if (loc <= 1)
      {
        // if youre here, you really done fucked up somehow, bad
        return -3;
      }
      status = PM->readDiskBlock(loc, buffer);
      if (status < 0)
      {
        return -3;
      }
    }
    //delete the last one in the chain of the dir inodes
    status=PM->writeDiskBlock(loc, reset);
    if(status<0) {
      return -3;
    }
    status=PM->returnDiskBlock(loc);
    if(status<0) {
      return -3;
    }
  }

  //get rid of the dir inode portion in the directory inode
  loc = find(1, dirname, dLen-2, 'D');
  int head=loc;
  PM->readDiskBlock(loc, buffer);
  for (int i=0; i<DM->getBlockSize(); i+=6) {
    if(((DM->getBlockSize()-4)==i)&&(buffer[i]!='#')) {
      //grab next continuation block
      i=0;
      loc=DiskManager::readNumber(buffer, DM->getBlockSize()-4);
      //do NOT overrite the BV or the root directory
      if(loc<=1) {
        //if youre here, you got problems kid, go home
        return -3;
      }
      status=PM->readDiskBlock(loc, buffer);
      if(status<0) {
        return -3;
      }
    }
    if((buffer[i]==dirname[dLen-1])&&(buffer[i+5]=='D')) {
      for(int j=i; j<(i+6); j++) {
        buffer[j]='#';
      }
      break;
    }
  }
  status=PM->writeDiskBlock(loc, buffer);
  if(status<0) {
    return -3;
  }
  status=shiftFixFolder(head);
  if(status<0) {
    return -3;
  }
  return 0;
}

int FileSystem::openFile(char *filename, int fLen, char mode, int lockId)
{
  if (mode != 'r' && mode != 'w' && mode != 'm') // Invalid mode
  {
    return -2;
  }

  // Finding the file
  int status = find(1, filename, fLen, 'F');
  if (status == -1) // Filename invalid
  {
    return -4;
  }
  else if (status == -2) // File does not exist
  {
    return -1;
  }

  char inode[DM->getBlockSize()];
  PM->readDiskBlock(status, inode);

  if (inode[lockSetAddr] == '1')
  {
    int realLockId = DiskManager::readNumber(inode, lockIdAddr);
    if (realLockId != lockId) // Lock Ids dont match
    {
      return -3;
    }
  }
  else
  {
    if (lockId != -1) // File isnt locked and lockId is not -1
    {
      return -3;
    }
  }

  // Creating open_file object to add to a vector
  open_file temp;
  temp.descriptor = descCounter;
  descCounter++;
  temp.mode = mode;
  temp.address = status;
  temp.rwPtr = 0;
  temp.fileName = filename;
  open_files.push_back(temp);

  return descCounter - 1;
}

int FileSystem::closeFile(int fileDesc)
{
  if (fileDesc < 1) // Invalid fileDesc is any non-positive integer
  {
    return -1;
  }
  int address;
  for (auto i = open_files.begin(); i != open_files.end(); ++i)
  {
    if (i->descriptor == fileDesc) // If we find the descriptor
    {
      address = i->address;
      open_files.erase(i);            // Erase the struct from the vector
      char inode[DM->getBlockSize()]; // Reflect that the file is no longer open in the inode
      PM->readDiskBlock(address, inode);
      int s = PM->writeDiskBlock(address, inode);
      if (s < 0)
      {
        return -2;
      }
      return 0;
    }
  }
  return -1; // No matching descriptor
}

int FileSystem::readFile(int fileDesc, char *data, int len)
{
  if (len < 0) // Length is negative
  {
    return -2;
  }

  open_file* file;
  int address = -9999;
  char mode;
  char inode[DM->getBlockSize()];
  for(unsigned int i=0; i<open_files.size(); i++) {
    if (open_files[i].descriptor == fileDesc)
    {
      file = &open_files[i];
      address = open_files[i].address;
      mode = open_files[i].mode;
      break;
    }
  }

  if (address == -9999) // File descriptor is invalid
  {
    return -1;
  }

  if (mode != 'r' && mode != 'm') // Operation not permitted
  {
    return -3;
  }

  // Now to do the actual reading
  PM->readDiskBlock(address, inode);

  int read = 0;
  char file_data[DM->getBlockSize()]; // Char array to store the data in before we read
  for (int i = 0; i < len; i++)
  {
    int result = file->rwPtr / DM->getBlockSize();
    if (result < 1) // We are in the first block
    {
      if(DiskManager::readNumber(inode, 6) == 0)
      {
        //We have reached the end of the file
        break;
      }
      PM->readDiskBlock(DiskManager::readNumber(inode, 2+OFFSET), file_data);
      if (file_data[file->rwPtr] == '#') // We have reached the end of the file
      {
        break;
      }
      data[i] = file_data[file->rwPtr];
      file->rwPtr++;
      read++;
    }
    else if (result < 2) // We are in the second block
    {
      if(DiskManager::readNumber(inode, 2+(2*OFFSET)) == 0)
      {
        //We have reached the end of the file
        break;
      }
      PM->readDiskBlock(DiskManager::readNumber(inode, 2+(2*OFFSET)), file_data);
      if (file_data[file->rwPtr - DM->getBlockSize()] == '#') // We have reached the end of the file
      {
        break;
      }
      data[i] = file_data[file->rwPtr - DM->getBlockSize() ];
      file->rwPtr++;
      read++;
    }
    else if (result < 3) // We are in the third block
    {
      if(DiskManager::readNumber(inode, 2+(3*OFFSET)) == 0)
      {
        //We have reached the end of the file
        break;
      }
      PM->readDiskBlock(DiskManager::readNumber(inode, 2+(3*OFFSET)), file_data);
      if (file_data[file->rwPtr - (DM->getBlockSize() * 2)] == '#') // We have reached the end of the file
      {
        break;
      }
      data[i] = file_data[file->rwPtr - (DM->getBlockSize() * 2)];
      file->rwPtr++;
      read++;
    }
    else // We are in indirect addressing
    {
      char addresses[DM->getBlockSize()];
      PM->readDiskBlock(DiskManager::readNumber(inode, 2+(4*OFFSET)), addresses);

      if(DiskManager::readNumber(inode, 2+(4*OFFSET)) == 0)
      {
        //We have reached the end of the file
        break;
      }

      if(addresses[(result-3)*4]== '#')
      {
        //We have reached the end of the file
        break;
      }

      int dataAddress = DiskManager::readNumber(addresses, (result-3)*4);
      if(dataAddress == 0)
      {
        //We have reached the end of the file;
        break;
      }
      PM->readDiskBlock(dataAddress, file_data);
      if(file_data[file->rwPtr - (DM->getBlockSize() * result)] == '#') // We have reached the end of the file
      {
        break;
      }

      data[i] = file_data[file->rwPtr - (DM->getBlockSize() * result)];
      file->rwPtr++;
      read++;
    }
  }
  return read;
}

int FileSystem::writeFile(int fileDesc, char *data, int len)
{
  if (len < 0) // Length is negative
  {
    return -2;
  }

  open_file* file;
  int address = -9999;
  char mode;
  for(unsigned int i=0; i<open_files.size(); i++) {
    if (open_files[i].descriptor == fileDesc)
    {
      file = &open_files[i];
      address = open_files[i].address;
      mode = open_files[i].mode;
      break;
    }
  }

  if (address == -9999) // Invalid file descriptor
  {
    return -1;
  }

  if (mode != 'w' && mode != 'm') // Operation is not permitted
  {
    return -3;
  }

  char inode[DM->getBlockSize()];
  char file_data[DM->getBlockSize()];
  PM->readDiskBlock(address, inode);
  int written = 0;
  for (int i = 0; i < len; i++)
  {
    int result = file->rwPtr / DM->getBlockSize();
    if (result == 0) // We are in the first block
    {
      // Make sure that there is an address for the block with data
      if (DiskManager::readNumber(inode, 2+OFFSET) == 0)
      {
        // Find a free block and give it to the file
        int newBlock = PM->getFreeDiskBlock();
        if (newBlock < 0) // Failure with getting a new disk block
        {
          PM->returnDiskBlock(newBlock);
          return -3;
        }
        int size=DiskManager::readNumber(inode, 2);
        DiskManager::writeNumber(inode, size+1, 2);
        DiskManager::writeNumber(inode, newBlock, 2+OFFSET); // Writing address to the inode
        PM->writeDiskBlock(address, inode);           // Writing the updated inode to the disk
      }
      //There is an address for block 1
      PM->readDiskBlock(DiskManager::readNumber(inode, 2+OFFSET), file_data);
      file_data[file->rwPtr] = data[i];
      int s = PM->writeDiskBlock(DiskManager::readNumber(inode, 2+OFFSET), file_data);
      if(s < 0) //Write failed
      {
        return -4;
      }
      written++;
      file->rwPtr++;
    }
    else if(result == 1) //We are in the second block
    {
      //Make sure that a block exists, if not create one
      if (DiskManager::readNumber(inode, 2+(2*OFFSET)) == 0)
      {
        // Find a free block and give it to the file
        int newBlock = PM->getFreeDiskBlock();
        if (newBlock < 0) // Failure with getting a new disk block
        {
          PM->returnDiskBlock(newBlock);
          return -3;
        }
        int size=DiskManager::readNumber(inode, 2);
        DiskManager::writeNumber(inode, size+1, 2);
        DiskManager::writeNumber(inode, newBlock, 2+(2*OFFSET)); // Writing address to the inode
        PM->writeDiskBlock(address, inode);           // Writing the updated inode to the disk
      }
      PM->readDiskBlock(DiskManager::readNumber(inode, 2+(2*OFFSET)), file_data);
      file_data[file->rwPtr - DM->getBlockSize()] = data[i];
      int s = PM->writeDiskBlock(DiskManager::readNumber(inode, 2+(2*OFFSET)), file_data);
      if(s < 0) //Write failed
      {
        return -4;
      }
      file->rwPtr++;
      written++;
    }
    else if(result == 2) //We are in the 3rd block
    {
      //Make sure that a block exists, if not create one
      if (DiskManager::readNumber(inode, 2+(3*OFFSET)) == 0)
      {
        // Find a free block and give it to the file
        int newBlock = PM->getFreeDiskBlock();
        if (newBlock < 0) // Failure with getting a new disk block
        {
          PM->returnDiskBlock(newBlock);
          return -3;
        }
        int size=DiskManager::readNumber(inode, 2);
        DiskManager::writeNumber(inode, size+1, 2);
        DiskManager::writeNumber(inode, newBlock, 2+(3*OFFSET)); // Writing address to the inode
        PM->writeDiskBlock(address, inode);           // Writing the updated inode to the disk
      }
      PM->readDiskBlock(DiskManager::readNumber(inode, 2+(3*OFFSET)), file_data);
      file_data[file->rwPtr - (DM->getBlockSize() * 2)] = data[i];
      int s = PM->writeDiskBlock(DiskManager::readNumber(inode, 2+(3*OFFSET)), file_data);
      if(s < 0) //Write failed
      {
        return -4;
      }
      file->rwPtr++;
      written++;
    }
    else //We are in indirect addressing
    {
      //How to handle indirect addressing ?????
      if(DiskManager::readNumber(inode, 2+(4*OFFSET)) == 0) //We do not have an address for the indirect addressing
      {
        int newBlock = PM->getFreeDiskBlock();
        if (newBlock < 0) // Failure with getting a new disk block
        {
          PM->returnDiskBlock(newBlock);
          return -3;
        }
        int size=DiskManager::readNumber(inode, 2);
        DiskManager::writeNumber(inode, size+2, 2);//two because of the block from down below being added to the indirect inode
        DiskManager::writeNumber(inode, newBlock, 2+(4*OFFSET)); // Writing address to the inode
        PM->writeDiskBlock(address, inode);           // Writing the updated inode to the disk

        char indirect[DM->getBlockSize()];
        PM->readDiskBlock(newBlock, indirect);
        //Now we have the block in our inode
        int firstIndirect = PM->getFreeDiskBlock();
        if(firstIndirect < 0)
        {
          PM->returnDiskBlock(firstIndirect);
          return -3;
        }

        DiskManager::writeNumber(indirect, firstIndirect, 0);
        PM->writeDiskBlock(newBlock, indirect);
      }
      //Now we have an indirect address block, find where we are going and then write
      int indirectAdd = DiskManager::readNumber(inode, 2+(4*OFFSET));
      char indirectAddresses[DM->getBlockSize()];
      PM->readDiskBlock(indirectAdd, indirectAddresses);

      char writer[DM->getBlockSize()];
      if(indirectAddresses[(result -3) * 4] == '#') //We do not have a block
      {
        int newNewBlock = PM->getFreeDiskBlock();
        if(newNewBlock < 0)
        {
          PM->returnDiskBlock(newNewBlock);
          return -3;
        }
        int size=DiskManager::readNumber(inode, 2);
        DiskManager::writeNumber(inode, size+1, 2);
        PM->writeDiskBlock(address, inode);
        DiskManager::writeNumber(indirectAddresses, newNewBlock, (result-3)*4);
        PM->writeDiskBlock(indirectAdd, indirectAddresses);
      }

      int dataInt = DiskManager::readNumber(indirectAddresses, (result-3) * 4);
      PM->readDiskBlock(dataInt, writer);
      writer[file->rwPtr - (DM->getBlockSize() * result)] = data[i];
      int s = PM->writeDiskBlock(dataInt, writer);
      if(s < 0) //Uh Oh!!! Invalide write
      {
        return -4;
      }

      written++;
      file->rwPtr++;
    }
  }

  int code = setAttribute(inode, 'M');
  if(code < 0) //Set attribute failed
  {
    return -4;
  }

  code = PM->writeDiskBlock(address, inode);
  if(code < 0)
  {
    return -4;
  }
  return written;
}

int FileSystem::appendFile(int fileDesc, char *data, int len) {
  if(len<0) {
    return -2;
  }
  //whats wrong with yoU? duh were dont writing nothing to the disk dumbo
  if(len==0) {
    return 0;
  }
  open_file* file=nullptr;
  for(unsigned int i=0; i<open_files.size(); i++) {
    if(open_files[i].descriptor==fileDesc) {
      file=&open_files[i];
      break;
    }
  }
  if(file==nullptr) {
    return -1;
  }
  if((file->mode!='w')&&(file->mode!='m')) {
    return -3;
  }
  char inode[DM->getBlockSize()];
  char filedata[DM->getBlockSize()];
  int status=PM->readDiskBlock(file->address, inode);
  if(status<0) {
    return -4;
  }
  int size=DiskManager::readNumber(inode, 2);
  size-=size>4?2:1;//remove the inodes from local var size
  if(size==0) {
    int newB=PM->getFreeDiskBlock();
    if(newB < 0)
    {
      return -3;
    }
    DiskManager::writeNumber(inode, newB, 2+OFFSET);
    int newSize = DiskManager::readNumber(inode, 2);
    DiskManager::writeNumber(inode, newSize + 1, 2);
    status=PM->writeDiskBlock(file->address, inode);
    if(status<0) {
      return -4;
    }
    size++;
  }
  //move rwPtr to EOF
  int eofAddr=0;
  if(size<=3) {//only dealing with direct addrs
    eofAddr=DiskManager::readNumber(inode, 2+(size*OFFSET));
    //need to convert to index now
    size--;
  }
  else {//need to deal with indirect addrs
    int addr=DiskManager::readNumber(inode, 2+(4*OFFSET));
    status=PM->readDiskBlock(addr, inode);
    if(status<0) {
      return -4;
    }
    size-=4;//remove the 3 direct inodes and indirect inode from size
    eofAddr=DiskManager::readNumber(inode, (size*OFFSET));
    //return size to the full size
    size+=3;//were wanting to be in a block so we locInBloc takes account for that in the rwptr by means of size in the calculation below
  }
  int locInBloc=0;
  status=PM->readDiskBlock(eofAddr, filedata);
  if(status<0) {
    return -4;
  }
  //may have an OBO, but in theory should be fine
  for(; (locInBloc<DM->getBlockSize())&&(filedata[locInBloc]!='#'); locInBloc++) {
  }
  file->rwPtr=locInBloc+(size*DM->getBlockSize());
  //rwPtr has exceeded max file size

  if(file->rwPtr>=(((3+(DM->getBlockSize()/OFFSET))*DM->getBlockSize()))) {
    return -3;
  }
  int something =  writeFile(fileDesc, data, len);
  return something==-4?-3:something;
}

int FileSystem::truncFile(int fileDesc, int offset, int flag)
{
  if(offset < 0 && flag != 0) //If the offset is negative the flag must be 0
  {
    return -1;
  }

  open_file* file = nullptr;
  int address = -9999;
  char mode;
  for(unsigned int i=0; i<open_files.size(); i++) {
    if (open_files[i].descriptor == fileDesc)
    {
      file = &open_files[i];
      address = open_files[i].address;
      mode = open_files[i].mode;
      break;
    }
  }

  if(file == nullptr) //File descriptor invalid
  {
    return -1;
  }

  if(mode == 'r') //mode is read
  {
    return -3;
  }

  int status = seekFile(fileDesc, offset, flag); //Setting the rwPtr
  if(status == -1) //Something is invalid
  {
    return -1;
  }

  if(status == -2) //Out of bounds rwPtr
  {
    return -2;
  }

  //Delete the data
  char inode[DM->getBlockSize()];
  PM->readDiskBlock(address, inode);

  int deleted = 0;
  char data[DM->getBlockSize()];
  bool eof = false;
  int fromThere = file->rwPtr;
  while(!eof) //Deleting the data in the block
  {
    int block = fromThere / DM->getBlockSize();
    if(block < 3)
    {
      if(DiskManager::readNumber(inode, (block *4) + 6) == 0) //We have reached the end of the file
      {
        eof = true;
      }
      else
      {
        PM->readDiskBlock(DiskManager::readNumber(inode, (block * 4) + 6), data);
        int ptrInBlock = fromThere - (DM->getBlockSize() * block);
        if(data[ptrInBlock] == '#')
        {
          //We have reached the end of the file
          eof = true;
        }
        else
        {
          data[ptrInBlock] = '#';
          deleted++;
          fromThere++;
          PM->writeDiskBlock(DiskManager::readNumber(inode, (block * 4) + 6), data);
        }
      }
    }
    else //Indirect addressing
    {
      if(DiskManager::readNumber(inode, 18) == 0) //End of file
      {
        eof = true;
      }
      else
      {
        char addresses[DM->getBlockSize()];
        PM->readDiskBlock(DiskManager::readNumber(inode, 18), addresses);

        int address = block - 3;
        if(address > 15 || DiskManager::readNumber(addresses, (address * 4)) == 0 || DiskManager::readName(addresses, (address * 4)) == '#') //16 * 4 = 64 which is out of bounds of the array or the readNumber fails
        {
          eof = true;
        }
        else
        {
          PM->readDiskBlock(DiskManager::readNumber(addresses, (address * 4)), data);
          int ptrInBlock = fromThere - (DM->getBlockSize() * block);
          if(data[ptrInBlock] == '#')
          {
            //We have reached the end of the file
            eof = true;
          }
          else
          {
            data[ptrInBlock] = '#';

            PM->writeDiskBlock(DiskManager::readNumber(addresses, (address * 4)), data);
            deleted++;
            fromThere++;
          }
        }
      }
    }
  }

  //Data has been deleted and we need to return the blocks
  int block;
  if(file->rwPtr != 0)
  {
    block = (file->rwPtr - 1) / DM->getBlockSize();
  }
  else
  {
    block = file->rwPtr  / DM->getBlockSize();
    PM->returnDiskBlock(DiskManager::readNumber(inode, 6));
  }

  for(int i = (block + 1); i < 3; i++) //For each block that isnt indirect addressing
  {
    PM->returnDiskBlock(DiskManager::readNumber(inode, (i * 4) + 6));
    DiskManager::writeNumber(inode, 0, (i * 4) + 6);
  }

  if(DiskManager::readNumber(inode, 18) > 1) //We have indirect inodes
  {
    char addresses[DM->getBlockSize()];
    PM->readDiskBlock(DiskManager::readNumber(inode, 18), addresses);
    int indirectAddr = block - 2;
    for(int i = indirectAddr; i < 16; i++)
    {
      if(DiskManager::readNumber(addresses, (i*4)) > 0)
      {
        PM->returnDiskBlock(DiskManager::readNumber(addresses, i * 4));
      }
      for(int j = i * 4; j < (i*4) + 4; j++)
      {
        addresses[j] = '#';
      }
    }
    PM->writeDiskBlock(DiskManager::readNumber(inode, 18), addresses);
  }
  int size = ((file->rwPtr - 1) / DM->getBlockSize()) + 1;
  if(size > 3)
  {
    size += 2;
  }
  else //We need to release the indirect inode
  {
    PM->returnDiskBlock(DiskManager::readNumber(inode, 18));
    DiskManager::writeNumber(inode, 0, 18);
    size += 1;
  }


  DiskManager::writeNumber(inode, size, 2);
  int code = setAttribute(inode, 'M');
  if(code < 0) //Set attribute failed
  {
    return -4;
  }
  PM->writeDiskBlock(address, inode);
  return deleted;
}

int FileSystem::seekFile(int fileDesc, int offset, int flag) {
  open_file* file;
  int address = -9999;
  for(unsigned int i=0; i<open_files.size(); i++) {
    if (open_files[i].descriptor == fileDesc)
    {
      file = &open_files[i];
      address = open_files[i].address;
      break;
    }
  }

  if(address == -9999) //Invalid file descriptor
  {
    return -1;
  }

  if(flag != 0 && offset < 0) //Negative offset out of bounds
  {
    return -1;
  }

  char inode[DM->getBlockSize()];
  PM->readDiskBlock(address, inode);

  if(flag == 0)
  {
    int newPtr = file->rwPtr + offset;
    if(newPtr < 0) //Out of bounds
    {
      return -2;
    }

    int blockPos = newPtr / DM->getBlockSize();
    int multiplier = 1;
    if(blockPos > 2) //Using multiplier to determine where we need to read the address from
    {
      multiplier = 4;
    }
    else if(blockPos == 2)
    {
      multiplier = 3;
    }
    else if(blockPos == 1)
    {
      multiplier = 2;
    }

    if(DiskManager::readNumber(inode, OFFSET * multiplier) == 0) //We have reached the end of the file and gone out of bounds
    {
      return -2;
    }

    if(blockPos > 2)
    {
      //Indirect addressing
      char addresses[DM->getBlockSize()];
      int addr = DiskManager::readNumber(inode, 2+(4*OFFSET));
      PM->readDiskBlock(addr, addresses);

      int actualAddress = DiskManager::readNumber(addresses, (blockPos - 3) * 4);
      if(actualAddress == 0) //Out of bounds
      {
        return -2;
      }
      char data[DM->getBlockSize()];
      PM->readDiskBlock(actualAddress, data);

      if(data[newPtr - ((DM->getBlockSize() * blockPos) - 1)] == '#') //We have gone past the end of the file
      {
        return -2;
      }

      file->rwPtr = newPtr;
    }
    else
    {
      //Direct addressing
      char data[DM->getBlockSize()];
      PM->readDiskBlock(DiskManager::readNumber(inode, multiplier * 2+OFFSET), data);

      if(data[(newPtr - (DM->getBlockSize() * blockPos)) - 1] == '#') //Out of bounds
      {
        return -2;
      }

      file->rwPtr = newPtr;
    }
  }
  else //rwPtr == offset
  {
    int blockPos = offset / DM->getBlockSize();
    if(blockPos > 2) //Indirect addressing
    {
      if(DiskManager::readNumber(inode, 2+(4*OFFSET)) == 0) //We have reached the end of the file
      {
        return -2;
      }

      char addresses[DM->getBlockSize()];
      PM->readDiskBlock(DiskManager::readNumber(inode, 2+(4*OFFSET)), addresses);

      int addressPos = (blockPos - 3) * 4;
      if(DiskManager::readNumber(addresses, addressPos) == 0)
      {
        return -2;
      }

      char data[DM->getBlockSize()];
      PM->readDiskBlock(DiskManager::readNumber(addresses, addressPos), data);

      if(data[(offset - (DM->getBlockSize() * blockPos)) - 1] == '#') //Out of bounds
      {
        return -2;
      }

      file->rwPtr = offset;
    }
    else
    {
      if(DiskManager::readNumber(inode, (OFFSET * blockPos) + 2+OFFSET) == 0) //Out of bounds
      {
        return -2;
      }

      char data[DM->getBlockSize()];
      PM->readDiskBlock(DiskManager::readNumber(inode, (OFFSET * blockPos) + 2+OFFSET), data);

      if(data[(offset - (DM->getBlockSize() * blockPos)) -1] == '#') //Out of bounds
      {
        return -2;
      }

      file->rwPtr = offset;
    }
  }

  return 0;
}

int FileSystem::renameFile(char *filename1, int fLen1, char *filename2, int fLen2)
{
  int pos = find(1, filename1, fLen1, 'F');
  // Check if filenames are valid
  bool newFileName = validFilename(filename2,fLen2);
  bool oldFileName = validFilename(filename1,fLen1);
  if (!newFileName || !oldFileName) {
    return -1;
  }
  // Check if file we want to rename exists
  int exists1 = find(1, filename1, fLen1, 'F');
  if (exists1 < 0) {
    return -2;
  }
  // Check if new file name already exists
  int exists2 = find(1, filename2, fLen2, 'F');
  if (exists2 > 0) {
    return -3;
  }
  // Check if file is open or locked
  for(unsigned int i=0; i<open_files.size(); i++) {
    if (open_files[i].fileName == filename1)
    {
      return -4;
    }
  }
  char inode[DM->getBlockSize()];
  PM->readDiskBlock(pos, inode);
  if (inode[lockSetAddr] == '1')
  {
    return -4;
  }

  inode[0] = filename2[fLen1 - 1];
  PM->writeDiskBlock(pos, inode);

  char dirInode[DM->getBlockSize()];
  if(fLen2 == 2)
  {
    //We are in the root directory so we need to edit that inode
    PM->readDiskBlock(1, dirInode);
    bool end = false;
    int pos = 0;
    int address = 1;
    while(!end) //Finding the name within the dir Inode
    {
      if(dirInode[pos] == filename1[fLen1 - 1] && dirInode[pos + 5] == 'F') //If it is a file and the name matches, change the name
      {
        dirInode[pos] = filename2[fLen2 - 1]; //Change the name
        PM->writeDiskBlock(address, dirInode);
        end = true;
      }
      pos+=6;

      if(pos > 60) //We need to get the extension
      {
        if(DiskManager::readNumber(dirInode, 60) == 0)
        {
          return -5; //No continuation couldnt be found to change
        }
        pos = 0;
        address = DiskManager::readNumber(dirInode, 60);
        PM->readDiskBlock(address, dirInode);
      }
    }
  }
  else //we need to find the directory, read in the dirInode, and then do the same process
  {
    int inodeAddress = find(1, filename1, fLen1 - 2, 'D');
    if(inodeAddress < 0)
    {
      //DirInode could not be found
      return -5;
    }

    PM->readDiskBlock(inodeAddress, dirInode);
    bool end = false;
    int pos = 0;
    while(!end) //Finding the name within the dir Inode
    {
      if(dirInode[pos] == filename1[fLen1 - 1] && dirInode[pos + 5] == 'F') //If it is a file and the name matches, change the name
      {
        dirInode[pos] = filename2[fLen2 - 1]; //Change the name
        PM->writeDiskBlock(inodeAddress, dirInode);
        end = true;
      }
      pos+=6;

      if(pos > 60) //We need to get the extension
      {
        if(DiskManager::readNumber(dirInode, 60) == 0)
        {
          return -5; //No continuation couldnt be found to change
        }
        pos = 0;
        inodeAddress = DiskManager::readNumber(dirInode, 60);
        PM->readDiskBlock(inodeAddress, dirInode);
      }
    }
  }

  return 0; // place holder so there is no warnings when compiling.
}

int FileSystem::renameDirectory(char *dirname1, int dLen1, char *dirname2, int dLen2)
{
  //int status = find(1, dirname1, dLen1, 'D');
  // Check if directory names are valid
  bool newDirName = validFilename(dirname2,dLen2);
  bool oldDirName = validFilename(dirname1,dLen1);
  if (!newDirName || !oldDirName) {
    return -1;
  }
  // Check if dir we want to rename exists
  int exists1 = find(1, dirname1, dLen1, 'D');
  if (exists1 < 0) {
    return -2;
  }
  // Check if new dir name already exists
  int exists2 = find(1, dirname2, dLen2, 'D');
  if (exists2 > 0) {
    return -3;
  }

  //Read in the inode and change it
  char inode[DM->getBlockSize()];
  PM->readDiskBlock(exists1, inode);
  inode[0] = dirname2[dLen2 - 1];

  PM->writeDiskBlock(exists1, inode);

  if(dLen1 == 2) //We need to change the root directory
  {
    char root[DM->getBlockSize()];
    //Read in the root inode and change it
    PM->readDiskBlock(1, root);
    int address = 1;
    bool end = false;
    int pos = 0;
    while(!end)
    {
      if(pos > 60)
      {
        if(DiskManager::readNumber(root, 60) == 0)
        {
          return -5; //There is no continuation
        }

        address = DiskManager::readNumber(root, 60);
        PM->readDiskBlock(address, root);
        pos = 0;
      }

      if(root[pos] == dirname1[dLen1 - 1] && root[pos + 5] == 'D')
      {
        root[pos] = dirname2[dLen2 - 1];
        PM->writeDiskBlock(address, root);
        end = true;
      }
      pos += 6;
    }
  }
  else //We need to find the parent directory, and change its name in that inode
  {
    char parent[dLen1 - 2];
    char parData[DM->getBlockSize()];
    for(int i = 0; i < dLen1 - 2; i++) //Getting parent dir name
    {
      parent[i] = dirname1[i];
    }

    //Find the parent dir
    int parentAddress = find(1, parent, dLen1 - 2, 'D');
    if(parentAddress < 0) //Couldnt find the parent dir to change the name
    {
      return -5;
    }

    PM->readDiskBlock(parentAddress, parData);
    bool end = false;
    int pos = 0;
    while(!end)
    {
      if(pos > 60)
      {
        if(DiskManager::readNumber(parData, 60) == 0)
        {
          return -5; //There is no continuation
        }

        parentAddress = DiskManager::readNumber(parData, 60);
        PM->readDiskBlock(parentAddress, parData);
        pos = 0;
      }

      if(parData[pos] == dirname1[dLen1 - 1] && parData[pos + 5] == 'D')
      {
        parData[pos] = dirname2[dLen2 - 1];
        PM->writeDiskBlock(parentAddress, parData);
        end = true;
      }
      pos += 6;
    }
  }
  return 0;
}

int FileSystem::getAttribute(char *filename, int fLen, char type) //returns -1 if file doesnt exist, -2 if there is no time set, -3 if invalid type passed, and 0 if successful
{
  int address = find(1, filename, fLen, 'F');
  if(address < 0) //Cound not find the file address
  {
    return -1;
  }

  char inode[DM->getBlockSize()];
  PM->readDiskBlock(address, inode);
  if(type == 'M') //Getting last modified time
  {
    for(int i = modifyAddr; i < modifyAddr + 14; i++)
    {
      if(inode[i] == '#') //the time is not set
      {
        return -2;
      }
    }
    return 0;
  }
  else if(type == 'C')
  {
    for(int i = createdAddr; i < createdAddr + 14; i++)
    {
      if(inode[i] == '#') //Created time has not been set
      {
        return -2;
      }
    }
    return 0;
  }
  else //Invalid type
  {
    return -3;
  }
}

int FileSystem::setAttribute(char* buffer, char type) //Returns 0 on success and -1 if there is an invalid type passed
{
  time_t currTime;
  tm* currtm;
  char thetime[14];

  time(&currTime);
  currtm = localtime(&currTime);
  strftime(thetime, 'Z', "%Y%m%d%H%M%S", currtm);

  if(type == 'M') //We are setting the modified time
  {
    int counter = 0;
    for(int i = modifyAddr; i < modifyAddr + 14; i++)
    {
      buffer[i] = thetime[counter];
      counter++;
    }
  }
  else if(type == 'C') //We are setting created time
  {
    int counter = 0;
    for(int i = createdAddr; i < createdAddr + 14; i++)
    {
      buffer[i] = thetime[counter];
      counter++;
    }
  }
  else
  {
    return -1; //Invalid type
  }
  return 0;
}
