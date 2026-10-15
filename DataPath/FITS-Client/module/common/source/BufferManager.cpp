/**********************************************************************************************************************************************************************************/
/*                                                                                                                                                                                */
/*  Copyright (c) 1990-2023 Fiery, LLC.  All Rights Reserved.                                                                                                                     */
/*                                                                                                                                                                                */
/*  Fiery, LLC ("Fiery") products contain certain trade secrets and confidential and proprietary information of Fiery.  Use, reproduction, disclosure, distribution by any means  */
/*  are prohibited, except pursuant to a written license from Fiery.  Modification, translation, reverse engineering, decompiling, disassembling, and creating derivative works   */
/*  based on this software are prohibited, except pursuant to a written license from Fiery. Use of copyright notice does not imply publication or disclosure.                     */
/*                                                                                                                                                                                */
/**********************************************************************************************************************************************************************************/
#include "BufferManager.h"

#include "ColorantManager.h"
#include "LogManager.h"

#include <chrono>
#include <thread>

using namespace FIERY::FITS::MODULE::BUFFER;


/**********************************************************************************************************************************************************************************/
/*                                                         local typedefs, structures, enumerations, and related defines                                                          */
/**********************************************************************************************************************************************************************************/


/**********************************************************************************************************************************************************************************/
/*                                                                                local variables                                                                                 */
/**********************************************************************************************************************************************************************************/
const uint32_t   MANAGER::SLEEP_IN_MILLISECONDS_CANCEL = 50;


/**********************************************************************************************************************************************************************************/
/*                                                                                     methods                                                                                    */
/**********************************************************************************************************************************************************************************/
MANAGER::MANAGER(const std::string & colorant, const uint64_t & buffer_pool_size_in_bytes) : m_colorant(colorant), m_buffer_list(), m_initialized(false), m_mutex(),
            m_p_pool_end(nullptr), m_p_pool_start(nullptr), m_p_pool_window_end(nullptr), m_p_pool_window_start(nullptr), m_pool_size_in_bytes(buffer_pool_size_in_bytes),
            m_pool_size_in_bytes_available_left_of_window(0), m_pool_size_in_bytes_available_right_of_window(0), m_pool_size_in_bytes_used(0), m_signal_deallocated(),
            m_throttled(false)
{
    ;
}

MANAGER::~MANAGER()
{
    MANAGER *   p_buffer_mgr = this;
    bool        successful   = true;


    // attempt to "take" the BUFFER "mutex".
    std::lock_guard<std::mutex>   lock(m_mutex);

    // check to see if the BUFFER "list" is "valid".
    if (!m_buffer_list.empty())
    {
        // if so, attempt to "remove" and "deallocate" every BUFFER.
        m_buffer_list.remove_if([p_buffer_mgr](auto & entry)
        {
            uint64_t      buffer_size_in_bytes = entry.buffer_size_in_bytes;
            std::string   image_id             = entry.image_id;
            std::string   job_id               = entry.job_id;
            bool          remove               = false;


            // attempt to "deallocate" the BUFFER.
            if (p_buffer_mgr->deallocate(entry))
            {
                FIERY::FITS::MODULE::LOG::MANAGER::LogNormal("FITS-Module[" + p_buffer_mgr->m_colorant + "]: BUFFER DELETED (JobId = " + job_id + ", ImageId = " + image_id +
                            ", Size = (Current = " + std::to_string(buffer_size_in_bytes) + "))");

                // if successful, update the status.
                remove = true;
            }

            // return the status.
            return(remove);
        });
    }

    // attempt to "deallocate" the BUFFER "pool".
    poolDeallocate();

    FIERY::FITS::MODULE::LOG::MANAGER::LogNormal("FITS-Module[" + m_colorant + "]: BUFFER POOL DELETED");
}

bool MANAGER::BufferCreateFromPool(const std::string & job_id, const std::string & image_id, const std::vector<uint8_t> & image_data, BUFFER & buffer)
{
    uint64_t   buffer_size_in_bytes = image_data.size();
    bool       successful           = false;


    // attempt to "take" the BUFFER "mutex".
    std::unique_lock<std::mutex>   lock(m_mutex);

    // check to see if the BUFFER "size" is "available" in the MANAGER.
    if ((buffer_size_in_bytes > m_pool_size_in_bytes_available_left_of_window) && (buffer_size_in_bytes > m_pool_size_in_bytes_available_right_of_window))
    {
        FIERY::FITS::MODULE::LOG::MANAGER::LogNormal("FITS-Module[" + m_colorant + "]: BUFFER THROTTLING (JobId = " + job_id + ", ImageId = " + image_id + ", Size = (Current = " +
                    std::to_string(buffer_size_in_bytes) + ", Total = " + std::to_string(m_pool_size_in_bytes_used) + "/" + std::to_string(m_pool_size_in_bytes) +
                    ", Window = (Left = " + std::to_string(m_pool_size_in_bytes_available_left_of_window) + ", Right = " +
                    std::to_string(m_pool_size_in_bytes_available_right_of_window) + ")))");

        // if not, update the local variables.
        m_throttled = true;

        // loop continuously -- checking to see if the BUFFER "size" is "available" in the MANAGER or the JOB is "canceled".
        while ((buffer_size_in_bytes > m_pool_size_in_bytes_available_left_of_window) && (buffer_size_in_bytes > m_pool_size_in_bytes_available_right_of_window) &&
                    !FIERY::FITS::MODULE::COLORANT::MANAGER::IsJobCanceled(job_id))
        {
            // if not, attempt to "wait" for the "deallocated" BUFFER "signal".
            m_signal_deallocated.wait(lock);
        }

        // update the local variables.
        m_throttled = false;
    }

    // check to see if the JOB is "canceled".
    if (!FIERY::FITS::MODULE::COLORANT::MANAGER::IsJobCanceled(job_id))
    {
        // if not, attempt to "allocate" the BUFFER.
        if (allocate(job_id, image_id, image_data, buffer))
        {
            // attempt to "add" the BUFFER to the "list".
            m_buffer_list.push_back(buffer);

            // attempt to "update" the BUFFER_MANAGER "pool" information.
            poolUpdate();

            FIERY::FITS::MODULE::LOG::MANAGER::LogNormal("FITS-Module[" + m_colorant + "]: BUFFER CREATED (JobId = " + job_id + ", ImageId = " + image_id + ", Size = (Current = " +
                        std::to_string(buffer_size_in_bytes) + ", Total = " + std::to_string(m_pool_size_in_bytes_used) + "/" + std::to_string(m_pool_size_in_bytes) +
                        ", Window = (Left = " + std::to_string(m_pool_size_in_bytes_available_left_of_window) + ", Right = " +
                        std::to_string(m_pool_size_in_bytes_available_right_of_window) + ")))");

            // update the status.
            successful = true;
        }
    }
    else
    {
        FIERY::FITS::MODULE::LOG::MANAGER::LogNormal("FITS-Module[" + m_colorant + "]: BUFFER CANCELED (JobId = " + job_id + ", ImageId = " + image_id + ", Size = (Current = " +
                    std::to_string(buffer_size_in_bytes) + ", Total = " + std::to_string(m_pool_size_in_bytes_used) + "/" + std::to_string(m_pool_size_in_bytes) +
                    ", Window = (Left = " + std::to_string(m_pool_size_in_bytes_available_left_of_window) + ", Right = " +
                    std::to_string(m_pool_size_in_bytes_available_right_of_window) + ")))");
    }

    // return the status.
    return(successful);
}

bool MANAGER::BufferDeleteFromPool(const std::string & job_id, const std::string & image_id)
{
    uint64_t    buffer_size_in_bytes = 0;
    bool        deallocated          = false;
    MANAGER *   p_buffer_mgr         = this;
    bool        successful           = false;


    // attempt to "take" the BUFFER "mutex".
    std::lock_guard<std::mutex>   lock(m_mutex);

    // check to see if the BUFFER "list" is "valid".
    if (!m_buffer_list.empty())
    {
        // if so, initialize the local vriables.
        uint64_t   buffer_list_size_initial = m_buffer_list.size();

        // attempt to "remove" and "deallocate" the BUFFER for the given JOB_ID.
        m_buffer_list.remove_if([p_buffer_mgr, job_id, image_id, &deallocated, &buffer_size_in_bytes](auto & entry)
        {
            bool  remove = false;


            // check to see if the BUFFER "entry" is associated with the given JOB_ID and IMAGE_ID.
            if ((entry.job_id == job_id) && (entry.image_id == image_id))
            {
                // if so, update the return variables.
                buffer_size_in_bytes = entry.buffer_size_in_bytes;

                // attempt to "deallocate" the BUFFER -- if successful, update the status.
                if (p_buffer_mgr->deallocate(entry))
                {
                    deallocated = true;
                    remove      = true;
                }
            }

            // return the status.
            return(remove);
        });

        // check to see if we "deallocated" a BUFFER.
        if (deallocated)
        {
            // if so, attempt to "update" the BUFFER_MANAGER "pool" information.
            poolUpdate();

            FIERY::FITS::MODULE::LOG::MANAGER::LogNormal("FITS-Module[" + m_colorant + "]: BUFFER DELETED (JobId = " + job_id + ", ImageId = " + image_id + ", Length = " +
                        std::to_string(m_buffer_list.size()) + ", Size = (Current = " + std::to_string(buffer_size_in_bytes) + ", Total = " +
                        std::to_string(m_pool_size_in_bytes_used) + "/" + std::to_string(m_pool_size_in_bytes) + ", Window = (Left = " +
                        std::to_string(m_pool_size_in_bytes_available_left_of_window) + ", Right = " + std::to_string(m_pool_size_in_bytes_available_right_of_window) + ")))");

            // attempt to "signal" the change.
            m_signal_deallocated.notify_all();
        }
    }

    // return the status.
    return(successful);
}

bool MANAGER::Initialize(void)
{
    // check to see if we need to "initialize" the BUFFER_MANAGER.
    if (!m_initialized)
    {
        // attempt to "allocate" the BUFFER "pool".
        if (poolAllocate())
        {
            // if successful, check to see if the BUFFER_MANAGER "pool" is "valid".
            if (m_p_pool_start != nullptr)
            {
                // if so, update the "management" variables.
                m_p_pool_end = m_p_pool_start + m_pool_size_in_bytes;

                // attempt to "update" the BUFFER_MANAGER "pool" information.
                poolUpdate();

                // update the "management" variables.
                m_initialized = true;
            }

            FIERY::FITS::MODULE::LOG::MANAGER::LogNormal("FITS-Module[" + m_colorant + "]: BUFFER POOL CREATED (Size = (Total = " + std::to_string(m_pool_size_in_bytes_used) +
                        "/" + std::to_string(m_pool_size_in_bytes) + ", Window = (Left = " + std::to_string(m_pool_size_in_bytes_available_left_of_window) + ", Right = " +
                        std::to_string(m_pool_size_in_bytes_available_right_of_window) + ")))");
        }
    }

    // return the status.
    return(m_initialized);
}

bool MANAGER::JobCancel(const std::string & job_id)
{
    bool        deallocated  = false;
    MANAGER *   p_buffer_mgr = this;
    bool        successful   = false;


    FIERY::FITS::MODULE::LOG::MANAGER::LogNormal("FITS-Module[" + m_colorant + "]: BUFFER CANCEL (JobId = " + job_id + ", Length = " + std::to_string(m_buffer_list.size()) + ")");

    // check to see if the BUFFER "list" is "throttled" -- if so, attempt to "signal" the change.
    if (m_throttled)
    {
        m_signal_deallocated.notify_all();
    }

    // attempt to "sleep" for a bit -- to let outstanding BUFFER "creates" for this JOB to complete.
    std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_IN_MILLISECONDS_CANCEL));

    // attempt to "take" the BUFFER "mutex".
    std::lock_guard<std::mutex>   lock(m_mutex);

    // check to see if the BUFFER "list" is "valid".
    if (!m_buffer_list.empty())
    {
        // if so, initialize the local vriables.
        uint64_t   buffer_list_size_initial = m_buffer_list.size();

        // attempt to "remove" and "deallocate" the BUFFER for the given JOB_ID.
        m_buffer_list.remove_if([p_buffer_mgr, job_id, &deallocated](auto & entry)
        {
            bool  remove = false;


            // check to see if the BUFFER "entry" is associated with the given JOB_ID.
            if (entry.job_id == job_id)
            {
                uint64_t    buffer_size_in_bytes = entry.buffer_size_in_bytes;


                // if so, attempt to "deallocate" the BUFFER.
                if (p_buffer_mgr->deallocate(entry))
                {
                    FIERY::FITS::MODULE::LOG::MANAGER::LogNormal("FITS-Module[" + p_buffer_mgr->m_colorant + "]: BUFFER CANCELED (JobId = " + job_id + ", Size = (Current = " +
                                std::to_string(buffer_size_in_bytes) + "))");

                    // if successful, update the status.
                    deallocated = true;
                    remove      = true;
                }
            }

            // return the status.
            return(remove);
        });

        // check to see if we "deallocated" any BUFFERs.
        if (deallocated)
        {
            // if so, attempt to "update" the BUFFER_MANAGER "pool" information.
            poolUpdate();

            FIERY::FITS::MODULE::LOG::MANAGER::LogNormal("FITS-Module[" + m_colorant + "]: BUFFER CANCELED (JobId = " + job_id + ", Size = (Total = " +
                        std::to_string(m_pool_size_in_bytes_used) + "/" + std::to_string(m_pool_size_in_bytes) + ", Window = (Left = " +
                        std::to_string(m_pool_size_in_bytes_available_left_of_window) + ", Right = " + std::to_string(m_pool_size_in_bytes_available_right_of_window) + ")))");

            // attempt to "signal" the change.
            m_signal_deallocated.notify_all();
        }
    }

    FIERY::FITS::MODULE::LOG::MANAGER::LogNormal("FITS-Module[" + m_colorant + "]: BUFFER CANCELED (JobId = " + job_id + ", Length = " + std::to_string(m_buffer_list.size()) +
                ")");

    // return the status.
    return(successful);
}

bool MANAGER::allocate(const std::string & job_id, const std::string & image_id, const std::vector<uint8_t> & image_data, BUFFER & buffer)
{
    uint64_t    buffer_size_in_bytes = image_data.size();
    uint8_t *   p_buffer_back        = m_p_pool_start;
    uint8_t *   p_buffer_front       = m_p_pool_start;
    bool        successful           = false;


    // initialize the return variables.
    buffer.clear();

    // check to see if the BUFFER "list" is "valid" -- if so, update the local variables.
    if (!m_buffer_list.empty())
    {
        p_buffer_back  = m_buffer_list.back().p_buffer;
        p_buffer_front = m_buffer_list.front().p_buffer;
    }

    // check to see if the BUFFER can be allocated from the "right" BUFFER_MANAGER "pool" AND the BUFFER "list" has NOT "wrapped around" the BUFFER_MANAGER "pool" (because we
    // always want to append the next BUFFER to the end of the "ring" BUFFER).
    if ((buffer_size_in_bytes <= m_pool_size_in_bytes_available_right_of_window) && (p_buffer_front <= p_buffer_back))
    {
        // if so, update the return variables.
        buffer.p_buffer             = m_p_pool_window_end;
        buffer.buffer_size_in_bytes = buffer_size_in_bytes;
        successful                  = true;
    }
    else if (buffer_size_in_bytes <= m_pool_size_in_bytes_available_left_of_window)
    {
        // otherwise, update the return variables (based on whether of not the BUFFER "list" has NOT "wrapped around" the BUFFER_MANAGER "pool".
        buffer.p_buffer             = (p_buffer_front <= p_buffer_back) ? m_p_pool_start : m_p_pool_window_end;
        buffer.buffer_size_in_bytes = buffer_size_in_bytes;
        successful                  = true;
    }

    // check to see if everything was successful.
    if (successful)
    {
        // attempt to "copy" the IMAGE "data" to the BUFFER "pool".
        if (successful = poolCopy(image_data.data(), buffer_size_in_bytes, buffer.p_buffer))
        {
            // if successful, update the return variables.
            buffer.job_id   = job_id;
            buffer.image_id = image_id;

            // update the "management" variables.
            m_pool_size_in_bytes_used += buffer_size_in_bytes;
        }
    }
        
    // return the status.
    return(successful);
}

bool MANAGER::deallocate(BUFFER & buffer)
{
    bool   successful = false;


    // check to see if the BUFFER is "valid".
    if (buffer.p_buffer != nullptr)
    {
        // update the "management" variables.
        m_pool_size_in_bytes_used -= buffer.buffer_size_in_bytes;

        // update the return variables.
        buffer.clear();
        successful = true;
    }

    // return the status.
    return(successful);
}

bool MANAGER::get(const std::string & job_id, const std::string & image_id, BUFFER & buffer)
{
    bool   successful = false;


    // check to see if the BUFFER "list" is "valid".
    if (!m_buffer_list.empty())
    {
        // if so, loop through the BUFFER "list", attempting to "find" the "appropriate" BUFFER.
        for (auto iterator = m_buffer_list.begin(); iterator != m_buffer_list.end(); iterator++)
        {
            // check to see if the JOB_ID and IMAGE_ID "match".
            if ((iterator->job_id == job_id) && (iterator->image_id == image_id))
            {
                // if so, update the return variables.
                buffer     = *iterator;
                successful = true;
            }
        }
    }

    // check to see if everything was successful -- if not, attempt to "log" the ERROR.
    if (!successful)
    {
        FIERY::FITS::MODULE::LOG::MANAGER::LogError("FITS-Module[" + m_colorant + "]: get FAILED (JobId = " + job_id + ", ImageId = " + image_id + ")");
    }

    // return the status.
    return(successful);
}

bool MANAGER::poolAllocate(void)
{
    bool   successful = false;


    /*************************************************************************************************************************************************/
    /*                                                                INSERT CODE HERE                                                               */
    /*                                                                                                                                               */
    /* PURPOSE: ALLOCATE THE MEMORY POOL USED TO STORE IMAGE DATA THAT WILL EVENTUALLY BE TRANSFERRED TO THE PRESS ELECTRONICS.                      */
    /* NOTES:   THIS MAY BE HEAP MEMORY (AS THE EXAMPLE BELOW USES), RESERVED MEMORY, OR SOME OTHER MECHANISM REQUIRED BY YOUR PRESS ELECTRONICS.    */
    /*                                                                                                                                               */
    /*************************************************************************************************************************************************/
    // check to see if we need to "allocate" the BUFFER_MANAGER "pool" -- if so, do it.
    if (m_p_pool_start == nullptr)
    {
        m_p_pool_start = new uint8_t[m_pool_size_in_bytes];
    }

    // update the status.
    successful = (m_p_pool_start != nullptr);

    // return the status.
    return(successful);
}

bool MANAGER::poolCopy(const uint8_t * p_buffer_source, const uint64_t & number_of_bytes_to_copy, uint8_t * p_buffer_destination)
{
    bool   successful = true;


    /*************************************************************************************************************************************************/
    /*                                                                INSERT CODE HERE                                                               */
    /*                                                                                                                                               */
    /* PURPOSE: COPY THE IMAGE DATA TO THE MEMORY BUFFER THAT WILL EVENTUALLY BE USED TO TRANSFER IMAGE DATA TO THE PRESS ELECTRONICS.               */
    /* NOTES:   THIS MAY BE HEAP MEMORY (AS THE EXAMPLE BELOW USES), RESERVED MEMORY, OR SOME OTHER MECHANISM REQUIRED BY YOUR PRESS ELECTRONICS.    */
    /*                                                                                                                                               */
    /*************************************************************************************************************************************************/
    // attempt to "copy" the IMAGE_DATA.
    std::memcpy(p_buffer_destination, p_buffer_source, number_of_bytes_to_copy);

    // return the status.
    return(successful);
}
bool MANAGER::poolDeallocate(void)
{
    bool   successful = false;


    /*************************************************************************************************************************************************/
    /*                                                                INSERT CODE HERE                                                               */
    /*                                                                                                                                               */
    /* PURPOSE: DEALLOCATE THE MEMORY POOL USED TO STORE IMAGE DATA THAT WILL EVENTUALLY BE TRANSFERRED TO THE PRESS ELECTRONICS.                    */
    /* NOTES:   THIS MAY BE HEAP MEMORY (AS THE EXAMPLE BELOW USES), RESERVED MEMORY, OR SOME OTHER MECHANISM REQUIRED BY YOUR PRESS ELECTRONICS.    */
    /*                                                                                                                                               */
    /*************************************************************************************************************************************************/
    // check to see if we need to "deallocate" the BUFFER_MANAGER "pool" -- if so, do it.
    if (m_p_pool_start != nullptr)
    {
        delete m_p_pool_start;
        m_p_pool_start = nullptr;
    }

    // update the status.
    successful = (m_p_pool_start == nullptr);

    // return the status.
    return(successful);
}

void MANAGER::poolUpdate(void)
{
    // check to see if the BUFFER "list" is "valid".
    if (m_buffer_list.empty())
    {
        // if so, "reset" the BUFFER_MANAGER "pool" information.
        m_p_pool_window_start                          = m_p_pool_start;
        m_p_pool_window_end                            = m_p_pool_start;
        m_pool_size_in_bytes_available_left_of_window  = 0;
        m_pool_size_in_bytes_available_right_of_window = m_pool_size_in_bytes;
    }
    else
    {
        // otherwise, initialize the local variables.
        BUFFER &    buffer_back                = m_buffer_list.back();
        BUFFER &    buffer_front               = m_buffer_list.front();
        uint64_t    buffer_size_in_bytes_back  = buffer_back.buffer_size_in_bytes;
        uint64_t    buffer_size_in_bytes_front = buffer_front.buffer_size_in_bytes;
        uint8_t *   p_buffer_back              = buffer_back.p_buffer;
        uint8_t *   p_buffer_front             = buffer_front.p_buffer;

        // update the BUFFER_MANAGER "pool" information.
        m_p_pool_window_start = p_buffer_front;
        m_p_pool_window_end   = p_buffer_back + buffer_size_in_bytes_back;

        // check to see if the BUFFER "list" has "wrapped around" the BUFFER_MANAGER "pool".
        if (p_buffer_front > p_buffer_back)
        {
            // if so, update the BUFFER_MANAGER "pool" information.
            m_pool_size_in_bytes_available_left_of_window = m_p_pool_window_start - m_p_pool_window_end;
        }
        else
        {
            // otherwise, update the BUFFER_MANAGER "pool" information.
            m_pool_size_in_bytes_available_left_of_window  = m_p_pool_window_start - m_p_pool_start;
            m_pool_size_in_bytes_available_right_of_window = m_p_pool_end - m_p_pool_window_end;
        }
    }
}