/**********************************************************************************************************************************************************************************/
/*                                                                                                                                                                                */
/*  Copyright (c) 1990-2023 Fiery, LLC.  All Rights Reserved.                                                                                                                     */
/*                                                                                                                                                                                */
/*  Fiery, LLC ("Fiery") products contain certain trade secrets and confidential and proprietary information of Fiery.  Use, reproduction, disclosure, distribution by any means  */
/*  are prohibited, except pursuant to a written license from Fiery.  Modification, translation, reverse engineering, decompiling, disassembling, and creating derivative works   */
/*  based on this software are prohibited, except pursuant to a written license from Fiery. Use of copyright notice does not imply publication or disclosure.                     */
/*                                                                                                                                                                                */
/**********************************************************************************************************************************************************************************/
#include "FITS-Module.h"

#include "ModuleManager.h"


/**********************************************************************************************************************************************************************************/
/*                                                                                local variables                                                                                 */
/**********************************************************************************************************************************************************************************/
FIERY::FITS::MODULE::MANAGER   module_mgr;


/**********************************************************************************************************************************************************************************/
/*                                                                                     methods                                                                                    */
/**********************************************************************************************************************************************************************************/
FITS_MODULE_PLUG_IN::FITS_MODULE_PLUG_IN()
{
    ;
}

FITS_MODULE_PLUG_IN::~FITS_MODULE_PLUG_IN()
{
    ;
}

bool FITS_MODULE_PLUG_IN::Initialize(const FIERY::FITS::MODULE::LOGGER & logger)
{
    return(module_mgr.Initialize(logger));
}

bool FITS_MODULE_PLUG_IN::Start(const std::list<std::string> & colorant_list)
{
    return(module_mgr.Start(colorant_list));
}

bool FITS_MODULE_PLUG_IN::Stop(void)
{
    return(module_mgr.Stop());
}

bool FITS_MODULE_PLUG_IN::ImageSave(const FIERY::FITS::MODULE::PROPERTIES::IMAGE & image_properties, const std::vector<uint8_t> & image_data)
{
    return(module_mgr.ImageSave(image_properties, image_data));
}
