/**********************************************************************************************************************************************************************************/
/*                                                                                                                                                                                */
/*  Copyright (c) 1990-2026 Fiery, LLC.  All Rights Reserved.                                                                                                                     */
/*                                                                                                                                                                                */
/*  Fiery, LLC ("Fiery") products contain certain trade secrets and confidential and proprietary information of Fiery.  Use, reproduction, disclosure, distribution by any means  */
/*  are prohibited, except pursuant to a written license from Fiery.  Modification, translation, reverse engineering, decompiling, disassembling, and creating derivative works   */
/*  based on this software are prohibited, except pursuant to a written license from Fiery. Use of copyright notice does not imply publication or disclosure.                     */
/*                                                                                                                                                                                */
/**********************************************************************************************************************************************************************************/
#ifndef __FITS_MODULE_H__
#define __FITS_MODULE_H__


/**********************************************************************************************************************************************************************************/
/*                                                                                    includes                                                                                    */
/**********************************************************************************************************************************************************************************/
#include "FITS-ModuleApi.h"

#include <boost/config.hpp>

#include <cstdint>
#include <list>
#include <string>
#include <vector>


/**********************************************************************************************************************************************************************************/
/*                                                                              compile-time defines                                                                              */
/**********************************************************************************************************************************************************************************/


/**********************************************************************************************************************************************************************************/
/*                                                            typedefs, structures, enumerations, and related defines                                                             */
/**********************************************************************************************************************************************************************************/
class FITS_MODULE_PLUG_IN : public FIERY::FITS::MODULE::PLUG_IN
{
    public:
    // public constructors and destructor.
    FITS_MODULE_PLUG_IN();
    virtual ~FITS_MODULE_PLUG_IN();

    // public "management" methods.
    bool Initialize(const FIERY::FITS::MODULE::LOGGER & logger, const std::string & ip_fiery_path_client_override = "");

    // public CONNECTION "management" methods.
    bool Start(const std::list<std::string> & colorant_list);
    bool Stop(void);

    // public COLORANT "management" methods.
    bool ImageSave(const FIERY::FITS::MODULE::PROPERTIES::IMAGE & image_properties, const std::vector<uint8_t> & image_data);
};

EXTERN_C BOOST_SYMBOL_EXPORT FITS_MODULE_PLUG_IN   FITS_Module;
FITS_MODULE_PLUG_IN   FITS_Module;
#endif   // __FITS_MODULE_H__
