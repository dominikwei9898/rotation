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

#include <boost/filesystem.hpp>
#include <boost/json.hpp>
#include <boost/optional.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <thread>

using namespace FIERY::FITS::OUTPUT;


/**********************************************************************************************************************************************************************************/
/*                                                                                local variables                                                                                 */
/**********************************************************************************************************************************************************************************/


/**********************************************************************************************************************************************************************************/
/*                                                                                     methods                                                                                    */
/**********************************************************************************************************************************************************************************/
MANAGER::MANAGER() : m_print_rate_delay_in_milliseconds(0.0), m_print_rate_images_per_minute(0), m_print_rate_time_point_previous(std::chrono::high_resolution_clock::now()),
            m_property_colorant_accumulator_enabled(false), m_property_image_data_processor_enabled(false), m_property_wait_for_ejected_enabled(false),
            m_transfer_time_in_milliseconds(0)
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


    FIERY::FITS::MODULE::LOG::MANAGER::LogError("FITS-Output-Simulator[" + image_properties.colorant + "]: ImageProcess FAILED (JobId = " + image_properties.job_id +
                ", ImageId = " + image_properties.image_id + ", ImageDataProcessor = " + std::to_string(m_property_image_data_processor_enabled) + ")");

    // return the status.
    return(successful);
}

bool MANAGER::ImageTransfer(const FIERY::FITS::MODULE::IMAGE::IMAGE & image, const std::string & job_name)
{
    bool   successful = true;


    // check to see if the TRANSFER "simulation" time is "valid".
    if (m_transfer_time_in_milliseconds != 0)
    {
        // if so, attempt to "simulate" the IMAGE "transfer" to the OUTPUT by "sleeping" for a fixed amount of time.
        std::this_thread::sleep_for(std::chrono::milliseconds(m_transfer_time_in_milliseconds));
    }

    if (!successful)
    {
        FIERY::FITS::MODULE::LOG::MANAGER::LogNormal("FITS-Output-Simulator[" + image.colorant + "]: ImageTransfer FAILED (JobId = " + image.job_id + ", ImageId = " +
                    image.image_id + ")");
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
        // if so, check to see if the TRANSFER "simulation" time is "valid".
        if (m_transfer_time_in_milliseconds != 0)
        {
            // if so, attempt to "simulate" the IMAGE "transfer" to the OUTPUT by "sleeping" for a fixed amount of time.
            std::this_thread::sleep_for(std::chrono::milliseconds(m_transfer_time_in_milliseconds));
        }
    }

    // check to see if everything was successful -- if not, attempt to "log" the "error".
    if (!successful)
    {
        FIERY::FITS::MODULE::LOG::MANAGER::LogError("FITS-Output-Simulator: ImageTransferAccumulated FAILED (JobId = " + job_id + ", ImageId = " + image_id +
                    ", ColorantAccumulator = " + std::to_string(m_property_colorant_accumulator_enabled) + ")");
    }

    // return the status.
    return(successful);
}

bool MANAGER::ImageWaitForEjected(const std::string & job_id, const std::string & image_id)
{
    bool   successful = true;


    // check to see if "wait for ejected" is "enabled".
    if (!m_property_wait_for_ejected_enabled)
    {
        // if not, update the status.
        successful = false;
    }
    else
    {
        std::chrono::duration<double_t, std::milli>   duration;
        double_t                                      duration_in_milliseconds;
        uint32_t                                      sleep_in_milliseconds_adjusted;


        // if so, initialize the local variables.
        duration                       = std::chrono::duration_cast<std::chrono::duration<double_t>>(std::chrono::high_resolution_clock::now() - \
                    m_print_rate_time_point_previous);
        duration_in_milliseconds       = duration.count();
        sleep_in_milliseconds_adjusted = (duration_in_milliseconds < m_print_rate_delay_in_milliseconds) ? lround(m_print_rate_delay_in_milliseconds - \
                    duration_in_milliseconds) : 0;

        // check to see if we need to "simulate" the IMAGE "ejected" rate from the OUTPUT -- if so, do it.
        if (sleep_in_milliseconds_adjusted != 0)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(sleep_in_milliseconds_adjusted));
        }

        // update the "management" variables.
        m_print_rate_time_point_previous = std::chrono::high_resolution_clock::now();
    }

    // check to see if everything was successful -- if not, attempt to "log" the "error".
    if (!successful)
    {
        FIERY::FITS::MODULE::LOG::MANAGER::LogError("FITS-Output-Simulator: ImageWaitForEjected FAILED (JobId = " + job_id + ", ImageId = " + image_id + ", WaitForEjected = " +
                    std::to_string(m_property_wait_for_ejected_enabled) + ")");
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
        FIERY::FITS::MODULE::LOG::MANAGER::LogNormal("FITS-Output-Simulator[Initialize]: COMPLETED");

        // update the status.
        successful = true;
    }

    // return the status.
    return(successful);
}

bool MANAGER::configurationGet(FIERY::FITS::OUTPUT::PROPERTIES::OUTPUT & output_properties)
{
    boost::property_tree::ptree   json_object;
    std::string                   raster_format;

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
        FIERY::FITS::MODULE::LOG::MANAGER::LogError("FITS-Output-Simulator[Configuration]: INVALID JSON File", true);
        successful = false;
    }

    // check to see if we successfully "read" the JSON "configuration" FILE.
    if (successful)
    {
        // if so, update the "management" variables.
        m_print_rate_images_per_minute          = json_object.get<uint32_t>("output_simulator.images_per_minute", 0);
        m_property_colorant_accumulator_enabled = json_object.get<bool>("output_simulator.colorant_accumulator_enabled", false);
        m_transfer_time_in_milliseconds         = json_object.get<uint32_t>("output_simulator.transfer_time_in_milliseconds", 0);

        // check to see if the IMAGE "rate" is "valid" -- if so, initialize the "management" variables.
        if (m_print_rate_images_per_minute != 0)
        {
            m_property_wait_for_ejected_enabled = true;
            m_print_rate_delay_in_milliseconds  = (60 * 1000) / m_print_rate_images_per_minute;
        }

        // update the return variables.
        output_properties.colorant_accumulator_enabled = m_property_colorant_accumulator_enabled;
        output_properties.image_data_processor_enabled = m_property_image_data_processor_enabled;
        output_properties.wait_for_ejected_enabled     = m_property_wait_for_ejected_enabled;
    }

    // return the status.
    return(successful);
}
