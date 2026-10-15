/**********************************************************************************************************************************************************************************/
/*                                                                                                                                                                                */
/*  (c) Copyright 1990-2023  Electronics for Imaging, Inc. ("EFI").     All Rights Reserved                                                                                       */
/*                                                                                                                                                                                */
/*  EFI products contain certain trade secrets and confidential and proprietary information of EFI.  Use, reproduction, disclosure, distribution by any means  are prohibited,    */
/*  except pursuant to a written license from EFI. Modification, translation, reverse engineering, decompiling, disassembling, and creating derivative works based on this        */
/*  software are prohibited, except pursuant to a written license from EFI. Use of copyright notice does not imply publication or disclosure.                                     */
/*                                                                                                                                                                                */
/*  Restricted Rights Legend:                                                                                                                                                     */
/*  Use, duplication, or disclosure by the Government is subject to restrictions as set forth in subparagraph (c)(1)(ii) of The Rights in Technical Data and Computer Software    */
/*  clause in DFARS 252.227-7013 or subparagraphs (c)(1) and (2) of the Commercial Computer Software -- Restricted Rights at 48 CFR 52.227-19, as applicable.                     */
/*                                                                                                                                                                                */
/**********************************************************************************************************************************************************************************/

#ifndef __EFI_FIERY_IMAGE_TRANSFER_SERVICE_IFC__
#define __EFI_FIERY_IMAGE_TRANSFER_SERVICE_IFC__

#include <cstdint>

#define FITS_HEADER_LENGTH 4    // Length in bytes of initial FITS header

// This struct defines a size member which will contain the actual size of the header sent (COMMON_HEADER). This is defined in fieryImageTrasferServiceIfc.proto
typedef struct
{
    uint8_t size[FITS_HEADER_LENGTH];
} FITS_HEADER_SIZE;

#endif