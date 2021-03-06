/**
 * Copyright (C) 2006 Koninklijke Philips Electronics N.V., All Rights Reserved.
 * This source code and any compilation or derivative thereof is the proprietary
 * information of Koninklijke Philips Electronics N.V. and is confidential in
 * nature. Under no circumstances is this software to be  exposed to or placed
 * under an Open Source License of any type without the expressed written
 * permission of Koninklijke Philips Electronics N.V.
 *
 * \file          tmbslHdmiTx_local.c
 *
 * \version       $Revision: 15 $
 *
 * \date          $Date: 29/10/07 14:11 $
 *
 * \brief         BSL driver component for the TDA998x HDMI Transmitter
 *
 * \section refs  Reference Documents
 * HDMI Driver - Outline Architecture.doc
 * HDMI Driver - tmbslHdmiTx - SCS.doc
 *
 * \section info  Change Information
 *
 * \verbatim
   $History: tmbslHdmiTx_local.c $
 *
 * *****************  Version 15  *****************
 * User: B.Vereecke     Date: 29/10/07   Time: 14:11
 * Updated in $/Source/tmbslHdmiTx/src
 * PR852 : remove external library dependancy
 *
 * *****************  Version 14  *****************
 * User: B.Vereecke    Date: 08/08/07   Time: 15:40
 * Updated in $/Source/tmbslHdmiTx/Src
 * PR 563 : Unused shadow value for register 
 *          E_REG_P00_INT_FLAGS_2_RW
 * 
 * *****************  Version 13  *****************
 * User: Burnouf       Date: 18/04/07   Time: 15:50
 * Updated in $/Source/tmbslHdmiTx/Src
 * PR 50 : Manage state ST_AWAIT_RX_SENSE for TDA9981
 * 
 * *****************  Version 12  *****************
 * User: Mayhew       Date: 21/09/06   Time: 15:50
 * Updated in $/Source/tmbslHdmiTx/Src
 * Cut code size in demo by using RETIF_BADPARAM
 * 
 * *****************  Version 10  *****************
 * User: Mayhew       Date: 15/09/06   Time: 16:08
 * Updated in $/Source/tmbslHdmiTx/Src
 * Reduce code size in setState by using local copies of DIS
 * 
 * *****************  Version 8  *****************
 * User: Mayhew       Date: 30/06/06   Time: 13:23
 * Updated in $/Source/tmbslHdmiTx/Src
 * EV_HDCP_BKSV_SECURE renamed to EV_HDCP_BKSV_REPEAT
 * 
 * *****************  Version 6  *****************
 * User: Mayhew       Date: 6/06/06    Time: 13:40
 * Updated in $/Source/tmbslHdmiTx/Src
 * Add checkUnitSetDis to reduce code size in most APIs
 * 
 * *****************  Version 4  *****************
 * User: Djw          Date: 24/05/06   Time: 11:20
 * Updated in $/Source/tmbslHdmiTx/Src
 * Added E_PAGE_10 for new Infoframe registers in N4.
 * 
 * *****************  Version 3  *****************
 * User: Mayhew       Date: 10/05/06   Time: 17:09
 * Updated in $/Source/tmbslHdmiTx/Src
 * Add HDCP state handling
 * 
 * *****************  Version 2  *****************
 * User: Djw          Date: 20/04/06   Time: 17:32
 * Updated in $/Source/tmbslHdmiTx/Src
 * Modification to prevent reading of shadow copies of interrupt flag
 * registers.
 * 
 * *****************  Version 1  *****************
 * User: Mayhew       Date: 4/04/06    Time: 16:30
 * Created in $/Source/tmbslHdmiTx/Src
 * Driver local code & data phase 2
   \endverbatim
 *
*/

/*============================================================================*/
/*                   FILE CONFIGURATION                                       */
/*============================================================================*/

/* Defining this symbol on the compiler command line excludes some API checks */
/* #define NO_RETIF_BADPARAM */

/*============================================================================*/
/*                   STANDARD INCLUDE FILES                                   */
/*============================================================================*/


/*============================================================================*/
/*                   PROJECT INCLUDE FILES                                    */
/*============================================================================*/
#include "tmbslHdmiTx.h"
#include "tmbslHdmiTx_local.h"

/*============================================================================*/
/*                   MACRO DEFINITIONS                                        */
/*============================================================================*/


/*============================================================================*/
/*                   TYPE DEFINITIONS                                         */
/*============================================================================*/


/*============================================================================*/
/*                   PUBLIC VARIABLE DEFINITIONS                              */
/*============================================================================*/

/** The array of object instances for all concurrently supported HDMI 
 *  Transmitter units
 */
RAM_DAT tmHdmiTxobject_t gHdmiTxInstance[HDMITX_UNITS_MAX];

/**
 * Lookup table to map register page index to actual page number
 */
CONST_DAT UInt8 kPageIndexToPage[E_PAGE_NUM] =
{
    0x00,   /* E_PAGE_00 */
    0x01,   /* E_PAGE_01 */
    0x02,   /* E_PAGE_02 */
    0x10,   /* E_PAGE_10 */
    0x11,   /* E_PAGE_11 */
    0x12    /* E_PAGE_12 */
};


/*============================================================================*/
/*                   STATIC VARIABLE DECLARATIONS                             */
/*============================================================================*/

/**
 * Lookup table to map an 8-bit mask to a number of left shifts
 * needed to shift a value starting at bit 0 onto the mask.
 * Indexed by mask 0-255. For example, mask 0x00 and 0x01 need
 * no shift, mask 0x02 needs one shift, mask 0x03 needs no shift,
 * mask 0x04 needs 2 shifts, etc.
 * Rows were formatted by "HDMI Driver - Register List.xls" and pasted here
 */
static CONST_DAT UInt8 kMaskToShift[256] =
{/* Mask index: */
 /*x0 x1 x2 x3 x4 x5 x6 x7 x8 x9 xA xB xC xD xE xF        */
    0, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, /* 0x */
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, /* 1x */
    5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, /* 2x */
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, /* 3x */
    6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, /* 4x */
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, /* 5x */
    5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, /* 6x */
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, /* 7x */
    7, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, /* 8x */
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, /* 9x */
    5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, /* Ax */
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, /* Bx */
    6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, /* Cx */
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, /* Dx */
    5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, /* Ex */
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0  /* Fx */
};

/*============================================================================*/
/*                   STATIC FUNCTION DECLARATIONS                             */
/*============================================================================*/

/*============================================================================*/
/*                   PUBLIC FUNCTION DEFINITIONS                              */
/*============================================================================*/

/*============================================================================*/
/* checkUnitSetDis                                                            */
/*============================================================================*/
tmErrorCode_t
checkUnitSetDis
(
    tmUnitSelect_t	 txUnit,
    tmHdmiTxobject_t **ppDis
)
{
    /* Return error if unit numbr is out of range */
    RETIF(txUnit < tmUnit0, TMBSL_ERR_HDMI_BAD_UNIT_NUMBER)
    RETIF(txUnit >= HDMITX_UNITS_MAX, TMBSL_ERR_HDMI_BAD_UNIT_NUMBER)

    /* Point to unit's Device Instance Structure */
    *ppDis = &gHdmiTxInstance[txUnit];

    /* Return if this device instance is not initialised */
    RETIF(!(*ppDis)->bInitialized, TMBSL_ERR_HDMI_NOT_INITIALIZED)

    return TM_OK;
}

/*============================================================================*/
/* getHwRegisters                                                             */
/*============================================================================*/
tmErrorCode_t
getHwRegisters
(
    tmHdmiTxobject_t *pDis, 
    UInt16 regShadPageAddr,
    UInt8 *pData, 
    UInt16 lenData
    )
{
    tmErrorCode_t   err;        /* Error code */
    UInt8           regShad;    /* The index to the register's shadow copy */
    UInt8           regPage;    /* The index to the register's page        */
    UInt8           regAddr;    /* The register's address on the page      */
    UInt8           newRegPage; /* The register's new page number          */
    tmbslHdmiTxSysArgs_t sysArgs; /* Arguments passed to system function   */

    /* Unpack 1st register's shadow index, page index and address */
    regShad = SPA2SHAD(regShadPageAddr);
    regPage = SPA2PAGE(regShadPageAddr);
    regAddr = SPA2ADDR(regShadPageAddr);
    newRegPage = kPageIndexToPage[regPage];
    
    /* Check length does not overflow page */
    RETIF_BADPARAM((regAddr+lenData) > E_REG_CURPAGE_ADR_W)

    /* Check 1st reg does not have a shadow - whole range assumed likewise */
    RETIF_BADPARAM(regShad != E_SNONE)

    /* Set page register if required */
    if (pDis->curRegPage != newRegPage)
    {
        /* All non-OK results are errors */
        sysArgs.slaveAddr       = pDis->uHwAddress;
        sysArgs.firstRegister   = E_REG_CURPAGE_ADR_W;
        sysArgs.lenData         = 1;
        sysArgs.pData           = &newRegPage;
        err = pDis->sysFuncWrite(&sysArgs);
        RETIF(err != TM_OK, TMBSL_ERR_HDMI_I2C_WRITE)
        pDis->curRegPage = newRegPage;
    }

    /* Get I2C register range - all non-OK results are errors */
    sysArgs.slaveAddr       = pDis->uHwAddress;
    sysArgs.firstRegister   = regAddr;
    sysArgs.lenData         = (UInt8)lenData;
    sysArgs.pData           = pData;
    err = pDis->sysFuncRead(&sysArgs);
    return (err == TM_OK) ? TM_OK : TMBSL_ERR_HDMI_I2C_READ;
}

/*============================================================================*/
/* getHwRegister                                                              */
/*============================================================================*/
tmErrorCode_t
getHwRegister
(
    tmHdmiTxobject_t *pDis, 
    UInt16  regShadPageAddr, 
    UInt8   *pRegValue
)
{
    tmErrorCode_t   err;        /* Error code */
    UInt8           regShad;    /* The index to the register's shadow copy */
    UInt8           regPage;    /* The index to the register's page        */
    UInt8           regAddr;    /* The register's address on the page      */
    UInt8           newRegPage; /* The register's new page number          */
    tmbslHdmiTxSysArgs_t sysArgs; /* Arguments passed to system function   */

    /* Unpack register shadow index, page index and address */
    regShad = SPA2SHAD(regShadPageAddr);
    regPage = SPA2PAGE(regShadPageAddr);
    regAddr = SPA2ADDR(regShadPageAddr);
    newRegPage = kPageIndexToPage[regPage];
    
    /* Set page register if required */
    if (pDis->curRegPage != newRegPage)
    {
        /* All non-OK results are errors */
        sysArgs.slaveAddr       = pDis->uHwAddress;
        sysArgs.firstRegister   = E_REG_CURPAGE_ADR_W;
        sysArgs.lenData         = 1;
        sysArgs.pData           = &newRegPage;
        err = pDis->sysFuncWrite(&sysArgs);
        RETIF(err != TM_OK, TMBSL_ERR_HDMI_I2C_WRITE)
        pDis->curRegPage = newRegPage;
    }

    if ((regShad != E_SNONE) 
    && (regShadPageAddr != E_REG_P00_INT_FLAGS_0_RW)
    && (regShadPageAddr != E_REG_P00_INT_FLAGS_1_RW)
#ifdef TMFL_TDA9981_SUPPORT
    && (regShadPageAddr != E_REG_P00_INT_FLAGS_2_RW)
#endif /* TMFL_TDA9981_SUPPORT */
    )
    {
        /* Get shadow copy - shadowed registers can't be read */
        /* Don't read shadow copy of interrupt status flags! */
        *pRegValue = pDis->shadowReg[regShad];
        return TM_OK;
    }    
    else
    {
        /* Get I2C register - all non-OK results are errors */
        sysArgs.slaveAddr       = pDis->uHwAddress;
        sysArgs.firstRegister   = regAddr;
        sysArgs.lenData         = 1;
        sysArgs.pData           = pRegValue;
        err = pDis->sysFuncRead(&sysArgs);
        return (err == TM_OK) ? TM_OK : TMBSL_ERR_HDMI_I2C_READ;
    }
}

/*============================================================================*/
/* setHwRegisters                                                             */
/*============================================================================*/
tmErrorCode_t
setHwRegisters
(
    tmHdmiTxobject_t *pDis, 
    UInt16 regShadPageAddr,
    UInt8 *pData, 
    UInt16 lenData
    )
{
    tmErrorCode_t   err;        /* Error code */
    UInt8           regShad;    /* The index to the register's shadow copy */
    UInt8           regPage;    /* The index to the register's page        */
    UInt8           regAddr;    /* The register's address on the page      */
    UInt8           newRegPage; /* The register's new page number          */
    tmbslHdmiTxSysArgs_t sysArgs; /* Arguments passed to system function   */

    /* Unpack 1st register's shadow index, page index and address */
    regShad = SPA2SHAD(regShadPageAddr);
    regPage = SPA2PAGE(regShadPageAddr);
    regAddr = SPA2ADDR(regShadPageAddr);
    newRegPage = kPageIndexToPage[regPage];
    
    /* Check length does not overflow page */
    RETIF_BADPARAM((regAddr+lenData) > E_REG_CURPAGE_ADR_W)

    /* Check 1st reg does not have a shadow - whole range assumed likewise */
    RETIF_BADPARAM(regShad != E_SNONE)

    /* Set page register if required - whole range is on same page */
    if (pDis->curRegPage != newRegPage)
    {
        /* All non-OK results are errors */
        sysArgs.slaveAddr       = pDis->uHwAddress;
        sysArgs.firstRegister   = E_REG_CURPAGE_ADR_W;
        sysArgs.lenData         = 1;
        sysArgs.pData           = &newRegPage;
        err = pDis->sysFuncWrite(&sysArgs);
        RETIF(err != TM_OK, TMBSL_ERR_HDMI_I2C_WRITE)
        pDis->curRegPage = newRegPage;
    }

    /* Write to I2C register range - all non-OK results are errors */
    sysArgs.slaveAddr       = pDis->uHwAddress;
    sysArgs.firstRegister   = regAddr;
    sysArgs.lenData         = (UInt8)lenData;
    sysArgs.pData           = pData;
    err = pDis->sysFuncWrite(&sysArgs);
    return (err == TM_OK) ? TM_OK : TMBSL_ERR_HDMI_I2C_WRITE;
}

/*============================================================================*/
/* setHwRegisterMsbLsb                                                        */
/*============================================================================*/
tmErrorCode_t
setHwRegisterMsbLsb
(
    tmHdmiTxobject_t *pDis, 
    UInt16  regShadPageAddr, 
    UInt16  regWord
)
{
    tmErrorCode_t   err;        /* Error code */
    UInt8           regPage;    /* The index to the register's page        */
    UInt8           regAddr;    /* The register's address on the page      */
    UInt8           newRegPage; /* The register's new page number          */
    UInt8           msbLsb[2];  /* The bytes from regWord                  */
    tmbslHdmiTxSysArgs_t sysArgs; /* Arguments passed to system function   */

    /* Unpack register shadow index, page index and address */
    regPage = SPA2PAGE(regShadPageAddr);
    regAddr = SPA2ADDR(regShadPageAddr);
    newRegPage = kPageIndexToPage[regPage];

    /* Unpack regWord bytes, MSB first */
    msbLsb[0] = (UInt8)(regWord >> 8);
    msbLsb[1] = (UInt8)(regWord & 0xFF);

    /* Set page register if required */
    if (pDis->curRegPage != newRegPage)
    {
        /* All non-OK results are errors */
        sysArgs.slaveAddr       = pDis->uHwAddress;
        sysArgs.firstRegister   = E_REG_CURPAGE_ADR_W;
        sysArgs.lenData         = 1;
        sysArgs.pData           = &newRegPage;
        err = pDis->sysFuncWrite(&sysArgs);
        RETIF(err != TM_OK, TMBSL_ERR_HDMI_I2C_WRITE)
        pDis->curRegPage = newRegPage;
    }

    /* No word registers are shadowed */
    
    /* Write to I2C - all non-OK results are errors */
    sysArgs.slaveAddr       = pDis->uHwAddress;
    sysArgs.firstRegister   = regAddr;
    sysArgs.lenData         = 2;
    sysArgs.pData           = &msbLsb[0];
    err = pDis->sysFuncWrite(&sysArgs);
    return (err == TM_OK) ? TM_OK : TMBSL_ERR_HDMI_I2C_WRITE;
}

/*============================================================================*/
/* setHwRegister                                                              */
/*============================================================================*/
tmErrorCode_t
setHwRegister
(
    tmHdmiTxobject_t *pDis, 
    UInt16  regShadPageAddr, 
    UInt8   regValue
)
{
    tmErrorCode_t   err;        /* Error code */
    UInt8           regShad;    /* The index to the register's shadow copy */
    UInt8           regPage;    /* The index to the register's page        */
    UInt8           regAddr;    /* The register's address on the page      */
    UInt8           newRegPage; /* The register's new page number          */
    tmbslHdmiTxSysArgs_t sysArgs; /* Arguments passed to system function   */

    /* Unpack register shadow index, page index and address */
    regShad = SPA2SHAD(regShadPageAddr);
    regPage = SPA2PAGE(regShadPageAddr);
    regAddr = SPA2ADDR(regShadPageAddr);
    newRegPage = kPageIndexToPage[regPage];
    
    /* Set page register if required */
    if (pDis->curRegPage != newRegPage)
    {
        /* All non-OK results are errors */
        sysArgs.slaveAddr       = pDis->uHwAddress;
        sysArgs.firstRegister   = E_REG_CURPAGE_ADR_W;
        sysArgs.lenData         = 1;
        sysArgs.pData           = &newRegPage;
        err = pDis->sysFuncWrite(&sysArgs);
        RETIF(err != TM_OK, TMBSL_ERR_HDMI_I2C_WRITE)
        pDis->curRegPage = newRegPage;
    }

    /* Set shadow copy */
    if (regShad != E_SNONE)
    {
        pDis->shadowReg[regShad] = regValue;
    }    
    
    /* Write to I2C - all non-OK results are errors */
    sysArgs.slaveAddr       = pDis->uHwAddress;
    sysArgs.firstRegister   = regAddr;
    sysArgs.lenData         = 1;
    sysArgs.pData           = &regValue;
    err = pDis->sysFuncWrite(&sysArgs);
    return (err == TM_OK) ? TM_OK : TMBSL_ERR_HDMI_I2C_WRITE;
}

/*============================================================================*/
/*  setHwRegisterField                                                        */
/*============================================================================*/
tmErrorCode_t
setHwRegisterField
(
    tmHdmiTxobject_t *pDis, 
    UInt16  regShadPageAddr, 
    UInt8   fieldMask, 
    UInt8   fieldValue
)
{
    tmErrorCode_t   err;        /* Error code */
    UInt8           regShad;    /* The index to the register's shadow copy */
    UInt8           regPage;    /* The index to the register's page        */
    UInt8           regAddr;    /* The register's address on the page      */
    UInt8           newRegPage; /* The register's new page number          */
    UInt8           regValue;   /* The register's current value            */
    tmbslHdmiTxSysArgs_t sysArgs; /* Arguments passed to system function   */

    /* Unpack register shadow index, page index and address */
    regShad = SPA2SHAD(regShadPageAddr);
    regPage = SPA2PAGE(regShadPageAddr);
    regAddr = SPA2ADDR(regShadPageAddr);
    newRegPage = kPageIndexToPage[regPage];
    
    /* Set page register if required */
    if (pDis->curRegPage != newRegPage)
    {
        /* All non-OK results are errors */
        sysArgs.slaveAddr       = pDis->uHwAddress;
        sysArgs.firstRegister   = E_REG_CURPAGE_ADR_W;
        sysArgs.lenData         = 1;
        sysArgs.pData           = &newRegPage;
        err = pDis->sysFuncWrite(&sysArgs);
        RETIF(err != TM_OK, TMBSL_ERR_HDMI_I2C_WRITE)
        pDis->curRegPage = newRegPage;
    }

    if (regShad != E_SNONE)
    {
        /* Get shadow copy */
        regValue = pDis->shadowReg[regShad];
    }
    else
    {
        /* Read I2C register value.
         * All bitfield registers are either shadowed or can be read.
         */
        sysArgs.slaveAddr       = pDis->uHwAddress;
        sysArgs.firstRegister   = regAddr;
        sysArgs.lenData         = 1;
        sysArgs.pData           = &regValue;
        err = pDis->sysFuncRead(&sysArgs);
        RETIF(err != TM_OK, TMBSL_ERR_HDMI_I2C_READ)
    }

    /* Reset register bits that are set in the mask */
    regValue = regValue & (UInt8)(~fieldMask);

    /* Shift the field value left to align its bits with the mask */
    fieldValue <<= kMaskToShift[fieldMask];

    /* Reset shifted field bits that are not set in the mask */
    fieldValue &= fieldMask;

    /* Set the shifted bitfield */
    regValue |= fieldValue;

    /* Set shadow copy */
    if (regShad != E_SNONE)
    {
        pDis->shadowReg[regShad] = regValue;
    }

    /* Write to I2C - all non-OK results are errors */
    sysArgs.slaveAddr       = pDis->uHwAddress;
    sysArgs.firstRegister   = regAddr;
    sysArgs.lenData         = 1;
    sysArgs.pData           = &regValue;
    err = pDis->sysFuncWrite(&sysArgs);
    return (err == TM_OK) ? TM_OK : TMBSL_ERR_HDMI_I2C_WRITE;
}

/*============================================================================*/
/* setHwRegisterFieldTable                                                    */
/*============================================================================*/
tmErrorCode_t
setHwRegisterFieldTable
(
    tmHdmiTxobject_t *pDis,
    const tmHdmiTxRegMaskVal_t *pTable
)
{
    tmErrorCode_t err; /* Error code */
    Int           i;   /* Table index */

    /* Set register, mask and value from table until terminator is reached */
    for (i = 0; pTable[i].Reg > 0; i++)
    {
        err = setHwRegisterField(pDis, pTable[i].Reg, pTable[i].Mask, pTable[i].Val);
        RETIF(err != TM_OK, err)
    }
    return TM_OK;
}

/*============================================================================*/
/* setState                                                                   */
/*============================================================================*/
tmErrorCode_t
setState
(
    tmHdmiTxobject_t   *pDis,
    tmbslHdmiTxEvent_t  event
)
{
    tmbslHdmiTxState_t state = pDis->state;
    UInt8 nIgnoredEvents = pDis->nIgnoredEvents;

    switch (state)
    {
    case ST_UNINITIALIZED:
        switch (event)
        {
        case EV_UNPLUGGED:          state = ST_DISCONNECTED;      break;
        case EV_PLUGGEDIN:          state = ST_AWAIT_EDID;        break;
        default:                    nIgnoredEvents++;             break;
        }                                                               
        break;                                                          
    case ST_DISCONNECTED:                                               
        switch (event)                                                  
        {                                                               
        case EV_DEINIT:             state = ST_UNINITIALIZED;     break;
        case EV_PLUGGEDIN:          state = ST_AWAIT_EDID;        break;
        case EV_STANDBY:            state = ST_STANDBY;           break;
        default:                    nIgnoredEvents++;             break;
        }                                                               
        break;                                                          
    case ST_AWAIT_EDID:                                                 
        switch (event)                                                  
        {                                                               
        case EV_DEINIT:             state = ST_UNINITIALIZED;     break;
        case EV_UNPLUGGED:          state = ST_DISCONNECTED;      break;
        case EV_STANDBY:            state = ST_STANDBY;           break;
#if defined (TMFL_TDA9981_SUPPORT) && defined(TMFL_RX_SENSE_ON)
		case EV_GETBLOCKDATA:       state = ST_AWAIT_RX_SENSE;	  break;
#else /* TMFL_TDA9981_SUPPORT & TMFL_RX_SENSE_ON */
        case EV_GETBLOCKDATA:       state = ST_SINK_CONNECTED;    break;
#endif /* TMFL_TDA9981_SUPPORT & TMFL_RX_SENSE_ON */
        default:                    nIgnoredEvents++;             break;
        }
        break;
#if defined (TMFL_TDA9981_SUPPORT) && defined(TMFL_RX_SENSE_ON)
   case ST_AWAIT_RX_SENSE:                                                 
        switch (event)                                                  
        {                                                               
        case EV_DEINIT:             state = ST_UNINITIALIZED;     break;
        case EV_UNPLUGGED:          state = ST_DISCONNECTED;      break;
        case EV_STANDBY:            state = ST_STANDBY;           break;
        case EV_SINKON:             state = ST_SINK_CONNECTED;    break;
        default:                    nIgnoredEvents++;             break;
        }
        break;
#endif /* TMFL_TDA9981_SUPPORT & TMFL_RX_SENSE_ON */
  case ST_SINK_CONNECTED:
        switch (event)
        {
        case EV_DEINIT:             state = ST_UNINITIALIZED;     break;
        case EV_UNPLUGGED:          state = ST_DISCONNECTED;      break;
        case EV_STANDBY:            state = ST_STANDBY;           break;
        case EV_SETINOUT:           state = ST_VIDEO_NO_HDCP;     break;
#if defined (TMFL_TDA9981_SUPPORT) && defined(TMFL_RX_SENSE_ON)
        case EV_SINKOFF:            state = ST_AWAIT_RX_SENSE;    break;
#endif /* TMFL_TDA9981_SUPPORT & TMFL_RX_SENSE_ON */
        default:                    nIgnoredEvents++;             break;
        }                                                               
        break;                                                          
    case ST_VIDEO_NO_HDCP:                                              
        switch (event)                                                  
        {                                                               
        case EV_DEINIT:             state = ST_UNINITIALIZED;     break;
        case EV_UNPLUGGED:          state = ST_DISCONNECTED;      break;
        case EV_STANDBY:            state = ST_STANDBY;           break;
        case EV_OUTDISABLE:         state = ST_SINK_CONNECTED;    break;
        case EV_HDCP_RUN:           state = ST_HDCP_WAIT_RX;      break;
#if defined (TMFL_TDA9981_SUPPORT) && defined(TMFL_RX_SENSE_ON)
        case EV_SINKOFF:            state = ST_AWAIT_RX_SENSE;    break;
#endif /* TMFL_TDA9981_SUPPORT & TMFL_RX_SENSE_ON */
        default:                    nIgnoredEvents++;             break;
        }                                                               
        break;                                                          
    case ST_HDCP_WAIT_RX:                                               
        switch (event)                                                  
        {                                                               
        case EV_DEINIT:             state = ST_UNINITIALIZED;     break;
        case EV_UNPLUGGED:          state = ST_DISCONNECTED;      break;
        case EV_STANDBY:            state = ST_STANDBY;           break;
        case EV_HDCP_BKSV_NREPEAT:  state = ST_HDCP_AUTHENTICATED;break;
        case EV_HDCP_BKSV_REPEAT:   state = ST_HDCP_WAIT_BSTATUS; break;
        case EV_HDCP_BKSV_NSECURE:  state = ST_HDCP_WAIT_RX;      break;
        case EV_HDCP_T0:            state = ST_HDCP_WAIT_RX;      break;
        case EV_HDCP_STOP:          state = ST_VIDEO_NO_HDCP;     break;
#if defined (TMFL_TDA9981_SUPPORT) && defined(TMFL_RX_SENSE_ON)
        case EV_SINKOFF:            state = ST_AWAIT_RX_SENSE;    break;
#endif /* TMFL_TDA9981_SUPPORT & TMFL_RX_SENSE_ON */
        default:                    nIgnoredEvents++;             break;
        }
        break;
    case ST_HDCP_WAIT_BSTATUS:
        switch (event)
        {
        case EV_DEINIT:             state = ST_UNINITIALIZED;     break;
        case EV_UNPLUGGED:          state = ST_DISCONNECTED;      break;
        case EV_STANDBY:            state = ST_STANDBY;           break;
        case EV_HDCP_BSTATUS_GOOD:  state = ST_HDCP_WAIT_SHA_1;   break;
        case EV_HDCP_T0:            state = ST_HDCP_WAIT_RX;      break;
        case EV_HDCP_STOP:          state = ST_VIDEO_NO_HDCP;     break;
#if defined (TMFL_TDA9981_SUPPORT) && defined(TMFL_RX_SENSE_ON)
        case EV_SINKOFF:            state = ST_AWAIT_RX_SENSE;    break;
#endif /* TMFL_TDA9981_SUPPORT & TMFL_RX_SENSE_ON */
        default:                    nIgnoredEvents++;             break;
        }
        break;
    case ST_HDCP_WAIT_SHA_1:
        switch (event)
        {
        case EV_DEINIT:             state = ST_UNINITIALIZED;     break;
        case EV_UNPLUGGED:          state = ST_DISCONNECTED;      break;
        case EV_STANDBY:            state = ST_STANDBY;           break;
        case EV_HDCP_KSV_SECURE:    state = ST_HDCP_AUTHENTICATED;break;
        case EV_HDCP_T0:            state = ST_HDCP_WAIT_RX;      break;
        case EV_HDCP_STOP:          state = ST_VIDEO_NO_HDCP;     break;
#if defined (TMFL_TDA9981_SUPPORT) && defined(TMFL_RX_SENSE_ON) 
        case EV_SINKOFF:            state = ST_AWAIT_RX_SENSE;    break;
#endif /* TMFL_TDA9981_SUPPORT & TMFL_RX_SENSE_ON */
        default:                    nIgnoredEvents++;             break;
        }
        break;
    case ST_HDCP_AUTHENTICATED:
        switch (event)
        {
        case EV_DEINIT:             state = ST_UNINITIALIZED;     break;
        case EV_UNPLUGGED:          state = ST_DISCONNECTED;      break;
        case EV_STANDBY:            state = ST_STANDBY;           break;
        case EV_HDCP_T0:            state = ST_HDCP_WAIT_RX;      break;
        case EV_HDCP_STOP:          state = ST_VIDEO_NO_HDCP;     break;
#if defined (TMFL_TDA9981_SUPPORT) && defined(TMFL_RX_SENSE_ON)
        case EV_SINKOFF:            state = ST_AWAIT_RX_SENSE;    break;
#endif /* TMFL_TDA9981_SUPPORT & TMFL_RX_SENSE_ON */
        default:                    nIgnoredEvents++;             break;
        }
        break;
    case ST_STANDBY:
        switch (event)
        {
        case EV_DEINIT:             state = ST_UNINITIALIZED;     break;
        case EV_RESUME_UNPLUGGED:   state = ST_DISCONNECTED;      break;
        case EV_RESUME_PLUGGEDIN:   state = ST_AWAIT_EDID;        break;
        default:                    nIgnoredEvents++;             break;
        }
        break;
    case ST_INVALID:
        nIgnoredEvents++;
        break;
    }
    pDis->state = state;
    pDis->nIgnoredEvents = nIgnoredEvents;
    return TM_OK;
}

/*============================================================================*/
/* lmemcpy                                                                     */
/*============================================================================*/
tmErrorCode_t
lmemcpy
(
    void *pTable1,
    const void * pTable2,
    UInt Size
)
{
    char *ptrSource = (char*) pTable1;
    char *endSource = (char*)pTable1 + Size;
    char *ptrDest = (char *)pTable2;

    RETIF_BADPARAM(pTable1 == Null)
    RETIF_BADPARAM(pTable2 == Null)

    while (endSource > ptrSource)
    {
        *(ptrSource++) = *(ptrDest++);
    }
    return TM_OK;
}

/*============================================================================*/
/* lmemset                                                                     */
/*============================================================================*/
tmErrorCode_t
lmemset
(
    void *pTable1,
    const UInt8 Value,
    UInt Size
)
{
    char *ptrSource = (char*) pTable1;
    char *endSource = (char*)pTable1 + Size;

    RETIF_BADPARAM(pTable1 == Null)

    while (endSource > ptrSource)
    {
        *(ptrSource++) = Value;
    }
    return TM_OK;
}

/*============================================================================*/
/*                   STATIC FUNCTION DEFINTIONS                               */
/*============================================================================*/

/*============================================================================*/
/*                     END OF FILE                                            */
/*============================================================================*/
