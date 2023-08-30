
#include    <stdio.h>
#include    <conio.h>
#include    "CiApi.h"
#include	"BFApi.h"
#include	"dsapi.h" 
#include	"BFErApi.h"

// prototype
void NewFrameCallBack(Bd Board, BFU32 Num, PBFVOID pUserData);
void CTabCallBack(Bd Board, BFU32 Num, PBFVOID pUserData);

// globals
int hDspSrf;
int g_CTabCount;
main()
{
	CiENTRY entry;
	Bd hBoard;
	PBFVOID pBitmap;

	BFU32	xsize, ysize, PixDepth;
	BFU32	ImageSize;
	MSG		Msg;
	BFU32	Type, Num, Init, SerNum;
	BFU32	UserInt;

	/*
	* Find and open the right board
	*/

	// find out what board to open
	if( DoBrdOpenDialog(BOD_HIDEJUSTOPEN, FF_BITFLOW_MODERN, &Type, &Num, &Init, &SerNum) )
	{
		return -1;
	}

	// get the entry
	if( CiSysBrdFind(Type, Num, &entry ))
	{
		printf("Could not find board.\n");
		return -1;
	}

	// Open the board
	if (CiBrdOpen(&entry, &hBoard, CiSysInitialize))
	{
		printf("Board could not be opened - exit.\n");
		return -1;
	}
	
	printf("Board has been opened.\n");

	/*
	* Open display surface
	*/

	// find out camera stuff
	CiBrdInquire(hBoard, CiCamInqXSize, &xsize);
	CiBrdInquire(hBoard, CiCamInqYSize0, &ysize);
	CiBrdInquire(hBoard, CiCamInqPixBitDepthDisplay, &PixDepth);
	CiBrdInquire(hBoard, CiCamInqDisplayFrameSize0, &ImageSize);

	hDspSrf = -1;
	g_CTabCount = 0;

	// create surface
	if (!DispSurfCreate((__int32 *)&hDspSrf, xsize, ysize, PixDepth, NULL))
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

	/*
	* Register call back
	*/

	if(CiCallBackAdd(hBoard, CiIntTypeEOD, &NewFrameCallBack,(PBFVOID) &UserInt))
	{
		printf("Error registering Call Back\n");
		return -1;
	}

	if (BFIsGn2(hBoard))
	{
		if(CiCallBackAdd(hBoard, BFIntTypeZStart, &CTabCallBack,(PBFVOID) &UserInt))
		{
			printf("Error registering Call Back\n");
			return -1;
		}
	}
	else
	{
		if(CiCallBackAdd(hBoard, CiIntTypeCTab | BFCBModeGrabOnly, &CTabCallBack,(PBFVOID) &UserInt))
		{
			printf("Error registering Call Back\n");
			return -1;
		}
	}

	/*
	* Set up for acquisition
	*/

	// set up board for acquisition to this bitmap
	if (CiAqSetup(hBoard, pBitmap, ImageSize, 0, CiDMABitmap, BFLutBypass, CiLut8Bit, CiQTabBank0, TRUE, CiQTabModeOneBank, AqEngJ))
	{
		BFErrorShow(hBoard);
		printf("Setting up for acquisition failed - exit.\n");
		return -1;
	}

	printf("Hit any key to start acquistion\n");
	BFgetch();

	/*
	* Start Acquisition 
	*/

	if (CiAqCommand(hBoard, CiConGrab, CiConAsync, CiQTabBank0, AqEngJ))
	{
		BFErrorShow(hBoard);
		printf("Acquisition commanded faile - exit.\n");
		return -1;
	}

	/*
	* Loop until key 
	*/

	printf("Hit any key to stop acquistion\n");

	// main work loop
	UserInt = 0;
	while(!_kbhit())
	{
		//needed for display window
		if(PeekMessage(&Msg,NULL,0,0,PM_REMOVE))
			DispatchMessage(&Msg);

		UserInt++;
	}

	/*
	* Clean up
	*/
	
	CiCallBackRemove(hBoard, CiIntTypeEOD);

	CiCallBackRemove(hBoard, CiIntTypeCTab);

	// stop acquistion
	CiAqCommand(hBoard, CiConAbort, CiConAsync, CiQTabBank0, AqEngJ);

	// clean up acquisition resources
	CiAqCleanUp(hBoard, AqEngJ);

	// close the display surface
	if (DispSurfIsOpen(hDspSrf))
		DispSurfClose(hDspSrf);

	// close system
	CiBrdClose(hBoard);

	return 0;
}

// bitmap callback
void NewFrameCallBack(Bd Board, BFU32 Num, PBFVOID pUserData)
{
	// if display is still open
	if (DispSurfIsOpen(hDspSrf))
	{	
		// send image to display
		DispSurfBlit(hDspSrf);
	}
}

// CTab callback
void CTabCallBack(Bd Board, BFU32 Num, PBFVOID pUserData)
{
	printf("FrameCount: %d, UserInt: %d\r",g_CTabCount++, *((PBFU32) pUserData));
}

