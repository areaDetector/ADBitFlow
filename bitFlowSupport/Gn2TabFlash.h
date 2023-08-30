//
// Creation:    Gn2TabFlash.h
// Created:     May 6, 2002
// Creator:     Bob Sheff
//
// Copyright (C) 1993-2002 by BitFlow, Inc.  All Rights Reserved.
//
// Gn2 Flash Register Id Definitons
//
// History:
//
// 05/06/02     rbs     Created file.
//

#if !defined(__Gn2TABFLASH__)
#define __Gn2TABFLASH__

// Flash Filter Flags for Gn2

typedef enum _Gn2FlashFilter
{
	CytonFlash70		= 0x00000001,		// Cyton with FPGA 70.
	CytonFlash110		= 0x00000002,		// Cyton with FPGA 110
	AxionFlash1XE		= 0x00000004,		// Axion 1XE
	AxionFlash2XE		= 0x00000008,		// Axion 2XE
	AxionFlash1XB		= 0x00000010,		// Axion 1XB
	AxionFlash2XB		= 0x00000020,		// Axion 2XB
	AxionFlash4XB		= 0x00000040,		// Axion 4XB
	AonFlash			= 0x00000080,		// Aon CXP1
	ClaxonFlashCXP2E	= 0x00000100,		// Claxon CXP2 Ecqologic
	ClaxonFlashCXP4E	= 0x00000200,		// Claxon CXP4 Ecqologic
	ClaxonFlashCXP2T2	= 0x00000400,		// Claxon CXP2 2 Transmit Ecqologic
	ClaxonFlashCXP4T4	= 0x00000800,		// Claxon CXP4 4 Transmit Ecqologic
	ClaxonFlashCXP2M	= 0x00001000,		// Claxon CXP2 MAXOM
	ClaxonFlashCXP4M	= 0x00002000,		// Claxon CXP4 MACOM
	ClaxonFlashFXP2		= 0x00004000,		// Claxon CXP2 Fiber
	ClaxonFlashFXP4		= 0x00008000,		// Claxon CXP4 Fiber
	AxionIIFlash1XE		= 0x00010000,		// Axion II 1XE
	AxionIIFlash2XE		= 0x00020000,		// Axion II 2XE
	CytonIIFlash70		= 0x00040000,		// Cyton II CXP2
	CytonIIFlash110		= 0x00080000,		// Cyton II CXP4
	AonIIFlash			= 0x00100000,		// Aon II CXP1
	ClaxonFlashCXP1E	= 0x00200000,		// Claxon CXP1 Ecqologic
} Gn2FlashFilter;

//
// Flash file Id codes for all Gn2 devices.
// these must be in the same order as the FlashEntry calls.
//

typedef enum _Gn2FlashId
{	
	FLASH_CTN70MUX = 0,
	FLASH_CTN110MUX,
	FLASH_AXN1XEMUX,
	FLASH_AXN2XEMUX,
	FLASH_AXN1XBMUX,
	FLASH_AXN2XBMUX,
	FLASH_AXN4XBMUX,
	FLASH_AONMUX,
	FLASH_CLAXON1EMUX,
	FLASH_CLAXON2EMUX,
	FLASH_CLAXON4EMUX,
	FLASH_CLAXON2T2MUX,
	FLASH_CLAXON4T4MUX,
	FLASH_CLAXON2MMUX,
	FLASH_CLAXON4MMUX,
	FLASH_CLAXON2FMUX,
	FLASH_CLAXON4FMUX,
	FLASH_AXNII1XEMUX,
	FLASH_AXNII2XEMUX,
	FLASH_AXNII2XBMUX,
	FLASH_AXNII4XBMUX,
	FLASH_CTNII70MUX,
	FLASH_CTNII110MUX,
	FLASH_AONIIMUX,

	FLASH_COUNT_GN2								// Number of flash file types. 
} Gn2FlashId;

#endif
