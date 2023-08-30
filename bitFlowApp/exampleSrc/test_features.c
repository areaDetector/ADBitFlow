#include    <stdio.h>
#include    <string.h>
#include    <conio.h>
#include  "CiApi.h"
#include	"BFApi.h"
#include	"BFGTLUtilApi.h"

int openIntegerNode(BFGTLDev hDev, char *nodeName, BFGTLNode *hNode) {
	// BFGTLUtil open the node
	size_t size;
  int isImplemented = BFGTLDevNodeExists(hDev, nodeName);
  if (!isImplemented) {
    printf("openIntegerNode, node %s is not implemented\n", nodeName);
    return -1;
  }
	if (BFGTLNodeOpen(hDev, nodeName, hNode)) {
	  printf("BFGTLNodeOpen failed to open node %s\n", nodeName);
		return -1;
  }
  int type;
  size = sizeof(type);
  if (BFGTLNodeRead(*hNode, BFGTL_NODE_TYPE, &type, &size)) {
	  printf("BFGTLNodeInquire BFGTL_NODE_TYPE failed for node %s\n", nodeName);
		return -1;
  }
  printf("openIntegerNode nodeName=%s, type=%d\n", nodeName, type);
  int access;
  size = sizeof(access);
  if (BFGTLNodeRead(*hNode, BFGTL_NODE_ACCESS, &access, &size)) {
	  printf("BFGTLNodeInquire BFGTL_NODE_ACCESS failed for node %s\n", nodeName);
		return -1;
  }
  printf("openIntegerNode nodeName=%s, access=%d\n", nodeName, access);
  return 0;
}

int readIntegerNode(BFGTLNode hNode, char *nodeName, BFU64 *value) {
  size_t size = sizeof(*value);
  int status = BFGTLNodeRead(hNode, BFGTL_NODE_VALUE, value, &size);
  if (status) {
	  printf("readIntegerNode: BFGTLNodeRead failed for node %s, status=%d (0x%x)\n", nodeName, status, status);
		return -1;
	}
  printf("readIntegerNode nodeName=%s, value=%lld\n", nodeName, *value);
  return 0;
}

int writeIntegerNode(BFGTLNode hNode, char *nodeName, BFU64 value) {
  size_t size = sizeof(value);
  int status = BFGTLNodeWrite(hNode, BFGTL_NODE_VALUE, &value, size);
  if (status) {
	  printf("writeIntegerNode: BFGTLNodeWrite failed for node %s, status=%d (0x%x)\n", nodeName, status, status);
		return -1;
	}
  printf("writeIntegerNode nodeName=%s, value=%lld\n", nodeName, value);
  return 0;
}

main(int argc, char *argv[])
{
	CiENTRY entry;
	BFU32	Options;
	BFU32	BoardNumber = 0xffff;
	BFCHAR	ModelSt[MAX_STRING];
	BFCHAR	FamilySt[MAX_STRING];
	BFU32	FamilyIndex;
	BFU32	CiFamily;
	BFBOOL	SepCam = FALSE;
	BFBOOL	CXPCom = FALSE;
	int i;
	BFBOOL Ready;
	BFCHAR CamName[MAX_STRING];
  BFGTLDev hBFGTLUtilDevice = BFNULL;
	BFGTLNode hNode;
  Bd hBoard;
  BFU64 value;
  char nodeName[MAX_STRING];

	if (CiSysBrdFind(CISYS_TYPE_ANY, 0, &entry))
	{
		printf("Could not find board.\n");
		return -1;
	}

	// set default options
	Options = CiSysInitialize;

	if (CiBrdOpen(&entry, &hBoard, Options))
	{
		printf("Could not open board.\n");
		return -1;
	}
	BFGetBoardStrings(hBoard, ModelSt, MAX_STRING, FamilySt, MAX_STRING, &FamilyIndex, &CiFamily);
	printf("Board \"%s(%d) - %s\" has been opened.\n", FamilySt, entry.Number, ModelSt);
	printf("Attached camera list:\n");
	i = 0;
	while (CiBrdCamGetFileName(hBoard, i, CamName, sizeof(CamName)) == CI_OK)
	{
		printf("%d - %s\n", i, CamName);
		i++;
	}

	printf("Board is acquiring from camera\n");
	CiConIsCameraReady(hBoard, &Ready);
	if (Ready)
		printf("System reports camera is ready\n");
	else
		printf("System reports camera not ready\n");

	// BFGTLUtil open device
	if (BFGTLDevOpen(hBoard, &hBFGTLUtilDevice)) {
		printf("Error opening BFGTL Device\n");
		return -1;
	}

  strcpy(nodeName, "BlackLevel");
  printf("\nTesting %s\n", nodeName);
  if (openIntegerNode(hBFGTLUtilDevice, nodeName, &hNode)) {
		printf("Error opening node %s\n", nodeName);
		return -1;
  }
  readIntegerNode(hNode, nodeName, &value);
  writeIntegerNode(hNode, nodeName, value+1);
  readIntegerNode(hNode, nodeName, &value);
  BFGTLNodeClose(hNode);

  strcpy(nodeName, "ExposureTime");
  printf("\nTesting %s\n", nodeName);
  if (openIntegerNode(hBFGTLUtilDevice, nodeName, &hNode)) {
		printf("Error opening node %s\n", nodeName);
		return -1;
  }
  readIntegerNode(hNode, nodeName, &value);
  writeIntegerNode(hNode, nodeName, value+1);
  readIntegerNode(hNode, nodeName, &value);
  BFGTLNodeClose(hNode);

  strcpy(nodeName, "FileAccessLength");
  printf("\nTesting %s\n", nodeName);
  if (openIntegerNode(hBFGTLUtilDevice, nodeName, &hNode)) {
		printf("Error opening node %s\n", nodeName);
		return -1;
  }
  readIntegerNode(hNode, nodeName, &value);
  writeIntegerNode(hNode, nodeName, value+1);
  readIntegerNode(hNode, nodeName, &value);
  BFGTLNodeClose(hNode);

  strcpy(nodeName, "Gamma");
  printf("\nTesting %s\n", nodeName);
  if (openIntegerNode(hBFGTLUtilDevice, nodeName, &hNode)) {
		printf("Error opening node %s\n", nodeName);
		return -1;
  }
  readIntegerNode(hNode, nodeName, &value);
  writeIntegerNode(hNode, nodeName, value+1);
  readIntegerNode(hNode, nodeName, &value);
  BFGTLNodeClose(hNode);

	// close system
	CiBrdClose(hBoard);

	return 0;
}

