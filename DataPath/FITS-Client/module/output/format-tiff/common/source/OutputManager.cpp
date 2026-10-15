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

#include "tiffio.h"
#include "tiffio.hxx"
#include "tiff.h"

#include <codecvt>
#include <sstream> 

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
thread_local uint32_t      job_number   = 0;

const std::string          MANAGER::DIRECTORY_RASTER        = "raster";
const uint32_t             MANAGER::TIFF_SCANLINES_PER_BAND = 512;


/**********************************************************************************************************************************************************************************/
/*                                                                                     methods                                                                                    */
/**********************************************************************************************************************************************************************************/
MANAGER::MANAGER() : m_directory(), m_file_enabled(false), m_file_path(), m_job_name_enabled(false), m_page_number_enabled(false), m_property_colorant_accumulator_enabled(false),
            m_property_image_data_processor_enabled(true), m_property_wait_for_ejected_enabled(false)
{
    ;
}

MANAGER::~MANAGER()
{
    ;
}

bool MANAGER::ImageProcess(const FIERY::FITS::MODULE::PROPERTIES::IMAGE & image_properties, const std::vector<uint8_t> & image_data_input, std::vector<uint8_t> & image_data_output)
{
    // attempt to "generate" the TIFF "data".
    return(generateImageData(image_properties, image_data_input, image_data_output));
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
            job_number++;
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
        /*                                              PURPOSE: TRANSFER TIFF IMAGE TO PRESS ELECTRONICS.                                               */
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
                job_number++;
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
            /*                                       PURPOSE: TRANSFER TIFF IMAGE (ACCUMULATED) TO PRESS ELECTRONICS.                                        */
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
        FIERY::FITS::MODULE::LOG::MANAGER::LogError("FITS-Output-Tiff: ImageTransferAccumulated FAILED (JobId = " + job_id + ", ImageId = " + image_id +
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
        FIERY::FITS::MODULE::LOG::MANAGER::LogNormal("FITS-Output-Tiff[Initialize]: COMPLETED");

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
        FIERY::FITS::MODULE::LOG::MANAGER::LogError("FITS-Output-Tiff[Configuration]: INVALID JSON File", true);
        successful = false;
    }

    // check to see if we successfully "read" the JSON "configuration" FILE.
    if (successful)
    {
        // if so, update the "management" variables.
        m_directory                             = json_object.get<std::string>("output_tiff.directory", "");
        m_file_enabled                          = json_object.get<bool>("output_tiff.file_enabled", false);
        m_job_name_enabled                      = json_object.get<bool>("output_tiff.job_name_enabled", false);
        m_page_number_enabled                   = json_object.get<bool>("output_tiff.page_number_enabled", false);
        m_property_colorant_accumulator_enabled = json_object.get<bool>("output_tiff.colorant_accumulator_enabled", false);
        m_property_wait_for_ejected_enabled     = json_object.get<bool>("output_tiff.wait_for_ejected_enabled", false);

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
                    FIERY::FITS::MODULE::LOG::MANAGER::LogError("FITS-Output-Tiff[Configuration]: FAILED Create (Directory = " + file_path_string + ")", true);
                    successful = false;
                }
            }
            catch (...)
            {
                FIERY::FITS::MODULE::LOG::MANAGER::LogError("FITS-Output-Tiff[Configuration]: EXCEPTION Create (Directory = " + file_path_string + ")", true);
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


    // check to see if the "user-defined" DIRECTORY is "valid" (this is done for internal Fiery QA/Testing purposes) -- update the return variables, accordingly.
    if (!m_directory.empty() && (m_directory != ""))
    {
        file_name = "Job" + std::to_string(job_number) + "_" + std::to_string(image_number) + "_" + image.colorant + ".tif";
    }
    else
    {
        file_name = ((m_job_name_enabled && !job_name.empty() && (job_name != "")) ? job_name : image.job_id) + (m_page_number_enabled ? "_P" + std::to_string(image_number) : \
                    "_" + image.image_id) + "_" + image.colorant + ".tif";
    }

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

bool MANAGER::generateImageData(const FIERY::FITS::MODULE::PROPERTIES::IMAGE & image_properties, const std::vector<uint8_t> & image_data_input, std::vector<uint8_t> & image_data_output)
{
    uint32_t               band_size_in_bytes;
    uint8_t *              p_buffer_to_write;
    uint8_t *              p_buffer_to_write_current;
    TIFF *                 p_tiff;
    std::ostringstream     stream;
    std::vector<uint8_t>   tiff_buffer_band;

    uint32_t               bits_per_pixel               = image_properties.bits_per_pixel;
    uint64_t               bytes_to_copy                = ((image_properties.number_of_pixels_x * bits_per_pixel) + (8 - 1)) / 8;
    uint32_t               bytes_per_scanline           = image_properties.bytes_per_scanline;
    std::string            file_path                    = "temp_" + image_properties.colorant;
    uint32_t               index_band                   = 0;
    uint64_t               number_of_bytes_to_write     = image_data_input.size();
    uint64_t               number_of_scanlines_to_write = image_properties.number_of_pixels_y;
    bool                   successful                   = false;

    const uint8_t *        p_buffer_to_read             = image_data_input.data();


    // initialize the return variables.
    image_data_output.clear();

    // calculate the local variables.
    band_size_in_bytes = bytes_per_scanline * TIFF_SCANLINES_PER_BAND;

    // initialize the local variables.
    tiff_buffer_band.resize(band_size_in_bytes);
    p_buffer_to_write = tiff_buffer_band.data();

    // attempt to "open" the TIFF "file".
    if ((p_tiff = TIFFStreamOpen(file_path.c_str(), &(stream))) != nullptr)
    {
        // update the local variables.
        successful = true;

        // attempt to "set" the TIFF "parameters".
        if (TIFFSetField(p_tiff, TIFFTAG_SUBFILETYPE, 0) && TIFFSetField(p_tiff, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT) &&
                    TIFFSetField(p_tiff, TIFFTAG_SAMPLESPERPIXEL, 1) && TIFFSetField(p_tiff, TIFFTAG_BITSPERSAMPLE, bits_per_pixel) &&
                    TIFFSetField(p_tiff, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG) && TIFFSetField(p_tiff, TIFFTAG_IMAGEWIDTH, image_properties.number_of_pixels_x) &&
                    TIFFSetField(p_tiff, TIFFTAG_IMAGELENGTH, image_properties.number_of_pixels_y) && TIFFSetField(p_tiff, TIFFTAG_RESOLUTIONUNIT, RESUNIT_INCH) &&
                    TIFFSetField(p_tiff, TIFFTAG_XRESOLUTION, static_cast<float>(image_properties.resolution_x)) &&
                    TIFFSetField(p_tiff, TIFFTAG_YRESOLUTION, static_cast<float>(image_properties.resolution_y)) &&
                    TIFFSetField(p_tiff, TIFFTAG_ROWSPERSTRIP, TIFF_SCANLINES_PER_BAND) && TIFFSetField(p_tiff, TIFFTAG_COMPRESSION, COMPRESSION_LZW) &&
                    TIFFSetField(p_tiff, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISWHITE))
        {
            // loop continuusly, attempting to "write" the IMAGE "data" a BAND at a time.
            while (number_of_bytes_to_write > band_size_in_bytes)
            {
                // initialize the local variables.
                p_buffer_to_write_current = p_buffer_to_write;

                // loop through the BAND, attempting to "copy" the IMAGE "data".
                for (auto index = 0; index < TIFF_SCANLINES_PER_BAND; index++)
                {
                    // attempt to "copy" the IMAGE "data".
                    std::memcpy(p_buffer_to_write_current, p_buffer_to_read, bytes_to_copy);

                    // update the local variables.
                    p_buffer_to_read          += bytes_per_scanline;
                    p_buffer_to_write_current += bytes_to_copy;
                }

                // attempt to "write" the BAND "data".
                if (TIFFWriteEncodedStrip(p_tiff, index_band, p_buffer_to_write, band_size_in_bytes) != band_size_in_bytes)
                {
                    // if unsuccessful, update the status and break out of the "for" loop.
                    successful = false;
                    break;
                }

                // update the "management" variables.
                number_of_bytes_to_write     -= band_size_in_bytes;
                number_of_scanlines_to_write -= TIFF_SCANLINES_PER_BAND;
                index_band++;
            }

            // check to see if there is any more IMAGE "data" to write".
            if (number_of_bytes_to_write != 0)
            {
                // if so, initialize the local variables.
                p_buffer_to_write_current = p_buffer_to_write;

                // loop through the BAND, attempting to "copy" the IMAGE "data".
                for (auto index = 0; index < number_of_scanlines_to_write; index++)
                {
                    // attempt to "copy" the IMAGE "data".
                    std::memcpy(p_buffer_to_write_current, p_buffer_to_read, bytes_to_copy);

                    // update the local variables.
                    p_buffer_to_read          += bytes_per_scanline;
                    p_buffer_to_write_current += bytes_to_copy;
                }

                // attempt to "write" the "partial" BAND "data".
                if (TIFFWriteEncodedStrip(p_tiff, index_band, p_buffer_to_write, number_of_bytes_to_write) != number_of_bytes_to_write)
                {
                    // if unsuccessful, update the status.
                    successful = false;
                }
            }
        }

        // attempt to "close" the TIFF "file".
        TIFFClose(p_tiff);

        // check to see if everything was successful.
        if (successful)
        {
            const std::string &   stream_string = stream.str();


            // if so, attempt to "copy" the TIFF "stream" to the return variables.
            image_data_output.reserve(stream_string.size());
            std::copy(stream_string.begin(), stream_string.end(), std::back_inserter(image_data_output));
        }
    }
    else
    {
        FIERY::FITS::MODULE::LOG::MANAGER::LogError("FITS-Output-Tiff[" + image_properties.colorant + "]: TIFFStreamOpen FAILED (Name = " + file_path + ")");
    }

    // return the status.
    return(successful);
}

