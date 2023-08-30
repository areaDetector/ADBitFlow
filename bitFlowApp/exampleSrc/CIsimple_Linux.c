/*****************************************************************************

	CIsimple.c			Source for BitFlow CI lib simple example program

	Sep 28,		2008	CIW/SJT

	© Copyright 2008, BitFlow, Inc. All rights reserved.

	Tabstops are 4

	$Author: steve $

	$Date: 2020/10/02 23:30:12 $

	$Id: CIsimple.c,v 1.16 2020/10/02 23:30:12 steve Exp $

*****************************************************************************/

/*==========================================================================*/
/*
**	For access to command line display.
*/
#include	<stdio.h>
#include	<stdarg.h>
#include	<string.h>
/*
**	For checking for keypress
*/
#include	<sys/time.h>
#include	<sys/types.h>
#include	<unistd.h>
/*
**	For access to BitFlow camera interface library.
*/
#include	"BFciLib.h"
/*==========================================================================*/
static int	sExitAns=0;						/* program exit code */
static tCIp	sCIp=NULL;						/* device open token */
static int	sNdx=0;							/* device index */
static int	sMaxFrames=0;					/* total frames to display */
static int	sSkipFrames=0;					/* skip between display */
static int	sDidFrames=0;					/* total frames handled */
static int	sBGok=0;						/* ok to be in background */
/*--------------------------------------------------------------------------*/
#define	SHOW(x)	{ (void)printf x ; (void)fflush(stdout); }
#define	ERR(x)	{ SHOW(("ERR: ")); SHOW(x); }

static char	*sArgv0=NULL;					/* name of executable */
static void ShowHelp(void)
{
	SHOW(("%s of " __DATE__ " at " __TIME__ "\n",sArgv0));
	SHOW(("   -h           display this message and exit\n"));
	SHOW(("\n"));
	SHOW(("   -x ndx       choose available device ndx (default 0)\n"));
	SHOW(("   -m maxFrames max frames to display (default infinite)\n"));
	SHOW(("   -s skipFr    frames to skip between display (default 0)\n"));
	SHOW(("   -b           program is backgrounded (no newline exit)\n"));
	SHOW(("\n"));
	SHOW(("  CIsimple: initialize an interface and display pixels\n"));
	SHOW(("      display ends with newline\n"));
	SHOW(("\n"));
}
/*==========================================================================*/
#include	<time.h>
#include	<sys/timeb.h>
static tCIDOUBLE GetTime(void)
/*
**	Return fractional seconds
*/
{
tCIDOUBLE		ans=0.0;

#ifdef _POSIX_TIMERS

struct timespec tp;

	(void)clock_gettime(CLOCK_MONOTONIC_RAW, &tp);
	ans = (tCIDOUBLE)tp.tv_sec;
	ans += ((tCIDOUBLE)tp.tv_nsec) / 1000000000.0;

#else

struct timeb	tb;

	(void)ftime(&tb);
	ans = tb.millitm;
	ans /= 1000.0;
	ans += tb.time;

#endif

	return(ans);
}
/*--------------------------------------------------------------------------*/
static int DecodeArgs(int argc, char **argv)
/*
**	Parse the input arguments.
*/
{
char	*str;

	argv += 1;	argc -= 1;					/* skip program name */

	while (argc-- > 0) {
	  str = *argv++;
	  if (str[0] != '-') {
		ERR(("Do not know '%s' arg\n",str));
		ShowHelp();
		sExitAns = 1;
		return(sExitAns);
		}
	  switch (str[1]) {
		case 'h':
		  ShowHelp();
		  return(1);
		case 'x':
		  (void)sscanf(*argv,"%d",&sNdx);
		  argv += 1;	argc -= 1;
		  break;
		case 'b':
		  sBGok = 1;
		  break;
		case 'm':
		  (void)sscanf(*argv,"%d",&sMaxFrames);
		  argv += 1;	argc -= 1;
		  break;
		case 's':
		  (void)sscanf(*argv,"%d",&sSkipFrames);
		  argv += 1;	argc -= 1;
		  break;
		default:
		  ERR(("Do not know arg '%s'\n",str));
		  ShowHelp();
		  sExitAns = 1;
		  return(sExitAns);
		}
	  }

	return(kCIEnoErr);
}
/*--------------------------------------------------------------------------*/
static int CheckForKeyboardInput(void)
/*
**	Return 0 if no input available from stdin, 1 else
**
**	Note: the console needs a newline in order to post input.
*/
{
fd_set			exceptfds,readfds,writefds;
struct timeval	tv;
int				ans;
char			buff[1024];

	FD_ZERO(&exceptfds);
	FD_ZERO(&readfds);
	FD_ZERO(&writefds);
	FD_SET(fileno(stdin),&readfds);
	(void)memset(&tv,'\0',sizeof(struct timeval));

	ans = select(1,&readfds,&writefds,&exceptfds,&tv);

	if ((ans == 1) && FD_ISSET(fileno(stdin),&readfds)) {
	  /*
	  **	Consume the line.
	  */
	  (void)fgets(buff,1024,stdin);
	  return(1);
	  }

	return(0);
}
/*--------------------------------------------------------------------------*/
static void InitAndGetDataUntilKeyPress(void)
/*
**	Illustrate a simple example board interaction sequence.
*/
{
tCIRC			circ;
tCIDOUBLE		a=-1.0,b,c,d;
tCIU64			totalBytes=0,totalLines=0;
tCIU32			i;
tCIU32			nPtrs;
tCIU8			**uPtrs=NULL;
tCIU8			*p8;
unsigned short	*p16;
tCIU32			*p32;
tCIU32			frameID,value,value2,value3,pixToShow;
tCIU8			*frameP;
tCIU32	nFrames,bitsPerPix,hROIoffset,hROIsize,vROIoffset,vROIsize,stride;
	/*
	**	Open the ndx'th frame grabber with write permission.
	*/
	if (kCIEnoErr != (circ = CiVFGopen(sNdx,kCIBO_writeAccess,&sCIp))) {
	  ERR(("CiVFGopen gave '%s'\n",CiErrStr(circ)));
	  sExitAns = 1;
	  return;
	  }
	/*
	**	Init the VFG with the config file specified by the DIP switches.
	*/
	if (kCIEnoErr != (circ = CiVFGinitialize(sCIp,NULL))) {
	  ERR(("CiVFGinitialize gave '%s'\n",CiErrStr(circ)));
	  sExitAns = 1;
	  goto andOut;
	  }
	/*
	**	Configure the VFG for 4 frame buffers.
	*/
	if (kCIEnoErr != (circ = CiDrvrBuffConfigure(sCIp,4,0,0,0,0))) {
	  ERR(("CiDrvrBuffConfigure gave '%s'\n",CiErrStr(circ)));
	  sExitAns = 1;
	  goto andOut;
	  }
	/*
	**	Determine buffer configuration.  We only need bitsPerPix and stride.
	*/
	if (kCIEnoErr != (circ = CiBufferInterrogate(sCIp,&nFrames,&bitsPerPix,
			&hROIoffset,&hROIsize,&vROIoffset,&vROIsize,&stride))) {
	  ERR(("CiBufferInterrogate gave '%s'\n",CiErrStr(circ)));
	  sExitAns = 1;
	  goto andOut;
	  }
	/*
	**	Get the buffer pointers for read/write access to buffers.
	**
	**	We ask for write access because we are going to clear the buffer
	**	lines after they are displayed.
	*/
	if (kCIEnoErr != (circ = CiMapFrameBuffers(sCIp,1,&nPtrs,&uPtrs))) {
	  ERR(("CiMapFrameBuffers gave '%s'\n",CiErrStr(circ)));
	  sExitAns = 1;
	  goto andOut;
	  }
	/*
	**	Decide how many per line we show.
	*/
	switch (bitsPerPix) {
	  case 64:
	  case 8:	pixToShow = 16;	break;
	  case 10:
	  case 12:	pixToShow = 12; break;
	  case 14:
	  case 16:
	  case 24:	pixToShow = 8; break;
	  case 30:
	  case 32:
	  case 36:
	  case 48:	pixToShow = 6; break;
	  default:
		ERR(("Do not know pixBitDepth of %d\n",bitsPerPix));
		sExitAns = 1;
		goto andOut;
	  }
	/*
	**	Reset acquisition and clear all error conditions.
	*/
	if (kCIEnoErr != (circ = CiAqSWreset(sCIp))) {
	  ERR(("CiAqSWreset gave '%s'\n",CiErrStr(circ)));
	  sExitAns = 1;
	  goto andOut;
	  }
	/*
	**	Clear the first line of all frame buffers.
	*/
	for (i=0; i<nPtrs; i++) { (void)memset(uPtrs[i],'\0',stride); }
	/*
	**	Tell the user how to stop this cleanly.
	*/
	if (0 == sBGok) {
SHOW(("Will now dump first line of frames until newline (skip %d, ndx %d)\n",
		sSkipFrames,sNdx));
	  } else {
SHOW(("Will now dump first line of frames until %d (skip %d, ndx %d)\n",
		sMaxFrames,sSkipFrames,sNdx));
	  }
	/*
	**	Start continuous acquisition: if sMaxFrames==0 then no limit on frames
	*/
	if (kCIEnoErr != (circ = CiAqStart(sCIp,sMaxFrames))) {
	  ERR(("CiAqStart gave '%s'\n",CiErrStr(circ)));
	  sExitAns = 1;
	  goto andOut;
	  }
	a = GetTime();
	/*
	**	Display each acquired frame 1st line in a loop.  Stop at newline.
	*/
	while (1) {
		/*
		**	Check to see if a frame is already available before waiting.
		*/
checkAgain:
	  switch (circ = CiGetOldestNotDeliveredFrame(sCIp,&frameID,&frameP)) {
		case kCIEnoErr:
		  /*
		  **	We have the frame.
		  */
		  break;
		case kCIEnoNewData:
		  /*
		  **	We need to wait for another frame.
		  */
		  if (kCIEnoErr != (circ = CiWaitNextUndeliveredFrame(sCIp,-1))) {
			switch (circ) {
			  case kCIEaqAbortedErr:
				SHOW(("CiWaitNextUndeliveredFrame gave '%s'\n",CiErrStr(circ)));
				break;
			  default:
				ERR(("CiWaitNextUndeliveredFrame gave '%s'\n",CiErrStr(circ)));
				sExitAns = 1;
			  }
			goto andOut;
			}
		  goto checkAgain;
		case kCIEaqAbortedErr:
		  SHOW(("CiGetOldestNotDeliveredFrame: acqistion aborted\n"));
		  goto andOut;
		default:
		  ERR(("CiGetOldestNotDeliveredFrame gave '%s'\n",CiErrStr(circ)));
		  sExitAns = 1;
		  goto andOut;
		}
	  /*
	  **	Allow skipping frames so display is not an issue.
	  */
	  if ((0 != sSkipFrames) && (0 != (sDidFrames % (sSkipFrames+1)))) {
		goto skipHere;
		}
	  /*
	  **	Display the frameID and the first line of frame data.
	  */
	  SHOW(("frameID %9d:",frameID));
	  switch (bitsPerPix) {
		case 64:
		case 8:
		  p8 = frameP;
		  for (i=0; i<pixToShow; i++) {
			value = *p8++;  SHOW((" %02X",value));
			}
		  break;
		case 10:
		case 12:
		  p16 = (tCIU16 *)frameP;
		  for (i=0; i<pixToShow; i++) {
			value = *p16++;  SHOW((" %03X",value));
			}
		  break;
		case 14:
		case 16:
		  p16 = (tCIU16 *)frameP;
		  for (i=0; i<pixToShow; i++) {
			value = *p16++;  SHOW((" %04X",value));
			}
		  break;
		case 24:
		  p8 = (tCIU8 *)frameP;
		  for (i=0; i<pixToShow; i++) {
			value = *p8++;
			value2 = *p8++; 
			value3 = *p8++;  SHOW((" %02X.%02X.%02X",value,value2,value3));
			}
		  break;
		case 30:
		  p32 = (tCIU32 *)frameP;
		  for (i=0; i<pixToShow; i++) {
			value = *p32++;
			value2 = (value >> 10) & 0x3FF;
			value3 = (value >> 20) & 0x3FF;
			value &= 0x03FF; SHOW((" %03X.%03X.%03X",value,value2,value3));
			}
		  break;
		case 32:
		  p32 = (tCIU32 *)frameP;
		  for (i=0; i<pixToShow; i++) {
			value = *p32++;  SHOW((" %08X",value));
			}
		  break;
		case 36:
		  p16 = (tCIU16 *)frameP;
		  for (i=0; i<pixToShow; i++) {
			value = *p16++; 
			value2 = *p16++; 
			value3 = *p16++;  SHOW((" %03X.%03X.%03X",value,value2,value3));
			}
		  break;
		case 48:
		  p16 = (tCIU16 *)frameP;
		  for (i=0; i<pixToShow; i++) {
			value = *p16++; 
			value2 = *p16++; 
			value3 = *p16++;  SHOW((" %04X.%04X.%04X",value,value2,value3));
			}
		  break;
		default:
		  ERR(("Do not know pixBitDepth of %d\n",bitsPerPix));
		  sExitAns = 1;
		  goto andOut;
		}
	  SHOW(("\n"));

skipHere:
	  totalLines += vROIsize;
	  totalBytes += stride * vROIsize;
	  sDidFrames += 1;
	  /*
	  **	Clear the line just displayed so we know it is rewritten by DMA.
	  */
	  (void)memset(frameP,'\0',stride);
	  /*
	  **	Break out of this loop on newline
	  */
	  if (0 == sBGok) { if (0 != CheckForKeyboardInput()) { break; } }
	  /*
	  **	Break out of loop if countdown hits zero
	  */
	  if ((0 != sMaxFrames) && (--sMaxFrames == 0)) { break; }
	  }

andOut:

#ifdef	STOP_AQ
#warning "STOP_AQ is set"
	/*
	**	Here is an easy way to stop acquisition after data is collected.
	*/
	if (kCIEnoErr != (circ = CiAqSWreset(sCIp))) {
	  ERR(("CiAqSWreset (end) gave '%s'\n",CiErrStr(circ)));
	  }
#endif

	/*
	**	Unmap the frame buffers.
	*/
	if ((NULL != uPtrs) && (kCIEnoErr != (circ = CiUnmapFrameBuffers(sCIp)))){
	  ERR(("CiUnmapFrameBuffers gave '%s'\n",CiErrStr(circ)));
	  }
	/*
	**	Close the access.
	*/
	if ((NULL != sCIp) && (kCIEnoErr != (circ = CiVFGclose(sCIp)))) {
	  ERR(("CiVFGclose gave '%s'\n",CiErrStr(circ)));
	  }
	/*
	**	Show data rate
	*/
	c = b = GetTime() - a;
	if ((a < 0.0) || (c < 0.001)) {
	  a = 0.0;
	  b = 0.0;
	  c = 0.0;
	  d = 0.0;
	  } else {
	  a = ((tCIDOUBLE)totalBytes)/c;
	  b = ((tCIDOUBLE)totalLines)/c;
	  d = ((tCIDOUBLE)sDidFrames)/c;
	  }
	SHOW((
"%d: Data rate %.1lf ln/s (%.1lf b/s) (%.1lf FPS) (%.1lf sec) after %d fr\n",
			sNdx,b,a,d,c,sDidFrames));

	return;
}
/*==========================================================================*/
int main(int argc, char **argv)
/*
**	Decode command line and acquire/display frames until EOF
*/
{
	sArgv0 = *argv;

	if (kCIEnoErr == DecodeArgs(argc,argv)) {
	  InitAndGetDataUntilKeyPress();
	  }

	return(sExitAns);
}
/*==========================================================================*/
/*
	$Log: CIsimple.c,v $
	Revision 1.16  2020/10/02 23:30:12  steve
	CLOCK_MONOTONIC is not always monotonic, so prefer CLOCK_MONOTONIC_RAW.

	Revision 1.15  2020/10/02 01:17:23  steve
	ftime is deprecated. Use clock_gettime.

	Revision 1.14  2017/05/31 14:10:39  steve
	before first release test

	Revision 1.13  2016/07/11 20:02:56  steve
	Gn2 xx-2Y/2YE, GPUD, DGMA

	Revision 1.12  2016/03/06 23:16:08  steve
	Support CXP_usualInit and support Axion

	Revision 1.11  2015-01-15 17:24:40  steve
	Cyton available

	Revision 1.10  2014-05-29 13:14:14  steve
	User lib c++ supporting Neon/Alta/Kbn/KbnCXP

	Revision 1.9  2013-03-07 15:23:05  steve
	Fix test details

	Revision 1.8  2013-03-03 18:58:11  steve
	Ready w/CXP

	Revision 1.7  2011-05-16 21:44:18  steve
	Demote aq abort from err.

	Revision 1.6  2011-05-16 21:11:15  steve
	Faster serial out.  Abort wakes all sleepers.

	Revision 1.5  2011-03-27 16:01:37  steve
	Minimize SIP restart time

	Revision 1.4  2010-06-03 14:34:29  steve
	Added DAM/SG diagnostics to CIcmdln.

	Revision 1.3  2008-10-16 02:50:34  steve
	Beta2: now have .so library.

	Revision 1.2  2008-10-08 18:13:50  steve
	Release test ready.

	Revision 1.1  2008-10-02 01:34:45  steve
	Now have valid doBuildRelease and CIWsimple


*/
