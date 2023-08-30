#pragma once
#ifndef INCLUDED__BITFLOW__BUFFER_INTERFACE_PROPERTIES__H
#define INCLUDED__BITFLOW__BUFFER_INTERFACE_PROPERTIES__H

#include "BufferInterface.h"

/**
 * \class BufferAcquisition::BufferInterface::Properties
 *
 * \brief Static properties of an open \ref BufferAcquisition::BufferInterface.
 */

namespace BufferAcquisition
{
    /**
     * \class InterfaceType
     * 
     * \brief Enumeration specifying the hardware interface type.
     */
    enum class InterfaceType
    {
        Unknown = 0,

        Analog = 1,

        Differential = 2,
        Dif = Differential,

        CameraLink = 3,
        CL = CameraLink,

        CoaXPress = 4,
        CXP = CoaXPress,
    };

    /**
     * \class Trait
     *
     * \brief Enumeration specifying traits that a board may or may not have.
     */
    enum class Trait
    {
        R2,
        Rv,
        R64,
        R64Board,
        CL,
        R3,
        PMC,
        PLDA,
        StreamSyncDMA,
        Kbn,
        Kbn4,
        Kbn2,
        KbnBase,
        KbnFull,
        Neon,
        NeonBase,
        NeonD,
        NeonQ,
        NeonDif,
        Alta,
        AltaAN,
        AltaCO,
        AltaYPC,
        Alta1,
        Alta2,
        Alta4,
        Master,
        Slave,
        EncDiv,
        NTG,
        KbnCXP,
        KbnCXP1,
        KbnCXP2,
        KbnCXP4,
        Gn2,
        Ctn,
        CXP,
        CtnCXP2,
        CtnCXP4,
        Axn,
        AxnBase,
        AxnII,
        CtnII,
        Axn1xE,
        Axn2xE,
        Axn1xB,
        Axn2xB,
        Axn4xB,
        Aon,
        AonII,
        AonCXP1,
        Clx,
        ClxCXP1,
        ClxCXP2,
        ClxCXP4,
        Synthetic,
        HasSerialPort
    };

    class BFDLL BufferInterface::Properties
    {
    public:
        bool test (const Trait trait) const;

        InterfaceType interface_type (void) const;

        BFU32 vfg_number (void) const;

        std::string model_name (void) const;
        bool get_model_name (char *const strBuf, size_t *const pBufSize) const;

        std::string family_name (void) const;
        bool get_family_name (char *const strBuf, size_t *const pBufSize) const;
        
        BFU32 family_index (void) const;

        BFU32 ci_index (void) const;

    private:
        // PrivateData members.
        struct PrivateData;
        PrivateData &m_pd;

        // Valid constructors/destructors.
        Properties (BufferInterface &bufin);
        ~Properties (void);

        // The BufferInterface class is our friend.
        friend class ::BufferAcquisition::BufferInterface;
        friend struct ::BufferAcquisition::BufferInterface::PrivateData;
        
        // Deleted methods.
        Properties (void);
        Properties (Properties const&);
        Properties& operator= (Properties const&);
    };
}

#endif // INCLUDED__BITFLOW__BUFFER_INTERFACE_PROPERTIES__H
