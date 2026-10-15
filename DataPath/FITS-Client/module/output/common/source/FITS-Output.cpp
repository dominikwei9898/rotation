/**********************************************************************************************************************************************************************************/
/*                                                                                                                                                                                */
/*  Copyright (c) 1990-2023 Fiery, LLC.  All Rights Reserved.                                                                                                                     */
/*                                                                                                                                                                                */
/*  Fiery, LLC ("Fiery") products contain certain trade secrets and confidential and proprietary information of Fiery.  Use, reproduction, disclosure, distribution by any means  */
/*  are prohibited, except pursuant to a written license from Fiery.  Modification, translation, reverse engineering, decompiling, disassembling, and creating derivative works   */
/*  based on this software are prohibited, except pursuant to a written license from Fiery. Use of copyright notice does not imply publication or disclosure.                     */
/*                                                                                                                                                                                */
/**********************************************************************************************************************************************************************************/
#include "FITS-Output.h"

#include "OutputManager.h"


/**********************************************************************************************************************************************************************************/
/*                                                                                local variables                                                                                 */
/**********************************************************************************************************************************************************************************/
FIERY::FITS::OUTPUT::MANAGER   output_mgr;


/**********************************************************************************************************************************************************************************/
/*                                                                                     methods                                                                                    */
/**********************************************************************************************************************************************************************************/
FITS_OUTPUT_PLUG_IN::FITS_OUTPUT_PLUG_IN()
{
    ;
}

FITS_OUTPUT_PLUG_IN::~FITS_OUTPUT_PLUG_IN()
{
    ;
}

bool FITS_OUTPUT_PLUG_IN::Initialize(const FIERY::FITS::MODULE::LOGGER & logger, FIERY::FITS::OUTPUT::PROPERTIES::OUTPUT & output_properties)
{
    return(output_mgr.Initialize(logger, output_properties));
}

bool FITS_OUTPUT_PLUG_IN::ImageProcess(const FIERY::FITS::MODULE::PROPERTIES::IMAGE & image_properties, const std::vector<uint8_t> & image_data_input, std::vector<uint8_t> & image_data_output)
{
    return(output_mgr.ImageProcess(image_properties, image_data_input, image_data_output));
}

bool FITS_OUTPUT_PLUG_IN::ImageTransfer(const FIERY::FITS::MODULE::IMAGE::IMAGE & image, const std::string & job_name)
{
    return(output_mgr.ImageTransfer(image, job_name));
}

bool FITS_OUTPUT_PLUG_IN::ImageTransferAccumulated(const std::list<FIERY::FITS::MODULE::IMAGE::IMAGE> & image_list, const std::string & job_name)
{
    return(output_mgr.ImageTransferAccumulated(image_list, job_name));
}

bool FITS_OUTPUT_PLUG_IN::ImageWaitForEjected(const std::string & job_id, const std::string & image_id)
{
    return(output_mgr.ImageWaitForEjected(job_id, image_id));
}
