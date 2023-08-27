/*****************************************************************************

	BFciLib.h			Generic camera interface API

	May 29,		2008	CIW/SJT original entry
	
	© Copyright 2008, BitFlow, Inc. All rights reserved.

	Tabstops are 4

	$Author: steve $

	$Date: 2020/11/12 23:23:19 $

	$Id: BFciLib.h,v 1.85 2020/11/12 23:23:19 steve Exp $

*****************************************************************************/
/*! \mainpage BitFlow Linux SDK Manual

This documentation generated for release _RELEASE_HERE_ of the BitFlow Linux SDK

- \subpage libtypes
- \subpage system
- \subpage interrupt
- \subpage gpout
- \subpage aqsetup
- \subpage acquisition
- \subpage consumebuf
- \subpage regaccess
- \subpage regfield
- \subpage clserial
- \subpage cxpio
- \subpage luts
- \subpage misc

*/
/*==========================================================================*/
#ifndef		_BFciLib_h_
#define		_BFciLib_h_	1
/*==========================================================================*/
#ifdef __cplusplus
extern "C" {
#endif
/*==========================================================================*/
/*!
** \defgroup libtypes Library data types
** @{
*/
typedef struct tCIinterface *tCIp;		//!< anonymous access token
typedef int					tCIRC;		//!< function result code 
typedef unsigned char		tCIU8;		//!< data pointer 
typedef unsigned short		tCIU16;		//!< CTABs mostly 
typedef unsigned int		tCIU32;		//!< everything but CTABs 
typedef unsigned long long	tCIU64;		//!< really big sizes 
typedef char				*tCISTRZ;	//!< null-terminated string 
typedef double				tCIDOUBLE;	//!< floating point 
typedef void				*tCIVOIDP;	//!< pointer to something 
/*!
**	NOTE:	All block pointers should be 4 byte aligned and all blockSize
**			values should be a multiple of 8 bytes.
**
**	NOTE:	All Cyton block pointers should be 8 byte aligned and all
**			Cyton blockSize values should be a multiple of 16 bytes.
*/
typedef struct {
	tCIU8					*block;		//!< data pointer for DMA 
	tCIU64					blockSize;	//!< size of this entry 
	} tCISGentry,*tCISGP;
/*--------------------------------------------------------------------------*/
/*!
**	Data types returned with typed data descriptors
*/
typedef enum {
	kBF_invalidData			= 0,		//!< descriptor is invalid 
	kBF_tCIU32,							//!< an unsigned integer 
	kBF_tCIDOUBLE,						//!< a floating point value 
	kBF_tCISTRZ,						//!< a '\0' terminated string 
	kBF_tCIU32_pair,					//!< two 32b values 
	kBF_tCIU32_list_pair,				//!< count then value pairs of 32b 
	kBF_tCIU32_tCISTRZ_pair				//!< number and a '\0' term string
	} tCIdataTypeEnums;
/*!
**	Information description within library.
**
**	Always terminated with { NULL, kBF_invalidData, NULL }
**
**	NOTE: never modify the descriptor data in any way
*/
typedef struct {
	tCIU32					dataType;	//!< one of tCIdataTypeEnums 
	tCIU32					dataID;		//!< not used at present 
	tCISTRZ					descriptor;	//!< pointer to description of value 
	tCIVOIDP				datap;		//!< pointer to data 
	} tCItypedData, *tCITDP;
/*--------------------------------------------------------------------------*/
/*!
**	Library error return codes.
*/
typedef enum {
	kCIEnoErr				= 0,		//!< no error
	kCIEfirstEC				= -30000,
	kCIEnoAccessibleVideo=kCIEfirstEC,	//!< no access to BitFlow /dev/video<n>
	kCIErangeErr,						//!< parameter out of range 
	kCIEmemoryErr,						//!< could not allocate memory 
	kCIEbadToken,						//!< invalid tCIp 
	kCIEnullArg,						//!< unexpected NULL argument 
	kCIEinvalidArg,						//!< arg not valid for frame grabber 
	kCIEnotInitialized,					//!< camera not initialized 
	kCIEnotConfigured,					//!< camera/DMA/poll()/mmap() not configured 
	kCIEexclusiveFail,					//!< board not or already open w/exclusive wr
	kCIEexclusiveAlready,				//!< board already open w/write 
	kCIEuserDMAnoWrite,					//!< no write access to DMA buffers 
	kCIEnoWrPermission,					//!< do not have write permission
	kCIEfileError,						//!< error opening/reading file 
	kCIEfailureErr,						//!< could not flash, mmap() etc.
	kCIEbadFileFormat,					//!< bad file format 
	kCIEbadCRC,							//!< file has bad CRC 
	kCIEparseErr,						//!< error parsing file 
	kCIEfileNotFound,					//!< could not find file 
	kCIEnoEnvVar,						//!< could not find kBFenvVar 
	kCIEversionErr,						//!< driver/library do not match 
	kCIEbadInstallErr,					//!< invalid install directory tree 
	kCIEnotSupported,					//!< operation not supported on hw 
	kCIEinvalidStateErr,				//!< operation invalid in this state 
	kCIEnoDataErr,						//!< no such data 
	kCIEdmaOverrunErr,					//!< possible DMA into undelivered frame 
	kCIEdataErr,						//!< HW and OVSTEP detected 
	kCIEdmaErr,							//!< DMA aborted and did not terminate 
	kCIEuserDMAerr,						//!< user-allocated DMA buffer gone 
	kCIEnoNewData,						//!< no undelivered frames 
	kCIEbufferUnderrunErr,				//!< not enough released buffers 
	kCIEinfoAgedOut,					//!< bufferID not on history list 
	kCIEwrongMode,						//!< consume buffers not configured 
	kCIEbadID,							//!< invalid bufferID or frameID 
	kCIEtimeoutErr,						//!< timeout expired error 
	kCIEaqAbortedErr,					//!< acquisition was aborted 
	kCIEresourceBusy,					//!< resource in use 
	kCIEnotOpen,						//!< resource not initilized 
	kCIEnotMaster,						//!< only master VFG can do this 
	kCIEnotCompatibleWithMaster,		//!< incompatible master VFG setting 
	kCIEdeviceRevisionErr,				//!< device unsupported by this board
	kCIEdataHWerr,						//!< HW detected 
	kCIEdataOVSTEPerr,					//!< OVSTEP detected 
	kCIEplatformFail,					//!< no DirectGMA/GPUDirect support
	kCIEnotImplemented,					//!< function not available 
	kCIEerrCodeLast						//!< -- invalid error code -- 
	} tCIerrorCode;
/*==========================================================================*/

/*	String constants associated with installation and configuration. */

/*!	The kBFenvVar is set to the base directory of the BitFlow install.  All
**	other strings are subdirectories.
*/
#define	kBFenvVar			"BITFLOW_INSTALL_DIRS"

#define	kBFbinDir			"bin"		//!< kernel module and test tools 

#define	kBFcameraDir		"camf"		//!< camera files 

#define	kBFfirmwareDir		"fshf"		//!< firmware 

#define	kBFconfigDir		"config"	//!< board configuration 
/*--------------------------------------------------------------------------*/
/*!
**	The BITFLOW_CUSTOM_USER_FLAGS env var controls some user library behavior.
**
**	It is recommended that this value not be set in the environment.
*/
#define	kBFusrCustomFlags	"BITFLOW_CUSTOM_USER_FLAGS"

typedef enum {
	kBFnoTouchAllPages		= 1,
	kBFCXPnoLinkOK			= 1<<1,
	kBFallPagesZero			= 1<<2
	} tCIusrCustomFlagsEnum;

/*!
**	The BITFLOW_FORMAT_PPC env var can supply pixPerClock if needed.
*/
#define	kBFformatPPC		"BITFLOW_FORMAT_PPC"
/*!
**	The BITFLOW_PER_IOCTL_DELAY env var can slow down i/o if needed.
*/
#define kBFenvPerIoctlDelay	"BITFLOW_PER_IOCTL_DELAY"
/*!
**	The BITFLOW_OVERRIDE_CAMFN env var can override camera file content
*/
#define	kBFenvOverrideCamFN	"BITFLOW_OVERRIDE_CAMFN"
/*!
**	The BITFLOW_CXP_RETRY env var can override the library default value
*/
#define	kBFdefaultCXPretry	0

#define	kBFenvCXPretry		"BITFLOW_CXP_RETRY"

/*!
**	The BITFLOW_CXP_DELAY env var can insert a delay (mSec units) between CXP
**	register writes during camera init.
*/
#define	kBFenvCXPioDelay	"BITFLOW_CXP_IO_DELAY"

#define	kBFdefaultCXPioDelay	0
/**@}*/
/*==========================================================================*/
/* System Version, Open, and Initialization */
/*==========================================================================*/
/*!
** \defgroup system System Version, Open and Initialization
** @{
*/
typedef enum { kCIlibMaxDevScan = 25 } tCImaxDevEnum;

//! Max /dev/video<n> to scan
/*!
**	Each physical frame grabber will have from 1 to 4 virtual frame grabber
**	(VFG) interfaces.  Each VFG is a separate linux device (/dev/video<n>).
**
**	If for some reason there are more than kCIlibMaxDevScan /dev/videoNN
**	devices then use this call to reset the library default scan limit.
**
**	This would imply more than 6 quad VFG framegrabbers installed in one PC...
*/
void CiSetMaxDevices(tCIU32 maxDev);	
/*--------------------------------------------------------------------------*/
//!  Return library revision level 
tCIRC	CiSysGetVersions(				
			tCIU32			*libVers,	//!< CI library version info 
			tCIU32			*drvVers	//!< BitFlow driver version info 
			);							/*	kCIEnoErr
											kCIEnullArg
											kCIEnoAccessibleVideo
											kCIEversionErr
										*/
/*--------------------------------------------------------------------------*/
//! Count the BitFlow VFG 
tCIRC	CiSysVFGenum(				
			tCIU32			*nFound		//!< returns number of interfaces 
			);							/*	kCIEnoErr
											kCIEnullArg
											kCIEnoAccessibleVideo
										*/
#define	CiSysBrdEnum CiSysVFGenum
/*--------------------------------------------------------------------------*/
//! Identify a single VFG
/*!
**	NOTE: DIP switch value only valid after firmware is loaded.
**
**	NOTE: CiSysVFGinfo2() supplies a superset of this information
**
**	This call is retained only for compatibility w/previous SDK releases.
*/
tCIRC	CiSysVFGinfo(					
			tCIU32			which,		//!< 0..nFound 
			tCIU32			*ifaceType,	//!< Neon, Karbon, etc. 
			tCIU32			*switches,	//!< DIP switch value for board 
			tCIU32			*devNdx,	//!< /dev/video<Ndx> access 
			tCISTRZ			busName		//!< user-supplied storage (64 char) 
			);							/*	kCIEnoErr
											kCIEnullArg
											kCIEnoAccessibleVideo
											kCIErangeErr
											kCIEfailureErr
										*/

typedef enum {
	kCICT_Neon				= 1,		//!< Neon frame grabber 
	kCICT_NeonDIF,						//!< NeonDIF frame grabber 
	kCICT_Karbon,						//!< Karbon frame grabber 
	kCICT_Alta,							//!< Alta frame grabber 
	kCICT_KbnCXP,						//!< Karbon CXP frame grabber (deprecated)
	kCICT_CtnCXP,						//!< Cyton CXP frame grabber
	kCICT_AxnCL,						//!< Axion CL frame grabber
	kCICT_AonCXP,						//!< Aon CXP frame grabber
	kCICT_ClxCXP,						//!< Claxon CXP frame grabber
	kCICT_VFGmask			= 31,		//!< Neon/Karbon/Alta/KCXP/Ctn/Axn/Aon/Clx
	/*!
	**	Flag to identify master interface on multi-interface board
	*/
	kCICT_isMaster			= 0x1000	//!< master frame grabber for board 
	} tCIcamTypeEnums;

#define	CiSysBrdInfo	CiSysVFGinfo
/*--------------------------------------------------------------------------*/
//! More info on a single VFG
/*!
**	ifaceType is one one tCIcamTypeEnums (see above)
**
**	NOTE: DIP switch value only valid after firmware is loaded.
*/
tCIRC	CiSysVFGinfo2(					
			tCIU32			which,		//!< 0..nFound 
			tCIU32			*ifaceType,	//!< Neon, Karbon, etc. 
			tCIU32			*switches,	//!< DIP switch value for board 
			tCIU32			*devNdx,	//!< /dev/video<Ndx> access 
			tCISTRZ			busName,	//!< user-supplied storage (64 char) 
			tCIU32			*infoHi,	//!< VFG INFOHI value 
			tCIU32			*infoLo,	//!< VFG INFOLO value 
			tCISTRZ			VFGname,	//!< user supplied storage (64 char) 
			tCIU32			*NUMAnode	//!< node for this VFG
			);							/*	kCIEnoErr
											kCIEnullArg
											kCIEnoAccessibleVideo
											kCIErangeErr
											kCIEfailureErr
										*/

#define	CiSysBrdInfo2	CiSysVFGinfo2
/*--------------------------------------------------------------------------*/
//! Open a VFG 
tCIRC	CiVFGopen(					
			tCIU32			devNdx,		//!< from CiSysVFGinfo() 
			tCIU32			modeFlags,	//!< access mode flags 
			tCIp			*cip		//!< access token 
			);							/*	kCIEnoErr
											kCIEnullArg
											kCIEnoAccessibleVideo
											kCIEmemoryErr
											kCIErangeErr
											kCIEexclusiveFail
											kCIEexclusiveAlready
											kCIEfileError
											kCIEversionErr
										*/

typedef enum {
	kCIBO_readOnly			= 0,		//!< cannot modify board state 
	kCIBO_writeAccess		= 1,		//!< can modify board state 
	kCIBO_exclusiveWrAccess	= 1 << 1	//!< no previous/subsequent wr access 
	} tCIboardOpenEnums;

#define	CiBrdOpen	CiVFGopen
/*--------------------------------------------------------------------------*/
//! Returns devNdx of CiVFGopen() 
tCIU32	CiVFGindex(tCIp cip);		

#define	CiBrdIndex	CiVFGindex
/*--------------------------------------------------------------------------*/
//! Return file descriptor for VFG
tCIRC	CiVFGfileDescriptor(		
			tCIp			cip,		//!< this frame grabber 
			int				*fdp		//!< file descriptor for poll() 
			);							/*	kCIEnoErr
											kCIEbadToken
											kCIEnullArg
										*/

#define	CiBrdFileDescriptor	CiVFGfileDescriptor
/*--------------------------------------------------------------------------*/
//! Return SysVFGinfo for open VFG
tCIRC	CiVFGqueryInfo(					
			tCIp			cip,		//!< this frame grabber 
			tCIU32			*ifaceType,	//!< Neon, Karbon, etc. 
			tCIU32			*switches,	//!< DIP switch value for board 
			tCIU32			*devNdx,	//!< /dev/video<Ndx> access 
			tCISTRZ			busName,	//!< user-supplied storage (64 char) 
			tCIU32			*infoHi,	//!< VFG INFOHI value 
			tCIU32			*infoLo,	//!< VFG INFOLO value 
			tCISTRZ			VFGname,	//!< user supplied storage (64 char) 
			tCIU32			*NUMAnode	//!< node for this VFG
			);							/*	kCIEnoErr
											kCIEnullArg
											kCIEfailureErr
										*/

#define	CiBrdQueryInfo	CiVFGqueryInfo
/*--------------------------------------------------------------------------*/
//! Get count of open file descriptors for this VFG
tCIRC CiGetOpenCount(
			tCIp			cip,		//!< this frame grabber 
			tCIU32			*nOpen		//!< number of open descriptors
			);							/*	kCIEnoErr
											kCIEbadToken
											kCIEnullArg
										*/
/*--------------------------------------------------------------------------*/
//! ? Is frame grabber initialized ? 
/*!
**	NOTE:	State usrDMAerr indicates that the frame grabber initialized user
**			DMA frame buffers and that both
**				a) the initializing process has released the buffers
**				and
**				b) another thread/process has an active mmap() of the buffers
**			The frame grabber will remain in this error state until all of
**			the mmap() to the user DMA frame buffers are released.
**
**	NOTE:	State dmaIsAborted indicates software aborted DMA because, e.g.,
**			SIP processing did not finish previous frame's DMA in time.
*/
tCIRC	CiVFGqueryState(				 
			tCIp			cip,		//!< this frame grabber 
			tCIU32			*state		//!< return state information 
			);							/*	kCIEnoErr
											kCIEbadToken
											kCIEnullArg
										*/

typedef enum {
	kCIBQ_shuttingDown		= 1,
	kCIBQ_startingUp,
	kCIBQ_usrDMAerr,
	kCIBQ_needsFirmware,
	kCIBQ_firmwareLoaded,
	kCIBQ_dmaBufferReady,
	kCIBQ_dmaIsActive,
	kCIBQ_dmaIsAborted,

	kCIBQ_stateUnknown		= 9999
	} tCIboardQueryEnums;

#define	CiBrdQueryState	CiVFGqueryState
/*--------------------------------------------------------------------------*/
//! Initialize board or VFG 
/*!
**	The environment variable BITFLOW_INSTALL_DIRS must be set
**
**	If kBFDEFAULTCFGFILE is used then the board DIP switches determine the
**	appropriate config file.
**
**	NOTE:	Only the master interface on a VFG board can download firmware.
**
**	NOTE:	All slave VFG camera files must be compatible w/master VFG.
*/
tCIRC	CiVFGinitialize(				
			tCIp			cip,		//!< this frame grabber 
			tCISTRZ			cfgFN		//!< kBFDEFAULTCFGFILE 
										//!< -- or -- 
										//!< file within BITFLOW_INSTALL_DIRS 
										//!< -- or -- 
										//!< full path to config file 
			);							/*	kCIEnoErr
											kCIEbadToken
											kCIEnullArg
											kCIEnoEnvVar
											kCIErangeErr
											kCIEmemoryErr
											kCIEinvalidArg
											kCIEnoWrPermission
											kCIEfileError
											kCIEfailureErr
											kCIEnotMaster
											kCIEnotCompatibleWithMaster
										*/

#define	kBFDEFAULTCFGFILE	NULL

#define	CiBrdInitialize	CiVFGinitialize
/*--------------------------------------------------------------------------*/
//! Initialize a VFG directly from a camera file
/*!
**	The environment variable BITFLOW_INSTALL_DIRS must be set if path does
**	not contain a '/'.  If path contains a '/' then the fileName is relative
**	to the current directory.  If path begins with '/' then it is absolute.
**
**	If the path contains a '/' then the file name is case sensitive.
*/
tCIRC	CiVFGapplyCameraFile(
			tCIp			cip,		//!< this VFG
			tCISTRZ			camFN		//!< [path][mode@]fileName
			);							/*	kCIEnoErr
											kCIEbadToken
											kCIEnullArg
											kCIEinvalidArg
											kCIEnoEnvVar
											kCIEnoWrPermission
											kCIEfileError
											kCIEfailureErr
											kCIEnotMaster
											kCIEnotCompatibleWithMaster
										*/
/*--------------------------------------------------------------------------*/
//! Interrogate configuration 
/*!
**	NOTE:	Passing a NULL *tCITDP ptr is OK; information not returned.
**
**	NOTE:	If the board is not initialized or not configured then some or
**			all of the returned values will be NULL.
**
**	NOTE:	The tCITDP content should not be modified in any way.
**
**	NOTE: CiSysVFGinquire2() supplies a superset of this information
**
**	This call is retained only for compatibility w/previous SDK releases.
*/
tCIRC	CiVFGinquire(					
			tCIp			cip,		//!< this frame grabber 
			tCITDP			*cfgFN,		//!< configuration file path 
			tCITDP			*config,	//!< configuration file content 
			tCITDP			*cameraFN,	//!< camera file path 
			tCITDP			*camera,	//!< camera file content 
			tCITDP			*firmFN,	//!< firmware file path 
			tCITDP			*firmware,	//!< firmware file content 
			tCITDP			*serial		//!< serial port owner 
			);							/*	kCIEnoErr
											kCIEbadToken
											kCIEnotInitialized
											kCIEfailureErr
											kCIEnoWrPermission
										*/

#define	CiBrdInquire	CiVFGinquire
/*--------------------------------------------------------------------------*/
tCIRC	CiVFGinquire2(					
			tCIp			cip,		//!< this frame grabber 
			tCITDP			*cfgFN,		//!< configuration file path 
			tCITDP			*config,	//!< configuration file content 
			tCITDP			*cameraFN,	//!< camera file path 
			tCITDP			*camera,	//!< camera file content 
			tCITDP			*firmFN,	//!< firmware file path 
			tCITDP			*firmware,	//!< firmware file content 
			tCITDP			*serial,	//!< serial port owner 
			tCITDP			*cxpLink,	//!< CXP link affinity
			tCITDP			*spare,		//!< whatever
			char			**fmtPtr	//!< format str ptr
			);							/*	kCIEnoErr
											kCIEbadToken
											kCIEnotInitialized
											kCIEfailureErr
											kCIEnoWrPermission
										*/

#define	CiBrdInquire2	CiVFGinquire2
/*--------------------------------------------------------------------------*/
//! Query camera file parameters initialized on the VFG.
tCIRC	CiCamFileInquire(
			tCIp			cip,				//!< this frame grabber 
			tCIU32			param,				//!< tCIcamFileQueryEnums parameter to query
			tCIU32			*value				//!< returns value of the inquiry parameter
			);
			
typedef enum {
	kCIcamFile_bitsPerPix		= 0,			//!< bit-depth
	kCIcamFile_packed,							//!< pixel packing (non-zero if packed)
	kCIcamFile_hROIoffset,						//!< horizontal ROI offset
	kCIcamFile_hROIsize,						//!< horizontal ROI size (aka, width)
	kCIcamFile_vROIoffset,						//!< vertical ROI offset
	kCIcamFile_vROIsize,						//!< vertical ROI size (aka, height)
	kCIcamFile_format,							//!< format of the image sensor
	kCIcamFile_acqTimeout,						//!< acquisition timeout in ms
	kCIcamFile_acqOffWhenClosing,				//!< camera acq stopped at close
	kCIcamFile_maxCXPversion,					//!< maximum supported CXP version
	kCIcamFile_bitDepthOption,					//!< bit-depth options (see tCIbitDepthOptionEnums)
	kCIcamFile_requiresTLParamsLocked,			//!< whether or not TLParamsLocked is required by the camera
	
	/*
	**	Flags to query metadata of the camera file parameters
	*/
	kCIcamFile_isDefault		= 0x80000000,	//!< determine if a parameter is Default/Auto
	} tCIcamFileQueryEnums;

typedef enum {
	kCIBPP_formatterNormal	= 0,				//!< No special processing
	kCIBPP_formatterRGBSwap	= 1,				//!< Swap R&B
	kCIBPP_formatterManual	= 2,				//!< use the manual tap format
	kCIBPP_formatterIgnore	= 4					//!< ignore bpp; assume 8b
	} tCIbitDepthOptionEnums;

/*--------------------------------------------------------------------------*/
//! Close a VFG
tCIRC	CiVFGclose(						
			tCIp			cip			//!< from CiVFGopen() 
			);							/*	kCIEnoErr
											kCIEnullArg
											kCIEbadToken
										*/
/**@}*/

#define	CiBrdClose	CiVFGclose
/*==========================================================================*/
/* Interrupt Signals */
/*==========================================================================*/
/*!
** \defgroup interrupt Interrupt Signal Handling
** @{
*/
//! Return events field of pollfd
short int CiPollConfigure(				
			tCIU32			desired		//!< one or more kCPPollEventEnums
			);

/*!
**	NOTE:	TRIG interrupts enabled by CiTrigInterruptEnable(), disabled by
**			CiTrigInterruptDisable().
**
**	NOTE:	SERIAL interrupts enabled by CiCLinit(), disabled by CiCLterm().
**
**	NOTE:	EncA/B interrupts enabled by CiEncInterruptEnable(), disabled by
**			CiEncInterruptDisable(). EncA/B interrupts reported as 'CTAB'.
**			EncA/B and DOWN_TRIG_RCVD only available on Cyton-class.
**
**	All other interrupts are managed by this library.
*/
typedef enum {
	kCIPEintHW				= 1,		//!< frame grabber HW interrupt 
	kCIPEintTRIG			= 1 << 1,	//!<  TRIG interrupt 
	kCIPEintSERIAL			= 1 << 2,	//!<  SERIAL interrupt 
	kCIPEintQUAD			= 1 << 3,	//!<  QUAD intrpt (CXP:DOWN_TRIG_RCVD) 
	kCIPEintDTR				= 1 << 3,	//!<  DOWN_TRIG_RCVD) 
	kCIPEintCTAB			= 1 << 4,	//!<  CTAB interrupt 
	kCIPEintEOF				= 1 << 5,	//!<  EOF interrupt (acquisition) 
	kCIPEintOVSTEP			= 1 << 6,	//!<  OVSTEP interrupt 
	kCIPEdrvrErr			= 1 << 7,	//!<      non-hardware driver error 
	kCIPEnoFirmware			= 1 << 8,	//!<      not ready to poll() 
	kCIPEintCXP				= 1 << 9,	//!<  CXP interrupt
	/*!
	**	These are all the possible interrupt sources from the framegrabber.
	*/
	kCIPEintPollAll			= kCIPEintHW | kCIPEintTRIG | kCIPEintSERIAL |
				kCIPEintQUAD | kCIPEintCTAB | kCIPEintEOF | kCIPEintOVSTEP |
				kCIPEdrvrErr | kCIPEnoFirmware | kCIPEintCXP,
	/*!
	**	These interrupts indicate errors in data collection.
	*/
	kCIPEintError			= kCIPEintHW | kCIPEintOVSTEP | kCIPEdrvrErr |
								kCIPEnoFirmware

	} kCIPollEventEnums;
/*--------------------------------------------------------------------------*/
//! Return mask of kCIPollEventEnums
/*!
**	After a call to poll() a POLLERR return indicates the frame grabber does
**	not yet have the firmware loaded.
**
**	This call is only needed if CiPollConfigure() asked for multiple events.
*/
/*!
**	NOTE:	The poll() system call can return w/o timeout and w/o any
**			enabled interrupt.  This will happen if a the process/thread is
**			signaled or if some other process/thread terminates acquisition
**			before the desired poll() interrupt is delivered.  If a signal
**			occurs then poll() will return EINTR.  Termination of acquisition
**			can be identified by calling CiAqGetStatus() and testing for the
**			kCIAQSaqAbort flag in the CiAqGetStatus() result.
*/

tCIU32	CiPollResult(					
			short int		revents		//!< returned by poll()
			);
/*--------------------------------------------------------------------------*/
//! Clear all pending poll() events.
/*!
**	This call only affects the pending events for this CiVFGopen() access.
**
**	Any pending events of other processes/paths are not affected.
*/
tCIRC	CiPollClear(					
			tCIp			cip,		//!< this frame grabber 
			tCIU32			mask		//!< one or more kCIPollEventEnums 
			);							/*	kCIEnoErr
											kCIEbadToken
											kCIEnotInitialized
										*/
/*--------------------------------------------------------------------------*/
//! Enable TRIG interrupts.
tCIRC	CiTrigInterruptEnable(			
			tCIp			cip			//!< this frame grabber 
			);							/*	kCIEnoErr
											kCIEbadToken
											kCIEnoWrPermission
											kCIEnotInitialized
										*/
/*--------------------------------------------------------------------------*/
//! Disable TRIG interrupts.
tCIRC	CiTrigInterruptDisable(			
			tCIp			cip			//!< this frame grabber
			);							/*	kCIEnoErr
											kCIEbadToken
											kCIEnoWrPermission
											kCIEnotInitialized
										*/
/*--------------------------------------------------------------------------*/
//! Enable ENCA/B interrupts.
tCIRC	CiEncInterruptEnable(			
			tCIp			cip,		//!< this frame grabber 
			tCIU32			zAelseB		//!< 0 ENCA, nonZero ENCB
			);							/*	kCIEnoErr
											kCIEbadToken
											kCIEnoWrPermission
											kCIEnotInitialized
											kCIEnotSupported
										*/
/*--------------------------------------------------------------------------*/
//! Disable ENCA/B interrupts.
tCIRC	CiEncInterruptDisable(			
			tCIp			cip,		//!< this frame grabber 
			tCIU32			zAelseB		//!< 0 ENCA, nonZero ENCB
			);							/*	kCIEnoErr
											kCIEbadToken
											kCIEnoWrPermission
											kCIEnotInitialized
											kCIEnotSupported
										*/
/*--------------------------------------------------------------------------*/
typedef enum {
	/*!
	**	Array index for interrupt counter access.
	**
	**	The counters are reset at each CiAqStart()
	*/
	kCIintCountHW			= 0,
	kCIintCountTRIG,					//!< TRIG ints not reset at AqStart 
	kCIintCountSERIAL,
	kCIintCountQUAD,					//!< CXP: QUAD is DOWN_TRIG_RCVD
	kCIintCountCTAB,
	kCIintCountEOF,
	kCIintCountOVSTEP,
	kCIintCountCXP,
	kCIintCountTOTAL,
	/*
	**	These are bitmasks, not counters
	*/
	kCIintBitsCXP,						//!< bitmask, not counter
	kCIintBitsGn2,						//!< bitmask, not counter
	/*!
	**	This many tCIU32 interrupt counters.
	*/
	kCINinterruptCounters,				//!< size of storage array
	/*
	**	These two are mapped to CTAB [Cyton/KbnCXP class boards]
	*/
	kCIintCountENCA	= kCIintCountCTAB,	//<! Cyton ENCA
	kCIintCountENCB	= kCIintCountCTAB,	//<! Cyton ENCB
	/*
	**	Cyton reports DOWN_TRIG_RCVD as QUAD
	*/
	kCIintCountDTR	= kCIintCountQUAD	//<! Cyton DOWN_TRIG_RCVD
	} tCIinterruptCounterEnums;

//! Interrupts since last AqStart.
tCIRC	CiGetIntCounts(					
			tCIp			cip,		//!< this frame grabber 
			tCIU32			*ic			//!< kCINinterruptCounters tCIU32
			);							/*	kCIEnoErr
											kCIEbadToken
											kCIEnullArg
											kCIErangeErr
										*/
/**@}*/
/*==========================================================================*/
/* GPOUT/GPIN/CCL Access */
/*==========================================================================*/
/*!
** \defgroup gpout GPOUT, GPIN, CCx Access
** @{
*/

//! Set the GPOUT source(s).
tCIRC	CiGPOUTCONset(					
			tCIp			cip,		//!< this frame grabber 
			tCIU32			which,		//!< 0..6 
			tCIU32			value		//!< value for source of GPOUT<which> 
			);							/*	kCIEnoErr
											kCIEbadToken
											kCIEnotInitialized
											kCIEnoWrPermission
											kCIEinvalidArg
										*/

typedef enum {
			kCIGPOC_CON4	=	0,		//!< host sets CON4 
			kCIGPOC_CT0		=	1,		//!< CT0 from CTAB 
			kCIGPOC_CT1		=	2,		//!< CT1 from CTAB 
			kCIGPOC_CT2		=	3,		//!< CT2 from CTAB 
			kCIGPOC_CT3		=	4,		//!< CT3 from CTAB 
			kCIGPOC_CLOCK	=	5,		//!< CLOCK; freq from CFREQ in CON1 
			kCIGPOC_Internal=	6,		//!< freq/duty cycle from CON17 
			kCIGPOC_ENC		=	7		//!< GPOUTCON0/GPOUTCON1 only 
			} tCIGPOUTCONenums;
/*--------------------------------------------------------------------------*/
//! Set the CCn source(s) 
tCIRC	CiCCnCONset(					
			tCIp			cip,		//!< this frame grabber 
			tCIU32			which,		//!< 1..4 
			tCIU32			value		//!< value for source of CC<which> 
			);							/*	kCIEnoErr
											kCIEbadToken
											kCIEnotInitialized
											kCIEnoWrPermission
											kCIEinvalidArg
										*/

typedef enum {
			/*
			**	******************************************
			**	Karbon, Neon, and Alta info
			**	******************************************
			*/
			kCICCOC_CT0		= 0,		//!< CT0 from CTAB 
			kCICCOC_CT1		= 1,		//!< CT1 from CTAB 
			kCICCOC_CT2		= 2,		//!< CT2 from CTAB 
			kCICCOC_onBoard	= 3,		//!< on-board signal 
			kCICCOC_CLOCK	= 4,		//!< on-board CLOCK signal 
			kCICCOC_GPIN0	= 5,		//!< GPIN0 
			kCICCOC_0		= 6,		//!< 0 
			kCICCOC_1		= 7,		//!< 1 
			/*
			** CC4 has a separate config value from 1, 2, and 3
			*/
			kCICCOC_CT3		= 4,		//!< only CC4; CLOCK not available 
			/*
			**	******************************************
			**	Karbon CXP info
			**	******************************************
			*/
			kCICCKCXP_0		= 0,		//!< 0
			kCICCKCXP_1		= 1,		//!< 1
			kCICCKCXP_CT0	= 2,		//!< CT0 from CTAB
			kCICCKCXP_CT1	= 3,		//!< CT1 from CTAB
			kCICCKCXP_CT2	= 4,		//!< CT2 from CTAB
			kCICCKCXP_CT3	= 5,		//!< CT3 from CTAB
			kCICCKCXP_TS	= 6,		//!< VFGx_TRIG_SEL
			kCICCKCXP_EAS	= 7,		//!< VFGx_ENCA_SEL
			kCICCKCXP_EBS	= 8,		//!< VFGx_ENCB_SEL
			kCICCKCXP_NTGx	= 9,		//!< VFGx_NTG
			kCICCKCXP_NTG0	= 10,		//!< VFG0_NTG
			/*
			**	******************************************
			**	Cyton CXP/Aon CXP/Claxon CXP/Axion CL  info
			**	******************************************
			*/
			kCICCCCXP_0		= 0,		//!< 0
			kCICCCCXP_1		= 1,		//!< 1
			kCICCCCXP_CT0	= 2,		//!< CT0 from TS
			kCICCCCXP_CT1	= 3,		//!< CT1 from TS
			kCICCCCXP_CT2	= 4,		//!< CT2 from TS
			kCICCCCXP_CT3	= 5,		//!< CT3 from TS
			kCICCCCXP_TS	= 6,		//!< VFGx_TRIG_SEL
			kCICCCCXP_EAS	= 7,		//!< VFGx_ENCA_SEL
			kCICCCCXP_EBS	= 8,		//!< VFGx_ENCB_SEL
			kCICCCCXP_NTGx	= 9,		//!< VFGx_TS
			kCICCCCXP_NTG0	= 10,		//!< VFG0_TS

			kCICCCCXP_last
			} tCICCCONenums;
/*--------------------------------------------------------------------------*/
//! Set the GPOUT values 
tCIRC	CiGPOUTset(						
			tCIp			cip,		//!< this frame grabber 
			tCIU32			mask,		//!< set bits w/'1' 
			tCIU32			value		//!< set to this value 
			);							/*	kCIEnoErr
											kCIEbadToken
											kCIEnotInitialized
											kCIEnoWrPermission
										*/
typedef enum {
	kCIGPOUT0				= 1,
	kCIGPOUT1				= 1 << 1,
	kCIGPOUT2				= 1 << 2,
	kCIGPOUT3				= 1 << 3,
	kCIGPOUT4				= 1 << 4,
	kCIGPOUT5				= 1 << 5,
	kCIGPOUT6				= 1 << 6,
	/*
	**	Only available on KbnCXP (deprecated)
	*/
	kCIGPOUT7				= 1 << 7,
	kCIGPOUT8				= 1 << 8,
	kCIGPOUT9				= 1 << 9,
	kCIGPOUT10				= 1 << 10,
	kCIGPOUT11				= 1 << 11
	} kCIGPOUTenums;
/*--------------------------------------------------------------------------*/
//! Get the GPOUT values 
tCIRC	CiGPOUTget(						
			tCIp			cip,		//!< this frame grabber 
			tCIU32			*value		//!< gets value; kCIGPOUTenums mask 
			);							/*	kCIEnoErr
											kCIEbadToken
											kCIEnullArg
											kCIEnotInitialized
										*/
/*--------------------------------------------------------------------------*/
//! Get the GPIN values
tCIRC	CiGPINget(						
			tCIp			cip,		//!< this frame grabber 
			tCIU32			*value		//!< gets value; kCIGPINenums mask 
			);							/*	kCIEnoErr
											kCIEbadToken
											kCIEnullArg
											kCIEnotInitialized
										*/
typedef enum {
	/*
	**	************************************
	**	Karbon, Neon, Alta GPIN status
	**	************************************
	*/
	kCIGPIN0				= 1,
	kCIGPIN1				= 1 << 1,
	kCIGPIN2				= 1 << 2,
	kCIGPIN3				= 1 << 3,
	kCIGPIN4				= 1 << 4,
	kCIRD_TRIG_DIFF			= 1 << 5,
	kCIRD_TRIG_TTL			= 1 << 6,
	kCIRD_TRIG_OPTO			= 1 << 7,
	kCIRD_ENC_DIFF			= 1 << 8,
	kCIRD_ENC_TTL			= 1 << 9,
	kCIRD_ENC_OPTO			= 1 << 10,
	kCIRD_FEN				= 1 << 11,
	/*!
	**	These two not connected to hardware; set/cleared by CiAqSWtrigEnc().
	*/
	kCIRD_swTrigger			= 1 << 12,
	kCIRD_swEncoder			= 1 << 13,
	/*
	**	************************************
	**	CXP GPIN status
	**	************************************
	*/
#define	kCIRDCXP_ENCB_DIF	kCIGPIN0
#define	kCIRDCXP_ENCB_TTL	kCIGPIN1
#define	kCIRDCXP_ENCB_VFG0	kCIGPIN2
#define	kCIRDCXP_ENCB_SW	kCIGPIN3
	/*
	**	CXP ENCA info is returned in these fields:
	*/
#define	kCIRDCXP_ENCA_DIF	kCIRD_ENC_DIFF
#define	kCIRDCXP_ENCA_TTL	kCIRD_ENC_TTL
#define	kCIRDCXP_ENCA_VFG0	kCIRD_ENC_OPTO
#define	kCIRDCXP_ENCA_SW	kCIRD_swEncoder
	/*
	**	CXP TRIG info is returned in these fields
	*/
#define	kCIRDCXP_TRIG_TTL	kCIRD_TRIG_TTL
#define	kCIRDCXP_TRIG_DIF	kCIRD_TRIG_DIFF
#define	kCIRDCXP_TRIG_VFG0	kCIRD_TRIG_OPTO
#define	kCIRDCXP_SW_TRIG	kCIRD_swTrigger
	/*
	**	Other CXP specific input info
	*/
#define	kCIRDCXP_SCAN_STEP	kCIGPIN4
	kCIRDCXP_BUTTON			= 1 << 16,
	kCIRDCXP_TRIG_IN		= 1 << 17,
	kCIRDCXP_TRIG_SELECTED	= 1 << 18,
	kCIRDCXP_ENCA_SELECTED	= 1 << 19,
	kCIRDCXP_ENCB_SELECTED	= 1 << 20,
	kCIRDCXP_TRIG_OUT		= 1 << 21
	} kCIGPINenums;
/**@}*/
/*==========================================================================*/
/* Acquisition Setup */
/*==========================================================================*/
/*!
** \defgroup aqsetup Acquisition Setup
** @{
*/

//! Driver allocates DMA resources.
/*!
**	Pass zeros to ROI parameters to retain camera/config file values
**
**	NOTE:	Pass count==0 to release all DMA buffers.
**
**	NOTE:	The hROIsize*vROIsize must specify a multiple of 8 bytes.
*/
tCIRC	CiDrvrBuffConfigure(			
			tCIp			cip,		//!< this frame grabber 
			tCIU32			count,		//!< # frame buffers desired; 0=>free 
			tCIU32			hROIoffset,	//!< skip pixels at camera line start 
			tCIU32			hROIsize,	//!< valid pixels within camera line 
			tCIU32			vROIoffset,	//!< skip rows at frame buffer start 
			tCIU32			vROIsize	//!< valid rows within frame buffer 
			);							/*	kCIEnoErr
											kCIEbadToken
											kCIEnotInitialized
											kCIEnotConfigured
											kCIEmemoryErr
											kCIEnoWrPermission
											kCIEfailureErr
											kCIEinvalidArg
											kCIEresourceBusy
										*/
/*--------------------------------------------------------------------------*/
//! User allocates DMA resources.
/*!
**	Pass zeros to ROI parameters to retain camera/config file values.
**
**	NOTE:	Each frame of data is page-aligned.  Be sure to allocate the
**			buffer with enough space for page-aligned frames.  If the buffer
**			size is too small then kCIEinvalidArg is returned.
**
**	NOTE:	Always call CiMapFrameBuffers() to get the actual frame pointers.
**			The buffer will be re-mapped into the callers memory space with
**			page-aligned frame pointers.
**
**	NOTE:	Once the user buffer is configured for DMA the memory is locked
**			and will not be unlocked until there are no active maps of the
**			underlying buffer in any process/thread.  The VFG must be opened
**			with exclusive write access in order to configure user DMA
**			buffers.
**
**	NOTE:	The hROIsize*vROIsize must specify a multiple of 8 bytes.
*/
tCIRC	CiUserBuffConfigure(			
			tCIp			cip,		//!< this frame grabber 
			tCIU32			count,		//!< # frame buffers desired; 0=>free 
			tCIU8			*buffer,	//!< memory for DMA frames 
			tCIU64			bufferBytes,//!< amount of memory allocated 
			tCIU32			hROIoffset,	//!< skip pixels at camera line start 
			tCIU32			hROIsize,	//!< valid pixels within camera line 
			tCIU32			vROIoffset,	//!< skip rows at frame buffer start 
			tCIU32			vROIsize	//!< valid rows within frame buffer 
			);							/*	kCIEnoErr
											kCIEbadToken
											kCIEnotInitialized
											kCIEnotConfigured
											kCIEnullArg,
											kCIEmemoryErr
											kCIEnoWrPermission
											kCIEfailureErr
											kCIEinvalidArg
											kCIEresourceBusy
											kCIEexclusiveFail
										*/
/*--------------------------------------------------------------------------*/
//! Scatter-gather DMA resources.
/*!
**	This function can be used to support, e.g., single camera DMA into
**	multiple VFG.  The camera data can be split left-right, top-bottom or
**	even/odd and this function provides a mechanism for the DMA data to be
**	delivered into contiguous frame buffers without any buffer copying.
**
**	Pass zeros to ROI parameters to retain camera/config file values.
**
**	NOTE:	Scatter-gather entries should be multiples of 8 bytes and each
**			block pointer should be aligned on 4 byte boundaries.  The total
**			bytes in the scatter-gather list should be a multiple of the VFG
**			frame size.  No scatter-gather entry should straddle a frame
**			boundary.  If any of these conditions is not met then
**			kCIEinvalidArg is returned.
**
**	NOTE:	This function will accept a pointer to memory which is already
**			locked by a driver, e.g., partial-page buffers from another
**			BitFlow VFG or a shmat() pointer.  If the memory is not already
**			locked then the BitFlow driver will lock it down and it will
**			remain locked until there are no active maps of the underlying
**			buffer in any process/thread.  The VFG must be opened with
**			exclusive write access in order to configure scatter-gather DMA
**			buffers.
**
**	WARNING!	If the memory is already locked then the driver does not
**				relock it.  In this case it is the caller's responsibility
**				to maintain the locked status of the DMA buffers while any
**				DMA is active.
**
**	NOTE:	When user scatter-gather DMA is configured the function
**			CiMapFrameBuffers() returns pointers to a dummy frame.  The
**			original information used to build the scatter-gather DMA
**			specification shuld be used to access the data frames.
**
**	NOTE:	CiUserBuffConfigureSG() is a low-level library tool used by
**			CiUserBuffConfigureSplit() and CiUserBuffConfigureEvenOdd().
**			Most callers should use the	CiUserBuffConfigureSplit() or the
**			CiUserBuffConfigureEvenOdd() interfaces directly and should avoid
**			this call.
**
**	NOTE:	MSB set on nSGentry => hard PCI addresses for DMA target (not
**			supported on 32b platform)
**
**	NOTE:	On Cyton no sgList[] item can specify an s-g entry which crosses
**			a page boundary.  Also, each s-g entry must be 16 byte aligned.
*/
tCIRC	CiUserBuffConfigureSG(			
			tCIp			cip,		//!< this frame grabber 
			tCIU32			nSGentry,	//!< # scatter-gather blocks; 0=>free 
			tCISGP			sgList,		//!< scatter-gather buffer list 
			tCIU32			hROIoffset,	//!< skip pixels at camera line start 
			tCIU32			hROIsize,	//!< valid pixels within camera line 
			tCIU32			vROIoffset,	//!< skip rows at frame buffer start 
			tCIU32			vROIsize	//!< valid rows within frame buffer 
			);							/*	kCIEnoErr
											kCIEbadToken
											kCIEnotInitialized
											kCIEnotConfigured
											kCIEinvalidArg,
											kCIEmemoryErr
											kCIEnoWrPermission
											kCIEfailureErr
											kCIEinvalidArg
											kCIEresourceBusy
											kCIEexclusiveFail
										*/
/*--------------------------------------------------------------------------*/
//! DMA 2 VFG into one user buffer.
/*!
**	This function provides high-level support for single camera DMA into
**	two VFG.  The camera data can be split left-right (vertHoriz LSB != 0) or
**	top-bottom (vertHoriz LSB == 0).  In addition the kBFupsideDownA/B flags
**	can be added to have either or both VFG DMA the data into the frame buffer
**	bottom-most row first.  Without the switches the data DMAs into the frame
**	buffer top-most row first.
**
**	Both VFG must have exclusive write access.
**
**	If the buffer is split left-right then vROIsizeA must equal vROIsizeB.
**	If the buffer is split top-bottom then hROIsizeA must equal hROIsizeB.
**
**	Data from VFG "A" is positioned "left" or "top" in the combined buffer.
**
**	The hROIsizeA/B * vROIsize must be a multiple of 8.
**
**	NOTE:	The process/thread which calls this function will receive
**			identical valid buffer pointers from CiMapFrameBuffers() on either
**			of the VFG.  The buffer pointers are not remapped but are instead
**			offsets from the userBuffer argument as passed into this function.
**
**			Other processes/threads calling CiMapFrameBuffers() will receive
**			pointers to a dummy frame.  If multiple processes/threads need to
**			access the data (from, e.g., shared memory passed into this
**          function) then it is the user's responsibility to provide
**			appropriate pointers to the other processes/threads.
**
**	NOTE:	This function will accept a pointer to memory which is already
**			locked by a driver, e.g., partial-page buffers from another
**			BitFlow VFG or a shmat() pointer.  If the memory is not already
**			locked then the BitFlow driver will lock it down and it will
**			remain locked until there are no active maps of the underlying
**			buffer in any process/thread.  The VFG must be opened with
**			exclusive write access in order to configure split DMA buffers.
**
**	WARNING!	If the memory is already locked then the driver does not
**				relock it.  In this case it is the caller's responsibility
**				to maintain the locked status of the DMA buffers while any
**				DMA is active on either VFG.
**
**	
**	NOTE:	It is OK to specify the two ROI such that one is empty, e.g., zero
**			horizontal and/or vertical size.  In the event of an empty ROI for
**			either VFG then a single line of DMA into a bit-bucket buffer is
**			configured for that VFG.
**
**			This allows the caller to arbitrarily configure the overall ROI
**          for a single camera/two VFG configuration without any change of
**			call sequence even when data from one of the VFGs is excluded from
**			the desired ROI.
*/
tCIRC	CiUserBuffConfigureSplit(		
			tCIp			cipA,		//!< first frame grabber 
			tCIU32			hROIoffsetA,//!< skip pixels at camera line start 
			tCIU32			hROIsizeA,	//!< valid pixels within camera line 
			tCIU32			vROIoffsetA,//!< skip rows at frame buffer start 
			tCIU32			vROIsizeA,	//!< valid rows within frame buffer 
			tCIp			cipB,		//!< second frame grabber 
			tCIU32			hROIoffsetB,//!< skip pixels at camera line start 
			tCIU32			hROIsizeB,	//!< valid pixels within camera line 
			tCIU32			vROIoffsetB,//!< skip rows at frame buffer start 
			tCIU32			vROIsizeB,	//!< valid rows within frame buffer 
			tCIU32			nFrames,	//!< number of DMA frames; 0 => free 
			tCIU8			*userBuffer,//!< buffer for combined DMA 
			tCIU64			buffSz,		//!< size of combined DMA buffer 
			tCIU32			vertHoriz	//!< see tBFsplitEnums (below) 
			);							/*	kCIEnoErr
											kCIEbadToken
											kCIEnotInitialized
											kCIEnotConfigured
											kCIEinvalidArg
											kCIEmemoryErr
											kCIEnoWrPermission
											kCIEfailureErr
											kCIEresourceBusy
											kCIEexclusiveFail
										*/
typedef enum {
	kBFsplitVertical		= 0,
	kBFsplitHorizontal		= 1,
	/*
	**	Flags to allow DMA into each frame from bottom line to top line.
	*/
	kBFupsideDownA			= 0x10,		//!< VFG A fills frame bottom up 
	kBFupsideDownB			= 0x20		//!< VFG B fills frame bottom up 
	} tBFsplitEnums;
/*--------------------------------------------------------------------------*/
//! DMA 2 VFG into one user buffer.
/*!
**	This function provides high-level support for single camera DMA into
**	two VFG.  The camera data is split between even and odd rows.  The
**	DMA buffers are merged to provide simple access to all row data.
**
**	Both VFG must have exclusive write access.
**
**	The hROIsize must be a multiple of 8.
**
**	Since the buffers merge even/odd lines there is a single specification
**	for the horizontal and vertical offset/size parameters.
**
**	NOTE:	The process/thread which calls this function will receive
**			identical valid buffer pointers from CiMapFrameBuffers() on either
**			of the VFG.  The buffer pointers are not remapped but are instead
**			offsets from the userBuffer argument as passed into this function.
**
**			Other processes/threads calling CiMapFrameBuffers() wil receive
**			pointers to a dummy frame.  If multiple processes/threads need to
**			access the data (from, e.g., shared memory passed into this
**          function) then it is the user's responsibility to provide
**			appropriate pointers to the other processes/threads.
**
**	NOTE:	This function will accept a pointer to memory which is already
**			locked by a driver, e.g., partial-page buffers from another
**			BitFlow VFG or a shmat() pointer.  If the memory is not already
**			locked then the BitFlow driver will lock it down and it will
**			remain locked until there are no active maps of the underlying
**			buffer in any process/thread.  The VFG must be opened with
**			exclusive write access in order to configure split DMA buffers.
**
**	WARNING!	If the memory is already locked then the driver does not
**				relock it.  In this case it is the caller's responsibility
**				to maintain the locked status of the DMA buffers while any
**				DMA is active on either VFG.
*/
tCIRC	CiUserBuffConfigureEvenOdd(		
			tCIp			cipEven,	//!< first frame grabber (even lines) 
			tCIp			cipOdd,		//!< second frame grabber (odd lines) 
			tCIU32			hROIoffset,	//!< skip pixels at camera line start 
			tCIU32			hROIsize,	//!< valid pixels within camera line 
			tCIU32			vROIoffset,	//!< skip rows at frame buffer start 
			tCIU32			vROIsizeOdd,//!< 1/2 total rows in frame 
			tCIU32			nFrames,	//!< number of DMA frames; 0 => free 
			tCIU8			*userBuffer,//!< buffer for combined DMA 
			tCIU64			buffSz		//!< size of combined DMA buffer 
			);							/*	kCIEnoErr
											kCIEbadToken
											kCIEnotInitialized
											kCIEnotConfigured
											kCIEinvalidArg
											kCIEmemoryErr
											kCIEnoWrPermission
											kCIEfailureErr
											kCIEresourceBusy
											kCIEexclusiveFail
										*/
/*--------------------------------------------------------------------------*/
//! Return DMA resource description.
/*!
**	The ROI returned is the actual hardware setting.  It may differ from the
**	values requested in CiDrvrBuffConfigure() or CiUserBuffConfigure() due to
**	hardware restrictions on 32b/64b transfer boundaries.
**
**	NOTE:	When split DMA is configured the original caller gets 0 h/v
**		ROI offsets, gets h/v ROI size and stride for the merged buffer.
*/
tCIRC	CiBufferInterrogate(			
			tCIp			cip,		//!< this frame grabber 
			tCIU32			*nFrames,	//!< number of frames returned here 
			tCIU32			*bitsPerPix,//!< bits per pixel 
			tCIU32			*hROIoffset,//!< skip pixels at camera line start 
			tCIU32			*hROIsize,	//!< valid pixels within camera line 
			tCIU32			*vROIoffset,//!< skip rows at frame buffer start 
			tCIU32			*vROIsize,	//!< valid rows within frame buffer 
			tCIU32			*stride		//!< bytes between adjacent rows 
			);							/*	kCIEnoErr
											kCIEbadToken
											kCIEnullArg
											kCIEnotInitialized
											kCIEnotConfigured
											kCIEuserDMAerr
											kCIErangeErr
										*/

#define	BYTES_PER_PIXEL(bitsPerPixel)	\
		(((bitsPerPixel) == 36) ? (6) : (((bitsPerPixel) + 7) >> 3))
/*--------------------------------------------------------------------------*/
//! Returns user ptrs to frame buff(s).
/*!
**	NOTE:	The frame buffers are kernel memory.  This call maps them into
**			user space.  Do NOT free the frame pointers.
**
**	NOTE:	DMA memory cannot be reconfigured so long as any process/thread
**			has an active map of the DMA buffers.
**
**	NOTE:	The nzWrite parameter should be 0 if the VFG was opened with
**			kCIBO_readOnly access.  In the absence of kernel PAT (page
**			attribute table) support there is always rd/wr access to the frame
**			buffers.  If the kernel is using PAT then write access to the
**			frame buffers is controlled by the modeFlags argument to
**			CiVFGopen().
**
**	NOTE:	When user scatter-gather DMA is configured this function returns
**			pointers to a dummy frame.  The original information used
**			to build the scatter-gather DMA specification shuld be used to
**			access the data frames.
**
**	NOTE:	When split DMA is configured the original caller gets pointers
**			offset from the original userBuffer pointer; both VFG get
**			identical pointers valid for the merged ROI.  Any access from
**			another process/thread gets pointers to a dummy frame.
*/
tCIRC	CiMapFrameBuffers(				
			tCIp			cip,		//!< this frame grabber 
			tCIU32			nzWrite,	//!< 0 => read only; else read/write 
			tCIU32			*nPtrs,		//!< rtns number of frame pointers 
			tCIU8			***uPtrs	//!< rtns frame pointers 
			);							/*	kCIEnoErr
											kCIEbadToken
											kCIEnullArg
											kCIEnotInitialized
											kCIEnotConfigured
											kCIEnoWrPermission
											kCIEmemoryErr
											kCIEuserDMAerr
											kCIEuserDMAnoWrite
										*/
/*--------------------------------------------------------------------------*/
//! Release user access to DMA buffs.
/*
**	NOTE:	DMA memory cannot be reconfigured so long as any CiVFGopen() call
**			has an active map of the kernel buffers.
*/
tCIRC	CiUnmapFrameBuffers(			
			tCIp			cip			//!< this frame grabber
			);							/*	kCIEnoErr
											kCIEbadToken
											kCIEnotInitialized
											kCIEnotConfigured
										*/
/*--------------------------------------------------------------------------*/
typedef union {
	tCIU32					allBits;
	/*
	**	******************************************
	**	Karbon, Neon, and Alta info
	**	******************************************
	*/
	struct {
		unsigned int trigDelay	: 10;	//!< delays trigger 8N lines 
		unsigned int trigIntCfg	: 2;	//!< 00rsrv/01rise/10fall/11bothEdge
		unsigned int trigSel	: 2;	//!< 00diff/01TTL/10opto/11FEN 
		unsigned int trigPol	: 1;	//!< 0 => assert is rising edge 
		unsigned int trigEnable	: 1;	//!< 0 => no hardware trigger 
		unsigned int _unsused	: 16;
		} vCfg;

	struct {
		unsigned int encDiv		: 10;	//!< divisor for encoder pulses 
		unsigned int encSel		: 2;	//!< 00diff/01TTL/10opto/11reserved *
		unsigned int encPol		: 1;	//!< 0 => assert is rising edge 
		unsigned int encEnable	: 1;	//!< 0 => no hardware encoder 
		unsigned int _unused	: 18;
		} hCfg;
	/*
	**	********************************************
	**	Karbon CXP, Cyton CXP, AON CXP, AxionCL info
	**	********************************************
	*/
	struct {
		unsigned int trigDelay	: 10;	//!< delays trigger 8N lines 
		unsigned int trigSel	: 6;	//!< (see documentation)
		unsigned int trigPol	: 1;	//!< 0 => assert is rising edge 
		unsigned int trigEnable	: 1;	//!< enable selected trigger
		unsigned int _unused	: 14;
		} vCfgCXP;

	struct {
		unsigned int encSelA	: 6;	//!< (see documentation)
		unsigned int encPolA	: 1;	//!< 0 => assert is rising edge 
		unsigned int encEnA		: 1;	//!< enable selected encoder A
		unsigned int encSelB	: 6;	//!< (see documentation)
		unsigned int encPolB	: 1;	//!< 0 => asser is rising edge
		unsigned int encEnB		: 1;	//!< enable selected encoder B
		unsigned int _unsused	: 16;
		} hCfgCXP;

	} tCItrigConfig;

//! Configure vert/horiz data trig.
/*!
**	NOTE: When reconfiguring the trigger it is best to fetch the current
**	configuration (set by the camera file) using CiTrigConfigGet(), modify
**	only the required fields, and then call CiTrigConfigure() with the
**	modifed structs.
*/
tCIRC	CiTrigConfigure(				
			tCIp			cip,		//!< this frame grabber 
			tCItrigConfig	vCfg,		//!< vertical trigger configuration 
			tCItrigConfig	hCfg		//!< horizontal trigger configuration 
			);							/*	kCIEnoErr
											kCIEbadToken
											kCIEnotInitialized
											kCIEnoWrPermission
										*/
/*--------------------------------------------------------------------------*/
//! Retrieve current trigger setup.
tCIRC	CiTrigConfigGet(				
			tCIp			cip,		//!< this frame grabber 
			tCItrigConfig	*vCfg,		//!< vertical trigger configuration 
			tCItrigConfig	*hCfg,		//!< horizontal trigger configuration 
			tCIU32			*mode		//!< configured trigger mode 
			);							/*	kCIEnoErr
											kCIEbadToken
											kCIEnullArg
											kCIEnotInitialized
										*/
/*--------------------------------------------------------------------------*/
//! Set vertical trigger mode.
tCIRC	CiTrigModeSet(					
			tCIp			cip,		//!< this frame grabber 
			tCIU32			mode		//!< one of tCIVTrigModeEnum
			);							/*	kCIEnoErr
											kCIEbadToken
											kCIEnotInitialized
											kCIEnoWrPermission
										*/

typedef enum {
	kCItrigUnknown			= -1,		//!< trig mode not known to library 
	kCItrigInvalid			= 0,		//!< invalid trigger mode 
	kCItrigFreeRun,						//!< board is slave to camera 
	/*
	**	Not valid for Cyton class VFG
	*/
	kCItrigPerFrame,					//!< trigger assert starts frame 
	kCItrigQualified,					//!< acquire while trigger asserted 
	/*!
	**	Only valid on Cyton class VFG
	*/
	kCItrigOneShot,						//!< trig asserts starts frame
	kCItrigOneShotStartAStopA,			//!< acquire while trigger asserted
	kCItrigOneShotStartAStopAlevel		//!< acq wh/trgr asserted; wait hi/lo
	} kCIVTrigModeEnum;
/**@}*/
/*==========================================================================*/
/* Acquisition */
/*==========================================================================*/
/*!
** \defgroup acquisition Acquisition
** @{
*/

//! Start filling first frame buffer.
/*!
**	NOTE:	If nFrames is 1 then hardware logic will cleanly stop the data
**			acquisition on frame boundaries.  If nFrames is greater than 1
**			then software logic stops the acquisition -- it is possible that
**			some data may be transferred into the nFrame+1 DMA buffer before
**			the software stop is issued.  Be sure to configure the DMA buffer
**			count accordingly.
*/
tCIRC	CiAqStart(						
			tCIp			cip,		//!< this frame grabber 
			tCIU32			nFrames		//!< 0 => continous, else frame count 
			);							/*	kCIEnoErr
											kCIEbadToken
											kCIEnullArg
											kCIEnotInitialized
											kCIEnotConfigured
											kCIEnoWrPermission
											kCIEinvalidStateErr
											kCIEmemoryErr
											kCIErangeErr
										*/
/*--------------------------------------------------------------------------*/
//! Begin Start-Stop Image Processing.
/*!
**	NOTE:	If nFrames is 1 then hardware logic will cleanly stop the data
**			acquisition on frame boundaries.  If nFrames is greater than 1
**			then software logic stops the acquisition -- it is possible that
**			some data may be transferred into the nFrame+1 DMA buffer before
**			the software stop is issued.  Be sure to configure the DMA buffer
**			count accordingly.
**
**	NOTE:	SIP initiates each frame on assertion of the TRIG signal and
**			stops collecting data when TRIG is deasserted.  This allows a line
**			scan camera to collect a variable number of lines in a frame.  The
**			VFG interrupt handler uses the deassertion of the TRIG signal to
**			cleanly move DMA to the start of the next frame buffer.
*/
tCIRC	CiAqStartSIP(					
			tCIp			cip,		//!< this frame grabber 
			tCIU32			nFrames		//!< 0 => continous, else frame count 
			);							/*	kCIEnoErr
											kCIEbadToken
											kCIEnullArg
											kCIEnotInitialized
											kCIEnotConfigured
											kCIEnoWrPermission
											kCIEinvalidStateErr
											kCIEmemoryErr
											kCIErangeErr
										*/
/*--------------------------------------------------------------------------*/
//! Start filling first frame buffer using High Frame Rate Polling.
/*!
**	NOTE:	This method is equivalent to CiAqStart, but uses a polling
**			thread to detect new frames, rather than the system ISR. In
**			very high frame rate scenarios, frame polling exhibits
**			better performance characteristics than the ISR. At more
**			typical frame rates, the ISR is less CPU intensive, so
**			CiAqStart should be prefered.
*/
tCIRC	CiAqStartHiFrameRate(						
			tCIp			cip,		//!< this frame grabber 
			tCIU32			nFrames		//!< 0 => continous, else frame count 
			);							/*	kCIEnoErr
											kCIEbadToken
											kCIEnullArg
											kCIEnotInitialized
											kCIEnotConfigured
											kCIEnoWrPermission
											kCIEinvalidStateErr
											kCIEmemoryErr
											kCIErangeErr
										*/
/*--------------------------------------------------------------------------*/
//! Terminate continous DMA after next EOF.
tCIRC	CiAqStop(						
			tCIp			cip			//!< this frame grabber
			);							/*	kCIEnoErr
											kCIEbadToken
											kCIEnotInitialized
											kCIEnotConfigured
											kCIEnoWrPermission
											kCIEinvalidStateErr
										*/
/*--------------------------------------------------------------------------*/
//! Terminate any DMA immediately.
/*!
**	NOTE:	All processes and threads sleeping inside poll() will be awakened
**			if any process or thread calls CiAqAbort() for this VFG.
*/
tCIRC	CiAqAbort(						
			tCIp			cip			//!< this frame grabber
			);							/*	kCIEnoErr
											kCIEbadToken
											kCIEnotInitialized
											kCIEnotConfigured
											kCIEnoWrPermission
										*/
/*--------------------------------------------------------------------------*/
//! Set software trig and/or encoder.
tCIRC	CiAqSWtrigEnc(					
			tCIp			cip,		//!< this frame grabber 
			tCIU32			trigValue,	//!< 0 => 0; 1 => 1; else no change 
			tCIU32			encValue	//!< 0 => 0; 1 => 1; else no change 
										//!< for CXP: 4lsb are ENCB/ENCA
			);							/*	kCIEnoErr
											kCIEbadToken
											kCIEnotInitialized
											kCIEnoWrPermission
										*/
/*--------------------------------------------------------------------------*/
//! Software reset to hardware/drvr.
/*!
**	NOTE:	All driver error conditions are cleared by this call.
*/
tCIRC	CiAqSWreset(					
			tCIp			cip			//!< this frame grabber
			);							/*	kCIEnoErr
											kCIEbadToken
											kCIEnotInitialized
											kCIEnoWrPermission
										*/
/*--------------------------------------------------------------------------*/
//! Describe the acquisition state.
tCIRC	CiAqGetStatus(					
			tCIp			cip,		//!< this frame grabber 
			tCIU32			*state,		//!< acquisition state 
			tCIU32			*nFrames,	//!< frames collected since AqStart 
			tCIU32			*nErrors	//!< errors detected since AqStart 
			);							/*	kCIEnoErr
											kCIEbadToken
											kCIEnullArg
											kCIEnotInitialized
											kCIEnotConfigured
											kCIEfailureErr
											kCIEuserDMAerr
										*/

typedef enum {
	kCIAQSinvalidState		= 0,
	kCIAQSidle,						//!< initial state, after Abort/SW reset 

	kCIAQSsnapShotInProcess,		//!< after start w/NZ nFrames 
	kCIAQSsnapShotDone,				//!< nFrames collected, DMA idle 

	kCIAQScontinuousInProcess,		//!< after Start w/0 nFrames
	kCIAQSerrorDone,				//!< forced to abort 

	kCIAQScontinuousStopping,		//!< after Stop requested, before EOF 
	kCIAQScontinuousDone,			//!< EOF after Stop rqst, DMA idle 

	kCIAQSstateMask			= 15,	//!< reserve 4b for actual state 
	/*
	**	A status bit to indicate SIP data acquisition.
	*/
	kCIAQSaqIsSIP			= 16,	//!< modifies snapShot/continuous states 
	/*
	**	A status bit to indicate "consume buffer" mode buffer underrun.
	**
	**	If set the driver has already aborted any DMA.
	*/
	kCIAQSaqUnderrun		= 32,	//!< modifies all states 
	/*
	**	A status bit to indicate CiAqAbort() stopped acquisition.
	*/
	kCIAQSaqAbort			= 64	//!< modifies all states 
	} kCIAqGetStatEnums;
/*--------------------------------------------------------------------------*/
//! Return at EOF of next undelivered.
/*!
**	NOTE:	Internally this function may call poll() and is the only library
**			function which will sleep the calling process/thread.
**
**			The kCIEtimeoutErr indicates the timeout expired without any
**			HW/OVSTEP errors and before a EOF interrupt was received.
**
**			The kCIEaqAbortedErr return indicates some other process or thread
**			terminated acquisition before a new frame was available.
*/
tCIRC	CiWaitNextUndeliveredFrame(		
			tCIp			cip,		//!< this frame grabber 
			int				timeoutMsec	//!< negative => infinite wait 
			);							/*	kCIEnoErr
											kCIEbadToken
											kCIEnullArg
											kCIEnotInitialized
											kCIEnotConfigured
											kCIEdmaOverrunErr
											kCIEdataErr
											kCIEdataHWerr
											kCIEdataOVSTEPerr
											kCIEdmaErr
											kCIEuserDMAerr
											kCIEbufferUnderrunErr
											kCIEtimeoutErr
											kCIEaqAbortedErr
										*/
/*--------------------------------------------------------------------------*/
//! Return most recently acquired.
/*!
**	NOTE:	When user scatter-gather DMA is configured the function
**			CiMapFrameBuffers() returns pointers to a dummy frame.  The
**			original information used to build the scatter-gather DMA
**			specification shuld be used to access the data frames.
**			The frameID returned by this routine is always accurate and
**			should be used in combination with the scatter-gather information
**			to locate the appropriate frame boundary.
*/
tCIRC	CiGetMostRecentFrame(			
			tCIp			cip,		//!< this frame grabber 
			tCIU32			*frameID,	//!< 0.. since StartAq 
			tCIU8			**frameData	//!< pointer to frame buffer 
			);							/*	kCIEnoErr
											kCIEbadToken
											kCIEnullArg
											kCIEnotInitialized
											kCIEnotConfigured
											kCIEdataErr
											kCIEdataHWerr
											kCIEdataOVSTEPerr
											kCIEdmaErr
											kCIEuserDMAerr
											kCIEnoNewData
											kCIEbufferUnderrunErr
											kCIEinfoAgedOut
											kCIEaqAbortedErr
										*/
/*--------------------------------------------------------------------------*/
//! Return oldest not delivered.
/*!
**	If kCIEdmaOverrunErr is returned then the data has been or is being
**	overwritten.
**
**	NOTE:	Frames are "delivered" by this function _only_.
**
**	NOTE:	When user scatter-gather DMA is configured the function
**			CiMapFrameBuffers() returns pointers to a dummy frame.  The
**			original information used to build the scatter-gather DMA
**			specification shuld be used to access the data frames.
**			The frameID returned by this routine is always accurate and
**			should be used in combination with the scatter-gather information
**			to locate the appropriate frame boundary.
*/
tCIRC	CiGetOldestNotDeliveredFrame(	
			tCIp			cip,		//!< this frame grabber 
			tCIU32			*frameID,	//!< 0.. since StartAq 
			tCIU8			**frameData	//!< pointer to frame buffer 
			);							/*	kCIEnoErr
											kCIEbadToken
											kCIEnullArg
											kCIEnotInitialized
											kCIEnotConfigured
											kCIEdmaOverrunErr
											kCIEdataErr
											kCIEdataHWerr
											kCIEdataOVSTEPerr
											kCIEdmaErr
											kCIEuserDMAerr
											kCIEnoNewData
											kCIEbufferUnderrunErr
											kCIEinfoAgedOut
											kCIEaqAbortedErr
										*/
/*--------------------------------------------------------------------------*/
//! Restart delivery from here.
/*
**	NOTE:	Use this function to effectively acknowledge delivery of frames
**			and clear the kCIEdmaOverrunErr without restarting acquisition.
**
**			This implements a "drop frames on the floor" functionality.
*/
tCIRC	CiResetDelivered(				
			tCIp			cip,		//!< this frame grabber 
			tCIU32			frameID		//!< delivery restarts at frameID+1 
			);							/*	kCIEnoErr
											kCIEbadToken
										*/
/*--------------------------------------------------------------------------*/
//! Return number of undelivered frames.
/*
**	Number of undelivered frames.
*/
tCIRC	CiGetUndeliveredCount(
			tCIp			cip,		//!< this frame grabber
			tCIU32			*count		//!< count of undelivered frames
			);							/*	kCIEnoErr
											kCIEbadToken
										*/
/*--------------------------------------------------------------------------*/
//! Reset HW/OVSTEP error w/o acquisition restart
tCIRC	CiResetHWOVSTEP(				
			tCIp			cip			//!< this frame grabber 
			);							/*	kCIEnoErr
											kCIEbadToken
											kCIEnoWrPermission
										*/
/*--------------------------------------------------------------------------*/
typedef struct {
	tCIU32				frameID;		//!< which frame 
	tCIU32				buffNdx;		//!< index of buffer in list 
	tCIU32				linesLastFrame;	//!< mostly for SIP 
	tCIU32				fill;			/* -- */
	tCIU64				timestamp;		//!< TSC when EOF interrupt reset 
	/*
	**	Note: future versions of BFciLib.h may add more fields to this struct.
	*/
	} tCIextraFrameInfo, *tCIXBIP;

//! Retrieve non-pixel information.
/*!
**	The caller should fill in the frameID field specifying which DMA frame is
**	of interest.  The remaining fields are filled in by the library.
**
**	If the frame has not yet completed then kCIEbadID is returned.
**
**	The driver maintains separate extra info for every frame in the DMA
**	buffer pool.  The extra info for each buffer is overwritten each time
**	the buffer is filled with an image frame.  If the extra information for
**	the requested frameID has been overwritten then kCIEinfoAgedOut is
**	returned.
*/
tCIRC	CiGetExtraFrameInfo(			
			tCIp			cip,		//!< this VFG 
			tCIU32			recSize,	//!< size of storage available 
			tCIXBIP			storeHere	//!< caller's storage 
			);							/*	kCIEnoErr
											kCIEbadToken
											kCIEnullArg
											kCIEnotInitialized
											kCIEnotConfigured
											kCIEbadID
											kCIEinfoAgedOut
											kCIErangeErr
										*/
/**@}*/
/*==========================================================================*/
/* Consume buffers */
/*==========================================================================*/
/*!
** \defgroup consumebuf Consume Buffers
**	The driver and library initialize for circular buffers.  The data fills
**	the first frame buffer, then the second, and so forth, into the last
**	of the nFrame buffers configured.  After the n'th frame buffer is filled
**	the next frame fills the first frame buffer again.
**
**	After the board is initialized and before any buffers are configured the
**	VFG and the library can be set to a "consume buffers" mode of operation.
**	In this mode the user releases buffers which will be queued for subsequent
**	DMA.  The order of buffer fill is determined by how the user releases
**	individual buffers from the suite of configured buffers.
**
**	When DMA is configured in "consume buffers" mode all frame buffers have
**	initial status of kCIbsUnassigend and there are no buffers available for
**	frame data.
**
**	The user must explicitly call CiReleaseBuffer() for each bufferID
**	(0..nFrames-1) in order to make buffers available for frame data.
**
**	When operating in "consume buffers" mode there is no deterministic
**	way to associate a frameID (count of EOF interrupts) with a bufferID
**	(index into the list of configured buffers).  The function
**	CiGetBufferID() will associate a bufferID with a frameID.  However,
**	knowledge of the bufferID/frameID association will "age out" of the
**	data structures if the user hangs onto the a buffer through multiple
**	subsequent EOF interrupts.
**
**	Once acquisition is started the user accesses data as usual via, e.g.,
**	CiGetOldestNotDeliveredFrame() or CiGetMostRecentFrame().  The
**	CiGetBufferID() call should be made immediately after the
**	CiGetOldestNotDeliveredFrame() or CiGetMostRecentFrame() call which
**	identifies the frameID associated with the buffer.
**
**	WHEN OPERATING IN "consume buffers" MODE THE bufferID SHOULD BE
**	IMMEDIATELY  FETCHED AND REMEMBERED UNTIL THE BUFFER CAN BE RELEASED
**	FOR SUBSEQUENT DMA.
**
**	Any buffer can be kept for as long as the user desires.  So long as
**	there are sufficient buffers available for all new frame data there is
**	no restriction on releasing any specific buffer (or buffers) back for
**	subsequent DMA.
**	
**	When the user is done with a buffer it is necessary to call
**	CiReleaseBuffer(), passing in the bufferID which was returned by
**	CiGetBufferID().  The buffer will be marked as kCIbsPending and is owned
**	by the driver until it is once again filled with new data.
**
**	If insufficient buffers are released for new DMA data the driver will
**	immediately stop DMA when the available buffers are exhausted.  The
**	CiGetMostRecentFrame() and CiGetOldestNotDeliveredFrame() functions will
**	return kCIEbufferUnderrunErr.  These functions will also return non-NULL
**	framePtr values for any valid buffers which were filled before the DMA
**	was stopped.  When all valid data is exhausted the functions will return
**	kCIEnoNewData.
**
**	NOTE: The user must continue to call CiGetOldestNotDeliveredFrame() until
**	kCIEnoNewData is returned.  When the buffers are processed be sure to call
**	CiReleaseBuffer().  Failure to do so will result in abandoned buffers
**	which are never again used for DMA.
**
**	A kCIEbufferUnderrunErr status is visible in the CiAqGetStatus() return
**	and cleared at the next start of acquisition and by any CiAqSWreset() call.
** @{
*/
/*--------------------------------------------------------------------------*/
//! Set the VFG/library buffer mode.
/*!
**	If the caller lacks write permission kCIEnoWrPermission is returned.
**
**	If DMA is already configured kCIEresourceBusy is returned.
*/
tCIRC	CiSetBufferMode(				
			tCIp			cip,		//!< this frame grabber 
			tCIU32			mode		//!< circular or consume 
			);							/*	kCIEnoErr
											kCIEbadToken
											kCIEnullArg
											kCIEinvalidArg
											kCIEnotInitialized
											kCIEnoWrPermission
											kCIEresourceBusy
										*/

typedef enum {
	kCIcircularBuffers		= 0,
	kCIconsumeBuffers		= 1
	} tCIbufferModeEnums;
/*--------------------------------------------------------------------------*/
//! Get the VFG/library buffer mode.
tCIRC	CiGetBufferMode(				
			tCIp			cip,		//!< this frame grabber 
			tCIU32			*mode		//!< circular or consume 
			);							/*	kCIEnoErr
											kCIEbadToken
											kCIEnullArg
										*/
/*--------------------------------------------------------------------------*/
//! Get the buffer ID of a frame.
tCIRC	CiGetBufferID(					
			tCIp			cip,		//!< this frame grabber 
			tCIU32			frameID,	//!< from CiGetMostRecentFrame()
									//!< or CiGetOldestNotDeliveredFrame()
										
			tCIU32			*buffID		//!< buffer ID 
			);							/*	kCIEnoErr
											kCIEbadToken
											kCIEnullArg
											kCIEnotInitialized
											kCIEwrongMode
											kCIEbadID
											kCIEinfoAgedOut
										*/
/*--------------------------------------------------------------------------*/
//! Get the status of a frame buffer.
tCIRC	CiGetBufferStatus(				
			tCIp			cip,		//!< this frame grabber 
			tCIU32			buffID,		//!< from CiGetBufferID() 
			tCIU32			*status		//!< status of frame buffer 
			);							/*	kCIEnoErr
											kCIEbadToken
											kCIEnullArg
											kCIEnotInitialized
											kCIEwrongMode
											kCIEbadID
										*/
typedef enum {
	/*!
	**	After DMA is configured all buffers have kCIbsUnassigned status
	*/
	kCIbsUnassigned			= 0,		//!< never released (owned by user) 
	/*!
	**	After a buffer is released it has one of these two status values
	*/
	kCIbsBusy				= 1,		//!< in DMA queue (owned by drvr) 
	kCIbsPending			= 2,		//!< ready for DMA (owned by drvr) 
	/*!
	**	After a buffer is filled it has this status value
	*/
	kCIbsFilled				= 3,		//!< frame complete (owned by user) 
	/*!
	**	Unknown status
	*/
	kCIbsUnknown			= 10
	} tCIbufferStatusEnums;
/*--------------------------------------------------------------------------*/
//! Release a frame buffer for DMA.
tCIRC CiReleaseBuffer(					
			tCIp			cip,		//!< this frame grabber 
			tCIU32			buffID		//!< from CiGetBufferID() 
			);							/*	kCIEnoErr
											kCIEbadToken
											kCIEnullArg
											kCIEnoWrPermission
											kCIEnotInitialized
											kCIEwrongMode
											kCIEbadID
										*/
/**@}*/
/*==========================================================================*/
/* CON Register Access */
/*==========================================================================*/
/*!
** \defgroup regaccess CON Register Access
** @{
*/
//! Number of 32b registers.
tCIRC	CiRegGetCount(					
			tCIp			cip,		//!< this frame grabber 
			tCIU32			*nReg		//!< returns number of CON registers
			);							/*	kCIEnoErr
											kCIEbadToken
											kCIEnullArg
										*/
/*--------------------------------------------------------------------------*/
//!< Read a 32b register.
tCIRC	CiRegPeek(						
			tCIp			cip,		//!< this frame grabber 
			tCIU32			ndx,		//!< CON register address in the range 0..nReg-1
			tCIU32			*result		//!< contents of register 
			);							/*	kCIEnoErr
											kCIEbadToken
											kCIErangeErr
											kCIEnullArg
											kCIEnotImplemented
										*/
/*--------------------------------------------------------------------------*/
//! Write a 32b register.
tCIRC	CiRegPoke(						
			tCIp			cip,		//!< this frame grabber 
			tCIU32			ndx,		//!< CON register address in the range 0..nReg-1
			tCIU32			value,		//!< new register value 
			tCIU32			mask		//!< only modify bits w/'1' in mask 
			);							/*	kCIEnoErr
											kCIEbadToken
											kCIErangeErr
											kCIEnoWrPermission
											kCIEnotImplemented
										*/
/*--------------------------------------------------------------------------*/
//! Return register name.
/*!
**	Query the CON register name. Do not modify the returned string since
**	it is static store.
*/
char	*CiRegName(
			tCIp			cip,		//!< this frame grabber 
			tCIU32			ndx			//!< CON register address in the range 0..nReg-1
			);							/*	NULL if not implemented or
											ndx out of range, else name
										*/
/*--------------------------------------------------------------------------*/
//! Return register Field indices
/*!
**	Query the CON register field indices. These may be used with the
**	CiField* methods to access/modify register sub-fields.
*/
tCIU32	CiRegGetFields(
			tCIp			cip,		//!< this frame grabber
			tCIU32			ndx,		//!< CON register address in the range 0..nReg-1
			tCIU32			*ndxAry,	//!< 32 tCIU32 array of sub-field indices
			tCIU32			*cnt		//!< the number of sub-field indices
			);							/*	kCIEnoErr
											kCIEbadToken
											kCIEnullArg
											kCIErangeErr
										*/
/**@}*/
/*==========================================================================*/
/* CON Register Field Access */
/*==========================================================================*/
/*!
** \defgroup regfield CON Register Field Access
**@{
*/

//! Return enumerate from string.
/*!
**	A way to map a label to a register field enumerate.  Pass, e.g., "HCOUNT"
**	to get the field enumerate associated with the HCOUNT field in CON6.
*/
int		CiFieldNdxFromStr(
			tCIp			cip,		//!< this frame grabber 
			char			*str		//!< name of the field enumerate
			);							/*	-1 label not found
											>=0 valid enumerate
										*/
/*--------------------------------------------------------------------------*/
//! Return field width.
/*!
**	Return the width of this field enumerate.
*/
int		CiFieldWidth(
			tCIp			cip,		//!< this frame grabber 
			int				fieldNdx	//!< index of the field
			);							/*	-1 ndx not found
											>=0 valid field width
										*/
/*--------------------------------------------------------------------------*/
//! Return label for enumerate.
/*!
**	A way to map an enumerate to a label.  Do not modify the returned string
**	since it is static store.
*/
char	*CiFieldNameFromNdx(
			tCIp			cip,		//!< this frame grabber 
			int				ndx			//!< index of the field
			);							/*	NULL if ndx out of range
											else label
										*/
/*--------------------------------------------------------------------------*/
//! Read a CON register field.
tCIRC	CiFieldPeek(					
			tCIp			cip,		//!< this frame grabber 
			tCIU32			ndx,		//!< index of the field to peek
			tCIU32			*result		//!< contents of register field 
			);							/*	kCIEnoErr
											kCIEbadToken
											kCIErangeErr
											kCIEnullArg
											kCIEnotImplemented
										*/
/*--------------------------------------------------------------------------*/
//! Write a CON register field.
tCIRC	CiFieldPoke(					
			tCIp			cip,		//!< this frame grabber 
			tCIU32			ndx,		//!< index of the field to poke
			tCIU32			value		//!< new field value 
			);							/*	kCIEnoErr
											kCIEbadToken
											kCIErangeErr
											kCIEnoWrPermission
											kCIEnotImplemented
										*/
/**@}*/
/*==========================================================================*/
/* Camera Link Functions */
/*==========================================================================*/
/*!
** \defgroup clserial Camera Link Serial Functions
** @{
*/
//! Initialize the frame grabber CL port.
/*!
**	This call sets the serial parameters and flushes any existing data.
**
**	The supported baud rates are all multiples of 9600 bps.\n
**\verbatim
**		baud	   	baud9600
**		9600	  	1
**		19200	 	2
**		38400		4
**		57600		6
**		115200		12
**		230400		24
**		460800		48
**		921600		96\endverbatim
**
**	NOTE:	The serial interface for a framegrabber can only be open and
**			accessed by one process at a time, except that CiCLinfo() will
**			return the settings irrespective of how the board was opened.
**
**	NOTE:	The 460800 and 921600 baud are only available on Axion.
*/
tCIRC	CiCLinit(						
			tCIp			cip,		//!< this frame grabber 
			tCIU32			dataBits,	//!< 5, 6, 7, or 8 
			tCIU32			parity,		//!< 0==none, 1==odd, or 2==even 
			tCIU32			stopBits,	//!< 1==1, 2==2, or 3==1.5 (only 5b) 
			tCIU32			baud9600	//!< 1, 2, 4, 6, 12, 24, or 48 
			);							/*	kCIEnoErr
											kCIEbadToken
											kCIEnotSupported
											kCIEinvalidArg
											kCIEnotInitialized
											kCIEresourceBusy
											kCIEnoWrPermission
											kCIEfailureErr
										*/
/*--------------------------------------------------------------------------*/
//! Return number of bytes in buffer.
tCIRC	CiCLgetBytesAvail(				
			tCIp			cip,		//!< this interface 
			tCIU32			*avail		//!< returns bytes available 
			);							/*	kCIEnoErr
											kCIEbadToken
											kCIEnullArg
											kCIEnotOpen
										*/
/*--------------------------------------------------------------------------*/
//! Read bytes from buffer.
tCIRC	CiCLreadBytes(					
			tCIp			cip,		//!< this interface 
			tCIU32			buffSize,	//!< max number of bytes to read 
			tCIU32			*bytesRead,	//!< number of bytes actually read 
			tCIU8			*buffer		//!< data buffer 
			);							/*	kCIEnoErr
											kCIEbadToken
											kCIEnullArg
											kCIEnotOpen
										*/
/*--------------------------------------------------------------------------*/
//! Write bytes to CL interface.
/*!
**	The serial FIFO may still hold outgoing data when this call returns.
*/
tCIRC	CiCLwriteBytesToFIFO(			
			tCIp			cip,		//!< this interface 
			tCIU32			buffSize,	//!< number of bytes to write 
			tCIU8			*buffer		//!< data buffer 
			);							/*	kCIEnoErr
											kCIEbadToken
											kCIEnullArg
											kCIEtimeoutErr
											kCIEnotOpen
										*/
/*--------------------------------------------------------------------------*/
//! Write bytes to CL interface.
/*!
**	Calls CiCLwriteBytesToFIFO() and then waits for all bytes to exit FIFO.
*/
tCIRC	CiCLwriteBytes(					
			tCIp			cip,		//!< this interface 
			tCIU32			buffSize,	//!< number of bytes to write 
			tCIU8			*buffer		//!< data buffer 
			);							/*	kCIEnoErr
											kCIEbadToken
											kCIEnullArg
											kCIEtimeoutErr
											kCIEnotOpen
										*/
/*--------------------------------------------------------------------------*/
//! Test status of CL output FIFO.
/*!
**	These bits can be set in the returned CameraLink output FIFO status
*/
tCIRC	CiCLgetOutputFIFOstatus(		
			tCIp			cip,		//!< this frame grabber 
			tCIU32			*status		//!< gets status 
			);							/*	kCIEnoErr
											kCIEbadToken
											kCIEnullArg
											kCIEnotOpen
											kCIEfailureErr
										*/

typedef enum {
			kBFCLoutFIFOhasRoom	= 1,	//!< room for at least one byte 
			kBFCLoutFIFOempty	= 1<<1,	//!< all data is gone 
			kBFCLoutputFIFOidle	= kBFCLoutFIFOhasRoom | kBFCLoutFIFOempty
			} tBFCLenums;
/*--------------------------------------------------------------------------*/
//! Get settings of CL serial port.
tCIRC	CiCLinfo(						
			tCIp			cip,		//!< this frame grabber 
			tCIU32			*dataBits,	//!< 5, 6, 7, or 8 
			tCIU32			*parity,	//!< 0==even, 1==odd, or 2==none 
			tCIU32			*stopBits,	//!< 1==1, 2==2, or 3==1.5 (only 5b) 
			tCIU32			*baud		//!< baud rate (NOT scaled) 
			);							/*	kCIEnoErr
											kCIEbadToken
											kCIEnullArg
											kCIEnotOpen
											kCIEfailureErr
										*/
/*--------------------------------------------------------------------------*/
//! Set/clear CameraLink loopback.
/*!
**	Diagnostic tool only: whatever is written is available to be read.
*/
tCIRC	CiCLloopback(					
			tCIp			cip,		//!< this interface 
			tCIU32			nzSet		//!< 0=>clear (default); else set 
			);							/*	kCIEnoErr
											kCIEbadToken
											kCIEnotOpen
										*/
/*--------------------------------------------------------------------------*/
//! Close CameraLink access.
tCIRC	CiCLterm(						
			tCIp			cip			//!< this interface 
			);							/*	kCIEnoErr
											kCIEnotOpen
										*/
/**@}*/
/*==========================================================================*/
/* CXP */
/*==========================================================================*/
/*!
** \defgroup cxpio CXP io
** @{
*/

//! Write to CXP register

tCIRC	CiCXPwriteReg(
			tCIp			cip,		//!< this frame grabber
			tCIU32			link,		//!< CXP link or kCiCXPuseMasterLink
			tCIU32			address,	//!< CXP register address
			tCIU32			value		//!< value to be written
			);							/*	kCIEnoErr
											kCIEbadToken
											kCIEnotSupported
											kCIEnoWrPermission
											kCIEfailureErr
											kCIEinvalidArg
											kCIEtimeoutErr
											kCIEnotInitialized
										*/
/*--------------------------------------------------------------------------*/
//! Read from CXP register

tCIRC	CiCXPreadReg(
			tCIp			cip,		//!< this frame grabber
			tCIU32			link,		//!< CXP link or kCiCXPuseMasterLink
			tCIU32			address,	//!< CXP register address
			tCIU32			*value		//!< value from read
			);							/*	kCIEnoErr
											kCIEbadToken
											kCIEnullArg
											kCIEnotSupported
											kCIEfailureErr
											kCIEinvalidArg
											kCIEtimeoutErr
											kCIEnotInitialized
										*/
/*--------------------------------------------------------------------------*/
//! Write to CXP data space

tCIRC	CiCXPwriteData(
			tCIp			cip,		//!< this frame grabber
			tCIU32			link,		//!< CXP link or kCiCXPuseMasterLink
			tCIU32			address,	//!< CXP data address
			tCIU8			*data,		//!< data to be written
			tCIU32			byteCount	//!< number of bytes to write
			);							/*	kCIEnoErr
											kCIEbadToken
											kCIEnullArg
											kCIEnotSupported
											kCIEnoWrPermission
											kCIEfailureErr
											kCIEinvalidArg
											kCIEtimeoutErr
											kCIEnotInitialized
										*/
/*--------------------------------------------------------------------------*/
//! Read from CXP data space (buffer size should be multiple of 4)

tCIRC	CiCXPreadData(
			tCIp			cip,		//!< this frame grabber
			tCIU32			link,		//!< CXP link or kCiCXPuseMasterLink
			tCIU32			address,	//!< CXP data address
			tCIU8			*buffer,	//!< CXP data goes here
			tCIU32			readBytes,	//!< data size requested
			tCIU32			*actualSz	//!< data size returned
			);							/*	kCIEnoErr
											kCIEbadToken
											kCIEnullArg
											kCIEnotSupported
											kCIEfailureErr
											kCIEinvalidArg
											kCIEtimeoutErr
											kCIEnotInitialized
											kCIErangeErr
										*/
/*--------------------------------------------------------------------------*/
//! Retrieve link affinity for VFG
/*!
**	If this VFG has no assigned link the return bitmask is 0
**
**	Otherwise it is a bitmask for which links affiliate w/this VFG.
**
**	MS8B describes link speed index (set by library).
*/
tCIRC	CiCXPgetLink(
			tCIp			cip,		//!< this frame grabber
			tCIU32			*links		//!< CXP link bitmask
			);							/*	kCIEnoErr
											kCIEbadToken
											kCIEnullArg
											kCIEnotSupported
											kCIEfailureErr
										*/
typedef enum {
	kCiCXPlinkBitMask	= 0x0F,
	kCiCXPlinkSpeedMask	= 0xFF000000,

	kCiCXPuseMasterLink	= 0x0FF

	} tCXPlinkEnums;
/*--------------------------------------------------------------------------*/
//! Assign link affinity for VFG

tCIRC	CiCXPsetLink(
			tCIp			cip,		//!< this frame grabber
			tCIU32			links		//!< CXP link bitmask
			);							/*	kCIEnoErr
											kCIEbadToken
											kCIEnullArg
											kCIEnotSupported
											kCIEfailureErr
											kCIEinvalidArg
										*/
/*--------------------------------------------------------------------------*/
//! Retrieve the host time value
/*!
**	Retrieve the current host time value for the grabber and link.
*/
tCIRC	CiCXPgetHostTime(				
			tCIp			cip,		//!< this frame grabber
			tCIU32			link,		//!< CXP link or kCiCXPuseMasterLink
			tCIU64			*hostTime	//!< return value of host time
			);
/*--------------------------------------------------------------------------*/
typedef struct {
	tCIU64			packetDeviceTime;	//!< packet timestamp from device
	tCIU64			packetHostTime;		//!< packet timestamp of host
	tCIU32			packetConnectionID;	//!< packet connection ID
	/*
	**	Note: future versions of BFciLib.h may add more fields to this struct.
	*/
	} tCiCXPheartbeat, *tCICXPHBP;

//! Retrieve the latest CXP heartbeat value
/*!
**	Retrieve the device and host timestamps of the latest CXP heartbeat
**	packet. This may be used for high precision synchronization of
**	acquisition data.
*/
tCIRC	CiCXPgetLastHeartbeat(			
			tCIp			cip,		//!< this frame grabber
			tCIU32			link,		//!< CXP link or kCiCXPuseMasterLink
			tCIU32			recSize,	//!< size of storage available 
			tCICXPHBP		storeHere	//!< caller's storage 
			);							/*	kCIEnoErr
											kCIEbadToken
											kCIEnullArg
											kCIEnotSupported
											kCIEfailureErr
											kCIEinvalidArg
											kCIErangeErr
										*/
/*--------------------------------------------------------------------------*/
typedef enum {
	kCImaxEventPayload = 256							//!< maximum event packet data size in 32-bit words.
	} tCiCXPeventPacketEnum;
	
typedef struct {
	tCIU32			masterHostConnectionID;				//!< connection ID on which the event packet was sent
	tCIU32			packetTag;							//!< packet tag of this event
	tCIU32			camCRC;								//!< CRC indicated by the camera
	tCIU32			hostCRC;							//!< CRC calculated by the host
	tCIU32			numWords;							//!< message data size in 32-bit words
	tCIU32			messageData[kCImaxEventPayload];	//!< payload data of the message
	/*
	**	Note: future versions of BFciLib.h may add more fields to this struct.
	*/
	} tCiCXPeventPacket, *tCICXPEP;

//! Read a CXP event packet from the event packet queue.
/*!
**	Read a CXP event packet from the event packet queue. Parse event
**	packet data using the CiCXPparseEventPacket method.
*/
tCIRC	CiCXPreadEventPacket(			
			tCIp			cip,		//!< this frame grabber
			tCIU32			link,		//!< CXP link or kCiCXPuseMasterLink
			tCIU32			recSize,	//!< size of storage available 
			tCICXPEP		storeHere	//!< caller's storage 
			);							/*	kCIEnoErr
											kCIEbadToken
											kCIEnullArg
											kCIEnotSupported
											kCIEfailureErr
											kCIEinvalidArg
											kCIErangeErr
											kCIEbadCRC
										*/
/*--------------------------------------------------------------------------*/
typedef struct {
	tCIU32	size;						//!< event message size in bytes
	tCIU32	nameSpace;					//!< event name space (see tCiCXPeventNamespaceEnum)
	tCIU32	eventID;					//!< event ID
	tCIU64	timeStamp;					//!< timestamp
	tCIU32	*data;						//!< event data
	/*
	**	Note: future versions of BFciLib.h may add more fields to this struct.
	*/
	} tCiCXPeventMessage, *tCICXPEM;

//! Parse one message stored in an event packet
/*!
**	Parse the contents of the event packet to retrieve the message at
**	index. If index exceeds packet size, message size is set to zero,
**	and kCIEnoErr is returned.
*/
tCIRC	CiCXPparseEventPacket(			
			tCIp			cip,		//!< this frame grabber
			tCIU32			index,		//!< index of message to parse
			tCIU32			evtPktSize,	//!< size of the CXP event packet structure
			tCICXPEP		evtPkt,		//!< CXP event packet to parse
			tCIU32			recSize,	//!< size of storage available 
			tCICXPEM		storeHere	//!< caller's storage 
			);							/*	kCIEnoErr
											kCIEbadToken
											kCIEnullArg
											kCIEnotSupported
											kCIEfailureErr
											kCIEinvalidArg
											kCIErangeErr
										*/

typedef enum {
	kCICXPEnamespaceGenICam		= 0,	//!< GenICam event message
	kCICXPEnamespaceCXP			= 1,	//!< CXP event message
	kCICXPEnamespaceDevice		= 2,	//!< Device event message
	kCICXPEnamespaceReserved	= 3		//!< Reserved event message namespace
	} tCiCXPeventNamespaceEnum;

/**@}*/
/*==========================================================================*/
/* CytonCXP and other second generation issues */
/*==========================================================================*/
/*!
** \defgroup cyton Cyton issues
** @{
*/

//! Get list of modes available for the present XXXX.bfml camera file

tCIRC	CiGetModes(
			tCIp			cip,		//!< this frame grabber
			tCITDP			*modeData	//!< list of modes (do not modify)
			);							/*	kCIEnoErr
											kCIEbadToken
											kCIEnullArg
											kCIEnotSupported
											kCIEfileErr
											kCIEfailureErr
											kCIEparseErr
										*/
/*--------------------------------------------------------------------------*/
/**@}*/
/*==========================================================================*/
/* LUTs */
/*==========================================================================*/
/*!
** \defgroup luts Lookup Tables
** @{
*/

//! Return count of LUT resources.
tCIRC	CiLUTcount(						
			tCIp			cip,		//!< this frame grabber 
			tCIU32			*nBank,		//!< number of banks 
			tCIU32			*nLane		//!< number of lanes 
			);							/*	kCIEnoErr
											kCIEbadToken
											kCIEnullArg
										*/
/*--------------------------------------------------------------------------*/
//! Return size of a specific LUT.
tCIRC	CiLUTsize(						
			tCIp			cip,		//!< this frame grabber 
			tCIU32			bank,		//!< this bank 
			tCIU32			lane,		//!< this lane 
			tCIU32			*size,		//!< returns number of entries 
			tCIU32			*width		//!< returns number of bits 
			);							/*	kCIEnoErr
											kCIEbadToken
											kCIEnullArg
											kCIErangeErr
										*/
/*--------------------------------------------------------------------------*/
//! Load a LUT.
tCIRC	CiLUTwrite(						
			tCIp			cip,		//!< this frame grabber 
			tCIU32			bank,		//!< this bank 
			tCIU32			lane,		//!< this lane 
			tCIU32			count,		//!< number to load 
			tCIU32			*values		//!< values to be loaded, 32b each 
			);							/*	kCIEnoErr
											kCIEbadToken
											kCIErangeErr
											kCIEnullArg
											kCIEnotInitialized
											kCIEnoWrPermission
										*/
/*--------------------------------------------------------------------------*/
//! Examine a LUT.
tCIRC	CiLUTread(						
			tCIp			cip,		//!< this frame grabber 
			tCIU32			bank,		//!< this bank 
			tCIU32			lane,		//!< this lane 
			tCIU32			count,		//!< number to read 
			tCIU32			*values		//!< values returned here 
			);							/*	kCIEnoErr
											kCIEbadToken
											kCIErangeErr
											kCIEnullArg
											kCIEnotInitialized
										*/
/**@}*/
/*==========================================================================*/
/* Quad Table Functions */
/*==========================================================================*/
/*
**	The linux BitFlow interface does not support access to the quad tables.
*/
/*==========================================================================*/
/* Miscellaneous */
/*==========================================================================*/
/*!
** \defgroup misc Miscellaneous
**
**	The driver debug levels range from 0 (no debug) up with higher levels
**	providing more verbose output.  A value of -1 signals no change to the
**	current driver debug level.
**
**	The bitflow.ko driver sends this debug information to the system log.
**
**	WARNING!	Extreme debug logging (values > 2) implements a delay on each
**				log statement.  The user process may seem to be "hung" but
**				there is no error: the OS is logging a flood of strings.
**
*****
**
**	The BFciLib library sends strings to a printf() style display function
** @{
*/
typedef void (*tCIstrDisplayF)(char *fmt, ...);

//!* For library and driver.
tCIRC	CiSetDebug(						
			tCIp			cip,		//!< this frame grabber
			int				drvDebug,	//!< driver debug level (0 default) 
			tCIstrDisplayF	dispFunc	//!< library debug display
			);							/*	kCIEnoErr
											kCIEbadToken
											kCIEnoWrPermission
										*/
/*--------------------------------------------------------------------------*/
//! Allow debug display of all CameraLink init, term, and i/o
tCIRC	CiSetDebugCL(				
			tCIp			cip,		//!< this VFG
			tCIstrDisplayF	dispFunc	//!< library debug display for CL
			);							/*	kCIEnoErr
											kCIEbadToken
										*/
/*--------------------------------------------------------------------------*/
//! String to system log via driver.
tCIRC	CiDriverLog(					
			tCIp			cip,		//!< this frame grabber 
			tCIU32			level,		//!< iff driver debug >= this value 
			char			*str		//!< this string 
			);							/*	kCIEnoErr
											kCIEbadToken
											kCIEnullArg
										*/
/*--------------------------------------------------------------------------*/
//! Get the hardware revision string for this VFG (if it exists).
tCIRC	CiReadHWrev(					
			tCIp			cip,		//!< this frame grabber 
			char			*str		//!< caller's storage (32 bytes min)
			);							/*	kCIEnoErr
											kCIEbadToken
											kCIEnullArg
											kCIEnotSupported
											kCIEnotMaster
										*/
/*--------------------------------------------------------------------------*/
//! Get the board serial number for this VFG (if it exists).
tCIRC	CiReadSerialNo(					
			tCIp			cip,		//!< this frame grabber 
			char			*str		//!< caller's storage (32 bytes min)
			);							/*	kCIEnoErr
											kCIEbadToken
											kCIEnullArg
											kCIEnotSupported
											kCIEnotMaster
										*/
/*--------------------------------------------------------------------------*/
//! Load a specific firmware file to VFG
tCIRC	CiLoadFirmware(		
			tCIp			cip,		//!< this frame grabber board
			char			*fName		//!< this firmware file
			);							/*	kCIEnoErr
											kCIEbadToken
											kCIEnullArg
											kCIEnotSupported
											kCIEnotMaster
											kCIEfileError
											kCIEfailureError
										*/
/*--------------------------------------------------------------------------*/
//! Attempt to configure for xawtv.
/*!
**	This call will reconfigure the interface for a NxM window with 8 bits
**	per pixel.  The window is is offset by hOffset and vOffset.
**
**	This is intended to support xawtv (v4l1) display from the camera.
*/
tCIRC	CiConfigROI(					
			tCIU32			devNdx,		//!< from CiSysVFGinfo() 
			tCIU32			hROIoffset,	//!< skipped pixels at start of line 
			tCIU32			hROIsize,	//!< horizontal pixels count 
			tCIU32			vROIoffset,	//!< skipped lines at start of frame 
			tCIU32			vROIsize	//!< vertical pixel count 
			);							/*	kCIEnoErr
											kCIEfailureErr
											kCIEuserDMAerr
										*/
/*--------------------------------------------------------------------------*/
//! Attempt to config for xawtv(v4l2).
/*!
**	This call will reconfigure the interface for a NxM window with 8 bits
**	per pixel.  The window is is offset by hOffset and vOffset.
**
**	This is intended to support xawtv (v4l2) display from the camera.
*/
tCIRC	CiConfigROI2(					
			tCIU32			devNdx,		//!< from CiSysVFGinfo() 
			tCIU32			nBuff,		//!< number of DMA buffers 
			tCIU32			hROIoffset,	//!< skipped pixels at start of line 
			tCIU32			hROIsize,	//!< horizontal pixels count 
			tCIU32			vROIoffset,	//!< skipped lines at start of frame 
			tCIU32			vROIsize	//!< vertical pixel count 
			);							/*	kCIEnoErr
											kCIEfailureErr
											kCIEuserDMAerr
										*/
/*--------------------------------------------------------------------------*/
//! Check integrity of token.
tCIRC CiVerifyCIP(tCIp cip);			
/*--------------------------------------------------------------------------*/
//! Text description of error code.
tCISTRZ	CiErrStr(tCIRC circ);			
/*--------------------------------------------------------------------------*/
//! Text description of format code.
tCISTRZ	CiFmtStr(int fmtCode);			
/*--------------------------------------------------------------------------*/
typedef union {
	tCIU32					allBits;

	struct {
		/*
		**	R64 version
		*/
		unsigned int aqStat		: 2;	//!< current AQSTAT command 
		unsigned int fActive	: 1;	//!< camera within vert aq window 
		unsigned int fCount		: 3;	//!< 3b counter on vert aq window 
		unsigned int lCount		: 2;	//!< 2b counter on LEN 
		unsigned int pCount		: 2;	//!< 2b counter on PCLK 
		unsigned int fenCount	: 2;	//!< 2b counter on FEN 
		unsigned int vCount		: 17;	//!< CTAB vCount 
		unsigned int hCount		: 2;	//!< CTAB hCount (2 LSB) 
		unsigned int _unused	: 1;
		} db;

	struct {
		/*
		**	Cyton/Aon version.
		*/
		unsigned int rcvCnt		: 4;	//!< PKT_RCVD_CNT
		unsigned int errCnt		: 4;	//!< CRC_ERR_CNT
		unsigned int dropCnt	: 4;	//!< PKT_DROP_CNT
		unsigned int gntCnt		: 4;	//!< PKT_GNT_CNT
		unsigned int unexp		: 8;	//!< UNEXP_PKT_TAG_CNT
		unsigned int speed		: 3;	//!< linkspeed
		unsigned int encd		: 1;	//!< RD_ENCDIV_SELECTED
		unsigned int encq		: 1;	//!< RD_ENCQ_SELECTED
		unsigned int encb		: 1;	//!< RD_ENCB_SELECTED
		unsigned int enca		: 1;	//!< RD_ENCA_SELECTED
		unsigned int trig		: 1;	//!< RD_TRIG_SELECTED
		} dbCtn;

	struct {
		/*
		**	Axion version.
		*/
		unsigned int fixed		: 13;	//!< TAP_FIXED_VAL (13 of 16)
		unsigned int tmode		: 1;	//!< TAP_MODE
		unsigned int o16		: 1;	//!< TAP_OUTPUT_16
		unsigned int fval		: 1;	//!< CL_USE_FVAL
		unsigned int dval		: 1;	//!< CL_USE_DVAL
		unsigned int chan		: 3;	//!< CL_CHAN_EN
		unsigned int cmode		: 3;	//!< CL_MODE
		unsigned int clClk0		: 1;	//!< 0_CL_CLOCK_DETECTED
		unsigned int clClk1		: 1;	//!< 1_CL_CLOCK_DETECTED
		unsigned int clClk2		: 1;	//!< 2_CL_CLOCK_DETECTED
		unsigned int clClk3		: 1;	//!< 3_CL_CLOCK_DETECTED
		unsigned int encd		: 1;	//!< RD_ENCDIV_SELECTED
		unsigned int encq		: 1;	//!< RD_ENCQ_SELECTED
		unsigned int encb		: 1;	//!< RD_ENCB_SELECTED
		unsigned int enca		: 1;	//!< RD_ENCA_SELECTED
		unsigned int trig		: 1;	//!< RD_TRIG_SELECTED
		} dbAxn;

	} tCIdiagStruct;

//! Retrieve diagnostic information.
tCIRC CiGetDiag(						
			tCIp			cip,		//!< this frame grabber 
			tCIdiagStruct	*ds			//!< info to here 
			);							/*	kCIEnoErr
											kCIEbadToken
											kCIEnullArg
										*/
/*--------------------------------------------------------------------------*/
typedef enum {
	kCIszCTAB			= 16			//!< length of compressed CTAB
	} tCIszCTABenum;

//! Read the compressed CTAB information
tCIRC CiReadCTABcompressed(
			tCIp			cip,		//!< this VFG
			tCIU32			horizVERT,	//!< 0 for H-CTAB, else V-CTAB
			tCIU32			*data		//!< put data here (kCIszCTAB len)
			);							/*	kCIEnoErr
											kCIEbadToken
											kCIEnullArg
											kCIEnotInitialized
											kCIEfailureErr
										*/
/*--------------------------------------------------------------------------*/
//! Write the compressed CTAB information
tCIRC CiWriteCTABcompressed(
			tCIp			cip,		//!< this VFG
			tCIU32			horizVERT,	//!< 0 for H-CTAB, else V-CTAB
			tCIU32			*data		//!< data from here (kCIszCTAB len)
			);							/*	kCIEnoErr
											kCIEbadToken
											kCIEnullArg
											kCIEnotInitialized
											kCIEnoWrPermission
											kCIEfailureErr
										*/
/**@}*/
/*==========================================================================*/
#ifdef	__cplusplus
}
#endif
/*==========================================================================*/
#endif		/* _BFciLib_h_ */
/*==========================================================================*/
/*
	$Log: BFciLib.h,v $
	Revision 1.85  2020/11/12 23:23:19  steve
	Implement optional BFML feature, requires_tlparamslocked. Add kCIcamFile_requiresTLParamsLocked.

	Revision 1.84  2020/10/02 21:15:56  steve
	*** empty log message ***

	Revision 1.83  2020/09/22 22:16:52  steve
	Clarify CiReg and CiField index function.

	Revision 1.82  2020/09/02 23:43:39  steve
	Add kCIcamFile_bitDepthOption.

	Revision 1.81  2020/06/24 23:09:24  steve
	Add CiRegName for Gn2 wide name support, and CiRegGetFields to allow dynamic query of CON register components.

	Revision 1.80  2020/04/25 00:03:03  steve
	Karbon CXP is deprecated.

	Revision 1.79  2020/04/22 23:55:43  steve
	Disable KCXP support. CXP 2.0 event packet read and parse CXP1 AfterConInit should always prefer master link; correctly placed CXP version negotiate. findLinkSpeed now matches Windows behavior. Additional Claxon integration.

	Revision 1.78  2020/04/01 23:25:38  steve
	maxCXPversion, CiCXPgetHostTime, and CiCXPgetLastHeartbeat.

	Revision 1.77  2020/03/06 00:15:55  steve
	Initial Claxon CXP support. Needs work.

	Revision 1.76  2019/11/28 00:14:10  steve
	Added kCIcamFile_isDefault inquire flag.

	Revision 1.75  2019/11/11 19:15:07  steve
	Added kCIcamFile_acqOffWhenClosing inquire parameter.

	Revision 1.74  2019/10/16 17:51:00  steve
	Added format and acquisition timeout inquire parameters.

	Revision 1.73  2019/04/29 21:32:30  steve
	Elaborate CiAqStartHiFrameRate description.

	Revision 1.72  2019/04/26 23:59:53  steve
	Initial implementation of High Frame Rate Polling

	Revision 1.71  2019/04/06 00:05:04  steve
	Implemented new BFciLib method CiCamFileInquire. This method allows for several camera file parameters to be queried immediately after CiVFGinitialize, so that the user need not configure a temporary buffer to extract those values. Furthermore, this method will always return the value taken from the camera file, rather than returning any custom ROI value.

	CiCamFileInquire may be accessed in CIcmdln as the "-m 49" command.

	Revision 1.70  2018/02/24 23:16:43  steve
	Fix 32b disk fillup

	Revision 1.69  2017/12/12 11:05:50  steve
	Fixes for axn+vivid

	Revision 1.68  2017/12/04 21:33:34  steve
	Much gn2 isr work; diag work; base options

	Revision 1.67  2017/10/30 02:50:29  steve
	Now w/down_trig_rcvd and fixes

	Revision 1.66  2017/09/29 09:13:59  steve
	Now dump all camf

	Revision 1.65  2017/05/31 14:10:38  steve
	before first release test

	Revision 1.64  2016/07/11 20:02:54  steve
	Gn2 xx-2Y/2YE, GPUD, DGMA

	Revision 1.63  2016/03/06 23:16:06  steve
	Support CXP_usualInit and support Axion

	Revision 1.62  2015/06/23 17:11:02  steve
	Ready for ndif and cxp2

	Revision 1.61  2015-01-24 19:35:51  steve
	Almost done

	Revision 1.60  2015-01-15 17:24:38  steve
	Cyton available

	Revision 1.59  2014-06-09 10:49:25  steve
	Address release test errs

	Revision 1.58  2014-05-29 13:14:14  steve
	User lib c++ supporting Neon/Alta/Kbn/KbnCXP

	Revision 1.57  2014-03-15 20:57:26  steve
	Reorg for c++, SDK 9.00

	Revision 1.56  2013-03-29 01:39:01  steve
	CXP master enable slaves even if fail init

	Revision 1.55  2013-03-18 03:29:31  steve
	Tweak test

	Revision 1.54  2013-03-08 20:13:07  steve
	Fix array size

	Revision 1.53  2013-03-08 18:58:44  steve
	Fix bad counter

	Revision 1.52  2013-03-03 18:58:10  steve
	Ready w/CXP

	Revision 1.51  2012-11-20 00:43:13  steve
	Fixed CCx_CON issue w/ISR

	Revision 1.50  2012-09-14 14:25:56  steve
	CiSetDebugCL(), CON51, fix sscanf(), cust kern dir

	Revision 1.49  2012-06-01 00:14:19  steve
	Final on 8.10

	Revision 1.48  2012-04-27 21:15:31  steve
	Fix CONFIG_X86_NOPAT, doc iommu, add CIsimpleSG

	Revision 1.47  2012-04-20 12:49:51  tim
	Reformatted and Rearranged comments for Doxygen. tim

	Revision 1.46  2012-02-09 22:01:36  steve
	Fix small ROI, ready kernel 3.x, add FORMAT

	Revision 1.45  2011-08-17 12:56:18  steve
	Compatible w/2.6.38.8\+; hooks for guidriver

	Revision 1.44  2011-05-20 12:47:44  steve
	Consistent serial i/o

	Revision 1.43  2011-05-19 21:56:55  steve
	Separated serial write/wait for max i/o speed.

	Revision 1.42  2011-05-16 21:11:15  steve
	Faster serial out.  Abort wakes all sleepers.

	Revision 1.41  2011-03-30 21:34:45  steve
	Changed snapshot to use hardare counter

	Revision 1.40  2011-03-27 16:01:37  steve
	Minimize SIP restart time

	Revision 1.39  2011-02-14 04:10:05  steve
	Karbon, SIP, DMA fixes

	Revision 1.38  2011-01-03 19:30:24  steve
	SIP lines, TSC, bugfix CCn, cb nolog err

	Revision 1.37  2010-11-17 18:42:30  steve
	Special return reg not mapped

	Revision 1.36  2010-11-17 14:29:31  steve
	First consume buffers

	Revision 1.35  2010-09-05 23:36:47  steve
	Now upside-down flags for split buffers

	Revision 1.34  2010-08-27 21:45:06  steve
	Final v7 w/customFlags, CIcustomTest1,2, etc

	Revision 1.33  2010-08-10 16:21:47  steve
	Now CIcustomTest1, customFlags module param, custom env var

	Revision 1.32  2010-08-04 16:59:49  steve
	Now support CLD/CLQ

	Revision 1.31  2010-02-24 21:21:09  steve
	v706 candidate w/SIP

	Revision 1.30  2009-07-15 20:20:52  steve
	Added CiBuffConfigureSplit(), delays on Kbn flash

	Revision 1.29  2009-06-18 13:37:54  steve
	Full ROI, s-g DMA, flush CL, Kbn flash delay

	Revision 1.28  2009-03-19 00:28:54  steve
	Added CiResetDelibered.  Added demo.

	Revision 1.27  2009-01-29 17:27:12  steve
	kernel NULL ptr fix

	Revision 1.26  2009-01-22 11:38:52  steve
	User-alloc DMA; full ROI; register fields

	Revision 1.25  2008-12-06 11:47:38  steve
	Basic user allocated DMA.

	Revision 1.24  2008-11-15 23:20:26  steve
	Now have (primitive) ROI

	Revision 1.23  2008-11-14 18:36:58  steve
	Alta in; STATUS now STATIC/DYNAMIC; DRIVER_LOG

	Revision 1.22  2008-10-31 13:52:26  steve
	Karbon ready for test

	Revision 1.21  2008-10-17 04:40:30  steve
	Fix typos.

	Revision 1.20  2008-10-16 02:50:24  steve
	Beta2: now have .so library.

	Revision 1.19  2008-10-08 18:13:40  steve
	Release test ready.

	Revision 1.18  2008-09-29 13:47:16  steve
	Stub out unimplemented functions.

	Revision 1.17  2008-09-29 06:41:12  steve
	CIexample now exists.  Version pumped.

	Revision 1.16  2008-09-17 18:00:54  steve
	Change serial i/o parity specification (now works)

	Revision 1.15  2008-09-15 04:14:57  steve
	CameraLink test in CIcmdln now available.

	Revision 1.14  2008-09-15 03:13:24  steve
	Now report TrigMode

	Revision 1.13  2008-09-15 03:01:33  steve
	Subset of trigger modes now available.

	Revision 1.12  2008-09-14 05:43:19  steve
	Now have CiCLinit() and CiCLloopback()

	Revision 1.11  2008-09-13 14:51:45  steve
	We have serial port locking

	Revision 1.10  2008-09-13 13:59:12  steve
	DESC_SET lock and GET_SERIAL_DATA added

	Revision 1.9  2008-09-12 19:32:48  steve
	UART register access available

	Revision 1.8  2008-09-11 04:25:27  steve
	CiDrvrBuffConfigure() is explicit

	Revision 1.7  2008-09-10 00:50:43  steve
	Now have Aq wait/get frame functions

	Revision 1.6  2008-09-04 18:33:10  steve
	Added CiGetDiag() and more AqStart work.

	Revision 1.5  2008-09-04 12:12:09  steve
	No limit on max BitFlow devices installed

	Revision 1.4  2008-09-03 13:43:14  steve
	Acquisition state working

	Revision 1.3  2008-08-31 16:02:27  steve
	Acquistion start/stop/abort logic in

	Revision 1.2  2008-08-18 23:01:49  steve
	First driver state management

	Revision 1.1.1.1  2008-08-28 14:59:55  steve
	Linux driver under CVS

*/
