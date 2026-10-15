/**********************************************************************************************************************************************************************************/
/*                                                                                                                                                                                */
/*  Copyright (c) 1990-2026 Fiery, LLC.  All Rights Reserved.                                                                                                                     */
/*                                                                                                                                                                                */
/*  Fiery, LLC ("Fiery") products contain certain trade secrets and confidential and proprietary information of Fiery.  Use, reproduction, disclosure, distribution by any means  */
/*  are prohibited, except pursuant to a written license from Fiery.  Modification, translation, reverse engineering, decompiling, disassembling, and creating derivative works   */
/*  based on this software are prohibited, except pursuant to a written license from Fiery. Use of copyright notice does not imply publication or disclosure.                     */
/*                                                                                                                                                                                */
/**********************************************************************************************************************************************************************************/
#include "ImageManager.h"

#include "LogManager.h"

#include <boost/property_tree/json_parser.hpp>

#include <sstream>

using namespace FIERY::FITS::MODULE::IMAGE;


/**********************************************************************************************************************************************************************************/
/*                                                         local typedefs, structures, enumerations, and related defines                                                          */
/**********************************************************************************************************************************************************************************/


/**********************************************************************************************************************************************************************************/
/*                                                                                local variables                                                                                 */
/**********************************************************************************************************************************************************************************/
PROPERTIES::MANAGER::JOB_IMAGE_PROPERTIES_MAP_MAP   PROPERTIES::MANAGER::m_job_image_properties_map_map;
std::mutex                     PROPERTIES::MANAGER::m_job_image_properties_map_map_mutex;


/**********************************************************************************************************************************************************************************/
/*                                                                                     methods                                                                                    */
/**********************************************************************************************************************************************************************************/
MANAGER::MANAGER(const std::string & colorant) : m_colorant(colorant), m_image_list(), m_image_mutex(), m_image_signal()
{
    ;
}

MANAGER::~MANAGER()
{
    ;
}

bool MANAGER::Abort(void)
{
    bool   successful = true;


    // attempt to "take" the IMAGE "mutex".
    std::lock_guard<std::mutex>   lock(m_image_mutex);

    // check to see if the IMAGE "list" is "valid".
    if (!m_image_list.empty())
    {
        // if so, attempt to "remove" and "deallocate" all IMAGEs.
        m_image_list.remove_if([](auto & entry)
        {
                // return the status.
                return(true);
        });
    }

    FIERY::FITS::MODULE::LOG::MANAGER::LogNormal("FITS-Module[" + m_colorant + "]: IMAGE ABORTED (Length = " + std::to_string(m_image_list.size()) + ")");

    // return the status.
    return(successful);
}

bool MANAGER::ImageCreate(const FIERY::FITS::MODULE::PROPERTIES::IMAGE & image_properties, const FITS::MODULE::BUFFER::BUFFER & buffer)
{
    IMAGE   image(image_properties, buffer.p_buffer, buffer.buffer_size_in_bytes);

    bool    successful = true;


    // attempt to "take" the IMAGE "mutex".
    std::lock_guard<std::mutex>   lock(m_image_mutex);

    // attempt to "add" the IMAGE to the "list".
    m_image_list.push_back(image);

    FIERY::FITS::MODULE::LOG::MANAGER::LogNormal("FITS-Module[" + m_colorant + "]: IMAGE CREATED (JobId = " + image.job_id + ", ImageId = " + image.image_id + ", Length = " +
                std::to_string(m_image_list.size()) + ")");

    // attempt to "signal" the change.
    m_image_signal.notify_all();

    // return the status.
    return(successful);
}

bool MANAGER::ImageDelete(const std::string & job_id, const std::string & image_id)
{
    std::string   colorant   = m_colorant;
    bool          successful = true;


    // attempt to "take" the IMAGE "mutex".
    std::lock_guard<std::mutex>   lock(m_image_mutex);

    // check to see if the IMAGE "list" is "valid".
    if (!m_image_list.empty())
    {
        // if so, attempt to "remove" the IMAGE for the given JOB_ID.
        m_image_list.remove_if([colorant, job_id, image_id](auto & entry)
        {
            // check to see if the IMAGE "entry" is associated with the given JOB_ID and IMAGE_ID.
            return((entry.job_id == job_id) && (entry.image_id == image_id));
        });
    }

    FIERY::FITS::MODULE::LOG::MANAGER::LogNormal("FITS-Module[" + m_colorant + "]: IMAGE DELETED (JobId = " + job_id + ", ImageId = " + image_id + ", Length = " +
                std::to_string(m_image_list.size()) + ")");

    // return the status.
    return(successful);
}

bool MANAGER::ImageGet(const std::string & job_id, const std::string & image_id, FITS::MODULE::IMAGE::IMAGE & image)
{
    bool   successful = true;


    // initialize the return variables.
    image.clear();

    // attempt to "take" the IMAGE "mutex".
    std::unique_lock<std::mutex>   lock(m_image_mutex);

    // loop continuously -- attempting to "get" the IMAGE.
    while (!get(job_id, image_id, image))
    {
        // if unsuccessful, attempt to "wait" for the IMAGE "signal".
        m_image_signal.wait(lock);
    }

    // return the status.
    return(successful);
}

bool MANAGER::JobCancel(const std::string & job_id)
{
    std::string   colorant   = m_colorant;
    bool          successful = true;


    // attempt to "take" the IMAGE "mutex".
    std::lock_guard<std::mutex>   lock(m_image_mutex);

    // check to see if the IMAGE "list" is "valid".
    if (!m_image_list.empty())
    {
        // if so, attempt to "remove" and "deallocate" the IMAGE for the given JOB_ID.
        m_image_list.remove_if([colorant, job_id](auto & entry)
        {
            // check to see if the IMAGE "entry" is associated with the given JOB_ID.
            return(entry.job_id == job_id);
        });
    }

    FIERY::FITS::MODULE::LOG::MANAGER::LogNormal("FITS-Module[" + m_colorant + "]: IMAGE CANCELED (JobId = " + job_id + ", Length = " + std::to_string(m_image_list.size()) + ")");

    // return the status.
    return(successful);
}

bool MANAGER::get(const std::string & job_id, const std::string & image_id, IMAGE & image)
{
    bool   successful = false;


    // check to see if the IMAGE "list" is "valid".
    if (!m_image_list.empty())
    {
        // if so, loop through the IMAGE "list", attempting to "find" the "appropriate" IMAGE.
        for (auto iterator = m_image_list.begin(); iterator != m_image_list.end(); iterator++)
        {
            // check to see if the JOB_ID and IMAGE_ID "match".
            if ((iterator->job_id == job_id) && (iterator->image_id == image_id))
            {
                // if so, update the return variables and break out of the "for" loop.
                image      = *iterator;
                successful = true;
                break;
            }
        }
    }

    // return the status.
    return(successful);
}

PROPERTIES::MANAGER::MANAGER()
{
    ;
}

PROPERTIES::MANAGER::~MANAGER()
{
    ;
}

bool PROPERTIES::MANAGER::Abort(void)
{
    bool   successful = true;


    // attempt to "take" the JOB_IMAGE_PROPERTIES_MAP_MAP "mutex".
    std::lock_guard<std::mutex>   lock(m_job_image_properties_map_map_mutex);

    // attempt to "delete" the JOB_IMAGE_PROPERTIES_MAP_MAP.
    m_job_image_properties_map_map.clear();

    // return the status.
    return(successful);
}

bool PROPERTIES::MANAGER::ImagePropertiesCreate(const std::string & job_id, const std::string & image_id, const boost::property_tree::ptree & json_object_image_properties)
{
    std::stringstream   string_stream;

    bool                successful = true;


    // attempt to "take" the JOB_IMAGE_PROPERTIES_MAP_MAP "mutex".
    std::lock_guard<std::mutex>   lock(m_job_image_properties_map_map_mutex);

    // attempt to "find" the JOB.
    auto iterator_job = m_job_image_properties_map_map.find(job_id);

    // check to see if the JOB "entry" already "exists".
    if (iterator_job != m_job_image_properties_map_map.end())
    {
        // if so, attempt to "add" the IMAGE_PROPERTIES to the JOB's IMAGE_PROPERTIES "map".
        iterator_job->second[image_id] = json_object_image_properties;
    }
    else
    {
        // otherwise, initialize the local variables.
        IMAGE_PROPERTIES_MAP   image_properties_map;

        // "add" the IMAGE_PROPERTIES to the IMAGE_PROPERTIES "map".
        image_properties_map[image_id] = json_object_image_properties;

        // attempt to "add" the IMAGE_PROPERTIES "map" to the JOB "map".
        m_job_image_properties_map_map[job_id] = image_properties_map;
    }

    // attempt to "write" the IMAGE_PROPERTIES to a "string" -- for logging purposes.
    boost::property_tree::json_parser::write_json(string_stream, json_object_image_properties, false);

    FIERY::FITS::MODULE::LOG::MANAGER::LogNormal("FITS-Module: IMAGE_PROPERTIES (JobId = " + job_id + ", ImageId = " + image_id + ", Properties = " + string_stream.str() + ")");

    // return the status.
    return(successful);
}

bool PROPERTIES::MANAGER::ImagePropertiesDelete(const std::string & job_id, const std::string & image_id)
{
    bool   successful = false;


    // attempt to "take" the JOB_IMAGE_PROPERTIES_MAP_MAP "mutex".
    std::lock_guard<std::mutex>   lock(m_job_image_properties_map_map_mutex);

    // attempt to "find" the JOB.
    auto iterator_job = m_job_image_properties_map_map.find(job_id);

    // check to see if the JOB "entry" already "exists".
    if (iterator_job != m_job_image_properties_map_map.end())
    {
        // if so, attempt to "remove" the IMAGE_PROPERTIES associated with the given IMAGE from the JOB's IMAGE_PROPERTIES "map".
        successful = (iterator_job->second.erase(image_id) > 0);

        // check to see if the JOB's IMAGE_PROPERTIES "map" is "empty" -- if so, attempt to "remove" the JOB.
        if (iterator_job->second.size() == 0)
        {
            m_job_image_properties_map_map.erase(job_id);
        }
    }

    // return the status.
    return(successful);
}

bool PROPERTIES::MANAGER::ImagePropertiesGet(const std::string & job_id, const std::string & image_id, boost::property_tree::ptree & json_object_image_properties)
{
    bool   successful = false;


    // initialize the return variables.
    json_object_image_properties.clear();

    // attempt to "take" the JOB_IMAGE_PROPERTIES_MAP_MAP "mutex".
    std::lock_guard<std::mutex>   lock(m_job_image_properties_map_map_mutex);

    // attempt to "find" the JOB.
    auto iterator_job = m_job_image_properties_map_map.find(job_id);

    // check to see if the JOB "entry" already "exists".
    if (iterator_job != m_job_image_properties_map_map.end())
    {
        // if so, attempt to "find" the IMAGE.
        auto iterator_image = iterator_job->second.find(image_id);

        // check to see if the IMAGE "entry" already "exists" -- if so, update the return variables.
        if (iterator_image != iterator_job->second.end())
        {
            json_object_image_properties = iterator_image->second;
            successful                   = true;
        }
    }

    // return the status.
    return(successful);
}

bool PROPERTIES::MANAGER::JobCancel(const std::string & job_id)
{
    bool   successful = false;


    // attempt to "take" the JOB_IMAGE_PROPERTIES_MAP_MAP "mutex".
    std::lock_guard<std::mutex>   lock(m_job_image_properties_map_map_mutex);

    // if so, attempt to "remove" the JOB from the JOB_IMAGE_PROPERTIES_MAP "map".
    successful = (m_job_image_properties_map_map.erase(job_id) > 0);

    // return the status.
    return(successful);
}