/**********************************************************************************************************************************************************************************/
/*                                                                                                                                                                                */
/*  Copyright (c) 1990-2023 Fiery, LLC.  All Rights Reserved.                                                                                                                     */
/*                                                                                                                                                                                */
/*  Fiery, LLC ("Fiery") products contain certain trade secrets and confidential and proprietary information of Fiery.  Use, reproduction, disclosure, distribution by any means  */
/*  are prohibited, except pursuant to a written license from Fiery.  Modification, translation, reverse engineering, decompiling, disassembling, and creating derivative works   */
/*  based on this software are prohibited, except pursuant to a written license from Fiery. Use of copyright notice does not imply publication or disclosure.                     */
/*                                                                                                                                                                                */
/**********************************************************************************************************************************************************************************/
#include "ImageManager.h"

#include "LogManager.h"

using namespace FIERY::FITS::MODULE::IMAGE;


/**********************************************************************************************************************************************************************************/
/*                                                         local typedefs, structures, enumerations, and related defines                                                          */
/**********************************************************************************************************************************************************************************/


/**********************************************************************************************************************************************************************************/
/*                                                                                local variables                                                                                 */
/**********************************************************************************************************************************************************************************/


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
