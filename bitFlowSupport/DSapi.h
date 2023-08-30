/***************************************************************************
*
* FILE: dsapi.h
*
* PURPOSE: Public interface into Display Surface DLL
*
* LEGAL THINGS:
*
* Copyright (C) 1994 by BitFlow, Inc.  All Rights Reserved.
*
* REVISION HISTORY:
*
* 07/11/96	    rjd - stolen from RAptor SDK DSDLL
* 11/10/2015    jtg - stolen from BitFlow SDK. Reimplemented to use the ImageView library as a backend.
*
***************************************************************************/

#ifndef INCLUDED__BIT_FLOW__DISP__SURF__API__H
#define INCLUDED__BIT_FLOW__DISP__SURF__API__H
#define _DSAPI_

#include "BFDef.h"

#define DISP_SURF_IS_IMAGE_VIEW (1)

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

    BFDLL BOOL BFCAPI DispSurfCreate(PBFS32 DspSurfHandle, BFU32  dx, BFU32  dy, BFU32 PixDepth, HWND hWndOwner);
    BFDLL BOOL BFCAPI DispSurfGetBitmap(BFS32 DspSurfHandle, PBFVOID *pBitmap);
    BFDLL BOOL BFCAPI DispSurfTop(BFS32 DspSurfHandle);
    BFDLL BOOL BFCAPI DispSurfBlit(BFS32 DspSurfHandle);
    BFDLL BOOL BFCAPI DispSurfFormatBlit(BFS32 DspSurfHandle, PBFU32 pBuffer, BFU32 PixDepth, BFU32 options);
    BFDLL BOOL BFCAPI DispSurfChangeSize(BFS32 DspSurfHandle, BFU32  dx, BFU32  dy, BFU32 PixDepth);
    BFDLL BOOL BFCAPI DispSurfGetLut(BFS32 DspSurfHandle, PBFU8  pLut);
    BFDLL BOOL BFCAPI DispSurfClose(BFS32 DspSurfHandle);
    BFDLL BOOL BFCAPI DispSurfIsOpen(BFS32 DspSurfHandle);
    BFDLL BOOL BFCAPI DispSurfDisableClose(BFS32 DspSurfHandle, BFBOOL Enabled);

    BFDLL BOOL BFCAPI DispSurfVersion(PBFU32 pMajorVersion, PBFU32 pMinorVersion);

    BFDLL BOOL BFCAPI DispSurfOffset(BFS32 DspSurfHandle, BFS32 DX, BFS32 DY);
    BFDLL BOOL BFCAPI DispSurfSetWindow(BFS32 DspSurfHandle, BFU32 Left, BFU32 Top, BFU32 Width, BFU32 Height);
    BFDLL BOOL BFCAPI DispSurfGetWindow(BFS32 DspSurfHandle, PBFU32 pLeft, PBFU32 pTop, PBFU32 pWidth, PBFU32 pHeight);
    BFDLL BOOL BFCAPI DispSurfTitle(BFS32 DspSurfHandle, PBFCHAR Title);
    BFDLL BOOL BFCAPI DispSurfGetZoom(BFS32 DspSurfHandle, PBFU32 pZoom);
    BFDLL BOOL BFCAPI DispSurfSetZoom(BFS32 DspSurfHandle, BFU32 Zoom);

    BFDLL BOOL BFCAPI DispSurfAfxCrtWorkaround_CrtSetDbgFlag(int flags);

#ifdef __cplusplus
}
#endif

#endif // INCLUDED__BIT_FLOW__DISP__SURF__API__H
