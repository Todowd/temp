
/* Driver 8*/

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include "disk.h"
#include "diskmanager.h"
#include "partitionmanager.h"
#include "filesystem.h"
#include "client.h"

using namespace std;

/*
  This driver will test the getAttributes() and setAttributes()
  functions. You need to complete this driver according to the
  attributes you have implemented in your file system, before
  testing your program.
  
  
  Required tests:
  get and set on the fs1 on a file
    and on a file that doesn't exist
    and on a file in a directory in fs1
    and on a file that doesn't exist in a directory in fs1

 fs2, fs3
  on a file both get and set on both fs2 and fs3

  samples are provided below.  Use them and/or make up your own.


*/

int main()
{

  Disk *d = new Disk(300, 64, const_cast<char *>("DISK1"));
  DiskPartition *dp = new DiskPartition[3];

  dp[0].partitionName = 'A';
  dp[0].partitionSize = 100;
  dp[1].partitionName = 'B';
  dp[1].partitionSize = 75;
  dp[2].partitionName = 'C';
  dp[2].partitionSize = 105;

  DiskManager *dm = new DiskManager(d, 3, dp);
  FileSystem *fs1 = new FileSystem(dm, 'A');
  FileSystem *fs2 = new FileSystem(dm, 'B');
  FileSystem *fs3 = new FileSystem(dm, 'C');
  Client *c1 = new Client(fs1);
  Client *c2 = new Client(fs2);
  Client *c3 = new Client(fs3);
  Client *c4 = new Client(fs1);
  Client *c5 = new Client(fs2);



  int r, f1;
  cout << endl << "begin driver 8" << endl;
  cout << "Written to work after running drivers 1-7" << endl;
  cout << "Filesystem 1 Tests" << endl;
  r = c1->myFS->createFile( const_cast<char *>("/w"), 2);
  cout << "rv from createFile /w is " << r << (r==0?" Correct": " failure") <<endl;   

  r = c1->myFS->getAttribute(const_cast<char *>("/w"), 2, 'Z');
  cout << "rv from getAttribute /w is " << r << (r==-3? " Correct, invalid type" : " failure") << endl;

  r = c1->myFS->getAttribute(const_cast<char *>("/w"), 2, 'C');
  cout << "rv from getAttribute /w is " << r << (r==0? " Correct, has stored time created" : " failure") << endl;

  r = c1->myFS->getAttribute(const_cast<char *>("/w"), 2, 'M');
  cout << "rv from getAttribute /w is " << r << (r==-2? " Correct, file has not been modified" : " failure") << endl;

  r = c1->myFS->getAttribute(const_cast<char *>("/z"), 2, 'M');
  cout << "rv from getAttribute /z is " << r << (r==-1? " Correct, file does not exist" : " failure") << endl;

  f1 = c1->myFS->openFile( const_cast<char *>("/w"), 2, 'w', -1);
  cout << "rv from openFile /w is " << f1 << (f1>0 ? " Correct file open (r) f1": " failure") <<endl;

  char buf1[37];
  for (int i = 0; i < 37; i++) buf1[i] = 's';

  r = c1->myFS->writeFile(f1, buf1, 5);
  cout << "rv from writeFile /w f1 is " << r <<(r==5 ? " Correct wrote 5 s": " failure") <<endl;

  r = c1->myFS->getAttribute(const_cast<char *>("/w"), 2, 'M');
  cout << "rv from getAttribute /w is " << r << (r==0? " Correct, file was recently modified" : " failure") << endl;

  r = c1->myFS->createFile( const_cast<char *>("/b"), 2);
  cout << "rv from createFile /b is " << r << (r==0?" Correct": " failure") <<endl;

  int f2;
  f2 = c1->myFS->openFile( const_cast<char *>("/b"), 2, 'w', -1);
  cout << "rv from openFile /b is " << f2 << (f2>0 ? " Correct file open (r) f2": " failure") <<endl;

  r = c1->myFS->appendFile(f2, buf1, 4);
  cout << "rv from appendFile is " << r << (r==4 ? " correct 4 characters written": " fail") <<endl;

  r = c1->myFS->getAttribute(const_cast<char *>("/b"), 2, 'M');
  cout << "rv from getAttribute /b is " << r << (r==0? " Correct, file was recently modified" : " failure") << endl;

  r = c1->myFS->truncFile(f2, 0, 1);
  cout << "rv from truncFile /b fs3 (f3) seek to 0, removed " << r << (r==4 ? " correct":" fail")<<endl;

  r = c1->myFS->getAttribute(const_cast<char *>("/b"), 2, 'M');
  cout << "rv from getAttribute /b is " << r << (r==0? " Correct, file was recently modified" : " failure") << endl;

  r = c1->myFS->closeFile(f1);
  cout << "rv from closeFile f5 /a is " << r << (r==0 ? " Correct file closed": " failure")<<endl;

  r = c1->myFS->closeFile(f2);
  cout << "rv from closeFile f5 /b is " << r << (r==0 ? " Correct file closed": " failure")<<endl;

  r= c1->myFS->createDirectory(const_cast<char *>("/v"), 2);
  cout << "rv from createDirectory /v is " << r << (r==0 ? " Correct directory created": " failure")<<endl; 

  r = c1->myFS->createFile( const_cast<char *>("/v/a"), 4);
  cout << "rv from createFile /v/a is " << r << (r==0?" Correct": " failure") <<endl;

  f2 = c1->myFS->openFile( const_cast<char *>("/v/a"), 4, 'w', -1);
  cout << "rv from openFile /v/a is " << f2 << (f2>0 ? " Correct file open (r) f2": " failure") <<endl;

  r = c1->myFS->getAttribute(const_cast<char *>("/v/a"), 4, 'C');
  cout << "rv from getAttribute /v/a is " << r << (r==0? " Correct, file was created and timestamp added" : " failure") << endl;

  r = c1->myFS->getAttribute(const_cast<char *>("/v/c"), 2, 'C');
  cout << "rv from getAttribute /v/c is " << r << (r==-1? " Correct, file in directory does not exist" : " failure") << endl;

  r = c1->myFS->closeFile(f2);
  cout << "rv from closeFile f5 /v/a is " << r << (r==0 ? " Correct file closed": " failure")<<endl;

  cout << endl;
  cout << "Filesystem 2 Tests" << endl;

  r = c2->myFS->deleteFile(const_cast<char *>("/o/o/o/d"), 8);  
  cout << "rv from deleteFile /o/o/o/d is " << r << (r==0 ? " Correct file deleted": " failure")<<endl;  

  r = c2->myFS->createFile(const_cast<char *>("/w"), 2);
  cout << "rv from createFile /w on fs2 is " << r << (r==0?" Correct": " failure") <<endl;

  f2 = c2->myFS->openFile( const_cast<char *>("/w"), 2, 'w', -1);
  cout << "rv from openFile /w on fs2 is " << f2 << (f2>0 ? " Correct file open (r) f2": " failure") <<endl;

  r = c2->myFS->getAttribute(const_cast<char *>("/w"), 2, 'C');
  cout << "rv from getAttribute /v/a is " << r << (r==0? " Correct, file was created and timestamp added" : " failure") << endl;

  r = c2->myFS->writeFile(f2, buf1, 5);
  cout << "rv from writeFile /w f2 on fs2 is " << r <<(r==5 ? " Correct wrote 5 s": " failure") <<endl;

  r = c2->myFS->getAttribute(const_cast<char *>("/w"), 2, 'M');
  cout << "rv from getAttribute /w is " << r << (r==0? " Correct, file was modified and timestamp added" : " failure") << endl;

  r = c2->myFS->closeFile(f2);
  cout << "rv from closeFile f5 /w is " << r << (r==0 ? " Correct file closed": " failure")<<endl;

  cout << endl;
  cout << "Filesystem 3 Tests" << endl;

  r = c3->myFS->createFile(const_cast<char *>("/w"), 2);
  cout << "rv from createFile /w on fs3 is " << r << (r==0?" Correct": " failure") <<endl;

  f2 = c3->myFS->openFile( const_cast<char *>("/w"), 2, 'w', -1);
  cout << "rv from openFile /w on fs3 is " << f2 << (f2>0 ? " Correct file open (r) f2": " failure") <<endl;

  r = c3->myFS->getAttribute(const_cast<char *>("/w"), 2, 'C');
  cout << "rv from getAttribute /w is " << r << (r==0? " Correct, file was created and timestamp added" : " failure") << endl;

  r = c3->myFS->writeFile(f2, buf1, 5);
  cout << "rv from writeFile /w f3 on fs2 is " << r <<(r==5 ? " Correct wrote 5 s": " failure") <<endl;

  r = c3->myFS->getAttribute(const_cast<char *>("/w"), 2, 'M');
  cout << "rv from getAttribute /w is " << r << (r==0? " Correct, file was modified and timestamp added" : " failure") << endl;

  r = c3->myFS->closeFile(f2);
  cout << "rv from closeFile f5 /w on fs3 is " << r << (r==0 ? " Correct file closed": " failure")<<endl;
  return 0;
}
