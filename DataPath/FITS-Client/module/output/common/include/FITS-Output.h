/**********************************************************************************************************************************************************************************/
/*                                                                                                                                                                                */
/*  Copyright (c) 1990-2023 Fiery, LLC.  All Rights Reserved.                                                                                                                     */
/*                                                                                                                                                                                */
/*  Fiery, LLC ("Fiery") products contain certain trade secrets and confidential and proprietary information of Fiery.  Use, reproduction, disclosure, distribution by any means  */
/*  are prohibited, except pursuant to a written license from Fiery.  Modification, translation, reverse engineering, decompiling, disassembling, and creating derivative works   */
/*  based on this software are prohibited, except pursuant to a written license from Fiery. Use of copyright notice does not imply publication or disclosure.                     */
/*                                                                                                                                                                                */
/**********************************************************************************************************************************************************************************/
#ifndef __FITS_OUTPUT_H__
#define __FITS_OUTPUT_H__


/**********************************************************************************************************************************************************************************/
/*                                                                                    includes                                                                                    */
/**********************************************************************************************************************************************************************************/
#include "FITS-OutputApi.h"

#include <boost/config.hpp>


/**********************************************************************************************************************************************************************************/
/*                                                                              compile-time defines                                                                              */
/**********************************************************************************************************************************************************************************/


/**********************************************************************************************************************************************************************************/
/*                                                            typedefs, structures, enumerations, and related defines                                                             */
/**********************************************************************************************************************************************************************************/
class FITS_OUTPUT_PLUG_IN : public FIERY::FITS::OUTPUT::PLUG_IN
{
    public:
    // public constructors and destructor.
    FITS_OUTPUT_PLUG_IN();
    virtual ~FITS_OUTPUT_PLUG_IN();

    // public "management" methods.
    bool Initialize(const FIERY::FITS::MODULE::LOGGER & logger, FIERY::FITS::OUTPUT::PROPERTIES::OUTPUT & output_properties);

    // public IMAGE "management" methods.
    bool ImageProcess(const FIERY::FITS::MODULE::PROPERTIES::IMAGE & image_properties, const std::vector<uint8_t> & image_data_input, std::vector<uint8_t> & image_data_output);
    bool ImageTransfer(const FIERY::FITS::MODULE::IMAGE::IMAGE & image, const std::string & job_name);
    bool ImageTransferAccumulated(const std::list<FIERY::FITS::MODULE::IMAGE::IMAGE> & image_list, const std::string & job_name);
    bool ImageWaitForEjected(const std::string & job_id, const std::string & image_id);
};

EXTERN_C BOOST_SYMBOL_EXPORT FITS_OUTPUT_PLUG_IN   FITS_Output;
FITS_OUTPUT_PLUG_IN   FITS_Output;
#endif   // __FITS_OUTPUT_H__
