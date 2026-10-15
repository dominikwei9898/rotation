/**********************************************************************************************************************************************************************************/
/*                                                                                                                                                                                */
/*  Copyright (c) 1990-2023 Fiery, LLC.  All Rights Reserved.                                                                                                                     */
/*                                                                                                                                                                                */
/*  Fiery, LLC ("Fiery") products contain certain trade secrets and confidential and proprietary information of Fiery.  Use, reproduction, disclosure, distribution by any means  */
/*  are prohibited, except pursuant to a written license from Fiery.  Modification, translation, reverse engineering, decompiling, disassembling, and creating derivative works   */
/*  based on this software are prohibited, except pursuant to a written license from Fiery. Use of copyright notice does not imply publication or disclosure.                     */
/*                                                                                                                                                                                */
/**********************************************************************************************************************************************************************************/
#ifndef __FITS_OUTPUT_API_H__
#define __FITS_OUTPUT_API_H__


/**********************************************************************************************************************************************************************************/
/*                                                                                    includes                                                                                    */
/**********************************************************************************************************************************************************************************/
#include "FITS-ModuleApi.h"

#include "ImageManager.h"

#include <cstdint>


/**********************************************************************************************************************************************************************************/
/*                                                                              compile-time defines                                                                              */
/**********************************************************************************************************************************************************************************/
#ifdef __cplusplus
#define EXTERN_C extern "C" 
#else
#define EXTERN_C extern
#endif //__cplusplus

#define FITS_OUTPUT_PLUG_IN_SYMBOLS   "FITS_Output"


/**********************************************************************************************************************************************************************************/
/*                                                            typedefs, structures, enumerations, and related defines                                                             */
/**********************************************************************************************************************************************************************************/
namespace FIERY
{
    namespace FITS
    {
        namespace OUTPUT
        {
            namespace PROPERTIES
            {
                // class declarations.
                class OUTPUT
                {
                    public:
                    // public "management" variables.
                    bool   colorant_accumulator_enabled;
                    bool   image_data_processor_enabled;
                    bool   wait_for_ejected_enabled;


                    // public constructors and destructor.
                    OUTPUT
                    (
                        const bool &   colorant_accumulator_enabled,
                        const bool &   image_data_processor_enabled,
                        const bool &   wait_for_ejected_enabled
                    ) : colorant_accumulator_enabled(colorant_accumulator_enabled), image_data_processor_enabled(image_data_processor_enabled),
                                wait_for_ejected_enabled(wait_for_ejected_enabled)
                    {
                        ;
                    };

                    OUTPUT()
                    {
                        clear();
                    };

                    OUTPUT
                    (
                        const OUTPUT &   output
                    ) : colorant_accumulator_enabled(output.colorant_accumulator_enabled), image_data_processor_enabled(output.image_data_processor_enabled),
                                wait_for_ejected_enabled(output.wait_for_ejected_enabled)
                    {
                        ;
                    };

                    virtual ~OUTPUT()
                    {
                        ;
                    };


                    // public "helper" methods.
                    void clear(void)
                    {
                        colorant_accumulator_enabled = false;
                        image_data_processor_enabled = false;
                        wait_for_ejected_enabled     = false;
                    };
                };
            }

            class PLUG_IN
            {
                public:
                // public pure virtual "management" methods.
                virtual bool Initialize(const FIERY::FITS::MODULE::LOGGER & logger, FIERY::FITS::OUTPUT::PROPERTIES::OUTPUT & output_properties) = 0;

                // public pure virtual IMAGE "management" methods.
                virtual bool ImageProcess(const FIERY::FITS::MODULE::PROPERTIES::IMAGE & image_properties, const std::vector<uint8_t> & image_data_input, std::vector<uint8_t> & image_data_output) = 0;
                virtual bool ImageTransfer(const FIERY::FITS::MODULE::IMAGE::IMAGE & image, const std::string & job_name) = 0;
                virtual bool ImageTransferAccumulated(const std::list<FIERY::FITS::MODULE::IMAGE::IMAGE> & image_list, const std::string & job_name) = 0;
                virtual bool ImageWaitForEjected(const std::string & job_id, const std::string & image_id) = 0;
            };
        }
    }
}
#endif   // __FITS_OUTPUT_API_H__