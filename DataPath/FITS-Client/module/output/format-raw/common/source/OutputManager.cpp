/**********************************************************************************************************************************************************************************/
/*                                                                                                                                                                                */
/*  Copyright (c) 1990-2023 Fiery, LLC.  All Rights Reserved.                                                                                                                     */
/*                                                                                                                                                                                */
/*  Fiery, LLC ("Fiery") products contain certain trade secrets and confidential and proprietary information of Fiery.  Use, reproduction, disclosure, distribution by any means  */
/*  are prohibited, except pursuant to a written license from Fiery.  Modification, translation, reverse engineering, decompiling, disassembling, and creating derivative works   */
/*  based on this software are prohibited, except pursuant to a written license from Fiery. Use of copyright notice does not imply publication or disclosure.                     */
/*                                                                                                                                                                                */
/**********************************************************************************************************************************************************************************/
#include "OutputManager.h"

#include "LogManager.h"

#include <codecvt>

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/json.hpp>
#include <boost/optional.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

using namespace FIERY::FITS::OUTPUT;


/**********************************************************************************************************************************************************************************/
/*                                                                                local variables                                                                                 */
/**********************************************************************************************************************************************************************************/
thread_local std::string   job_id_current;

thread_local uint32_t      image_number = 0;

const std::string          MANAGER::DIRECTORY_RASTER = "raster";


/**********************************************************************************************************************************************************************************/
/*                                                                                     methods                                                                                    */
/**********************************************************************************************************************************************************************************/
MANAGER::MANAGER() : m_directory(), m_file_enabled(false), m_file_path(), m_job_name_enabled(false), m_page_number_enabled(false), m_property_colorant_accumulator_enabled(false),
            m_property_image_data_processor_enabled(false), m_property_wait_for_ejected_enabled(false)
{
    ;
}

MANAGER::~MANAGER()
{
    ;
}

bool MANAGER::ImageProcess(const FIERY::FITS::MODULE::PROPERTIES::IMAGE & image_properties, const std::vector<uint8_t> & image_data_input, std::vector<uint8_t> & image_data_output)
{
    bool   successful = false;


    FIERY::FITS::MODULE::LOG::MANAGER::LogError("FITS-Output-Raw[" + image_properties.colorant + "]: ImageProcess FAILED (JobId = " + image_properties.job_id +
                ", ImageId = " + image_properties.image_id + ", ImageDataProcessor = " + std::to_string(m_property_image_data_processor_enabled) + ")");

    // return the status.
    return(successful);
}

bool MANAGER::ImageTransfer(const FIERY::FITS::MODULE::IMAGE::IMAGE & image, const std::string & job_name)
{
    bool   successful = true;


    // check to see if we need to "write" the IMAGE to "file".
    if (m_file_enabled)
    {
        std::string   file_name;


        // if so, check to see if we need to update the "management" variables -- if so, do it.
        if (job_id_current != image.job_id)
        {
            image_number   = 0;
            job_id_current = image.job_id;
        }

        // update the "management" variables.
        image_number++;

        // attempt to "get" the "filename".
        filenameGet(image, job_name, file_name);

        // attempt to "write" the IMAGE "data".
        successful = fileWrite(image, file_name);
    }
    else
    {
        /*************************************************************************************************************************************************/
        /*                                                               INSERT CODE HERE                                                                */
        /*                                                                                                                                               */
        /*                                               PURPOSE: TRANSFER RAW IMAGE TO PRESS ELECTRONICS.                                               */
        /*                                                                                                                                               */
        /*************************************************************************************************************************************************/
        // attempt to "transfer" the IMAGE "data" -- since we need to know when this IMAGE is "ejected", we need to associate the IMAGE "ID" with the IMAGE "data" being
        // "transferred".
        ; // e.g. -> successful = pressElectronicsTransfer(image.image_id, image.p_buffer, image.buffer_size_in_bytes);
    }

    // return the status.
    return(successful);
}

bool MANAGER::ImageTransferAccumulated(const std::list<FIERY::FITS::MODULE::IMAGE::IMAGE> & image_list, const std::string & job_name)
{
    std::string   image_id   = (!image_list.empty() ? image_list.front().image_id : "UNKNOWN");
    std::string   job_id     = (!image_list.empty() ? image_list.front().job_id : "UNKNOWN");
    bool          successful = true;


    // check to see if "colorant accumulator" is "enabled".
    if (!m_property_colorant_accumulator_enabled)
    {
        // if not, update the status.
        successful = false;
    }
    else
    {
        // check to see if we need to "write" the IMAGE to "file".
        if (m_file_enabled)
        {
            // otherwise, check to see if we need to update the "management" variables -- if so, do it.
            if (job_id_current != job_id)
            {
                image_number   = 0;
                job_id_current = job_id;
            }

            // update the "management" variables.
            image_number++;

            // update the status.
            successful = true;

            // loop through the IMAGEs -- attempting to "process" each IMAGE.
            for (auto & entry_image : image_list)
            {
                std::string   file_name;


                // attempt to "get" the "filename".
                filenameGet(entry_image, job_name, file_name);

                // attempt to "write" the IMAGE "data" -- if unsuccessful, update the status.
                if (!fileWrite(entry_image, file_name))
                {
                    successful = false;
                }
            }
        }
        else
        {
            /*************************************************************************************************************************************************/
            /*                                                               INSERT CODE HERE                                                                */
            /*                                                                                                                                               */
            /*                                        PURPOSE: TRANSFER RAW IMAGE (ACCUMULATED) TO PRESS ELECTRONICS.                                        */
            /*                                                                                                                                               */
            /*************************************************************************************************************************************************/
            // loop through the IMAGEs -- attempting to "transfer" each COLORANT.
            for (auto & entry_image : image_list)
            {
                // attempt to "transfer" the IMAGE "data" -- since we need to know when this IMAGE is "ejected", we need to associate the IMAGE "ID" with the IMAGE "data" being
                // "transferred".
                ; // e.g. -> successful = pressElectronicsTransfer(image.image_id, image.p_buffer, image.buffer_size_in_bytes);
            }
        }
    }

    // check to see if everything was successful -- if not, attempt to "log" the "error".
    if (!successful)
    {
        FIERY::FITS::MODULE::LOG::MANAGER::LogError("FITS-Output-Raw: ImageTransferAccumulated FAILED (JobId = " + job_id + ", ImageId = " + image_id +
                    ", ColorantAccumulator = " + std::to_string(m_property_colorant_accumulator_enabled) + ")");
    }

    // return the status.
    return(successful);
}

bool MANAGER::ImageWaitForEjected(const std::string & job_id, const std::string & image_id)
{
    bool   successful = true;


    // check to see if we "wrote" the IMAGE to "file".
    if (!m_file_enabled)
    {
        /*************************************************************************************************************************************************/
        /*                                                                INSERT CODE HERE                                                               */
        /*                                                                                                                                               */
        /* PURPOSE: WAIT FOR PRESS ELECTRONICS TO SIGNAL THAT THE GIVEN IMAGE HAS BEEN "EJECTED" -- ENSURING IT WILL NOT NEED TO BE RECOVERED/REPRINTED. */
        /*                                                                                                                                               */
        /*************************************************************************************************************************************************/
        // attempt to "wait" for the IMAGE to be "ejected" -- since we associated the IMAGE "ID" with the IMAGE "data" being "transferred", wait for a "signal" for that IMAGE "ID".
        ; // e.g. -> successful = pressElectronicsWaitForEjected(image.image_id);

        // if not, check to see if everything was successful -- if not, attempt to "log" the "error".
        if (!successful)
        {
            FIERY::FITS::MODULE::LOG::MANAGER::LogError("FITS-Output-Raw: ImageWaitForEjected FAILED (JobId = " + job_id + ", ImageId = " + image_id + ", WaitForEjected = " +
                        std::to_string(m_property_wait_for_ejected_enabled) + ")");
        }
    }

    // return the status.
    return(successful);
}

bool MANAGER::Initialize(const FIERY::FITS::MODULE::LOGGER & logger, FIERY::FITS::OUTPUT::PROPERTIES::OUTPUT & output_properties)
{
    bool   successful = false;


    // update the "management" variables.
    FIERY::FITS::MODULE::LOG::MANAGER::Initialize(logger);

    // attempt to "get" the "configuration".
    if (configurationGet(output_properties))
    {
        FIERY::FITS::MODULE::LOG::MANAGER::LogNormal("FITS-Output-Raw[Initialize]: COMPLETED");

        // update the status.
        successful = true;
    }

    // return the status.
    return(successful);
}

bool MANAGER::configurationGet(FIERY::FITS::OUTPUT::PROPERTIES::OUTPUT & output_properties)
{
    std::string                   file_path_string;
    boost::property_tree::ptree   json_object;

    boost::filesystem::path       path_json_configuration_file = boost::filesystem::current_path() / FIERY::FITS::MODULE::CONSTANT::FILENAME_CONFIGURATION_JSON;
    bool                          successful                   = true;


    // initialize the return variables.
    output_properties.clear();

    // attempt to "read" the JSON "configuration" FILE.
    try
    {
        boost::property_tree::read_json(path_json_configuration_file.string(), json_object);
    }
    catch (...)
    {
        FIERY::FITS::MODULE::LOG::MANAGER::LogError("FITS-Output-Raw[Configuration]: INVALID JSON File", true);
        successful = false;
    }

    // check to see if we successfully "read" the JSON "configuration" FILE.
    if (successful)
    {
        // if so, update the "management" variables.
        m_directory                             = json_object.get<std::string>("output_raw.directory", "");
        m_file_enabled                          = json_object.get<bool>("output_raw.file_enabled", false);
        m_job_name_enabled                      = json_object.get<bool>("output_raw.job_name_enabled", false);
        m_page_number_enabled                   = json_object.get<bool>("output_raw.page_number_enabled", false);
        m_property_colorant_accumulator_enabled = json_object.get<bool>("output_raw.colorant_accumulator_enabled", false);
        m_property_wait_for_ejected_enabled     = json_object.get<bool>("output_raw.wait_for_ejected_enabled", false);

        // update the return variables.
        output_properties.colorant_accumulator_enabled = m_property_colorant_accumulator_enabled;
        output_properties.image_data_processor_enabled = m_property_image_data_processor_enabled;
        output_properties.wait_for_ejected_enabled     = m_property_wait_for_ejected_enabled;

        // check to see if the "user-defined" DIRECTORY is "valid" (this is only done for QA/Testing purposes) -- initialize the "management" variables, accordingly.
        if (!m_directory.empty() && (m_directory != ""))
        {
            boost::filesystem::path   path_raster_directory(m_directory);

            m_file_path = path_raster_directory;
        }
        else
        {
            m_file_path = boost::filesystem::current_path() / DIRECTORY_RASTER;
        }

        // initialize the local variables.
        file_path_string = m_file_path.string();

        // check to see if we need to "create" the RASTER "directory" -- if so, do it.
        if (!(boost::filesystem::exists(file_path_string)))
        {
            try
            {
                if (!boost::filesystem::create_directories(file_path_string))
                {
                    FIERY::FITS::MODULE::LOG::MANAGER::LogError("FITS-Output-Raw[Configuration]: FAILED Create (Directory = " + file_path_string + ")", true);
                    successful = false;
                }
            }
            catch (...)
            {
                FIERY::FITS::MODULE::LOG::MANAGER::LogError("FITS-Output-Raw[Configuration]: EXCEPTION Create (Directory = " + file_path_string + ")", true);
                successful = false;
            }
        }
    }

    // return the status.
    return(successful);
}

bool MANAGER::filenameGet(const FIERY::FITS::MODULE::IMAGE::IMAGE & image, const std::string & job_name, std::string & file_name)
{
    bool   successful = true;


    // update the return variables
    file_name = ((m_job_name_enabled && !job_name.empty() && (job_name != "")) ? job_name : image.job_id) + (m_page_number_enabled ? "_P" + std::to_string(image_number) : "_" + \
                image.image_id) + "_X" + std::to_string(image.number_of_pixels_x) + "xY" + std::to_string(image.number_of_pixels_y) + "xB" + \
                std::to_string(image.bits_per_pixel) + "_" + image.colorant + ".raw";

    // return the status.
    return(successful);
}

bool MANAGER::fileWrite(const FIERY::FITS::MODULE::IMAGE::IMAGE & image, const std::string & file_name)
{
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>>   converter;
    boost::filesystem::path                                  file_path;

    bool                                                     successful = false;


    // check to see if the "filename" is "valid".
    if (!file_name.empty() && (file_name != ""))
    {
        // if so, initialize the local variables.
        file_path = m_file_path / converter.from_bytes(file_name);

        // attmpt to "create" and "open" the RASTER "file".
        boost::filesystem::ofstream   file(file_path.wstring(), std::ios::binary);

        // check to see if the RASTER "file" is "open".
        if (file.is_open())
        {
            // if so, attempt to "write" the IMAGE "data".
            file.write(reinterpret_cast<const char *>(image.p_buffer), image.buffer_size_in_bytes);

            // attempt to "close" the FILE.
            file.close();

            // update the status.
            successful = true;
        }
        else
        {
            FIERY::FITS::MODULE::LOG::MANAGER::LogError("fileWrite[" + image.colorant + "]: Open FAILED (Name = " + file_path.string() + ")");
        }
    }

    // return the status.
    return(successful);
}
