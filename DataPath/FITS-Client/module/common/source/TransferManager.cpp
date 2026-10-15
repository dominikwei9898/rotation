/**********************************************************************************************************************************************************************************/
/*                                                                                                                                                                                */
/*  Copyright (c) 1990-2023 Fiery, LLC.  All Rights Reserved.                                                                                                                     */
/*                                                                                                                                                                                */
/*  Fiery, LLC ("Fiery") products contain certain trade secrets and confidential and proprietary information of Fiery.  Use, reproduction, disclosure, distribution by any means  */
/*  are prohibited, except pursuant to a written license from Fiery.  Modification, translation, reverse engineering, decompiling, disassembling, and creating derivative works   */
/*  based on this software are prohibited, except pursuant to a written license from Fiery. Use of copyright notice does not imply publication or disclosure.                     */
/*                                                                                                                                                                                */
/**********************************************************************************************************************************************************************************/
#include "TransferManager.h"

#include "ColorantManager.h"
#include "LogManager.h"

#include <boost/algorithm/string.hpp>

using namespace FIERY::FITS::MODULE::TRANSFER;


/**********************************************************************************************************************************************************************************/
/*                                                         local typedefs, structures, enumerations, and related defines                                                          */
/**********************************************************************************************************************************************************************************/


/**********************************************************************************************************************************************************************************/
/*                                                                                local variables                                                                                 */
/**********************************************************************************************************************************************************************************/


/**********************************************************************************************************************************************************************************/
/*                                                                                     methods                                                                                    */
/**********************************************************************************************************************************************************************************/
REQUEST::REQUEST(const std::string & job_id, const std::string & job_name, const std::string & image_id, const std::list<std::string> & colorant_list) :
            m_colorant_requested_list(colorant_list), m_count(0), m_count_maximum(colorant_list.size()), m_count_signal(), m_image_id(image_id), m_job_id(job_id),
            m_job_name(job_name), m_mutex()
{
    ;
}

REQUEST::~REQUEST()
{
    ;
}

bool REQUEST::IsColorantRequired(const std::string & colorant)
{
    bool   successful = false;


    // loop through the "requested" COLORANT "list", attempting to "find" the given COLORANT.
    for (auto iterator = m_colorant_requested_list.begin(); iterator != m_colorant_requested_list.end(); iterator++)
    {
        // check to see if the COLORANT "matches".
        if (*iterator == colorant)
        {
            // if so, update the local variables and break out of the "for" loop.
            successful = true;
            break;
        }
    }

    // return the status.
    return(successful);
}

bool REQUEST::Transferred(const std::string & colorant)
{
    bool   found      = false;
    bool   successful = false;


    // loop through the "requested" COLORANT "list", attempting to "find" the "appropriate" IMAGE.
    for (auto iterator = m_colorant_requested_list.begin(); iterator != m_colorant_requested_list.end(); iterator++)
    {
        // check to see if the COLORANT "matches".
        if (*iterator == colorant)
        {
            // if so, update the local variables and break out of the "for" loop.
            found = true;
            break;
        }
    }

    // check to see if the COLORANT was "found".
    if (found)
    {
        // if so, attempt to "take" the REQUEST "mutex".
        std::lock_guard<std::mutex>   lock(m_mutex);

        // attempt to "update" the REQUEST.
        m_count++;

        // attempt to "signal" the change.
        m_count_signal.notify_all();

        // update the status.
        successful = true;
        FIERY::FITS::MODULE::LOG::MANAGER::LogNormal("FITS-Module[" + colorant + "]: TRANSFERRED (JobId = " + m_job_id + ", ImageId = " + m_image_id + ")");
    }

    // return the status.
    return(successful);
}

bool REQUEST::WaitForTransferred(void)
{
    bool   successful = true;


    // attempt to "take" the REQUEST "mutex".
    std::unique_lock<std::mutex>   lock(m_mutex);

    // loop continuously, waiting for the REQUEST to be "transferred".
    while (m_count != m_count_maximum)
    {
        // if unsuccessful, attempt to "wait" for the REQUEST "signal".
        m_count_signal.wait(lock);
    }

    // return the status.
    return(successful);
}

MANAGER::MANAGER() : m_output_properties(), m_p_module_mgr(nullptr), m_p_output_plug_in(nullptr), m_request_list(), m_request_mutex(), m_request_signal(), m_thread(),
            m_thread_enabled(true)
{
    ;
}

MANAGER::~MANAGER()
{
    ;
}

bool MANAGER::JobCancel(const std::string & job_id)
{
    bool   successful = true;


    // attempt to "take" the REQUEST "mutex".
    std::lock_guard<std::mutex>   lock(m_request_mutex);

    // check to see if the REQUEST "list" is "valid".
    if (!m_request_list.empty())
    {
        // if so, attempt to "remove" and "deallocate" the REQUEST for the given JOB_ID.
        m_request_list.remove_if([job_id](auto & entry)
        {
            bool  remove = false;


            // check to see if the REQUEST "entry" is associated with the given JOB_ID.
            if (entry->JobIdGet() == job_id)
            {
                FIERY::FITS::MODULE::LOG::MANAGER::LogNormal("FITS-Module: REQUEST CANCELED (JobId = " + job_id + ", ImageId = " + entry->ImageIdGet() + ")");

                // if successful, update the status.
                remove = true;
            }

            // return the status.
            return(remove);
        });
    }

    FIERY::FITS::MODULE::LOG::MANAGER::LogNormal("FITS-Module: REQUEST CANCELED (JobId = " + job_id + ", Length = " + std::to_string(m_request_list.size()) + ")");

    // return the status.
    return(successful);
}

bool MANAGER::Start(FIERY::FITS::MODULE::MANAGER * p_module_mgr, const boost::shared_ptr<FIERY::FITS::OUTPUT::PLUG_IN> & p_output_plug_in, const FIERY::FITS::OUTPUT::PROPERTIES::OUTPUT & output_properties)
{
    bool   successful = false;


    // check the arguments.
    if (p_module_mgr != nullptr)
    {
        // initialize the "management" variables.
        m_output_properties = output_properties;
        m_p_module_mgr      = p_module_mgr;
        m_p_output_plug_in  = p_output_plug_in;

        // attempt to "run" the MANAGER.
        m_thread = std::thread(std::bind(&MANAGER::run, this));

        // update the status.
        successful = true;
    }

    // return the status.
    return(successful);
}

bool MANAGER::Stop(void)
{
    bool   successful = true;


    // attempt to "wait" for the MANAGER to "stop".
    waitForStop();

    FIERY::FITS::MODULE::LOG::MANAGER::LogNormal("FITS-Module[TransferManager]: STOPPED");

    // return the status.
    return(successful);
}

bool MANAGER::RequestCreate(const std::string & job_id, const std::string & job_name, const std::string & image_id, const std::list<std::string> & colorant_list, REQUEST * & p_request)
{
    bool   successful = false;


    // check to see if the JOB is "canceled".
    if (!FIERY::FITS::MODULE::COLORANT::MANAGER::IsJobCanceled(job_id))
    {
        // if not, attempt to "create" the REQUEST.
        if ((p_request = new REQUEST(job_id, job_name, image_id, colorant_list)) != nullptr)
        {
            // if successful, attempt to "take" the REQUEST "mutex".
            std::lock_guard<std::mutex>   lock(m_request_mutex);

            // attempt to "add" the REQUEST to the REQUEST "list".
            m_request_list.push_back(p_request);

            FIERY::FITS::MODULE::LOG::MANAGER::LogNormal("FITS-Module: PRINT (JobId = " + job_id + ", ImageId = " + image_id + ", Colorants = <" +
                        boost::algorithm::join(colorant_list, ",") + ">, Length = " + std::to_string(m_request_list.size()) + ")");

            // attempt to "signal" the change.
            m_request_signal.notify_all();

            // update the status.
            successful = true;
        }
    }
    else
    {
        FIERY::FITS::MODULE::LOG::MANAGER::LogNormal("FITS-Module: PRINT Canceled (JobId = " + job_id + ", ImageId = " + image_id + ")");
    }

    // return the status.
    return(successful);
}

bool MANAGER::requestGet(REQUEST * & p_transfer_request)
{
    bool   successful = false;


    // initialize the return vaiables.
    p_transfer_request = nullptr;

    // attempt to "take" the REQUEST "mutex".
    std::unique_lock<std::mutex>   lock(m_request_mutex);

    // loop continuously, waiting for a "valid" REQUEST (or the MANAGER to "stop").
    while (m_request_list.empty() && m_thread_enabled)
    {
        // if unsuccessful, attempt to "wait" for the REQUEST "signal".
        m_request_signal.wait(lock);
    }

    // check to see if the REQUEST "list" is "valid".
    if (!m_request_list.empty() && m_thread_enabled)
    {
        // attempt to "get" the "next" REQUEST.
        p_transfer_request = m_request_list.front();

        // attempt to "remove" the REQUEST from the "list".
        m_request_list.pop_front();

        // update the status.
        successful = true;
    }

    // return the status.
    return(successful);
}

void MANAGER::run(void)
{
    REQUEST *   p_transfer_request;


    FIERY::FITS::MODULE::LOG::MANAGER::LogNormal("FITS-Module[TransferManager]: RUNNING");

    // loop continuously while "enabled".
    while (m_thread_enabled)
    {
        // attempt to "get" the "next" REQUEST.
        if (requestGet(p_transfer_request))
        {
            // if successful, check to see if the JOB is "canceled".
            if (!FIERY::FITS::MODULE::COLORANT::MANAGER::IsJobCanceled(p_transfer_request->JobIdGet()))
            {
                // if not, attempt to "wait" for the REQUEST/IMAGE to be "transferred".
                if (p_transfer_request->WaitForTransferred())
                {
                    // if successful, attempt to "wait" for the REQUEST/IMAGE to be "ejected".
                    if (waitForEjected(p_transfer_request))
                    {
                        // if successful, attempt to "send" IMAGE_PRINTED to the WEB_SOCKET "manager".
                        if (m_p_module_mgr->ImagePrinted(p_transfer_request->JobIdGet(), p_transfer_request->ImageIdGet()))
                        {
                            FIERY::FITS::MODULE::LOG::MANAGER::LogNormal("FITS-Module: PRINTED (JobId = " + p_transfer_request->JobIdGet() + ", ImageId = " +
                                        p_transfer_request->ImageIdGet() + ", Colorants = <" + boost::algorithm::join(p_transfer_request->ColorantListGet(), ",") + ">, Length = " +
                                        std::to_string(m_request_list.size()) + ")", true);
                        }
                    }
                }
            }
            else
            {
                FIERY::FITS::MODULE::LOG::MANAGER::LogNormal("FITS-Module: PRINTED Canceled (JobId = " + p_transfer_request->JobIdGet() + ", ImageId = " +
                            p_transfer_request->ImageIdGet() + ", Colorants = <" + boost::algorithm::join(p_transfer_request->ColorantListGet(), ",") + ">, Length = " +
                            std::to_string(m_request_list.size()) + ")");
            }

            // attempt to "delete" the REQUEST.
            delete p_transfer_request;
        }
    }
}

bool MANAGER::waitForEjected(REQUEST * p_transfer_request)
{
    bool   successful = true;


    // check to see if we need to "wait" for the IMAGE "ejected".
    if (m_output_properties.wait_for_ejected_enabled)
    {
        // if so, attempt to "process" the IMAGE "data" -- if unsuccessful, return immediately.
        if (!m_p_output_plug_in->ImageWaitForEjected(p_transfer_request->JobIdGet(), p_transfer_request->ImageIdGet()))
        {
            FIERY::FITS::MODULE::LOG::MANAGER::LogNormal("FITS-Module[TransferManager]: ImageWaitForEjected FAILED");
            return(successful);
        }
    }

    // return the status.
    return(successful);
}

void MANAGER::waitForStop(void)
{
    // update the "management" variables.
    m_thread_enabled = false;

    // attempt to "signal" the change.
    m_request_signal.notify_all();

    // check to see if we can "wait" for the THREAD to "exit" -- if so, do it.
    if (m_thread.joinable())
    {
        m_thread.join();
    }
}