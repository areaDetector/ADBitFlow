//
// Creation:    Gn2Def.h
// Created:     May 6, 2002
// Creator:     Bob Sheff
//
// Copyright (C) 1993-2002 by BitFlow, Inc.  All Rights Reserved.
//
// Gn2 Public Definitions
//
// History:
//
// 05/06/02     rbs     Created file.
//

#if !defined(__Gn2DEF__)
#define __Gn2DEF__

#include "BFDef.h"

// Sizes

#define Gn2CTAB_MAXADDR		(128 * 1024)

// General Purpose Output Pins

typedef enum _Gn2GPOutPin
{
    Gn2GPOut0	= BFGPOut0,
    Gn2GPOut1	= BFGPOut1,
    Gn2GPOut2	= BFGPOut2,
    Gn2GPOut3	= BFGPOut3,
    Gn2GPOut4	= BFGPOut4,
    Gn2GPOut5	= BFGPOut5,
    Gn2GPOut6	= BFGPOut6,
    Gn2GPOut7	= BFGPOut7,
    Gn2GPOut8	= BFGPOut8,
    Gn2GPOut9	= BFGPOut9,
    Gn2GPOut10	= BFGPOut10,
    Gn2GPOut11	= BFGPOut11,
} Gn2GPOutPin, *Gn2GPOutPinPtr;




// ctabs

typedef enum _Gn2CTabControl
{
    Gn2CTab				= BFCTab,
    Gn2HCTab			= BFHCTab,
    Gn2VCTab			= BFVCTab,
    Gn2HCTabHStart		= BFHCTab0,
    Gn2HCTabHReset		= BFHCTab1,
    Gn2HCTabENHLoad		= BFHCTab2,
    Gn2HCTabReserved	= BFHCTab3,
    Gn2HCTabGPH0		= BFHCTab4,
    Gn2HCTabGPH1		= BFHCTab5,
    Gn2HCTabGPH2		= BFHCTab6,
    Gn2HCTabGPH3		= BFHCTab7,
    Gn2VCTabVStart		= BFVCTab0,
    Gn2VCTabVReset		= BFVCTab1,
    Gn2VCTabENVLoad		= BFVCTab2,
    Gn2VCTabIRQ			= BFVCTab3,
    Gn2VCTabGPV0		= BFVCTab4, 
    Gn2VCTabGPV1		= BFVCTab5,
    Gn2VCTabGPV2		= BFVCTab6,
    Gn2VCTabGPV3		= BFVCTab7,
} Gn2CTabControl, *Gn2CTabControlPtr;

// System Probe Events
//
// 0x00-0x03 : Basic Events (Clocks,Timers,Immediate,Never)
// 0x04-0x07 : Timing Sequencer Aux outputs
// 0x08-0xF  : IO (ENCA,ENCB,TRIG,ENCQ,ENQDIV)
// 0x10-0x1F : StreamSync, PCIe, Acquisition Engine
// 0x20-0x3F : Protocol Specific front end (Camera Link for Axion, CXP for Cyton/AON)
//
// -----------------------------------------------------------
typedef enum _SP_EVENTS 
{
	// --------------------------------------------------------
	// Basic Events
	// --------------------------------------------------------
	SPE_NEVER         = 0x00,    // No Event  - Never fires
	SPE_IMMEDIATE     = 0x01,    // Immediate - Fires every SYS_CLK, same as SYS_CLK.  Redundant but included anyways becasue the label IMMEDIATE is more intuitive
	SPE_SYS_CLK       = 0x02,    // 125MHz system clk
	SPE_100_NS        = 0x03,    // 100 nanosecond pulse

	// --------------------------------------------------------
	// Timing Sequencer for advanced timing
	// --------------------------------------------------------
	SPE_TSAUX_0       = 0x04,    // Timing Sequencer Aux 0
	SPE_TSAUX_1       = 0x05,    // Timing Sequencer Aux 0
	SPE_TSAUX_2       = 0x06,    // Timing Sequencer Aux 0
	SPE_TSAUX_3       = 0x07,    // Timing Sequencer Aux 0
                                
	// --------------------------------------------------------
	// IO Events
	// --------------------------------------------------------
	SPE_TRIG          = 0x08,    // system trigger
	SPE_ENCA          = 0x09,    // encoder A 
	SPE_ENCB          = 0x0a,    // encoder B
	SPE_ENCQ          = 0x0b,    // encoder Q
	SPE_ENCDIV        = 0x0c,    // encoder Div

	// --------------------------------------------------------
	// PCIe Events
	// --------------------------------------------------------
	SPE_PCI_SENT      = 0x10,    // PCI packets sent (16-byte packets)
	SPE_PCI_DROP      = 0x11,    // PCI packets dropped 
	SPE_PCI_BUSY      = 0x12,    // PCIe bus towards the host is busy

	SPE_INTERRUPT     = 0x0f,    // System interrupt for this VFG
                                
	// --------------------------------------------------------
	// Protocol Specific (Camera Link for axion)
	// --------------------------------------------------------
	SPE_CL_CLK_B      = 0x20,   // CL Clk   Base Channel
	SPE_CL_CLK_M      = 0x21,   // CL Clk   Med  Channel
	SPE_CL_CLK_F      = 0x22,   // CL Clk   Full Channel
	SPE_PLL_LOCK_B    = 0x23,   // PLL Lock Base Channel
	SPE_PLL_LOCK_M    = 0x24,   // PLL Lock Med  Channel
	SPE_PLL_LOCK_F    = 0x25,   // PLL Lock Full Channel
	SPE_FVAL_B        = 0x26,   // FVAL     Base Channel
	SPE_FVAL_M        = 0x27,   // FVAL     Med  Channel
	SPE_FVAL_F        = 0x28,   // FVAL     Full Channel
	SPE_LVAL_B        = 0x29,   // LVAL     Base Channel
	SPE_LVAL_M        = 0x2A,   // LVAL     Med  Channel
	SPE_LVAL_F        = 0x2B,   // LVAL     Full Channel
	SPE_DVAL_B        = 0x2C,   // DVAL     Base Channel
	SPE_DVAL_M        = 0x2D,   // DVAL     Med  Channel
	SPE_DVAL_F        = 0x2E,   // DVAL     Full Channel 
	SPE_CC1           = 0x2F,   // CC1
	SPE_CC2           = 0x30,   // CC2
	SPE_CC3           = 0x31,   // CC3
	SPE_CC4           = 0x32,    // CC3


	// Possible Future events                                
//    SPE_PCIE_PKT_SENT   // PCIe packet sent
//    SPE_PCIE_PKT_RCVD   // PCIe packet received
//    SPE_PCIE_BUS_BUSY   // PCIe bus is busy
//    SPE_PCIE_1K_SENT    // 1K worth of data transfered to PCIe, for bandwidth measurements
//    SPE_INTERRUPT       // System interrupt(s)
                                                                 
} SP_EVENTS;

// System Probe Functions
//   
typedef enum    _SP_FUNCS 
{
	SPF_RISE   = 0x00,
	SPF_FALL   = 0x01,
	SPF_HIGH   = 0x02,
	SPF_LOW    = 0x03,
} SP_FUNCS;

typedef enum    _SP_COUNT_MODE
{ 
      SPCM_CURR  = 0x00,            // Current:    on each update SP_STAT.count will reflect the current event capture 
      SPCM_ACC   = 0x01,            // Accumulate: on each update SP_STAT.count will reflect the Accumulation of all event captures since SP_CON.arm.  Averaging can be handled by s/w in conjuction with the SP_LIMIT.limit 
      SPCM_MIN   = 0x02,            // Minimum:    on each update SP_STAT.count will reflect the Minimum of all event captures since SP_CON.arm 
      SPCM_MAX   = 0x03             // Minimum:    on each update SP_STAT.count will reflect the Maximum of all event captures since SP_CON.arm 
} SP_COUNT_MODE; 

#endif

