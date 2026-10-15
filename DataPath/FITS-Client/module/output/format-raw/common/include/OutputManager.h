/**********************************************************************************************************************************************************************************/
/*                                                                                                                                                                                */
/*  Copyright (c) 1990-2023 Fiery, LLC.  All Rights Reserved.                                                                                                                     */
/*                                                                                                                                                                                */
/*  Fiery, LLC ("Fiery") products contain certain trade secrets and confidential and proprietary information of Fiery.  Use, reproduction, disclosure, distribution by any means  */
/*  are prohibited, except pursuant to a written license from Fiery.  Modification, translation, reverse engineering, decompiling, disassembling, and creating derivative works   */
/*  based on this software are prohibited, except pursuant to a written license from Fiery. Use of copyright notice does not imply publication or disclosure.                     */
/*                                                                                                                                                                                */
/**********************************************************************************************************************************************************************************/
#ifndef __OUTPUT_MANAGER_H__
#define __OUTPUT_MANAGER_H__


/**********************************************************************************************************************************************************************************/
/*                                                                                    includes                                                                                    */
/**********************************************************************************************************************************************************************************/
#include "FITS-OutputApi.h"

#include <boost/filesystem.hpp>

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
namespace FIERY
{
    namespace FITS
    {
        namespace OUTPUT
        {
            // class declarations.
            class MANAGER
            {
                public:
                // public constructors and destructor.
                MANAGER();
                virtual ~MANAGER();

                // public "management" methods.
                bool Initialize(const FIERY::FITS::MODULE::LOGGER & logger, FIERY::FITS::OUTPUT::PROPERTIES::OUTPUT & output_properties);

                // public IMAGE "management" methods.
                bool ImageProcess(const FIERY::FITS::MODULE::PROPERTIES::IMAGE & image_properties, const std::vector<uint8_t> & image_data_input, std::vector<uint8_t> & image_data_output);
                bool ImageTransfer(const FIERY::FITS::MODULE::IMAGE::IMAGE & image, const std::string & job_name);
                bool ImageTransferAccumulated(const std::list<FIERY::FITS::MODULE::IMAGE::IMAGE> & image_list, const std::string & job_name);
                bool ImageWaitForEjected(const std::string & job_id, const std::string & image_id);


                private:
                // private "helper" methods.
                bool configurationGet(FIERY::FITS::OUTPUT::PROPERTIES::OUTPUT & output_properties);
                bool filenameGet(const FIERY::FITS::MODULE::IMAGE::IMAGE & image, const std::string & job_name, std::string & file_name);
                bool fileWrite(const FIERY::FITS::MODULE::IMAGE::IMAGE & image, const std::string & file_name);

                // private "management" variables.
                std::string               m_directory;
                bool                      m_file_enabled;
                boost::filesystem::path   m_file_path;
                bool                      m_job_name_enabled;
                bool                      m_page_number_enabled;
                bool                      m_property_colorant_accumulator_enabled;
                bool                      m_property_image_data_processor_enabled;
                bool                      m_property_wait_for_ejected_enabled;

                // private static "management" constants.
                static const std::string   DIRECTORY_RASTER;
            };
        }
    }
}
#endif   // __OUTPUT_MANAGER_H__
