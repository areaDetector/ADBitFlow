//-----------------------------------------------------------------------------
//  (c) 2006-2008 by STEMMER IMAGING GmbH
//  Project: GenTL
//  Author:  RST
//  $Header: /cvs/genicam/genicam/source/TLClients/Stemmer/examples/SimpleNoDisp/Port.cpp,v 1.2 2008/12/23 21:39:55 rupert_stelz Exp $
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
    \file     $Source: /cvs/genicam/genicam/source/TLClients/Stemmer/examples/SimpleNoDisp/Port.cpp,v $
    \brief    GenApi port implementation
    \version  $Revision: 1.2 $
    \date     $Date: 2008/12/23 21:39:55 $
*/

/* MODIFIED:        6/19/2012, 3/6/2013
 * AUTHOR:          Jeremy Greene
 * ORGANIZATION:    BitFlow, Inc.
 */

#if defined(_WIN32) && defined(_DEBUG)
#   define _CRTDBG_MAP_ALLOC
#   include <stdlib.h>
#   include <crtdbg.h>
#endif

#include "Port.h"
#include "GenICamUtilities.h"
#include "GenICamUtilities_Private.h"

using namespace std;
using namespace GenICam;
using namespace GenApi;

// ---------------------------------------------------------------------------
/// \brief CTOR
///
// ---------------------------------------------------------------------------
GenTL_CTI::GCPort::GCPort ( PORT_HANDLE hPort, GenTL_CTI *const funcs)
  : m_hPort     ( hPort )
  , m_gFuncs    ( funcs )
{
    m_portName = "Unknown";
    try
    {
        INFO_DATATYPE dType;
        size_t iSize;
        if (GC_ERR_SUCCESS == m_gFuncs->GCGetPortInfo(hPort, PORT_INFO_MODULE, &dType, nullptr, &iSize))
        {
            std::vector<char> portNameBuf (iSize);
            if (GC_ERR_SUCCESS == m_gFuncs->GCGetPortInfo(hPort, PORT_INFO_MODULE, &dType, portNameBuf.data(), &iSize))
            {
                portNameBuf.push_back(0);
                m_portName = portNameBuf.data();
            }
        }
    }
    catch (...)
    { }
}

// ---------------------------------------------------------------------------
/// \brief DTOR
///
// ---------------------------------------------------------------------------
GenTL_CTI::GCPort::~GCPort ( void )
{ }

// ---------------------------------------------------------------------------
/// \brief Reads a chunk of bytes from the port
///
/// \param pBuffer
/// \param Address
/// \param Length
// ---------------------------------------------------------------------------
void GenTL_CTI::GCPort::Read (void *pBuffer, int64_t Address, int64_t Length)
{
    if (!m_gFuncs->blockPortReads())
    {
        size_t iSize = (size_t)Length;
        const GC_ERROR gcrc = m_gFuncs->GCReadPort(m_hPort, Address, pBuffer, &iSize);
        m_gFuncs->m_pd.log_port_read(m_portName, gcrc, pBuffer, Address, Length);
    }
}

// ---------------------------------------------------------------------------
/// \brief Writes a chunk of bytes to the port
///
/// \param pBuffer
/// \param Address
/// \param Length
// ---------------------------------------------------------------------------
void GenTL_CTI::GCPort::Write (const void *pBuffer,int64_t Address, int64_t Length)
{
    if (!m_gFuncs->blockPortWrites())
    {
        size_t iSize = (size_t)Length;
        const GC_ERROR gcrc = m_gFuncs->GCWritePort(m_hPort, Address, pBuffer, &iSize);
        m_gFuncs->m_pd.log_port_write(m_portName, gcrc, pBuffer, Address, Length);
    }
}

// ---------------------------------------------------------------------------
/// \brief Retrieve the access mode for this port
///
/// \return GenApi::EAccessMode
// ---------------------------------------------------------------------------
EAccessMode GenTL_CTI::GCPort::GetAccessMode (void) const
{
    const bool readable = !m_gFuncs->blockPortReads();
    const bool writable = !m_gFuncs->blockPortWrites();

    if (readable)
    {
        if (writable)
            return RW;
        return RO;
    }
    else if (writable)
        return WO;
    else
        return NA;
}
