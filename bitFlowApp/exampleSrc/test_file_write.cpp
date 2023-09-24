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

  epicsTime startTime, endTime;
  startTime = epicsTime::getCurrent();
  int status;
  unsigned long bytesWritten;
  char *buff = new char[XSIZE*YSIZE];

  printf("Creating file\n");
  #ifdef _WIN32
    HANDLE fileHandle;
    fileHandle = CreateFileA(FILE_NAME, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
  #else
    int fileHandle = creat(FILE_NAME, 0777);
  #endif  
  
  for (int i=0; i<NUM_FRAMES; i++) {
    #ifdef _WIN32
      //printf("writing frame %d\n", i);
      status = WriteFile(fileHandle, buff, XSIZE*YSIZE, &bytesWritten, 0);
    #else
      bytesWritten = write(fileHandle, buff, XSIZE*YSIZE);
    #endif
  }
    
  printf("Closing file\n");
  #ifdef _WIN32
    status = CloseHandle(fileHandle);
  #else 
    status = close(fileHandle);
  #endif
  endTime = epicsTime::getCurrent();
  double elapsedTime = endTime - startTime;
  printf("Elapsed time: %f, I/O rate = %f MB/s\n", elapsedTime,  double(XSIZE)*YSIZE*NUM_FRAMES/elapsedTime/1024./1024.);
 
}