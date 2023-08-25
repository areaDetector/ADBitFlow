//-----------------------------------------------------------------------------
//  (c) 2006-2008 by STEMMER IMAGING GmbH
//  Project: GenTL
//  Author:  RST
//  $Header: /cvs/genicam/genicam/source/TLClients/Stemmer/examples/SimpleNoDisp/Port.h,v 1.2 2008/12/23 21:39:55 rupert_stelz Exp $
//
//  License: This file is published under the license of the EMVA GenICam  Standard Group.
//  A text file describing the legal terms is included in  your installation as 'GenICam_license.pdf'.
//  If for some reason you are missing  this file please contact the EMVA or visit the website
//  (http://www.genicam.org) for a full copy.
//
//  THIS SOFTWARE IS PROVIDED BY THE EMVA GENICAM STANDARD GROUP "AS IS"
//  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
//  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE EMVA GENICAM STANDARD  GROUP
//  OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,  SPECIAL,
//  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT  LIMITED TO,
//  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,  DATA, OR PROFITS;
//  OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY  THEORY OF LIABILITY,
//  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT  (INCLUDING NEGLIGENCE OR OTHERWISE)
//  ARISING IN ANY WAY OUT OF THE USE  OF THIS SOFTWARE, EVEN IF ADVISED OF THE
//  POSSIBILITY OF SUCH DAMAGE.
//-----------------------------------------------------------------------------
/*!
    \file     $Source: /cvs/genicam/genicam/source/TLClients/Stemmer/examples/SimpleNoDisp/Port.h,v $
    \brief    GenApi port implementation
    \version  $Revision: 1.2 $
    \date     $Date: 2008/12/23 21:39:55 $
*/

/* MODIFIED:        6/19/2012
 * AUTHOR:          Jeremy Greene
 * ORGANIZATION:    BitFlow, Inc.
 */

#ifndef INCLUDED__GCPORT__H
#define INCLUDED__GCPORT__H

#include <BFResolveGenTL.h>
#include "GenApi/GenApi.h"

#include "GenICamUtilities.h"

#if GENTL_H_AT_LEAST_V(1,5,0)
    using namespace GenTL;
#else
    using namespace GenICam::Client;
#endif

class GenTL_CTI::GCPort : public GenApi::IPort
{
public:
    GCPort (PORT_HANDLE hPort, GenTL_CTI *const funcs);
    ~GCPort (void);

    //! Reads a chunk of bytes from the port
    virtual void Read (void *pBuffer, int64_t Address, int64_t Length);

    //! Writes a chunk of bytes to the port
    virtual void Write (const void *pBuffer, int64_t Address, int64_t Length);

    virtual GenApi::EAccessMode GetAccessMode (void) const;

private:
    PORT_HANDLE         m_hPort;
    GenTL_CTI *const    m_gFuncs;
    std::string         m_portName;

    // Illegal.
    GCPort (GCPort const&);
    GCPort& operator= (GCPort const&);
};

#endif // INCLUDED__GCPORT__H
