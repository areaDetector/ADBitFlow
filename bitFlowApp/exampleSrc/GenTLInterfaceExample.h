/* FILE:        GenTLInterfaceExample.h
 * DATE:        12/20/2016
 * AUTHOR:      Jeremy Greene
 * COPYRIGHT:   BitFlow, Inc., 2012-2016
 * DESCRIPTION: A basic example program, demonstrating the following functionality of the
 *              GenTL Interface static library:
 *               1) Enumerate all available CTI files.
 *               2) Load a specific GenTL producer from a CTI file.
 *               3) Load one of the producer ports, and that port's XML node-map.
 *               2) Allow the user to get/set values from nodes on the node map.
 */

#ifndef INCLUDED__GEN_T_L__INTERFACE__EXAMPLE__H
#define INCLUDED__GEN_T_L__INTERFACE__EXAMPLE__H

// Header file defining the GenTL_CTI class.
#include "../GenTLInterface/GenICamUtilities.h"

#include <cstdint>
#include <cstdio>

using namespace GenApi;
using namespace GenICam;

#if GENTL_H_AT_LEAST_V(1,5,0)
	using namespace GenTL;
#else
	using namespace GenICam::Client;
#endif

std::ostream& printData   (std::ostream &ostrm, const INFO_DATATYPE type, const void *const data, const size_t dataLen);
std::ostream& printErrors (std::ostream &ostrm, GenTL_CTI *const cti);

#endif // INCLUDED__GEN_T_L__INTERFACE__EXAMPLE__H

