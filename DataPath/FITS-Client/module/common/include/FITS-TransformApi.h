/**********************************************************************************************************************************************************************************/
/*                                                                                                                                                                                */
/*  Copyright (c) 1990-2026 Fiery, LLC.  All Rights Reserved.                                                                                                                     */
/*                                                                                                                                                                                */
/*  Fiery, LLC ("Fiery") products contain certain trade secrets and confidential and proprietary information of Fiery.  Use, reproduction, disclosure, distribution by any means  */
/*  are prohibited, except pursuant to a written license from Fiery.  Modification, translation, reverse engineering, decompiling, disassembling, and creating derivative works   */
/*  based on this software are prohibited, except pursuant to a written license from Fiery. Use of copyright notice does not imply publication or disclosure.                     */
/*                                                                                                                                                                                */
/**********************************************************************************************************************************************************************************/
#ifndef __FITS_TRANSFORM_API_H__
#define __FITS_TRANSFORM_API_H__


/**********************************************************************************************************************************************************************************/
/*                                                                                    includes                                                                                    */
/**********************************************************************************************************************************************************************************/
#include "FITS-ModuleApi.h"

#include "ImageManager.h"

#include <boost/property_tree/ptree.hpp>

#include <cstdint>


/**********************************************************************************************************************************************************************************/
/*                                                                              compile-time defines                                                                              */
/**********************************************************************************************************************************************************************************/
#ifdef __cplusplus
#define EXTERN_C extern "C" 
#else
#define EXTERN_C extern
#endif //__cplusplus

#define FITS_TRANSFORM_PLUG_IN_SYMBOLS   "FITS_Transform"


/**********************************************************************************************************************************************************************************/
/*                                                            typedefs, structures, enumerations, and related defines                                                             */
/**********************************************************************************************************************************************************************************/
namespace FIERY
{
    namespace FITS
    {
        namespace TRANSFORM
        {
            class PLUG_IN
            {
                public:
                // public pure virtual "management" methods.
                virtual bool Initialize(const FIERY::FITS::MODULE::LOGGER & logger) = 0;

                // public pure virtual IMAGE "management" methods.
                virtual bool ImageTransform(const FIERY::FITS::MODULE::PROPERTIES::IMAGE & image_properties, const std::vector<uint8_t> & image_data_input, std::vector<uint8_t> & image_data_output) = 0;
            };
        }
    }
}
#endif   // __FITS_TRANSFORM_API_H__