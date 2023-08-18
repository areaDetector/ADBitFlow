/* FILE:        GenTLInterfaceExample.cpp
 * DATE:        6/19/2012, 3/6/2013, 12/20/2016
 * AUTHOR:      Jeremy Greene
 * COPYRIGHT:   BitFlow, Inc., 2012-2016
 * DESCRIPTION: A basic example program, demonstrating the following functionality of the
 *              GenTL Interface static library:
 *               1) Enumerate all available CTI files.
 *               2) Load a specific GenTL producer from a CTI file.
 *               3) Load one of the producer ports, and that port's XML node-map.
 *               2) Allow the user to get/set values from nodes on the node map.
 */

#if defined(_WIN32) && defined(_DEBUG)
#   define _CRTDBG_MAP_ALLOC
#   include <stdlib.h>
#   include <crtdbg.h>
#endif

// Header file defining the GenTL_CTI class.
#include "../GenTLInterface/GenICamUtilities.h"

// Local include file.
#include "GenTLInterfaceExample.h"

// Header file for the GenTLView live acquisition preview.
#include "GenTLView.h"

// GenICam header files we depend upon. These are also included by GenICamUtilities.h
#include <Base/GCUtilities.h>
#include <GenApi/GenApi.h>

using namespace GenApi;
using namespace GenICam;

#include <BFResolveGenTL.h>
#if GENTL_H_AT_LEAST_V(1,5,0)
    using namespace GenTL;
#else
    using namespace GenICam::Client;
#endif

#if defined(_WIN32)
#   include <conio.h>
#elif defined(__GNUC__)
#   include <poll.h>
#else
#   error Platform implementation missing.
#endif

#include <cstdint>
#include <cstdio>

#include <iostream>
#include <fstream>
#include <algorithm>

#include <list>
#include <vector>
#include <string>

#include <thread>

bool openInterfaceAndDevice (GenTL_CTI *const cti, const TL_HANDLE hTL, IF_HANDLE *const pIf, DEV_HANDLE *const pDev, GenTL_CTI::PortPtr *const pRemDevPort, GenTL_CTI::NodeMapPtr *const pRemDevNodeMap);
bool openDevice             (GenTL_CTI *const cti, const IF_HANDLE hIf, DEV_HANDLE *const pDev, GenTL_CTI::PortPtr *const pRemDevPort, GenTL_CTI::NodeMapPtr *const pRemDevNodeMap);
void execNodeConsole        (GenTL_CTI *const cti, const TL_HANDLE hTL);
GC_ERROR execEventViewer    (GenTL_CTI *const cti, const EVENTSRC_HANDLE hSrc);

int main (void)
{
#if defined(_WIN32) && defined(_DEBUG)
    _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

    // Find all available CTI files.
    std::vector<GenTL_CTI_string> fileList;
    if (!GenTL_CTI::findCtiFiles(fileList))
    {
        std::cerr << "There are no CTI files installed on this system.\n";
        return EXIT_SUCCESS;
    }

    // Print all available CTI file paths.
    std::cout << "Available CTI files:\n";
    for (size_t i = 0, cnt = fileList.size(); i < cnt; i++)
    {
#if defined(_WIN32)
        std::wcout << L" (" << i << L") " << fileList.at(i) << L"\n";
#elif defined(__GNUC__)
        std::cout << " (" << i << ") " << fileList.at(i) << "\n";
#else
#   error Local platform implementation missing.
#endif
    }

    // Prompt the user to select a file to open.
    unsigned int fileToLoad;
    do
    {
        std::cout << "Which CTI file to load (0 to " << (fileList.size() - 1) << ")? ";

        std::cin.clear();
        std::cin >> fileToLoad;
    }
    while (fileToLoad >= fileList.size());

#if defined(_WIN32)
    std::wcout << L"Loading CTI file (" << fileToLoad << L") " << fileList.at(fileToLoad) << L"\n";
#elif defined(__GNUC__)
    std::cout << "Loading CTI file (" << fileToLoad << ") " << fileList.at(fileToLoad) << "\n";
#else
#   error Local platform implementation missing.
#endif

    // Open the CTI file.
    GenTL_CTI::Ptr cti = GenTL_CTI::openCtiFilePath(fileList.at(fileToLoad));
    if (!cti)
    {
        std::cerr << "The CTI file could not be opened as a GenTL producer.\n";
        return EXIT_SUCCESS;
    }
    
    /* From here on out, we will be accessing the GenTL functions. These may throw a runtime_error,
     * if the function fails to load, so catch that, if necesssary. */
    bool ctiOpened = false;
    try
    {
        // Handles and other variables we'll be using.
        GC_ERROR    err;

        TL_HANDLE   hTL;

        // Attempt to initialize the GenTL library.
        err = cti->GCInitLib();
        if (err != GC_ERR_SUCCESS)
        {
            std::cerr << "Unable to initialize the GenTL producer library.\n";
            return EXIT_SUCCESS;
        }

        ctiOpened = true;
        std::cout << "Initialized the GenTL producer.\n";

        // Attempt to load the Transport Layer port.
        err = cti->TLOpen(&hTL);
        if (err != GC_ERR_SUCCESS)
        {
            cti->GCCloseLib();
            std::cerr << "Unable to load GenICam Transport Layer system.\n";
            return EXIT_SUCCESS;
        }
        
        std::cout << "Loaded the Transport Layer.\n";

        // Execute the node-console, allowing basic user interaction with the node map.
        execNodeConsole(cti.get(), hTL);

        // Cleanup the node-map
        std::cout << "Closed the remote device node-map.\n";

        // Attempt to cleanup the GenTL library.
        err = cti->GCCloseLib();
        if (err != GC_ERR_SUCCESS)
        {
            std::cerr << "Unable to cleanup the GenTL producer library.\n";
            return EXIT_SUCCESS;
        }

        std::cout << "Uninitialized the GenTL producer.\n";
    }
    catch (std::exception const& err)
    {
        if (ctiOpened)
            cti->GCCloseLib();
        std::cerr << "The GenTL_CTI library threw an exception: " << err.what() << "\n";
    }

    return EXIT_SUCCESS;
}

// Open a GenTL Device from among the provided list.
bool openInterfaceAndDevice (GenTL_CTI *const cti, const TL_HANDLE hTL, IF_HANDLE *const pIf, DEV_HANDLE *const pDev, GenTL_CTI::PortPtr *const pRemDevPort, GenTL_CTI::NodeMapPtr *const pRemDevNodeMap)
{
    GC_ERROR err;
    unsigned int ifCnt, ifSel;
    size_t iSize;

    if (*pDev)
    {
        std::cout << "\nClosing previously opened TLDevice.\n";

        err = cti->DevClose(*pDev);
        if (GC_ERR_SUCCESS != err)
        {
            std::cerr << "Failed to close the TLDevice.\n";
            return false;
        }
    }

    pRemDevPort->reset();
    pRemDevNodeMap->reset();
    *pDev = nullptr;

    if (*pIf)
    {
        std::cout << "\nClosing previously opened TLInterface.\n";

        err = cti->IFClose(*pIf);
        if (GC_ERR_SUCCESS != err)
        {
            std::cerr << "Failed to close the TLInterface.\n";
            return false;
        }
    }

    *pIf = nullptr;

    // Update the interface list.
    err = cti->TLUpdateInterfaceList(hTL, nullptr, GENTL_INFINITE);
    if (GC_ERR_SUCCESS != err)
    {
        cti->GCCloseLib();
        std::cerr << "Unable to update the interface list.\n";
        return false;
    }

    // Attempt to load the first transport layer interface.
    // Determine the number of interfaces available.
    err = cti->TLGetNumInterfaces(hTL, &ifCnt);
    if (err != GC_ERR_SUCCESS)
    {
        cti->GCCloseLib();
        std::cerr << "Unable to determine the number of TLInterfaces available.\n";
        return false;
    }

    if (!ifCnt)
    {
        cti->GCCloseLib();
        std::cerr << "There are no interfaces available to be loaded.\n";
        return false;
    }

    std::cout << "\nThere are " << ifCnt << " TLInterface(s) available:\n";

    // Query the TLInterfaceID's.
    std::vector<std::string> interfaceIDs (ifCnt);
    for (unsigned int i = 0; i < ifCnt; i++)
    {
        // Determine the string length.
        err = cti->TLGetInterfaceID(hTL, i, 0, &iSize);
        if (err != GC_ERR_SUCCESS)
        {
            cti->GCCloseLib();
            std::cerr << "Unable to determine the TLInterface string ID length.\n";
            return false;
        }

        if (!iSize)
        {
            cti->GCCloseLib();
            std::cerr << "The first interface ID is zero lengthed, thus invalid.\n";
            return false;
        }

        // Query the interface ID string.
        std::vector<char> strVec (iSize);
        err = cti->TLGetInterfaceID(hTL, i, strVec.data(), &iSize);
        if (err != GC_ERR_SUCCESS)
        {
            cti->GCCloseLib();
            std::cerr << "Unable to load the first interface ID.\n";
            return false;
        }

        // Store the interface ID.
        interfaceIDs[i] = strVec.data();

        std::cout << " (" << i << ") " << interfaceIDs[i] << "\n";
    }

    do
    {
        int i;
        std::cout << "Which to open (0 to " << (ifCnt - 1) << ", -1 to skip)? ";

        std::cin.clear();
        std::cin >> i;

        if (-1 == i)
            return false;
        ifSel = i;
    }
    while (ifSel >= ifCnt);

    // Load the interface.
    err = cti->TLOpenInterface(hTL, interfaceIDs[ifSel].c_str(), pIf);
    if (err != GC_ERR_SUCCESS)
    {
        cti->GCCloseLib();
        std::cerr << "Unable to load the first TLInterface.\n";
        return false;
    }

    std::cout << "Loaded the \"" << interfaceIDs[ifSel] << "\" TLInterface.\n";

    return openDevice(cti, *pIf, pDev, pRemDevPort, pRemDevNodeMap);
}

bool openDevice (GenTL_CTI *const cti, const IF_HANDLE hIf, DEV_HANDLE *const pDev, GenTL_CTI::PortPtr *const pRemDevPort, GenTL_CTI::NodeMapPtr *const pRemDevNodeMap)
{
    GC_ERROR err;
    unsigned int devCnt, devSel, devFlagSel;
    size_t iSize;

    if (*pDev)
    {
        std::cout << "\nClosing previously opened TLDevice.\n";

        err = cti->DevClose(*pDev);
        if (GC_ERR_SUCCESS != err)
        {
            std::cerr << "Failed to close the TLDevice.\n";
            return false;
        }
    }

    pRemDevPort->reset();
    pRemDevNodeMap->reset();
    *pDev = nullptr;

    // Update the device list.
    err = cti->IFUpdateDeviceList(hIf, nullptr, GENTL_INFINITE);
    if (GC_ERR_SUCCESS != err)
    {
        cti->GCCloseLib();
        std::cerr << "Unable to update the device list.\n";
        return false;
    }

    // Determine the number of devices available.
    err = cti->IFGetNumDevices(hIf, &devCnt);
    if (err != GC_ERR_SUCCESS)
    {
        cti->GCCloseLib();
        std::cerr << "Unable to determine the number of devices available.\n";
        return false;
    }

    if (0 == devCnt)
    {
        std::cout << "\nNo TLDevice available.\n";
        return false;
    }

    // Query the TLInterfaceID's.
    std::vector<std::string> deviceIDs (devCnt);
    for (unsigned int i = 0; i < devCnt; i++)
    {
        // Determine the string length.
        err = cti->IFGetDeviceID(hIf, i, 0, &iSize);
        if (err != GC_ERR_SUCCESS)
        {
            cti->GCCloseLib();
            std::cerr << "Unable to determine the TLInterface string ID length.\n";
            return false;
        }

        if (!iSize)
        {
            cti->GCCloseLib();
            std::cerr << "The first interface ID is zero lengthed, thus invalid.\n";
            return false;
        }

        // Query the interface ID string.
        std::vector<char> strVec (iSize);

        err = cti->IFGetDeviceID(hIf, i, strVec.data(), &iSize);
        if (err != GC_ERR_SUCCESS)
        {
            cti->GCCloseLib();
            std::cerr << "Unable to load the first interface ID.\n";
            return false;
        }

        // Store the interface ID.
        deviceIDs[i] = strVec.data();
    }

    std::cout << "\nThere are " << deviceIDs.size() << " TLDevice(s) available:\n";
    for (size_t i = 0; deviceIDs.size() > i; i++)
        std::cout << " (" << i << ") " << deviceIDs[i] << "\n";

    do
    {
        int i;
        std::cout << "Which to open (0 to " << (deviceIDs.size() - 1) << ", -1 to skip)? ";

        std::cin.clear();
        std::cin >> i;

        if (-1 == i)
            return false;
        devSel = i;
    }
    while (devSel >= deviceIDs.size());

    do
    {
        std::cout << "Device open flags:\n "
            << DEVICE_ACCESS_READONLY << " = DEVICE_ACCESS_READONLY\n "
            << DEVICE_ACCESS_CONTROL << " = DEVICE_ACCESS_CONTROL\n "
            << DEVICE_ACCESS_EXCLUSIVE << " = DEVICE_ACCESS_EXCLUSIVE\n"
            << "Which open flag (" << DEVICE_ACCESS_READONLY << ", " << DEVICE_ACCESS_CONTROL << ", or " << DEVICE_ACCESS_EXCLUSIVE << ")? ";

        std::cin.clear();
        std::cin >> devFlagSel;

        switch (devFlagSel)
        {
        case DEVICE_ACCESS_READONLY: case DEVICE_ACCESS_CONTROL: case DEVICE_ACCESS_EXCLUSIVE:
            break;
        default:
            devFlagSel = DEVICE_ACCESS_NONE;
            break;
        }
    }
    while (DEVICE_ACCESS_NONE == devFlagSel);

    // Load the device.
    err = cti->IFOpenDevice(hIf, deviceIDs[devSel].c_str(), devFlagSel, pDev);
    if (GC_ERR_SUCCESS != err)
    {
        std::cerr << "Unable to load the selected TLDevice.\n";
        return false;
    }

    std::cout << "Loaded the \"" << deviceIDs[devSel] << "\" TLDevice.\n";

    // Attempt to retrieve the TLDevice's remote device ports.
    PORT_HANDLE hRemDevPort;
    err = cti->DevGetPort(*pDev, &hRemDevPort);
    if (GC_ERR_SUCCESS != err)
    {
        std::cerr << "Unable to load the TLDevice port.\n";
        return false;
    }

    std::cout << "Loaded remote device port.\n";

    // Attempt to load the remote device's first GenTL node map.
    *pRemDevNodeMap = cti->getNodeMap(hRemDevPort, *pRemDevPort, "", 0);
    if (!pRemDevNodeMap->get())
    {
        std::cerr << "Unable to load the remote device node-map.\n";
        return false;
    }

    std::cout << "Loaded remote device node map.\n";

    return true;
}

// Allow the user to read and write values from the node map.
void execNodeConsole (GenTL_CTI *const cti, const TL_HANDLE hTL)
{
    const char helpText[] =
        "Node operations:\n"
        "  \"help\" prints this help text\n"
        "  \"gtlacq\" opens a simple acquisition demo\n"
        "  \"gtlacq_adv\" opens an interactive acquisition demo\n"
        "  \"gtlview\" opens a simple acquisition demo with viewer window\n"
        "  \"gtlview_adv\" opens an interactive acquisition demo with viewer window\n"
        "  \"evtview\" waits for GenTL events to signal\n"
        "  \"setif\" set the TLInterface (and TLDevice)\n"
        "  \"setdev\" set the TLDevice for the current TLInterface\n"
        "  \"setport\" set the current node port to edit\n"
        "  \"list\" prints all available node names\n"
        "  \"find <text>\" lists all node names containing <text> (case insensitive)\n"
        "  \"tree <text>\" print the tree of all nodes beneath <text> (try Root)\n"
        "  \"info <text>\" print information about the <text> node\n"
        "  \"[get] <node>\" prints the current value of <node> (\"get\" is implicit)\n"
        "  \"inv <node>\" invalidates <node> cache, then prints the current value\n"
        "  \"set <node> <value>\" attempts to set <node> to <value>\n"
        "  \"exit\" quits\n";

    std::string cmd = "setif";

    // Attempt to open a TLDevice.
    IF_HANDLE hIf = nullptr;
    DEV_HANDLE hDev = nullptr;

    PORT_HANDLE hModulePort = GENTL_INVALID_HANDLE;
    GenTL_CTI::PortPtr activeNodePort, remDevPort;
    GenTL_CTI::NodeMapPtr activeNodeMap, remDevNodeMap;

    do
    {
        // Catch any exceptions, and continue.
        try
        {
            if (cmd == "help")
                std::cout << helpText;
            else if (cmd == "gtlacq")
            {
                if (!hDev)
                    std::cerr << "\nUnable to run gtlacq without a device. Please run setdev, first.\n";
                else
                {
                    execGenTLAcq(cti, hDev, remDevNodeMap);
                    std::cout << "\ngtlacq has returned. Resuming the node console.\n\n";
                }
            }
            else if (cmd == "gtlacq_adv")
            {
                if (!hDev)
                    std::cerr << "\nUnable to run gtlacq_adv without a device. Please run setdev, first.\n";
                else
                {
                    execGenTLAcqAdv(cti, hDev, remDevNodeMap);
                    std::cout << "\ngtlacq_adv has returned. Resuming the node console.\n\n";
                }
            }
            else if (cmd == "gtlview")
            {
                if (!hDev)
                    std::cerr << "\nUnable to run gtlview without a device. Please run setdev, first.\n";
                else
                {
                    execGenTLView(cti, hDev, remDevNodeMap);
                    std::cout << "\ngtlview has returned. Resuming the node console.\n\n";
                }
            }
            else if (cmd == "gtlview_adv")
            {
                if (!hDev)
                    std::cerr << "\nUnable to run gtlview_adv without a device. Please run setdev, first.\n";
                else
                {
                    execGenTLViewAdv(cti, hDev, remDevNodeMap);
                    std::cout << "\ngtlview_adv has returned. Resuming the node console.\n\n";
                }
            }
            else if (cmd == "evtview")
                execEventViewer(cti, hModulePort);
            else if (cmd == "setif")
            {
                activeNodePort.reset();
                activeNodeMap.reset();

                if (openInterfaceAndDevice(cti, hTL, &hIf, &hDev, &remDevPort, &remDevNodeMap))
                {
                    std::cout << "Setting current port to the remote device port.\n";
                    hModulePort = hDev;
                    activeNodePort = remDevPort;
                    activeNodeMap = remDevNodeMap;
                }
                else if (hIf)
                    std::cerr << "No device was opened. No port set.\n";
                else
                    std::cerr << "No interface or device was opened. No port set.\n";
            }
            else if (cmd == "setdev")
            {
                if (hIf)
                {
                    activeNodePort.reset();
                    activeNodeMap.reset();

                    if (openDevice(cti, hIf, &hDev, &remDevPort, &remDevNodeMap))
                    {
                        std::cout << "Setting current port to the remote device port.\n";
                        hModulePort = hDev;
                        activeNodePort = remDevPort;
                        activeNodeMap = remDevNodeMap;
                    }
                    else
                        std::cerr << "No device was opened. No port set.\n";
                }
                else
                    std::cerr << "Cannot setdev, because no interface has been opened. Try setif, instead.\n";
            }
            else if (cmd == "setport")
            {
                do
                {
                    activeNodeMap.reset();
                    activeNodePort.reset();

                    std::string portName;
                    std::cout
                        << "\nPort module options:"
                        "\n  \"tlsys\" opens the TLSystem port";

                    if (hIf)
                        std::cout << "\n  \"tlif\" opens the TLInterface port";

                    if (hDev)
                        std::cout << "\n  \"tldev\" opens the TLDevice port";

                    if (remDevNodeMap.get())
                        std::cout << "\n  \"rdev\" selects the remote device port.";

                    std::cout << "\nWhich port to open? ";

                    std::cin.clear();
                    std::cin >> portName;

                    hModulePort = GENTL_INVALID_HANDLE;
                    if (portName == "tlsys")
                        hModulePort = hTL;
                    else if (hIf && portName == "tlif")
                        hModulePort = hIf;
                    else if (hDev && portName == "tldev")
                        hModulePort = hDev;
                    else if (remDevNodeMap.get() && portName == "rdev")
                    {
                        activeNodePort = remDevPort;
                        activeNodeMap = remDevNodeMap;
                    }

                    if (GENTL_INVALID_HANDLE != hModulePort)
                    {
                        // Attempt to load the remote device's first GenTL node map.
                        activeNodeMap = cti->getNodeMap(hModulePort, activeNodePort, "", 0);
                        if (!activeNodeMap.get())
                            std::cerr << "Unable to load the module port node map.\n";
                        else
                            std::cout << "Loaded the module port node map.\n";
                    }
                    else
                        hModulePort = hDev;
                }
                while (!activeNodeMap.get());
            }
            else if (cmd == "list")
            {
                if (!activeNodeMap.get())
                    std::cerr << "\n\"" << cmd << "\" cannot be run without a port.\n";
                else
                {
                    // Print the names of all available nodes.
                    std::cout << "Available nodes (\"-- <node name> -> <display name>\"):\n";
                    GenApi::NodeList_t nodeList;
                    activeNodeMap->_GetNodes(nodeList);

                    for (GenApi::NodeList_t::iterator Iter = nodeList.begin(), End = nodeList.end(); Iter != End; Iter++)
                        std::cout << " -- " << (*Iter)->GetName().c_str() << " -> \"" << (*Iter)->GetDisplayName().c_str() << "\"\n";
                }
            }
            else if (cmd == "find")
            {
                if (!activeNodeMap.get())
                    std::cerr << "\n\"" << cmd << "\" cannot be run without a port.\n";
                else
                {
                    // Print the names of all available nodes.
                    std::string toFind;
                    std::cin >> toFind;
                    std::transform(toFind.begin(), toFind.end(), toFind.begin(), ::tolower);

                    std::cout << "Nodes found (\"-- <node name> -> <display name>\"):\n";
                    GenApi::NodeList_t nodeList;
                    activeNodeMap->_GetNodes(nodeList);

                    for (GenApi::NodeList_t::iterator Iter = nodeList.begin(), End = nodeList.end(); Iter != End; Iter++)
                    {
                        std::string name = (*Iter)->GetName().c_str();
                        std::transform(name.begin(), name.end(), name.begin(), ::tolower);
                        if (name.find(toFind) != std::string::npos)
                            std::cout << " -- " << (*Iter)->GetName().c_str() << " -> \"" << (*Iter)->GetDisplayName().c_str() << "\"\n";
                    }
                }
            }
            else if (cmd == "tree")
            {
                if (!activeNodeMap.get())
                    std::cerr << "\n\"" << cmd << "\" cannot be run without a port.\n";
                else
                {
                    std::string rootName;
                    std::cin >> rootName;
                    const GenApi::CNodePtr rootNode (activeNodeMap->_GetNode(rootName.c_str()));
                    const GenApi::CCategoryPtr rootCat (rootNode);

                    if (rootCat)
                    {
                        std::cout << "\n + " << rootNode->GetName().c_str();

                        typedef std::pair<FeatureList_t, FeatureList_t::iterator> FeaturePair;
                        std::list<FeaturePair> stack;
                        stack.resize(1);

                        auto *activePair = &stack.back();
                        rootCat->GetFeatures(activePair->first);
                        activePair->second = activePair->first.begin();

                        while (stack.size())
                        {
                            activePair = &stack.back();

                            while (activePair->first.end() != activePair->second)
                            {
                                const std::string inset (stack.size() + 1, ' ');
                                std::cout << "\n" << inset;

                                GenApi::CNodePtr subNode (*activePair->second++);
                                GenApi::CCategoryPtr subCat (subNode);
                                GenApi::CEnumerationPtr subEnum (subNode);
                                if (subCat)
                                {
                                    std::cout << " + " << subNode->GetName().c_str();

                                    FeaturePair tmpPair;
                                    stack.push_back(tmpPair);
                                    activePair = &stack.back();
                                    subCat->GetFeatures(activePair->first);
                                    activePair->second = activePair->first.begin();
                                }
                                else if (subEnum)
                                {
                                    std::cout << " - " << subNode->GetName().c_str();

                                    StringList_t enumEntries;
                                    subEnum->GetSymbolics(enumEntries);
                                    for (auto const& enumEntry : enumEntries)
                                        std::cout << "\n" << inset << "   * " << enumEntry.c_str();
                                }
                                else if (subNode)
                                    std::cout << " - " << subNode->GetName().c_str();
                                else
                                    std::cerr << " ERROR: Null feature pointer.";
                            }
                            stack.pop_back();
                        }

                        std::cout << "\n";
                    }
                    else if (rootNode)
                        std::cout << "\"" << rootName << "\" is not a category.\n";
                    else
                        std::cerr << "Found no node named \"" << rootName << "\".\n";
                }
            }
            else if (cmd == "info")
            {
                if (!activeNodeMap.get())
                    std::cerr << "\n\"" << cmd << "\" cannot be run without a port.\n";
                else
                {
                    std::string infoName;
                    std::cin >> infoName;
                    const GenApi::CNodePtr infoNode (activeNodeMap->_GetNode(infoName.c_str()));

                    if (infoNode)
                    {
                        std::cout << "Device Name: " << infoNode->GetDeviceName().c_str() << "\n";
                        std::cout << "Display Name: " << infoNode->GetDisplayName().c_str() << "\n";

                        std::cout << "Principal Interface Type: ";
                        switch (infoNode->GetPrincipalInterfaceType())
                        {
                        case GenApi::intfIValue: std::cout << "IValue\n"; break;
                        case GenApi::intfIBase: std::cout << "IBase\n"; break;
                        case GenApi::intfIInteger: std::cout << "IInteger\n"; break;
                        case GenApi::intfIBoolean: std::cout << "IBoolean\n"; break;
                        case GenApi::intfICommand: std::cout << "ICommand\n"; break;
                        case GenApi::intfIFloat: std::cout << "IFloat\n"; break;
                        case GenApi::intfIString: std::cout << "IString\n"; break;
                        case GenApi::intfIRegister: std::cout << "IRegister\n"; break;
                        case GenApi::intfICategory: std::cout << "ICategory\n"; break;
                        case GenApi::intfIEnumeration: std::cout << "IEnumeration\n"; break;
                        case GenApi::intfIEnumEntry: std::cout << "IEnumEntry\n"; break;
                        case GenApi::intfIPort: std::cout << "IPort\n"; break;
                        }

                        std::cout << "Access Mode: ";
                        switch (infoNode->GetAccessMode())
                        {
                        case GenApi::NI: std::cout << "Not Implemented\n"; break;
                        case GenApi::NA: std::cout << "Not Available\n"; break;
                        case GenApi::WO: std::cout << "Write Only\n"; break;
                        case GenApi::RO: std::cout << "Read Only\n"; break;
                        case GenApi::RW: std::cout << "Read/Write\n"; break;
                        default: std::cout << "ERROR\n"; break;
                        }

                        std::cout << "Description: " << infoNode->GetDescription().c_str() << "\n";
                        std::cout << "Tool Tip: " << infoNode->GetToolTip().c_str() << "\n";

                        const GenApi::CEnumerationPtr infoEnum (infoNode);
                        if (infoEnum)
                        {
                            std::cout << "Enumeration Entries:\n";

                            StringList_t enumEntries;
                            infoEnum->GetSymbolics(enumEntries);
                            for (auto const& enumEntry : enumEntries)
                                std::cout << " * " << enumEntry.c_str() << "\n";
                        }
                    }
                    else
                        std::cerr << "Found no node named \"" << infoName << "\".\n";
                }
            }
            else if (cmd == "set")
            {
                if (!activeNodeMap.get())
                    std::cerr << "\n\"" << cmd << "\" cannot be run without a port.\n";
                else
                {
                // Attempt to set a value on the node, from a string.
                    std::string newValue;

                    std::cin >> cmd;
                    std::cin >> newValue;

                    CNodePtr node = activeNodeMap->_GetNode(cmd.c_str());
                    if (!node)
                        std::cerr << "The specified node does not exist.\n";
                    else
                    {
                        const CValuePtr valuePtr (node);
                        if (valuePtr)
                        {
                            try
                            {
                                valuePtr->FromString(newValue.c_str());
                            }
                            catch (std::exception const& err)
                            {
                                std::cerr << "Node value set failed: " << err.what() << "\n";
                            }
                        }
                        else
                            std::cout << "Node \"" << node->GetDisplayName().c_str() << "\" is not a value node.\n";
                    }
                }
            }
            else
            {
                // Attempt to find a node of the specified name, and print its name and value.
                bool ignoreCache = false;
                if (cmd == "get")
                    std::cin >> cmd;
                else if (cmd == "inv")
                {
                    ignoreCache = true;
                    std::cin >> cmd;
                }

                if (!activeNodeMap.get())
                    std::cerr << "\n\"get\" cannot be run without a port.\n";
                else
                {
                    CNodePtr node = activeNodeMap->_GetNode(cmd.c_str());
                    if (!node)
                        std::cerr << "There is no node called \"" << cmd << "\".\n";
                    else
                    {
                        const CValuePtr valuePtr (node);
                        if (valuePtr)
                            std::cout << "Node \"" << node->GetDisplayName().c_str() << "\" value is \"" << valuePtr->ToString(false, ignoreCache) << "\"\n";
                        else
                            std::cout << "Node \"" << node->GetDisplayName().c_str() << "\" is not a value node.\n";
                    }
                }
            }
        }
        catch (GenICam::GenericException const& err)
        {
            std::cerr << "\nA GenICam exception was thrown: " << err.what() << "\n";
        }
        catch (std::exception const& err)
        {
            std::cerr << "\nAn exception was thrown: " << err.what() << "\n";
        }
        catch (...)
        {
            std::cerr << "\nCaught an unknown exception.\n";
        }

        printErrors(std::cerr, cti);

        // Clear the input buffer.
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        // Get the next command.
        std::cout << ">>";
        std::cin >> cmd;
    }
    while (cmd != "exit");
}

// This function waits for events on the given handle until GC_ERR_ABORT.
void eventThreadFunc (bool *const a_keepRunning, GenTL_CTI *const cti, EVENT_TYPE evtType, EVENT_HANDLE hEvt, size_t evtSizeMax, const std::uint64_t a_maxCnt, std::ofstream *const a_fileOut, const bool a_verboseView)
{
    GC_ERROR err;
    INFO_DATATYPE iType;

    std::vector<std::uint8_t> data (evtSizeMax);

    auto lastTime = std::chrono::steady_clock::now();

    std::uint64_t eventCount = 0, lastEventCount = 0;

    while (*a_keepRunning && (0 == a_maxCnt || a_maxCnt > eventCount))
    {
        // Wait for an event.
        size_t iSize = data.size();
        err = cti->EventGetData(hEvt, data.data(), &iSize, GENTL_INFINITE);
        if (GC_ERR_ABORT == err)
            break;
        else if (GC_ERR_SUCCESS != err)
            std::cerr << GenTL_CTI::gentlError(err, "EventGetData returned without data.") << "\n";
        else
        {
            size_t iFieldSize;
            std::vector<std::uint8_t> fieldData;

            // Count this event.
            ++eventCount;

            // Decide what we're going to display.
            bool printThisEvent = a_verboseView;

            const auto timeNow = std::chrono::steady_clock::now();
            if (timeNow - lastTime >= std::chrono::seconds(1))
            {
                printThisEvent = true;
                const double evtPS = 1000. * double(eventCount - lastEventCount) / (double)std::chrono::duration_cast<std::chrono::milliseconds>(timeNow - lastTime).count();
                std::cout.precision(2);
                std::cout << "\n" << eventCount << " events at " << std::fixed << evtPS << " EPS\n";
                lastTime = timeNow;
                lastEventCount = eventCount;
            }

            if (printThisEvent)
                std::cout << "\n[" << (eventCount - 1) << "]\n";

            // Get the EVENT_DATA_ID
            err = cti->EventGetDataInfo(hEvt, data.data(), iSize, EVENT_DATA_ID, &iType, nullptr, &iFieldSize);
            if (GC_ERR_SUCCESS == err)
            {
                fieldData.resize(iFieldSize);
                err = cti->EventGetDataInfo(hEvt, data.data(), iSize, EVENT_DATA_ID, &iType, fieldData.data(), &iFieldSize);
            }

            if (GC_ERR_SUCCESS != err)
                std::cerr << GenTL_CTI::gentlError(err, "Unable to query EVENT_DATA_ID.") << "\n";
            else if (printThisEvent)
                printData(std::cout << "EVENT_DATA_ID = ", iType, fieldData.data(), iFieldSize) << "\n";

            // Get the EVENT_DATA_NUMID
            err = cti->EventGetDataInfo(hEvt, data.data(), iSize, EVENT_DATA_NUMID, &iType, nullptr, &iFieldSize);
            if (GC_ERR_SUCCESS == err)
            {
                fieldData.resize(iFieldSize);
                err = cti->EventGetDataInfo(hEvt, data.data(), iSize, EVENT_DATA_NUMID, &iType, fieldData.data(), &iFieldSize);
            }

            if (GC_ERR_SUCCESS != err)
                std::cerr << GenTL_CTI::gentlError(err, "Unable to query EVENT_DATA_NUMID.") << "\n";
            else if (printThisEvent)
                printData(std::cout << "EVENT_DATA_NUMID = ", iType, fieldData.data(), iFieldSize) << "\n";

            // Get the EVENT_DATA_VALUE
            err = cti->EventGetDataInfo(hEvt, data.data(), iSize, EVENT_DATA_VALUE, &iType, nullptr, &iFieldSize);
            if (GC_ERR_SUCCESS == err)
            {
                fieldData.resize(iFieldSize);
                err = cti->EventGetDataInfo(hEvt, data.data(), iSize, EVENT_DATA_VALUE, &iType, fieldData.data(), &iFieldSize);
            }

            if (GC_ERR_SUCCESS != err)
                std::cerr << GenTL_CTI::gentlError(err, "Unable to query EVENT_DATA_VALUE.") << "\n";
            else if (EVENT_REMOTE_DEVICE == evtType)
            {
                // For now, we assume EVENT_REMOTE_DEVICE is a CXP event.
                if (sizeof(std::uint32_t) * 3 > iFieldSize)
                    std::cerr << "Insufficient data for a CXP event.\n";
                else
                {
                    const std::uint32_t *const msgData = reinterpret_cast<std::uint32_t*>(fieldData.data());

                    // First word is the header.
                    const std::uint32_t msgHeader = msgData[0];
                    const unsigned int msgSize = (msgHeader >> 16) & ((1u << 16) - 1u);
                    const unsigned int msgNamespace = (msgHeader >> 14) & ((1u << 2) - 1u);
                    const unsigned int msgReserved = (msgHeader >> 12) & ((1u << 2) - 1u);
                    const unsigned int msgEventID = (msgHeader >> 0) & ((1u << 12) - 1u);

                    // Next two words are the timestamp.
                    std::uint64_t msgTimestamp = msgData[1];
                    msgTimestamp <<= 32;
                    msgTimestamp |= msgData[2];

                    // Print the header fields.
                    if (printThisEvent)
                    {
                        std::cout <<
                            "EVENT_DATA_VALUE:"
                            "\n  Size      = " << msgSize <<
                            "\n  Namespace = " << msgNamespace <<
                            "\n  Reserved  = " << msgReserved <<
                            "\n  EventID   = " << msgEventID <<
                            "\n  Timestamp = " << msgTimestamp <<
                            "\n  Data      = ";

                        // Data starts at the third word of the message.
                        const size_t dataSize = msgSize - sizeof(std::uint32_t) * 3;
                        printData(std::cout, INFO_DATATYPE_BUFFER, &msgData[3], dataSize) << "\n";
                    }

                    // Print the file output. This is formatted resemble as closely as possible the EventMonitor tool.
                    if (a_fileOut->good())
                    {
                        char tmpStr[128];

                        *a_fileOut <<
                            "\nMessage Size: " << msgSize << "\n"
                            "Message Namespace: " << msgNamespace << "\n"
                            "Event ID: " << msgEventID << "\n";

                        sprintf(tmpStr, "Timestamp: 0x%08x'%08x\n", std::uint32_t(msgTimestamp >> 32), std::uint32_t(msgTimestamp & 0xffffffff));
                        *a_fileOut << tmpStr;

                        *a_fileOut << "Data\n";

                        const size_t dataSize = msgSize - sizeof(std::uint32_t) * 3;
                        std::uint8_t *pBytes = (std::uint8_t*)&msgData[3];
                        for (std::uint32_t byteNum = 0; byteNum < dataSize; ++byteNum, ++pBytes)
                        {
                            sprintf(tmpStr, "%d 0x%02x\n", byteNum, *pBytes);
                            *a_fileOut << tmpStr;
                        }
                    }
                }
            }
            else if (printThisEvent)
            {
                // Print the value naively.
                printData(std::cout << "EVENT_DATA_VALUE = ", iType, fieldData.data(), iFieldSize) << "\n";
            }
        }
    }

    *a_keepRunning = false;
}
GC_ERROR execEventViewer (GenTL_CTI *const cti, const EVENTSRC_HANDLE hSrc)
{
    GC_ERROR err;
    EVENT_TYPE evtType;
    EVENT_HANDLE hEvt;

    std::cout << "Event types:"
        "\n " << EVENT_ERROR << " = EVENT_ERROR"
        "\n " << EVENT_NEW_BUFFER << " = EVENT_NEW_BUFFER"
        "\n " << EVENT_FEATURE_INVALIDATE << " = EVENT_FEATURE_INVALIDATE"
        "\n " << EVENT_FEATURE_CHANGE << " = EVENT_FEATURE_CHANGE"
        "\n " << EVENT_REMOTE_DEVICE << " = EVENT_REMOTE_DEVICE"
        "\n " << EVENT_MODULE << " = EVENT_MODULE"
        "\nWhich event type? ";
    std::cin >> evtType;

    bool printVerbose = false;
    std::string outputFileName;
    std::ofstream outputFileStrm;

    int mode = -1;
    while (0 > mode || mode > 2)
    {
        std::cout << "Output modes:"
            "\n (0) Default"
            "\n (1) Verbose"
            "\n (2) File"
            "\nWhich output mode? ";

        std::cin >> mode;
        switch (mode)
        {
        case 0:
            break;
        case 1:
            printVerbose = true;
            break;
        case 2:
            std::cout << "File name: ";
            std::cin >> outputFileName;

            outputFileStrm.open(outputFileName);
            if (!outputFileStrm)
            {
                std::cout << "There was a problem opening that file.\n";
                outputFileStrm.close();
                mode = -1;
            }
            break;
        }
    }

    std::cout << "Max event count (0 for infinite)? ";
    std::uint64_t maxCount;
    std::cin >> maxCount;
    std::cin.ignore();

    err = cti->GCRegisterEvent(hSrc, evtType, &hEvt);
    if (GC_ERR_SUCCESS != err)
        std::cerr << GenTL_CTI::gentlError(err, "Unable to register the event") << "\n";
    else
    {
        INFO_DATATYPE iType;
        size_t iSizeMax;
        size_t iSize = sizeof(iSizeMax);
        err = cti->EventGetInfo(hEvt, EVENT_SIZE_MAX, &iType, &iSizeMax, &iSize);
        if (GC_ERR_SUCCESS != err)
            std::cerr << GenTL_CTI::gentlError(err, "Unable to query EVENT_SIZE_MAX") << "\n";
        else
        {
            std::cout << "Entering event wait loop. Press Enter/Return to quit.\n";

            // Start the event thread.
            bool keepRunning = true;
            std::thread evtThread (&eventThreadFunc, &keepRunning, cti, evtType, hEvt, iSizeMax, maxCount, &outputFileStrm, printVerbose);

            if (0 == maxCount)
            {
                // Wait for some user input.
                std::string t;
                std::getline(std::cin, t);

                // Kill the event thread, and wait for its completion.
                keepRunning = false;
                cti->EventKill(hEvt);

                if (evtThread.joinable())
                    evtThread.join();
            }
            else
            {
                // Wait for some user input, or for the thread to exit.
                for (;;)
                {
#if defined(_WIN32)

                    if (_kbhit())
                    {
                        _getch();
                        keepRunning = false;
                        cti->EventKill(hEvt);
                        break;
                    }
                    else if (!keepRunning)
                        break;
                    else
                        std::this_thread::sleep_for(std::chrono::milliseconds(10));

#elif defined(__GNUC__)

                    // Poll file-descriptor zero (STDIN) for POLLIN.
                    struct pollfd pfd = { .fd=0, .events=POLLIN, .revents=0 };
                    poll(&pfd, 1, 10);
                    if (pfd.revents & POLLIN)
                    {
                        keepRunning = false;
                        cti->EventKill(hEvt);
                        break;
                    }

#else
#   error Platform implementation missing.
#endif
                }

                // Wait for the event to complete.
                if (evtThread.joinable())
                    evtThread.join();
            }
        }

        err = cti->GCUnregisterEvent(hSrc, evtType);
    }

    return err;
}

// Print a GenTL data type value to the output stream.
std::ostream& printData (std::ostream &ostrm, const INFO_DATATYPE type, const void *const data, const size_t dataLen)
{
    size_t offset;
    std::string str;
    switch (type)
    {
    case INFO_DATATYPE_STRING:
        str.assign(reinterpret_cast<const char*>(data), dataLen);
        ostrm << "[STR] " << str;
        break;

    case INFO_DATATYPE_STRINGLIST:
        ostrm << "[STRLIST] { \"";
        offset = 0;
        str.assign(reinterpret_cast<const char*>(data));
        while (str.length() > 0)
        {
            ostrm << str << "\", \"";
            offset += str.length() + 1;
            str.assign(reinterpret_cast<const char*>(data) + offset);
        }
        ostrm << "\" }";
        break;

    case INFO_DATATYPE_INT16:
        ostrm << "[INT16] " << *reinterpret_cast<const std::int16_t*>(data);
        break;
    case INFO_DATATYPE_UINT16:
        ostrm << "[UINT16] " << *reinterpret_cast<const std::uint16_t*>(data);
        break;
    case INFO_DATATYPE_INT32:
        ostrm << "[INT32] " << *reinterpret_cast<const std::int32_t*>(data);
        break;
    case INFO_DATATYPE_UINT32:
        ostrm << "[UINT32] " << *reinterpret_cast<const std::uint32_t*>(data);
        break;
    case INFO_DATATYPE_INT64:
        ostrm << "[INT64] " << *reinterpret_cast<const std::int64_t*>(data);
        break;
    case INFO_DATATYPE_UINT64:
        ostrm << "[UINT64] " << *reinterpret_cast<const std::uint64_t*>(data);
        break;
    case INFO_DATATYPE_FLOAT64:
        ostrm << "[FLOAT64] " << *reinterpret_cast<const double*>(data);
        break;
    case INFO_DATATYPE_BOOL8:
        ostrm << "[BOOL8] " << (*reinterpret_cast<const std::uint8_t*>(data) ? "true" : "false");
        break;
    case INFO_DATATYPE_SIZET:
        ostrm << "[SIZET] " << *reinterpret_cast<const size_t*>(data);
        break;

    case INFO_DATATYPE_PTR:
        ostrm << "[PTR] " << std::hex << *reinterpret_cast<const size_t*>(data) << std::dec;
        break;
    case INFO_DATATYPE_PTRDIFF:
        ostrm << "[PTRDIFF] " << std::hex << *reinterpret_cast<const std::ptrdiff_t*>(data) << std::dec;
        break;

    default:
    case INFO_DATATYPE_UNKNOWN:
        ostrm << "[UNKNOWN TYPE=" << type << "] ";
    case INFO_DATATYPE_BUFFER:
        ostrm << "[BUFFER] " << std::hex;
        for (size_t i = 0; dataLen > i; i++)
            ostrm << (std::uint32_t)reinterpret_cast<const std::uint8_t*>(data)[i] << " ";
        ostrm << std::dec;
        break;
    }

    return ostrm;
}

// Print any queued errors.
std::ostream& printErrors (std::ostream &ostrm, GenTL_CTI *const cti)
{
    GC_ERROR err = cti->lastError();

    if (GC_ERR_SUCCESS != err && GC_ERR_TIMEOUT != err)
    {
        // Disable error recording, here.
        const bool recordErrors = cti->recordErrors(false);

        // Print the recorded error message.
        ostrm << GenTL_CTI::gentlError(cti->popLastError(), "GenTL Interface recorded an error") << "\n";

        // Determine if the GenTL has any error messages, and if so, determine the size.
        GC_ERROR errorCode;
        size_t messageSize = 0;
        err = cti->GCGetLastError(&errorCode, 0, &messageSize);

        if (GC_ERR_SUCCESS == err)
        {
            if (GC_ERR_SUCCESS != errorCode)
            {
                // Retrieve the error, and set the output message to that retrieved.
                std::vector<char> errorStr (messageSize, 0);
                err = cti->GCGetLastError(&errorCode, errorStr.data(), &messageSize);

                if (GC_ERR_SUCCESS == err)
                    ostrm << "GenTL producer last error: " << GenTL_CTI::gentlError(errorCode, errorStr.data()) << "\n";
                else
                {
                    ostrm << GenTL_CTI::gentlError(errorCode, "Last error (no message)") << "\n";
                    ostrm << GenTL_CTI::gentlError(err, "Failed to retrieve the last GenTL error string") << "\n";;
                }
            }
        }
        else
            ostrm << GenTL_CTI::gentlError(err, "Failed to retrieve the last GenTL error") << "\n";

        // Restore the error recording state.
        cti->recordErrors(recordErrors);
    }

    return ostrm;
}
