#ifdef _WIN32
  #include <Windows.h>
  #include <Fileapi.h>
#else
  #include <unistd.h>
  #include <fcntl.h>
#endif
#include <stdio.h>
#include <epicsTime.h>

#define XSIZE 1920
#define YSIZE 1080
#define NUM_FRAMES 6000
#define FILE_NAME "test.raw"

char buff[XSIZE*YSIZE];

int main(int argc, char *argv[]) {

  epicsTime t1, t2, t3, t4;
  int status;
  unsigned long bytesWritten;
  int numFrames = NUM_FRAMES;
  char *buff = new char[XSIZE*YSIZE];
  
  if (argc >= 2) {
    numFrames = atoi(argv[1]);
  }

  printf("Creating file with %d frames\n", numFrames);
  t1 = epicsTime::getCurrent();
  #ifdef _WIN32
    HANDLE fileHandle;
    fileHandle = CreateFileA(FILE_NAME, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
  #else
    int fileHandle = creat(FILE_NAME, 0777);
  #endif  
  
  printf("Writing data\n");
  t2 = epicsTime::getCurrent();
  for (int i=0; i<numFrames; i++) {
    #ifdef _WIN32
      //printf("writing frame %d\n", i);
      status = WriteFile(fileHandle, buff, XSIZE*YSIZE, &bytesWritten, 0);
    #else
      bytesWritten = write(fileHandle, buff, XSIZE*YSIZE);
    #endif
  }
  t3 = epicsTime::getCurrent();
    
  printf("Closing file\n");
  #ifdef _WIN32
    status = CloseHandle(fileHandle);
  #else 
    status = close(fileHandle);
  #endif
  t4 = epicsTime::getCurrent();
  printf("Time to open file:  %f\n", t2-t1);
  printf("Time to write data: %f\n", t3-t2);
  printf("Time to close file: %f\n", t4-t3);
  printf("Total time:         %f\n", t4-t1);
  printf("I/O rate (MB/s):    %f\n", double(XSIZE)*YSIZE*numFrames/(t3-t2)/1024./1024.);

}