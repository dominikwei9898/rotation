/**********************************************************************************************************************************************************************************/
/*                                                                                                                                                                                */
/*  Copyright (c) 1990-2026 Fiery, LLC.  All Rights Reserved.                                                                                                                     */
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
const uint32_t      MANAGER::BUFFER_POOL_SIZE_PER_COLORANT_IN_MEGABYTES_DEFAULT = 2048;
const std::string   MANAGER::FIERY_PATH_CLIENT_IP_DEFAULT                       = "127.0.0.1";
const uint32_t      MANAGER::FIERY_PATH_CLIENT_PORT_DEFAULT                     = 9085;


/**********************************************************************************************************************************************************************************/
/*                                                                                     methods                                                                                    */
/**********************************************************************************************************************************************************************************/
MANAGER::MANAGER() : m_colorant_properties(), m_fiery_path_client_properties(), m_fits_output_plugin_name(), m_logger(), m_rotation_properties(), m_web_socket_mgr()
{
    ;
}

MANAGER::~MANAGER()
{
    ;
}

bool MANAGER::Abort(void)
{
    bool   successful = false;


    FIERY::FITS::MODULE::LOG::MANAGER::LogNormal("ABORT");

    // attempt to "abort" all known IMAGE_PROPERTIES.
    FIERY::FITS::MODULE::IMAGE::PROPERTIES::MANAGER::Abort();

    // attempt to "abort" all known JOBs.
    successful = FIERY::FITS::MODULE::COLORANT::MANAGER::Abort();

    // return the status.
    return(successful);
}

bool MANAGER::ImagePrinted(const std::string & job_id, const std::string & image_id)
{
    bool   successful = false;


    // attempt to "delete" the IMAGE_PROPERTIES.
    FIERY::FITS::MODULE::IMAGE::PROPERTIES::MANAGER::ImagePropertiesDelete(job_id, image_id);

    // attempt to "send" IMAGE_PRINTED to the WEB_SOCKET "manager".
    successful = m_web_socket_mgr.ImagePrinted(job_id, image_id);

    // return the status.
    return(successful);
}

bool MANAGER::ImagePropertiesCreate(const std::string & job_id, const std::string & image_id, const boost::property_tree::ptree & json_object_settings)
{
    bool   successful = false;


    // attempt to "create" the IMAGE_PROPERTIES.
    successful = FIERY::FITS::MODULE::IMAGE::PROPERTIES::MANAGER::ImagePropertiesCreate(job_id, image_id, json_object_settings);

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

bool MANAGER::ImageTransfer(const std::string & job_id, const std::string & job_name, const std::string & image_id, const std::list<std::string> & colorant_list_valid, const boost::property_tree::ptree & json_object_settings)
{
    bool   successful = false;


    // attempt to "transfer" the IMAGE.
    successful = FIERY::FITS::MODULE::COLORANT::MANAGER::ImageTransfer(job_id, job_name, image_id, colorant_list_valid, json_object_settings);

    // return the status.
    return(successful);
}

bool MANAGER::Initialize(const FIERY::FITS::MODULE::LOGGER & logger, const std::string & ip_fiery_path_client_override)
{
    bool   successful = false;


    // update the "management" variables.
    m_logger = logger;

    // attempt to "initialize" the LOG_MANAGER.
    FIERY::FITS::MODULE::LOG::MANAGER::Initialize(m_logger);

    // attempt to "get" the "configuration".
    if (configurationGet(ip_fiery_path_client_override))
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

    // attempt to "cancel" all known IMAGE_PROPERTIES associated with the given JOB.
    FIERY::FITS::MODULE::IMAGE::PROPERTIES::MANAGER::JobCancel(job_id);

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
    if (FIERY::FITS::MODULE::COLORANT::MANAGER::Start(this, m_colorant_properties, m_fits_output_plugin_name, m_rotation_properties))
    {
        // if successful, attempt to "start" the WEB_SOCKET "manager".
        successful = m_web_socket_mgr.Start(this, m_fiery_path_client_properties);
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

bool MANAGER::configurationGet(const std::string & ip_fiery_path_client_override)
{
    boost::property_tree::ptree   json_object;

    bool                          successful = true;


    // attempt to "read" the JSON "configuration" FILE.
    try
    {
        boost::property_tree::read_json(FIERY::FITS::MODULE::CONSTANT::FILENAME_CONFIGURATION_JSON, json_object);
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
        m_colorant_properties.buffer_pool_delayed_allocate = json_object.get<bool>("colorant.buffer_pool_delayed_allocate", false);
        m_colorant_properties.buffer_pool_size_in_bytes    = json_object.get<uint64_t>("colorant.buffer_pool_size_in_megabytes",
                    BUFFER_POOL_SIZE_PER_COLORANT_IN_MEGABYTES_DEFAULT) * 1024 * 1024;
        m_fiery_path_client_properties.ip                  = json_object.get<std::string>("fiery_path_client.ip", FIERY_PATH_CLIENT_IP_DEFAULT);
        m_fiery_path_client_properties.port                = json_object.get<uint32_t>("fiery_path_client.port", FIERY_PATH_CLIENT_PORT_DEFAULT);
        m_fits_output_plugin_name                          = json_object.get<std::string>("output.plugin", "");
        m_rotation_properties.enabled                      = json_object.get<bool>("rotation.enabled", false);
        m_rotation_properties.degrees                      = json_object.get<uint32_t>("rotation.degrees", 0);

        // check to see we need to "override" the FieryPath "ip" -- if so, do it.
        if (!ip_fiery_path_client_override.empty())
        {
            FIERY::FITS::MODULE::LOG::MANAGER::LogNormal("FITS-Module[Configuration]: OVERRIDING (IP = <" + ip_fiery_path_client_override + ">)");
            m_fiery_path_client_properties.ip = ip_fiery_path_client_override;
        }

        // check to see if the "management" variables are "valid" -- if not, update the status.
        if (m_colorant_properties.buffer_pool_size_in_bytes == 0)
        {
            FIERY::FITS::MODULE::LOG::MANAGER::LogError("FITS-Module[Configuration]: INVALID Colorant Buffer Size", true);
            successful = false;
        }
        else if (m_fiery_path_client_properties.ip.empty())
        {
            FIERY::FITS::MODULE::LOG::MANAGER::LogError("FITS-Module[Configuration]: INVALID FieryPath-Client IP", true);
            successful = false;
        }
        else if (m_fiery_path_client_properties.port == 0)
        {
            FIERY::FITS::MODULE::LOG::MANAGER::LogError("FITS-Module[Configuration]: INVALID FieryPath-Client Port", true);
            successful = false;
        }
        else if (m_fits_output_plugin_name.empty())
        {
            FIERY::FITS::MODULE::LOG::MANAGER::LogError("FITS-Module[Configuration]: INVALID Output Plugin", true);
            successful = false;
        }
        else if (m_rotation_properties.enabled && (m_rotation_properties.degrees != 0) && (m_rotation_properties.degrees != 90) && (m_rotation_properties.degrees != 180) && (m_rotation_properties.degrees != 270))
        {
            FIERY::FITS::MODULE::LOG::MANAGER::LogError("FITS-Module[Configuration]: INVALID Rotation Degrees (Degrees = " +
                        std::to_string(m_rotation_properties.degrees) + ") -- MUST be 0, 90, 180, or 270", true);
            successful = false;
        }
    }

    // return the status.
    return(successful);
}
