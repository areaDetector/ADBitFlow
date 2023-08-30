#if defined(WIN32) && defined(_DEBUG)
#   define _CRTDBG_MAP_ALLOC
#   include <stdlib.h>
#   include <crtdbg.h>
#endif

#include    <stdio.h>
#include    <conio.h>
#include    "CiApi.h"
#include	"BFApi.h"
#include	"BFErApi.h"
#include	"DSApi.h"
#include	"BFGTLUtilApi.h"

#define     ERR_LIMIT   3   // Allow a few repeats to facilitate camera configuration debugging.
#define		USE_DISPLAY		// define to create a live display of the image

#ifdef USE_DISPLAY
UINT Thread(LPVOID lpdwParam);
int hDspSrf = -1;  // handle to display surface
PBFVOID pBitmap = BFNULL;
HANDLE hThread;
DWORD dwThreadId;
#endif

Bd		hBoard;
BFBOOL Running = FALSE;
BFBOOL KeepUpdating = FALSE;

main(int argc, char *argv[])
{
	CiENTRY entry;
	PBFU8	HostBuf;
#ifndef USE_DISPLAY
	PBFU16	p16Buf;
	PBFU32	p32Buf;
#else
	HANDLE hThread;
	DWORD dwThreadId;
#endif
	BFBOOL	WasOneShot = FALSE;
	BFBOOL	ContinuousData = FALSE;
	BFU32	keeplooping;
	BFU32	ImageSize, BitDepth;
	BFU32	overflowcount = 0;
	BFU32	Type, Num, Init, SerNum;
	BFU32	Switch = 0, Connector = 0;
	BFBOOL	ChooseBoard = TRUE;
	BFBOOL	DefaultCam = TRUE;
	BFCHAR	CamFileName[MAX_STRING];
	BFBOOL	NeedsHelp = FALSE;
	BFU32	Options;
	BFU32	BoardNumber = 0xffff;
	BFCHAR	ModelSt[MAX_STRING];
	BFCHAR	FamilySt[MAX_STRING];
	BFU32	FamilyIndex;
	BFU32	CiFamily;
	BFBOOL	SepCam = FALSE;
	BFBOOL	CXPCom = FALSE;
	PBFCNF	pSepCam;
	int i;
	BFS32 keyhit;
	BFU32 LastRC;
	BFCHAR Str[MAX_STRING];
	BFBOOL Ready;
	BFCHAR CamName[MAX_STRING];
	BFCHAR NodeName[MAX_STRING];
	BFCHAR ValueBuf[MAX_STRING];
	BFSIZET ValueBufLen;
    BFGTLDev hBFGTLUtilDevice = BFNULL;
	BFGTLNode hNode;


#if defined(_MSC_VER) && defined(_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif


	// parse arguments
	if (argc != 1)
	{
		
		for (i = 1; i < argc; i++)
		{
			if (_stricmp(argv[i], "-B") == 0)
			{
				if (i + 1 >= argc)
					NeedsHelp = TRUE;
				else
				{
					// get the switch argument
					sscanf_s(argv[i + 1], "%d", &BoardNumber);
					ChooseBoard = FALSE;
				}
			}
			else if (_stricmp(argv[i], "-Switch") == 0)
			{
				if (i + 1 >= argc)
					NeedsHelp = TRUE;
				else
				{
					// get the switch argument
					sscanf_s(argv[i + 1], "%d", &Switch);
					ChooseBoard = FALSE;
				}
			}
			else if (_stricmp(argv[i], "-Con") == 0)
			{
				if (i + 1 >= argc)
					NeedsHelp = TRUE;
				else
				{
					// get the switch argument
					sscanf_s(argv[i + 1], "%d", &Connector);
					ChooseBoard = FALSE;
				}
			}
			else if (_stricmp(argv[i], "-Cam") == 0)
			{
				if (i + 1 >= argc)
					NeedsHelp = TRUE;
				else
				{
					// get the switch argument
					sscanf_s(argv[i + 1], "%s", &CamFileName, MAX_STRING);
					DefaultCam = FALSE;
				}
			}
			else if (_stricmp(argv[i], "-SepCam") == 0)
			{
				SepCam = TRUE;
			}
			else if (_stricmp(argv[i], "-CXPCom") == 0)
			{
				if (i + 1 >= argc)
					NeedsHelp = TRUE;
				else
				{
					// get the switch argument
					sscanf_s(argv[i + 1], "%s", &NodeName, MAX_STRING);
					CXPCom = TRUE;
				}
			}
			else if (_stricmp(argv[i], "-?") == 0 || _stricmp(argv[i], "-help") == 0)
			{
				NeedsHelp = TRUE;
			}
		}
	}
				

	if (NeedsHelp)
	{
		printf("Usage:\n");
		printf("CiSimple [ -B BrdNum | -Switch N -Con C ] [-Cam CamFileName [-SepCam]] [-CXPCom NodeName]\n");
		printf("-?, -help   Show this message\n");
		printf("-B BrdNum   Open board number BrdNum (do not use with -Switch and -Con)\n");
		printf("-Switch     Open board with switch set to N (do not use with -Brd)\n");
		printf("-Con        Open board associated with Connector (do not use with -Brd)\n");
		printf("-Cam CamFileName       Use the camera file CamFileName instead of what is set in registry\n");
		printf("-SepCam     Open the camera file as a separate step \n");
		printf("-CXPCom NodeName       Use BFGTLUtil to write to CXP parameter NodeName\n");
		return -1;
		
	}

	// was the board specified?
	if (ChooseBoard)
	{
		// no arguments passed in, use the board open dialog
		if( DoBrdOpenDialog(BOD_HIDEJUSTOPEN, FF_BITFLOW_MODERN, &Type, &Num, &Init, &SerNum) )
		{
			return -1;
		}

		if( CiSysBrdFind(Type, Num, &entry ))
		{
			printf("Could not find board.\n");
			return -1;
		}
	}
	else
	{
		if (BoardNumber != 0xffff)
		{
			if( CiSysBrdFind(CISYS_TYPE_ANY, BoardNumber, &entry ))
			{
				printf("Could not find board.\n");
				return -1;
			}
		}
		else
		{
			if (CiSysBoardFindSWConnector(FF_BITFLOW_MODERN, Switch, Connector, &entry))
			{
				printf("Error: Board with Switch = %d and Connector = %d not found\n", Switch, Connector);
				return -1;
			}
		}

	}

	// set default options
	Options = CiSysInitialize;

	if (DefaultCam)
	{
		// open the board using default camera file
		if (CiBrdOpen(&entry, &hBoard, Options))
		{
			printf("Could not open board.\n");
			return -1;
		}
	}
	else
	{
		if (SepCam)
		{
			// open the board using default camera file
			if (CiBrdOpen(&entry, &hBoard, Options))
			{
				printf("Could not open board.\n");
				return -1;
			}

			if (CiCamOpen(hBoard, CamFileName, &pSepCam))
			{
				printf("Could not open camera file %s.\n", CamFileName);
				return -1;
			}

			if (CiBrdCamSetCur(hBoard, pSepCam, CiSysConfigure))
			{
				printf("Could not open camera file %s.\n", CamFileName);
				return -1;
			}
		}
		else
		{
			// open the board using camera file passed in
			if (CiBrdOpenCam(&entry, &hBoard, Options, CamFileName))
			{
				printf("Could not open board.\n");
				return -1;
			}
		}
	}

	if (BFIsCtnII(hBoard)) printf("Cyton II\n");
	if (BFIsAxnII(hBoard)) printf("Axion II\n");
	if (BFIsAonII(hBoard)) printf("Aon II\n");

	BFGetBoardStrings(hBoard, ModelSt, MAX_STRING, FamilySt, MAX_STRING, &FamilyIndex, &CiFamily);

	printf("Board \"%s(%d) - %s\" has been opened.\n", FamilySt, entry.Number, ModelSt);

	printf("Attached camera list:\n");
	i = 0;
	while (CiBrdCamGetFileName(hBoard, i, CamName, sizeof(CamName)) == CI_OK)
	{
		printf("%d - %s\n", i, CamName);
		i++;
	}


	if (BFIsSynthetic(hBoard))
		printf("Board is using synthetic image\n");
	else
	{
		printf("Board is acquiring from camera\n");
		CiConIsCameraReady(hBoard, &Ready);
		if (Ready)
			printf("System reports camera is ready\n");
		else
			printf("System reports camera not ready\n");

	}

	if (CXPCom && BFIsCXP(hBoard))
	{
		if (BFIsCXP(hBoard))
		{
			// BFGTLUtil open device
			if (BFGTLDevOpen(hBoard, &hBFGTLUtilDevice) != BF_OK)
			{
				printf("Error opening BFGTL Device\n");
				CXPCom = FALSE;
			}
		}
		else
		{
			printf("CXPCom option specified but board is not CXP\n");
		}
	}


	// find out how big the image is
	CiBrdInquire(hBoard,CiCamInqFrameSize0,&ImageSize);
	CiBrdInquire(hBoard,CiCamInqPixBitDepth,&BitDepth);

	// allocate host memory to hold the image
	HostBuf = (PBFU8)_aligned_malloc(ImageSize, 4096);

	// did we get the memory?
	if (HostBuf == BFNULL)
	{
		printf("Host memory buffer could not be allocated - exit.\n");
		CiBrdClose(hBoard);
		return -1;
	}


#ifdef USE_DISPLAY

	KeepUpdating = TRUE;

	hThread = CreateThread(BFNULL,0,(LPTHREAD_START_ROUTINE)Thread,BFNULL, 0, &dwThreadId);
	
	if (hThread == BFNULL) 
	{
		printf("Cannot create thread.\n");
		
		CiBrdClose(hBoard);
		return -1;
	}

	SetPriorityClass(GetCurrentProcess(),ABOVE_NORMAL_PRIORITY_CLASS);


	// wait for thread
	while (!Running)
		Sleep(10);


	// clear memory
	memset(HostBuf, 0, ImageSize);
	// send image to display
	DispSurfFormatBlit(hDspSrf, (PBFU32) HostBuf, BitDepth, BFDISP_FORMAT_NORMAL);

#endif



	// set up board for acquisistion
	if (CiAqSetup(hBoard,  (PBFVOID)HostBuf,  ImageSize, 0, CiDMADataMem,
				  CiLutBypass, CiLut8Bit, CiQTabBank0, TRUE, CiQTabModeOneBank, 
				  AqEngJ))
	{
		BFErrorShow(hBoard);
		printf("Setting up for acquisition failed - exit.\n");
		free(HostBuf);
		CiBrdClose(hBoard);
		return -1;
	}

	printf("Hit 'q' key to quit\n");
	if (CXPCom) printf("Hit 'c' to change %s\n", NodeName);

	// main work loop, snap an image, then process until key hit
	keeplooping = 0;
	keyhit = 0;
	while(keyhit != (BFS32)'q'  && keeplooping < ERR_LIMIT)
	{

		// snap one frame
		if (CiAqCommand(hBoard, CiConSnap, CiConWait, CiQTabBank0, AqEngJ))
		{
			BFErrorShow(hBoard);
			keeplooping++;
		}
		
#ifdef USE_DISPLAY
		if (DispSurfIsOpen(hDspSrf))
		{
			// send image to display
			DispSurfFormatBlit(hDspSrf, (PBFU32)HostBuf, BitDepth, BFDISP_FORMAT_NORMAL);
		}
		
#else
		// processing goes here
		if (BitDepth == 8)
			printf("Pixel[0,0] = %x, Pixel[1,0] = %x, OVF = %x\n",*HostBuf,*(HostBuf + 1),overflowcount);
		else if (BitDepth <= 16)
		{
			p16Buf = (PBFU16) HostBuf;
			printf("Pixel[0,0] = %x, Pixel[1,0] = %x, OVF = %x\n",*p16Buf,*(p16Buf + 1),overflowcount);
		}
		else if (BitDepth <= 32)
		{
			p32Buf = (PBFU32) HostBuf;
			printf("Pixel[0,0] = %x, Pixel[1,0] = %x, OVF = %x\n",*p32Buf,*(p32Buf + 1),overflowcount);
		}
#endif
		if (BFkbhit())
			keyhit = BFgetch();

		if (CXPCom && hBFGTLUtilDevice && keyhit == (BFS32) 'c')
		{
			// BFGTLUtil open the node
			if (BFGTLNodeOpen(hBFGTLUtilDevice, NodeName, &hNode) == BF_OK)
			{
				// BFGTLUtil read the node
				ValueBufLen = sizeof(ValueBuf);
				BFGTLNodeRead(hNode, BFGTL_NODE_VALUE_STR, ValueBuf, &ValueBufLen);
				printf("Node: %s, Value: %s\n", NodeName, ValueBuf);

				printf("Node: %s, Enter new value: ", NodeName);
				scanf_s("%s", &ValueBuf, sizeof(ValueBuf));
				printf("\n");
				// BFGTLUtil write the node
				ValueBufLen = sizeof(ValueBuf);
				BFGTLNodeWrite(hNode, BFGTL_NODE_VALUE_STR, ValueBuf, ValueBufLen);
				keyhit = 0;
			}
			else
			{
				 printf("Node: %s does not exist in this camera.\n", NodeName);
			}
			// BFGTLUtil write command
		}
	
	}

	KeepUpdating = FALSE;

	// wait for thread end
	while (Running)
		Sleep(10);

	// did loop stop because of too many errors?
	if (keeplooping >= ERR_LIMIT)
		printf("Too many errors occurred - exit.\n");

	// absorb key stroke
	if (BFkbhit()) BFgetch();

	// clean up acquisition resources
	CiAqCleanUp(hBoard, AqEngJ);

	// BFGTLUtil close device
	if (CXPCom && hBFGTLUtilDevice)
		BFGTLDevClose(hBFGTLUtilDevice);

	// free buffer
	_aligned_free(HostBuf);

	/*
	* There should not be any errors in the stack at this point
	* but the code below walks the error stack and prints out 
	* any errors. This is here mainly to illustrate how this
	* can be done. 
	*/

	// Check to see if there are any errors on the stack
	BFErrorGetLast(hBoard, &LastRC);

	// if there are errors, walk the error stack and print them out 
	while (LastRC != BF_OK)
	{
		// get the error text and print it out
		BFErrorGetMes(hBoard, LastRC, Str, sizeof(Str));
		printf(Str);
		printf("\n");

		// remove error from stack
		BFErrorClearLast(hBoard);

		// check for more errors
		BFErrorGetLast(hBoard, &LastRC);
	}

	if (SepCam)
		CiCamClose(hBoard, pSepCam);

	// close system
	CiBrdClose(hBoard);

	return 0;
}

#ifdef USE_DISPLAY
UINT Thread(LPVOID lpdwParam)
{
	BFU32 xsize,ysize;
	MSG Msg;
	BFU32 BitDepth;
	
	CiBrdInquire(hBoard,CiCamInqXSize,&xsize);
	CiBrdInquire(hBoard,CiCamInqYSize0,&ysize);
	CiBrdInquire(hBoard,CiCamInqPixBitDepthDisplay,&BitDepth);

	// create display surface
	if (!DispSurfCreate((__int32 *)&hDspSrf, xsize, ysize, BitDepth, BFNULL))
	{
		printf("Display surface could not be opened - exit.\n");
		return -1;
	}


	// get pointer to bitmap data memory
	if (!DispSurfGetBitmap(hDspSrf,&pBitmap))
	{
		printf("Memory buffer for display surface's bitmap could not be obtained - exit.\n");
		return -1;
	}

	Running = TRUE;
	
	while (KeepUpdating)
	{
		//needed for display window
		if(PeekMessage(&Msg,BFNULL,0,0,PM_REMOVE))
			DispatchMessage(&Msg);
	}

	DispSurfClose(hDspSrf);

	Running = FALSE;

	return 0;
}
#endif