#ifndef INCLUDED__B_F_G_T_L__UTIL__A_P_I__H
#define INCLUDED__B_F_G_T_L__UTIL__A_P_I__H

/* FILE:        BFGTLUtilApi.h
 * DATE:        4/27/2015
 * AUTHOR:      Jeremy Greene
 * COMPANY:     BitFlow, Inc.
 * COPYRIGHT:   Copyright (C) 2015, BitFlow, Inc.
 * DESCRIPTION: API for the BitFlow GenTL interface, used to query and control GenICam device
 *              properties.
 */
 
#include "BFGTLUtilDef.h"

#ifdef __cplusplus
extern "C" {
#endif

//// Library verification.

// BFGTLUtilIsGood
//   Verify that the BFGTLUtil library can be loaded. BFGTLUtil.dll should be delay-loaded
//   for this to work as intended.
#if defined(_WIN32)
#   include <Windows.h>
#   define BFGTLUtilIsGood() (!!LoadLibrary(TEXT("BFGTLUtil.dll")))
#elif defined(__GNUC__)
#   include <dlfcn.h>
#   define BFGTLUtilIsGood() (dlopen("libBFGTLUtil.so", RTLD_LAZY))
#else
#   define BFGTLUtilIsGood() (0)
#   error "BFGTLUtil is not yet supported on this platform."
#endif

//// Device access.

// Device handle open. Opens with the "default" XML file, if any.
//      hBoard      -- Board to open the GenTL device of.
//      hDev        -- [OUT] Return pointer of the opened device.
BFGTLUTIL_IMPORT_EXPORT_ERR       BFGTLDevOpen            (BFGTLUtilBd hBoard, PBFGTLDev hDev);

// Advanced device open with improved performance. An accurate CISYS_TYPE_ANY board number
// must be provided. Opens with the "default" XML file, if any.
//      boardNumber -- Board number to be opened. On Linux systems, this is the V4L devNdx returned by
//                     CiSysVFGinfo2 or CiVFGindex.
//      hBoard      -- Board to open the GenTL device of.
//      options     -- Device open options. NULL opens with default options, equivalent to BFGTLDevOpen.
//      hDev        -- [OUT] Return pointer of the opened device.
BFGTLUTIL_IMPORT_EXPORT_ERR       BFGTLDevOpenEx          (BFGTLUtilU32 boardNumber, BFGTLUtilBd hBoard, BFGTLUtilU32 options, PBFGTLDev hDev);

// Device handle close.
//      hDev        -- Handle of the device to close.
BFGTLUTIL_IMPORT_EXPORT_ERR       BFGTLDevClose           (BFGTLDev hDev);

//// Node accessors.

// Retrieve the number of nodes in the device.
//      hDev        -- Handle of the device.
//      pNodeCount  -- The number of nodes available in this device.
BFGTLUTIL_IMPORT_EXPORT_ERR       BFGTLDevNodeEnum        (BFGTLDev hDev, PBFGTLUtilSizeT pNodeCount);

// Retrieve the name of a node by its internal index. If the "name" buffer pointer is NULL, succeed,
// returning just the required buffer size.
//      hDev        -- Handle of the device.
//      index       -- Index value of the node to retrieve.
//      name        -- [Optional] [OUT] Name of the node at index. Input may be NULL.
//      pSize       -- [IN] sizeof the name buffer
//                     [OUT] size required for the output string (including null terminator).    
BFGTLUTIL_IMPORT_EXPORT_ERR       BFGTLDevNodeName        (BFGTLDev hDev, BFGTLUtilSizeT index, BFGTLUtilChar *const name, PBFGTLUtilSizeT pSize);

// Return TRUE or FALSE, whether or not the device contains the named node.
//      hDev        -- Handle of the device.
//      nodeName    -- Null-terminated string name of the node to find.
BFGTLUTIL_IMPORT_EXPORT_BOOL       BFGTLDevNodeExists      (BFGTLDev hDev, const BFGTLUtilChar *nodeName);

// Open a reference handle to the named node. BFGTLNodeClose must subsequently be called on each node opened.
//      hDev        -- Handle of the device.
//      nodeName    -- Null-terminated string name of the node to retrieve.
//      hNode       -- [OUT] Return pointer of the node being retrieved.
BFGTLUTIL_IMPORT_EXPORT_ERR       BFGTLNodeOpen           (BFGTLDev hDev, const BFGTLUtilChar *nodeName, PBFGTLNode hNode);

//// Generic node controls.

// Close the node referenced by the handle. This must be called on each node opened.
//      hNode       -- Handle of the node to close.
BFGTLUTIL_IMPORT_EXPORT_ERR       BFGTLNodeClose          (BFGTLNode hNode);

// Retrieve the value of a node property.
//      hNode       -- [IN] Handle of the node to inquire.
//      field       -- [IN] The BFGTLNodeField field to inquire upon.
//      propCode    -- [IN] The BFGTLNodeInqProp code to query.
//      pValue      -- [OUT] Return pointer of the node property value.
BFGTLUTIL_IMPORT_EXPORT_ERR       BFGTLNodeInquire        (BFGTLNode hNode, const BFGTLUtilU32 field, const BFGTLUtilU32 propCode, PBFGTLUtilU32 pValue);

// Retrieve the value of the device node using the literal value type. Use BFGTLNodeInquire to determine a node's
// type and accessibility. pValue may be NULL, to query just the require buffer byte size.
//      hNode       -- [IN] Handle of the node to read.
//      field       -- [IN] The BFGTLNodeField field to inquire upon.
//      pValue      -- [Optional] [OUT] Return pointer of the node field value. May be NULL, to query just the
//                          required buffer size.
//      pSize       -- [IN] IF pValue != NULL, pointer to the size of the pValue buffer.
//                     [OUT] IF return code is BF_OK or BF_BFGTL_NODE_SIZE_ERR, the buffer byte size required
//                          to read the entire data value, ELSE, undefined.
BFGTLUTIL_IMPORT_EXPORT_ERR       BFGTLNodeRead           (BFGTLNode hNode, const BFGTLUtilU32 field, BFGTLUtilVoid *const pValue, BFGTLUtilSizeT *const pSize);

// Write the value of the device node using the literal value type. Use BFGTLNodeInquire to determine a node's type.
//      hNode       -- [IN] Handle of the node to write.
//      field       -- [IN] The BFGTLNodeField field to inquire upon.
//      pValue      -- [IN] Pointer to the data value pointer. If setting BFGTL_NODE_VALUE on a
//                          BFGTL_NODE_TYPE_COMMAND, this may be NULL.
//      iSize       -- [IN] The size of the pValue buffer. If setting BFGTL_NODE_VALUE on a
//                          BFGTL_NODE_TYPE_COMMAND, this may be ZERO.
BFGTLUTIL_IMPORT_EXPORT_ERR       BFGTLNodeWrite          (BFGTLNode hNode, const BFGTLUtilU32 field, const BFGTLUtilVoid *const pValue, const BFGTLUtilSizeT iSize);

#ifdef __cplusplus
}
#endif

#endif // INCLUDED__B_F_G_T_L__UTIL__A_P_I__H
