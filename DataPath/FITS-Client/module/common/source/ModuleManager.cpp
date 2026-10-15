/**********************************************************************************************************************************************************************************/
/*                                                                                                                                                                                */
/*  Copyright (c) 1990-2023 Fiery, LLC.  All Rights Reserved.                                                                                                                     */
/*                                                                                                                                                                                */
/*  Fiery, LLC ("Fiery") products contain certain trade secrets and confidential and proprietary information of Fiery.  Use, reproduction, disclosure, distribution by any means  */
/*  are prohibited, except pursuant to a written license from Fiery.  Modification, translation, reverse engineering, decompiling, disassembling, and creating derivative works   */
/*  based on this software are prohibited, except pursuant to a written license from Fiery. Use of copyright notice does not imply publication or disclosure.                     */
/*                                                                                                                                                                                */
/**********************************************************************************************************************************************************************************/
#include "ModuleManager.h"

#include "ColorantManager.h"
#include "LogManager.h"
#include "WebSocketManager.h"

#include <boost/filesystem.hpp>
#include <boost/json.hpp>
#include <boost/optional.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

using namespace FIERY::FITS::MODULE;


/**********************************************************************************************************************************************************************************/
/*                                                                                local variables                                                                                 */
/**********************************************************************************************************************************************************************************/


/**********************************************************************************************************************************************************************************/
/*                                                                                     methods                                                                                    */
/**********************************************************************************************************************************************************************************/
MANAGER::MANAGER() : m_colorant_properties(), m_fiery_path_server_properties(), m_fits_output_plugin_name(), m_logger(), m_web_socket_mgr()
{
    ;
}

MANAGER::~MANAGER()
{
    ;
}

bool MANAGER::ImagePrinted(const std::string & job_id, const std::string & image_id)
{
    bool   successful = false;


    // attempt to "send" IMAGE_PRINTED to the WEB_SOCKET "manager".
    successful = m_web_socket_mgr.ImagePrinted(job_id, image_id);

    // return the status.
    return(successful);
}

bool MANAGER::ImageSave(const FIERY::FITS::MODULE::PROPERTIES::IMAGE & image_properties, const std::vector<uint8_t> & image_data)
{
    bool   successful = false;


    // attempt to "create" the IMAGE.
    successful = FIERY::FITS::MODULE::COLORANT::MANAGER::ImageCreate(image_properties, image_data);

    // return the status.
    return(successful);
}

bool MANAGER::ImageTransfer(const std::string & job_id, const std::string & job_name, const std::string & image_id, const std::list<std::string> & colorant_list_valid)
{
    bool   successful = false;


    // attempt to "transfer" the IMAGE.
    successful = FIERY::FITS::MODULE::COLORANT::MANAGER::ImageTransfer(job_id, job_name, image_id, colorant_list_valid);

    // return the status.
    return(successful);
}

bool MANAGER::Initialize(const FIERY::FITS::MODULE::LOGGER & logger)
{
    bool   successful = false;


    // update the "management" variables.
    m_logger = logger;

    // update the "initialize" the LOG_MANAGER.
    FIERY::FITS::MODULE::LOG::MANAGER::Initialize(m_logger);

    // attempt to "get" the "configuration".
    if (configurationGet())
    {
        FIERY::FITS::MODULE::LOG::MANAGER::LogNormal("FITS-Module[Initialize]: COMPLETED");

        // update the status.
        successful = true;
    }

    // return the status.
    return(successful);
}

bool MANAGER::JobCancel(const std::string & job_id)
{
    bool   successful = false;


    FIERY::FITS::MODULE::LOG::MANAGER::LogNormal("CANCEL (JobId = " + job_id + ")");

    // attempt to "cancel" the JOB.
    successful = FIERY::FITS::MODULE::COLORANT::MANAGER::JobCancel(job_id);

    // return the status.
    return(successful);
}

bool MANAGER::Start(const std::list<std::string> & colorant_list)
{
    bool   successful = false;


    // update the "management" variables.
    m_colorant_properties.colorant_list = colorant_list;

    // attempt to "start" the COLORANT "manager".
    if (FIERY::FITS::MODULE::COLORANT::MANAGER::Start(this, m_colorant_properties, m_fits_output_plugin_name))
    {
        // if successful, attempt to "start" the WEB_SOCKET "manager".
        successful = m_web_socket_mgr.Start(this, m_fiery_path_server_properties);
    }

    // check to see if everything was successful.
    if (!successful)
    {
        FIERY::FITS::MODULE::LOG::MANAGER::LogError("FITS-Module[Start]: FAILED");
    }

    // return the status.
    return(successful);
}

bool MANAGER::Stop(void)
{
    bool   successful = false;


    // attempt to "stop" the COLORANT "manager".
    if (FIERY::FITS::MODULE::COLORANT::MANAGER::Stop())
    {
        // if successful, attempt to "stop" the WEB_SOCKET "manager".
        successful = m_web_socket_mgr.Stop();
    }

    // check to see if everything was successful.
    if (!successful)
    {
        FIERY::FITS::MODULE::LOG::MANAGER::LogError("FITS-Module[Stop] FAILED");
    }

    // return the status.
    return(successful);
}

bool MANAGER::configurationGet(void)
{
    boost::property_tree::ptree   json_object;

    boost::filesystem::path       path_current                 = boost::filesystem::current_path();
    boost::filesystem::path       path_json_configuration_file = path_current / FIERY::FITS::MODULE::CONSTANT::FILENAME_CONFIGURATION_JSON;
    bool                          successful                   = true;


    // attempt to "read" the JSON "configuration" FILE.
    try
    {
        boost::property_tree::read_json(path_json_configuration_file.string(), json_object);
    }
    catch (...)
    {
        FIERY::FITS::MODULE::LOG::MANAGER::LogError("FITS-Module[Configuration]: INVALID JSON File", true);
        successful = false;
    }

    // check to see if we successfully "read" the JSON "configuration" FILE.
    if (successful)
    {
        // if so, update the "management" variables.
        m_colorant_properties.buffer_pool_size_in_bytes = json_object.get<uint64_t>("colorant.buffer_pool_size_in_megabytes", 0) * 1024 * 1024;
        m_fiery_path_server_properties.ip               = json_object.get<std::string>("fiery_path_client.ip", "");
        m_fiery_path_server_properties.port             = json_object.get<uint32_t>("fiery_path_client.port", 0);
        m_fits_output_plugin_name                       = json_object.get<std::string>("output.plugin", "");

        // check to see if the "management" variables are "valid" -- if not, update the status.
        if (m_colorant_properties.buffer_pool_size_in_bytes == 0)
        {
            FIERY::FITS::MODULE::LOG::MANAGER::LogError("FITS-Module[Configuration]: INVALID Colorant Buffer Size", true);
            successful = false;
        }
        else if (m_fiery_path_server_properties.ip.empty() || (m_fiery_path_server_properties.ip == ""))
        {
            FIERY::FITS::MODULE::LOG::MANAGER::LogError("FITS-Module[Configuration]: INVALID FieryPath IP", true);
            successful = false;
        }
        else if (m_fiery_path_server_properties.port == 0)
        {
            FIERY::FITS::MODULE::LOG::MANAGER::LogError("FITS-Module[Configuration]: INVALID FieryPath Port", true);
            successful = false;
        }
        else if (m_fits_output_plugin_name.empty() || (m_fits_output_plugin_name == ""))
        {
            FIERY::FITS::MODULE::LOG::MANAGER::LogError("FITS-Module[Configuration]: INVALID Output Plugin", true);
            successful = false;
        }
    }

    // return the status.
    return(successful);
}
