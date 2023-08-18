#ifndef INCLUDED_GEN_T_L_VIEW_H
#define INCLUDED_GEN_T_L_VIEW_H

/* FILE:        GenTLView.h
 * DATE:        1/5/2017
 * AUTHOR:      Jeremy Greene
 * COMPANY:     BitFlow, Inc.
 * DESCRIPTION: Methods to run a simple or an advanced (interactive) acquisition from
 *              a GenTL producer, displaying each frame in an ImageView window.
 */

// Header file defining the GenTL_CTI class.
#include "../GenTLInterface/GenICamUtilities.h"

// GenICam header files we depend upon. These are also included by GenICamUtilities.h
#include <Base/GCUtilities.h>
#include <GenApi/GenApi.h>

#include <BFResolveGenTL.h>

using namespace GenApi;
using namespace GenICam;

#if GENTL_H_AT_LEAST_V(1,5,0)
	using namespace GenTL;
#else
	using namespace GenICam::Client;
#endif

// Simple acquisition with no viewer.
void execGenTLAcq       (GenTL_CTI *const cti, const DEV_HANDLE hDev, GenTL_CTI::NodeMapPtr const& remDevNodeMap);

// Simple acquisition with no viewer.
void execGenTLAcqAdv    (GenTL_CTI *const cti, const DEV_HANDLE hDev, GenTL_CTI::NodeMapPtr const& remDevNodeMap);

// Simple GenTLView.
void execGenTLView      (GenTL_CTI *const cti, const DEV_HANDLE hDev, GenTL_CTI::NodeMapPtr const& remDevNodeMap);

// Advanced, interactive GenTLView.
void execGenTLViewAdv   (GenTL_CTI *const cti, const DEV_HANDLE hDev, GenTL_CTI::NodeMapPtr const& remDevNodeMap);

#endif // INCLUDED_GEN_T_L_VIEW_H
