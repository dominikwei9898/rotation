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

#include <cstring>
#include <codecvt>

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/json.hpp>
#include <boost/optional.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

using namespace FIERY::FITS::OUTPUT;


/**********************************************************************************************************************************************************************************/
/*                                                         local typedefs, structures, enumerations, and related defines                                                          */
/**********************************************************************************************************************************************************************************/
typedef struct
{
    uint32_t   number_of_pixels_x;
    uint32_t   number_of_pixels_y;
    uint32_t   resolution_x;
    uint32_t   resolution_y;
    uint32_t   number_of_colorants;
    uint32_t   contains_white;
    uint32_t   bits_per_pixel;
    uint32_t   dot_size;
    uint32_t   rip_id;
    uint32_t   colorant_flags;
    uint32_t   image_size_enabled;
    uint32_t   image_width_integer;
    uint32_t   image_width_fractional;
    uint32_t   image_height_integer;
    uint32_t   image_height_fractional;
    uint32_t   reserved[23];
} PRN_HEADER;


/**********************************************************************************************************************************************************************************/
/*                                                                                local variables                                                                                 */
/**********************************************************************************************************************************************************************************/
thread_local std::string         job_id_current;

thread_local uint32_t            image_number = 0;

const std::vector<std::string>   MANAGER::COLORANT_LIST_PROCESSING_ORDER = {"cyan", "magenta", "yellow", "black", "lightcycan", "lightmagenta", "lightyellow", "orange", "green", "blue", "white", "varnish"};
const std::string                MANAGER::DIRECTORY_RASTER               = "raster";


/**********************************************************************************************************************************************************************************/
/*                                                                                     methods                                                                                    */
/**********************************************************************************************************************************************************************************/
MANAGER::MANAGER() : m_directory(), m_file_enabled(false), m_file_path(), m_job_name_enabled(false), m_page_number_enabled(false), m_property_colorant_accumulator_enabled(true),
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


    FIERY::FITS::MODULE::LOG::MANAGER::LogError("FITS-Output-Prn[" + image_properties.colorant + "]: ImageProcess FAILED (JobId = " + image_properties.job_id +
                ", ImageId = " + image_properties.image_id + ", ImageDataProcessor = " + std::to_string(m_property_image_data_processor_enabled) + ")");

    // return the status.
    return(successful);
}

bool MANAGER::ImageTransfer(const FIERY::FITS::MODULE::IMAGE::IMAGE & image, const std::string & job_name)
{
    bool   successful = false;


    FIERY::FITS::MODULE::LOG::MANAGER::LogError("FITS-Output-Prn[" + image.colorant + "]: ImageTransfer FAILED (JobId = " + image.job_id + ", ImageId = " + image.image_id +
                ", ColorantAccumulator = " + std::to_string(m_property_colorant_accumulator_enabled) + ")");

    // return the status.
    return(successful);
}

bool MANAGER::ImageTransferAccumulated(const std::list<FIERY::FITS::MODULE::IMAGE::IMAGE> & image_list, const std::string & job_name)
{
    FIERY::FITS::MODULE::IMAGE::IMAGE   image_front = image_list.front();
    bool                                successful  = false;


    // check to see if "colorant accumulator" is "enabled".
    if (m_property_colorant_accumulator_enabled)
    {
        std::vector<uint8_t>   image_data;


        // if so, attempt to "generate" the PRN "data".
        if (generateImageData(image_list, image_data))
        {
            // if successful, check to see if we need to "write" the IMAGE to "file".
            if (m_file_enabled)
            {
                std::string   file_name;


                // if so, check to see if we need to update the "management" variables -- if so, do it.
                if (job_id_current != image_front.job_id)
                {
                    image_number   = 0;
                    job_id_current = image_front.job_id;
                }

                // update the "management" variables.
                image_number++;

                // attempt to "get" the "filename".
                filenameGet(image_front, job_name, file_name);

                // attempt to "write" the IMAGE "data".
                successful = fileWrite(image_data, file_name);
            }
            else
            {
                /*************************************************************************************************************************************************/
                /*                                                               INSERT CODE HERE                                                                */
                /*                                                                                                                                               */
                /*                                               PURPOSE: TRANSFER PRN IMAGE TO PRESS ELECTRONICS.                                               */
                /*                                                                                                                                               */
                /*************************************************************************************************************************************************/
                // attempt to "transfer" the IMAGE "data" -- since we need to know when this IMAGE is "ejected", we need to associate the IMAGE "ID" with the IMAGE "data" being
                // "transferred".
                ; // e.g. -> successful = pressElectronicsTransfer(image_front.image_id, image_data);
            }
        }
    }

    // check to see if everything was successful -- if not, attempt to "log" the "error".
    if (!successful)
    {
        FIERY::FITS::MODULE::LOG::MANAGER::LogError("FITS-Output-Prn: ImageTransferAccumulated FAILED (JobId = " + image_front.job_id + ", ImageId = " + image_front.image_id +
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
            FIERY::FITS::MODULE::LOG::MANAGER::LogError("FITS-Output-Prn: ImageWaitForEjected FAILED (JobId = " + job_id + ", ImageId = " + image_id + ", WaitForEjected = " +
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
        FIERY::FITS::MODULE::LOG::MANAGER::LogNormal("FITS-Output-Prn[Initialize]: COMPLETED");

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
        FIERY::FITS::MODULE::LOG::MANAGER::LogError("FITS-Output-Prn[Configuration]: INVALID JSON File", true);
        successful = false;
    }

    // check to see if we successfully "read" the JSON "configuration" FILE.
    if (successful)
    {
        // if so, update the "management" variables.
        m_directory                         = json_object.get<std::string>("output_prn.directory", "");
        m_file_enabled                      = json_object.get<bool>("output_prn.file_enabled", false);
        m_job_name_enabled                  = json_object.get<bool>("output_prn.job_name_enabled", false);
        m_page_number_enabled               = json_object.get<bool>("output_prn.page_number_enabled", false);
        m_property_wait_for_ejected_enabled = json_object.get<bool>("output_prn.wait_for_ejected_enabled", false);

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
                    FIERY::FITS::MODULE::LOG::MANAGER::LogError("FITS-Output-Prn[Configuration]: FAILED Create (Directory = " + file_path_string + ")", true);
                    successful = false;
                }
            }
            catch (...)
            {
                FIERY::FITS::MODULE::LOG::MANAGER::LogError("FITS-Output-Prn[Configuration]: EXCEPTION Create (Directory = " + file_path_string + ")", true);
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
                image.image_id) + ".prn";

    // return the status.
    return(successful);
}

bool MANAGER::fileWrite(const std::vector<uint8_t> & image_data, const std::string & file_name)
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
            file.write(reinterpret_cast<const char *>(image_data.data()), image_data.size());

            // attempt to "close" the FILE.
            file.close();

            // update the status.
            successful = true;
        }
        else
        {
            FIERY::FITS::MODULE::LOG::MANAGER::LogError("fileWrite: Open FAILED (Name = " + file_path.string() + ")");
        }
    }

    // return the status.
    return(successful);
}

bool MANAGER::generateImageData(const std::list<FIERY::FITS::MODULE::IMAGE::IMAGE> & image_list, std::vector<uint8_t> & image_data_output)
{
    FIERY::FITS::MODULE::IMAGE::IMAGE   image_front = image_list.front();
    bool                                successful  = false;


    // check to see if the IMAGE "list" is "valid".
    if (!image_list.empty())
    {
        uint32_t                                         bytes_per_scanline;
        uint32_t                                         bytes_per_scanline_to_copy;
        std::vector<FIERY::FITS::MODULE::IMAGE::IMAGE>   image_list_processing_order;
        uint64_t                                         number_of_bytes_image_data;
        uint64_t                                         number_of_bytes_total;
        uint32_t                                         number_of_scanlines;
        uint8_t *                                        p_buffer_current;
        PRN_HEADER                                       prn_header;

        bool                                             contains_white  = false;
        uint64_t                                         offset_in_bytes = 0;


        // loop through the "processing order" COLORANT "list" -- attempting to "create" the "processing order" IMAGE "list".
        for (auto & entry_colorant : COLORANT_LIST_PROCESSING_ORDER)
        {
            // loop through the IMAGE "list", attempting to "create" the "processing order" IMAGE "list".
            for (auto & entry_image : image_list)
            {
                // attempt to "find" the given COLORANT.
                if (entry_image.colorant == entry_colorant)
                {
                    // if successful, attempt to "add" the IMAGE to the "processing order" IMAGE "list".
                    image_list_processing_order.push_back(entry_image);

                    // check to see if the "colorant" is "white" -- if so, update the local variables.
                    if (entry_colorant == "white")
                    {
                        contains_white = true;
                    }
                }
            }
        }

        // initialize the PRN "header"
        std::memset(&(prn_header), 0, sizeof(prn_header));
        prn_header.number_of_pixels_x  = static_cast<uint32_t>(image_front.number_of_pixels_x);
        prn_header.number_of_pixels_y  = static_cast<uint32_t>(image_front.number_of_pixels_y);
        prn_header.resolution_x        = static_cast<uint32_t>(image_front.resolution_x);
        prn_header.resolution_y        = static_cast<uint32_t>(image_front.resolution_y);
        prn_header.number_of_colorants = static_cast<uint32_t>(image_list_processing_order.size());
        prn_header.contains_white      = contains_white;
        prn_header.bits_per_pixel      = image_front.bits_per_pixel;

        // initialize the local variables.
        bytes_per_scanline  = image_front.bytes_per_scanline;
        number_of_scanlines = prn_header.number_of_pixels_y;

        // calculate the local variables.
        bytes_per_scanline_to_copy = bytes_per_scanline;   // ((image_front.number_of_pixels_x * image_front.bits_per_pixel) + 7) / 8;
        number_of_bytes_image_data = bytes_per_scanline_to_copy * number_of_scanlines * image_list_processing_order.size();
        number_of_bytes_total      = sizeof(prn_header) + number_of_bytes_image_data;

        // attempt to "reserve/resize" the PRN "data".
        image_data_output.reserve(number_of_bytes_total);
        image_data_output.resize(number_of_bytes_total);

        // initialize the local variables.
        p_buffer_current = image_data_output.data();

        // attempt to "copy" the PRN "header".
        std::memcpy(p_buffer_current, reinterpret_cast<const void *>(&(prn_header)), sizeof(prn_header));

        // update the local variables.
        p_buffer_current += sizeof(prn_header);

        // loop through the "scanlines, attempting attempting to "create" the PRN "data".
        for (uint32_t index = 0; index < number_of_scanlines; index++)
        {
            // loop through the "processing order" IMAGE "list", attempting to "create" the PRN "data".
            for (auto & entry_image : image_list_processing_order)
            {
                // attempt to "copy" the PRN "data" (excluding any "padding" introduce by the Fiery).
                std::memcpy(p_buffer_current, reinterpret_cast<const void *>(entry_image.p_buffer + offset_in_bytes), bytes_per_scanline_to_copy);

                // update the local variables.
                p_buffer_current += bytes_per_scanline_to_copy;
            }

            // update the local variables.
            offset_in_bytes += bytes_per_scanline;
        }

        // update the status.
        successful = true;
    }

    // return the status.
    return(successful);
}