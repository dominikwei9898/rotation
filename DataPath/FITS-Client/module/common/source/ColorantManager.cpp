/**********************************************************************************************************************************************************************************/
/*                                                                                                                                                                                */
/*  Copyright (c) 1990-2026 Fiery, LLC.  All Rights Reserved.                                                                                                                     */
/*                                                                                                                                                                                */
/*  Fiery, LLC ("Fiery") products contain certain trade secrets and confidential and proprietary information of Fiery.  Use, reproduction, disclosure, distribution by any means  */
/*  are prohibited, except pursuant to a written license from Fiery.  Modification, translation, reverse engineering, decompiling, disassembling, and creating derivative works   */
/*  based on this software are prohibited, except pursuant to a written license from Fiery. Use of copyright notice does not imply publication or disclosure.                     */
/*                                                                                                                                                                                */
/**********************************************************************************************************************************************************************************/
#include "ColorantManager.h"

#include "BufferManager.h"
#include "LogManager.h"
#include "RasterManager.h"

#include <boost/dll/import.hpp>
#include <boost/filesystem.hpp>

#include <cstring>
#include <map>
#include <mutex>

using namespace FIERY::FITS::MODULE::COLORANT;


/**********************************************************************************************************************************************************************************/
/*                                                         local typedefs, structures, enumerations, and related defines                                                          */
/**********************************************************************************************************************************************************************************/


/**********************************************************************************************************************************************************************************/
/*                                                                                local variables                                                                                 */
/**********************************************************************************************************************************************************************************/

std::map<std::string, COLORANT *>                 MANAGER::m_colorant_map;
std::mutex                                        MANAGER::m_colorant_mutex;
std::list<MANAGER::JOB_CANCELED>                  MANAGER::m_job_canceled_list;
std::mutex                                        MANAGER::m_job_canceled_mutex;
FIERY::FITS::OUTPUT::PROPERTIES::OUTPUT           MANAGER::m_output_properties;
FIERY::FITS::MODULE::MANAGER *                    MANAGER::m_p_module_mgr;
boost::shared_ptr<FIERY::FITS::OUTPUT::PLUG_IN>   MANAGER::m_p_output_plug_in;
FIERY::FITS::MODULE::PROPERTIES::ROTATION         MANAGER::m_rotation_properties;
FIERY::FITS::MODULE::TRANSFER::MANAGER            MANAGER::m_transfer_mgr;

const uint32_t   MANAGER::TIMEOUT_IN_MILLISECONDS_CANCEL = 5000;


/**********************************************************************************************************************************************************************************/
/*                                                                                     methods                                                                                    */
/**********************************************************************************************************************************************************************************/
COLORANT::COLORANT(const std::string & colorant, const FIERY::FITS::MODULE::PROPERTIES::COLORANT & colorant_properties, const boost::shared_ptr<FIERY::FITS::OUTPUT::PLUG_IN> & p_output_plug_in, const FIERY::FITS::OUTPUT::PROPERTIES::OUTPUT & output_properties, const FIERY::FITS::MODULE::PROPERTIES::ROTATION & rotation_properties) :
            m_buffer_mgr(colorant, colorant_properties.buffer_pool_size_in_bytes, colorant_properties.buffer_pool_delayed_allocate), m_colorant(colorant), m_image_mgr(colorant),
            m_initialized(false), m_output_properties(output_properties), m_p_output_plug_in(p_output_plug_in), m_rotation_properties(rotation_properties), m_thread(),
            m_thread_enabled(true), m_transfer_request_list(), m_transfer_request_mutex(), m_transfer_request_signal()
{
    ;
}

COLORANT::~COLORANT()
{
    // attempt to "wait" for the COLORANT to "stop".
    waitForStop();
}

bool COLORANT::Abort(void)
{
    bool   successful = true;


    FIERY::FITS::MODULE::LOG::MANAGER::LogNormal("FITS-Module[" + m_colorant + "]: COLORANT ABORT");

    // attempt to "abort" all known JOBs in the BUFFER_MANAGER -- if unsuccessful, update the status.
    if (!m_buffer_mgr.Abort())
    {
        successful = false;
    }

    // attempt to "abort" all known JOBs in the IMAGE_MANAGER -- if unsuccessful, update the status.
    if (!m_image_mgr.Abort())
    {
        successful = false;
    }

    // attempt to "abort" all known JOBs in the FITS-OUTPUT "plug-in" -- if unsuccessful, update the status.
    if (!m_p_output_plug_in->Abort())
    {
        successful = false;
    }

    FIERY::FITS::MODULE::LOG::MANAGER::LogNormal("FITS-Module[" + m_colorant + "]: COLORANT ABORTED");

    // return the status.
    return(successful);
}

bool COLORANT::ImageCreate(const FIERY::FITS::MODULE::PROPERTIES::IMAGE & image_properties, const std::vector<uint8_t> & image_data)
{
    FIERY::FITS::MODULE::BUFFER::BUFFER      buffer;
    FIERY::FITS::MODULE::PROPERTIES::IMAGE   image_properties_rotated;
    std::vector<uint8_t>                     image_data_processed;
    std::vector<uint8_t>                     image_data_rotated;

    bool                                      successful = false;


    // check to see if RASTER "rotation" is "enabled" -- if so, attempt to "rotate" the IMAGE "data" (updating the associated IMAGE "properties" to reflect any "width"/"height"
    // change caused by a 90/270 degree rotation) before any further "processing" takes place.  ALL "downstream" code (FITS-Transform/FITS-Output "plug-ins", the BUFFER "pool",
    // and the IMAGE "manager") MUST use the (possibly) ROTATED IMAGE "properties" -- not the ORIGINAL ones -- since the "buffer" layout no longer matches the ORIGINAL IMAGE
    // "properties" otherwise.
    if (m_rotation_properties.enabled && (m_rotation_properties.degrees != 0))
    {
        if (!FIERY::FITS::MODULE::RASTER::MANAGER::Rotate(image_properties, image_data, m_rotation_properties.degrees, image_properties_rotated, image_data_rotated))
        {
            FIERY::FITS::MODULE::LOG::MANAGER::LogError("FITS-Module[" + m_colorant + "]: Rotate FAILED (JobId = " + image_properties.job_id + ", ImageId = " +
                        image_properties.image_id + ")");
            return(successful);
        }
    }
    else
    {
        // otherwise, "pass-through" the ORIGINAL IMAGE "properties" and "data" unchanged.
        image_properties_rotated = image_properties;
        image_data_rotated       = image_data;
    }

    // check to see if we need to "process" the IMAGE "data".
    if (m_output_properties.image_data_processor_enabled)
    {
        // if so, attempt to "process" the IMAGE "data" -- if unsuccessful, return immediately.
        if (!m_p_output_plug_in->ImageProcess(image_properties_rotated, image_data_rotated, image_data_processed))
        {
            FIERY::FITS::MODULE::LOG::MANAGER::LogError("FITS-Module[" + m_colorant + "]: ImageProcess FAILED");
            return(successful);
        }
    }

    // attempt to "create" the BUFFER from the BUFFER "pool".
    if (m_buffer_mgr.BufferCreateFromPool(image_properties_rotated.job_id, image_properties_rotated.image_id,
                m_output_properties.image_data_processor_enabled ? image_data_processed : image_data_rotated, buffer))
    {
        // if successful, attempt to "create" the IMAGE.
        successful = m_image_mgr.ImageCreate(image_properties_rotated, buffer);
    }

    // return the status.
    return(successful);
}

bool COLORANT::ImageDelete(const std::string & job_id, const std::string & image_id)
{
    bool   successful = true;


    // attempt to "delete" the IMAGE -- if unsuccessful, update the status.
    if (!m_image_mgr.ImageDelete(job_id, image_id))
    {
        successful = false;
    }

    // attempt to "delete" the BUFFER from the BUFFER "pool" -- if unsuccessful, update the status.
    if (!m_buffer_mgr.BufferDeleteFromPool(job_id, image_id))
    {
        successful = false;
    }

    // return the status.
    return(successful);
}

bool COLORANT::ImageGet(const std::string & job_id, const std::string & image_id, FITS::MODULE::IMAGE::IMAGE & image)
{
    return(m_image_mgr.ImageGet(job_id, image_id, image));
}

bool COLORANT::ImageTransfer(FIERY::FITS::MODULE::TRANSFER::REQUEST * p_transfer_request)
{
    bool   successful = true;


    // attempt to "take" the TRANSFER_REQUEST "mutex".
    std::lock_guard<std::mutex>   lock(m_transfer_request_mutex);

    // attempt to "add" the TRANSFER_REQUEST to the "list".
    m_transfer_request_list.push_back(p_transfer_request);

    // attempt to "signal" the change.
    m_transfer_request_signal.notify_all();

    // return the status.
    return(successful);
}

bool COLORANT::Initialize(void)
{
    // check to see if we need to "initialize" the COLORANT.
    if (!m_initialized)
    {
        // if so, attempt to "initialize" the BUFFER_MANAGER.
        if (m_buffer_mgr.Initialize())
        {
            // if successful, attempt to "run" the COLORANT.
            m_thread = std::thread(std::bind(&COLORANT::run, this));

            // update the "management" variables.
            m_initialized = true;
        }
    }

    // return the status.
    return(m_initialized);
}

bool COLORANT::JobCancel(const std::string & job_id)
{
    bool   successful = true;


    FIERY::FITS::MODULE::LOG::MANAGER::LogNormal("FITS-Module[" + m_colorant + "]: COLORANT CANCEL");

    // attempt to "cancel" the JOB in the BUFFER_MANAGER -- if unsuccessful, update the status.
    if (!m_buffer_mgr.JobCancel(job_id))
    {
        successful = false;
    }

    // attempt to "cancel" the JOB in the IMAGE_MANAGER -- if unsuccessful, update the status.
    if (!m_image_mgr.JobCancel(job_id))
    {
        successful = false;
    }

    FIERY::FITS::MODULE::LOG::MANAGER::LogNormal("FITS-Module[" + m_colorant + "]: COLORANT CANCELED");

    // return the status.
    return(successful);
}

bool COLORANT::get(FIERY::FITS::MODULE::TRANSFER::REQUEST * & p_transfer_request)
{
    bool   successful = false;


    // initialize the return vaiables.
    p_transfer_request = nullptr;

    // attempt to "take" the TRANSFER_REQUEST "mutex".
    std::unique_lock<std::mutex>   lock(m_transfer_request_mutex);

    // loop continuously, waiting for a "valid" TRANSFER_REQUEST (or the MANAGER to "stop").
    while (m_transfer_request_list.empty() && m_thread_enabled)
    {
        // if unsuccessful, attempt to "wait" for the TRANSFER_REQUEST "signal".
        m_transfer_request_signal.wait(lock);
    }

    // check to see if the TRANSFER_REQUEST "list" is "valid".
    if (!m_transfer_request_list.empty() && m_thread_enabled)
    {
        // attempt to "get" the "next" TRANSFER_REQUEST.
        p_transfer_request = m_transfer_request_list.front();

        // attempt to "remove" the TRANSFER_REQUEST from the "list".
        m_transfer_request_list.pop_front();

        // update the status.
        successful = true;
    }

    // return the status.
    return(successful);
}

bool COLORANT::output(FIERY::FITS::MODULE::TRANSFER::REQUEST * p_transfer_request)
{
    FIERY::FITS::MODULE::IMAGE::IMAGE   image;

    bool                                successful = false;

    const std::string                   image_id   = p_transfer_request->ImageIdGet();
    const std::string                   job_id     = p_transfer_request->JobIdGet();


    // check to see if the JOB is "canceled".
    if (!MANAGER::IsJobCanceled(job_id))
    {
        // if not, attempt to "get" the IMAGE.
        if (m_image_mgr.ImageGet(job_id, image_id, image))
        {
            // if successful, attempt to "transfer" the IMAGE.
            if (m_p_output_plug_in->ImageTransfer(image, p_transfer_request->JobNameGet(), p_transfer_request->ImageSettingsGet()))
            {
                // if successful, attempt to "set" the REQUEST as "transferred".
                successful = p_transfer_request->Transferred(m_colorant);
            }

            // attempt to "delete" the IMAGE -- if unsuccessful, update the status.
            if (!ImageDelete(job_id, image_id))
            {
                successful = false;
            }
        }
    }
    else
    {
        FIERY::FITS::MODULE::LOG::MANAGER::LogNormal("MANAGER::ImageTransfer: CANCELED (JobId = " + job_id + ", ImageId = " + image_id + ")");

        // attempt to "set" the REQUEST as "transferred" -- in case the JOB was canceled during the "processing" of the "transfer" REQUEST.
        p_transfer_request->Transferred(m_colorant);
    }

    // return the status.
    return(successful);
}

void COLORANT::run(void)
{
    FIERY::FITS::MODULE::TRANSFER::REQUEST *   p_transfer_request;


    FIERY::FITS::MODULE::LOG::MANAGER::LogNormal("FITS-Module[" + m_colorant + "]: RUNNING");

    // loop continuously while "enabled".
    while (m_thread_enabled)
    {
        // attempt to "get" the "next" TRANSFER_REQUEST.
        if (get(p_transfer_request))
        {
            // if successful, attempt to "output" the IMAGE.
            output(p_transfer_request);
        }
    }

    FIERY::FITS::MODULE::LOG::MANAGER::LogNormal("FITS-Module[" + m_colorant + "]: STOPPED");
}

void COLORANT::waitForStop(void)
{
    // update the "management" variables.
    m_thread_enabled = false;

    // attempt to "signal" the change.
    m_transfer_request_signal.notify_all();

    // check to see if we can "wait" for the THREAD to "exit" -- if so, do it.
    if (m_thread.joinable())
    {
        m_thread.join();
    }
}

MANAGER::MANAGER()
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


    // attempt to "take" the COLORANT_MAP "mutex".
    std::lock_guard<std::mutex>   lock(m_colorant_mutex);

    // loop through all COLORANT_MAP entries -- attempting to "abort" all known JOBs for all COLORANTs.
    for (auto & entry_colorant : m_colorant_map)
    {
        if (!entry_colorant.second->Abort())
        {
            // if unsuccessful, update the status.
            successful = false;
        }
    }

    // attempt to "abort" all known JOBs in the OUTPUT "manager" -- if unsuccessful, update the status.
    if (!m_transfer_mgr.Abort())
    {
        successful = false;
    }

    // return the status.
    return(successful);
}

bool MANAGER::ImageCreate(const FIERY::FITS::MODULE::PROPERTIES::IMAGE & image_properties, const std::vector<uint8_t> & image_data)
{
    COLORANT *   p_colorant = nullptr;
    bool         successful = false;


    // attempt to "get" the "associated" COLORANT.
    if (colorantGet(image_properties.colorant, p_colorant))
    {
        // if successful, attempt to "create" the IMAGE.
        successful = p_colorant->ImageCreate(image_properties, image_data);
    }

    // check to see if everything was successful -- if not (and JOB is NOT "canceled"), attempt to "log" the ERROR.
    if (!successful && !IsJobCanceled(image_properties.job_id))
    {
        FIERY::FITS::MODULE::LOG::MANAGER::LogError("FITS-Module[" + image_properties.colorant + "]: ImageCreate FAILED (JobId = " + image_properties.job_id + ", ImageId = " +
                    image_properties.image_id + ")");
    }

    // return the status.
    return(successful);
}

bool MANAGER::ImageTransfer(const std::string & job_id, const std::string & job_name, const std::string & image_id, const std::list<std::string> & colorant_list_valid, const boost::property_tree::ptree & json_object_settings)
{
    std::list<COLORANT *>                      colorant_list;
    std::list<std::string>                     colorant_name_list;
    FIERY::FITS::MODULE::TRANSFER::REQUEST *   p_transfer_request;

    bool                                       successful = false;


    // check to see if the JOB is "canceled".
    if (!IsJobCanceled(job_id))
    {
        // if not, attempt to "get" the COLORANT "list".
        if (colorantListGet(colorant_list_valid, colorant_list))
        {
            // if successful, check to see if the COLORANT "list" is "valid".
            if (!colorant_list.empty())
            {
                // if so, loop through all COLORANT entries -- attempting to initialize the local variables.
                for (auto & entry_colorant : colorant_list)
                {
                    colorant_name_list.push_back(entry_colorant->NameGet());
                }
            
                // attempt to "create" a TRANSFER_REQUEST.
                if (m_transfer_mgr.RequestCreate(job_id, job_name, image_id, colorant_name_list, json_object_settings, p_transfer_request))
                {
                    // if successful, initialize the local variables.
                    uint32_t   copy_number = json_object_settings.get<uint32_t>("copyNumber", 0);
                    bool       job_mode    = json_object_settings.get<bool>("jobMode", false);

                    // update the status.
                    successful = true;

                    // check to see if we're in JOB "mode" and this is NOT the "first" COPY.
                    if (job_mode && (copy_number > 1))
                    {
                        FIERY::FITS::MODULE::LOG::MANAGER::LogNormal("MANAGER::ImageTransfer: JOB MODE (JobId = " + job_id + ", ImageId = " + image_id + + ", CopyNumber = " +
                                    std::to_string(copy_number) + ")");

                        // if so, loop through all COLORANT entries -- attempting to "set" the REQUEST as "transferred" (since the IMAGE does NOT exist).
                        for (auto & entry_colorant : colorant_list)
                        {
                            if (!p_transfer_request->Transferred(entry_colorant->NameGet()))
                            {
                                successful = false;
                            }
                        }
                    }
                    else
                    {
                        // otherwise, check to see if the "accumulator" COLORANT is "enabled".
                        if (m_output_properties.colorant_accumulator_enabled)
                        {
                            FIERY::FITS::MODULE::IMAGE::IMAGE              image;
                            std::list<FIERY::FITS::MODULE::IMAGE::IMAGE>   image_list;


                            // if so, loop through all COLORANT entries -- attempting to "get" the "accumulated" IMAGEs.
                            for (auto & entry_colorant : colorant_list)
                            {
                                // attempt to "get" the IMAGE.
                                if (entry_colorant->ImageGet(job_id, image_id, image))
                                {
                                    // if successful, attempt to "add" the IMAGE to the IMAGE "list".
                                    image_list.push_back(image);
                                }
                            }

                            // check to see if the IMAGE "list" is "valid".
                            if (!image_list.empty())
                            {
                                // if so, attempt to "transfer" the "accumulated" IMAGE.
                                if (m_p_output_plug_in->ImageTransferAccumulated(image_list, colorant_list_valid, p_transfer_request->JobNameGet(), json_object_settings))
                                {
                                    // if successful, loop through all COLORANT entries -- attempting to "delete" the "accumulated" COLORANTs.
                                    for (auto & entry_colorant : colorant_list)
                                    {
                                        // attempt to "delete" the IMAGE -- if unsuccessful, update the status.
                                        if (!entry_colorant->ImageDelete(job_id, image_id))
                                        {
                                            successful = false;
                                        }

                                        // attempt to "set" the REQUEST as "transferred" -- if unsuccessful, update the status.
                                        if (!p_transfer_request->Transferred(entry_colorant->NameGet()))
                                        {
                                            successful = false;
                                        }
                                    }
                                }
                            }
                        }
                        else
                        {
                            // otherwise, loop through all COLORANT entries -- attempting to "process" the individual COLORANTs.
                            for (auto & entry_colorant : colorant_list)
                            {
                                // attempt to "transfer" the IMAGE -- if unsuccessful, update the status.
                                if (!entry_colorant->ImageTransfer(p_transfer_request))
                                {
                                    successful = false;
                                }
                            }
                        }
                    }
                }
            }
            else
            {
                FIERY::FITS::MODULE::LOG::MANAGER::LogNormal("MANAGER::ImageTransfer: NO COLORANTs (JobId = " + job_id + ", ImageId = " + image_id + + ")");

                // otherwise, attempt to "send" IMAGE_PRINTED to the WEB_SOCKET "manager" (since the IMAGE does NOT exist).
                successful = m_p_module_mgr->ImagePrinted(job_id, image_id);
            }
        }
    }
    else
    {
        FIERY::FITS::MODULE::LOG::MANAGER::LogNormal("MANAGER::ImageTransfer: CANCELED (JobId = " + job_id + ", ImageId = " + image_id + ")");
    }

    // return the status.
    return(successful);
}

bool MANAGER::IsJobCanceled(const std::string & job_id)
{
    boost::posix_time::ptime   timestamp_current;

    bool                       successful                      = false;
    uint32_t                   timeout_in_milliseconds_maximum = TIMEOUT_IN_MILLISECONDS_CANCEL;


    // attempt to "take" the JOB_CANCELED "mutex".
    std::lock_guard<std::mutex>   lock(m_job_canceled_mutex);

    // initialize the local variables.
    timestamp_current = boost::posix_time::microsec_clock::local_time();

    // attempt to "remove" any JOB_CANCEL "entry" that exceeds the "maximum" TIMEOUT.
    m_job_canceled_list.remove_if([timestamp_current, timeout_in_milliseconds_maximum](auto & entry)
    {
        boost::posix_time::time_duration   duration = timestamp_current - entry.timestamp;
        bool                               remove   = false;


        // check to see if the JOB_CANCEL "entry" has reached the "maximum" TIMEOUT -- if so, update the status.
        if (duration.total_milliseconds() >= timeout_in_milliseconds_maximum)
        {
            FIERY::FITS::MODULE::LOG::MANAGER::LogNormal("MANAGER::isJobCanceled: TIMEOUT (JobId = " + entry.id + ")");
            remove = true;
        }

        // return the status.
        return(remove);
    });

    // loop through the JOB_CANCELED, attempting to "find" the JOB.
    for (auto iterator = m_job_canceled_list.begin(); iterator != m_job_canceled_list.end(); iterator++)
    {
        // check to see if the JOB_ID "matches".
        if (iterator->id == job_id)
        {
            // if so, update the return variables.
            iterator->timestamp = boost::posix_time::microsec_clock::local_time();

            // update the status.
            successful = true;
        }
    }

    // return the status.
    return(successful);
}

bool MANAGER::JobCancel(const std::string & job_id)
{
    bool   successful = true;


    // attempt to "set" the "canceled" JOB.
    jobCancelSet(job_id);

    // attempt to "take" the COLORANT_MAP "mutex".
    std::lock_guard<std::mutex>   lock(m_colorant_mutex);

    // loop through all COLORANT_MAP entries -- attempting to "cancel" the JOB for all COLORANTs.
    for (auto & entry_colorant : m_colorant_map)
    {
        if (!entry_colorant.second->JobCancel(job_id))
        {
            // if unsuccessful, update the status.
            successful = false;
        }
    }

    // attempt to "cancel" the JOB in the OUTPUT "manager" -- if unsuccessful, update the status.
    if (!m_transfer_mgr.JobCancel(job_id))
    {
        successful = false;
    }

    // return the status.
    return(successful);
}

bool MANAGER::Start(FIERY::FITS::MODULE::MANAGER * p_module_mgr, const FIERY::FITS::MODULE::PROPERTIES::COLORANT & colorant_properties, const std::string & fits_output_plugin_name, const FIERY::FITS::MODULE::PROPERTIES::ROTATION & rotation_properties)
{
    std::map<std::string, COLORANT *>::iterator   iterator;
    boost::filesystem::path                       path_output_plug_in;

    COLORANT *                                    p_colorant = nullptr;
    bool                                          successful = false;


    // attempt to "validate" the arguments.
    if (colorant_properties.colorant_list.empty())
    {
        FIERY::FITS::MODULE::LOG::MANAGER::LogError("COLORANT::MANAGER::Start: Invalid COLORANTS");
        return(successful);
    }

    if (colorant_properties.buffer_pool_size_in_bytes == 0)
    {
        FIERY::FITS::MODULE::LOG::MANAGER::LogError("COLORANT::MANAGER::Start: Invalid BUFFER_POOL_SIZE_IN_BYTES");
        return(successful);
    }

    if (fits_output_plugin_name.empty())
    {
        FIERY::FITS::MODULE::LOG::MANAGER::LogError("COLORANT::MANAGER::Start: Invalid OUTPUT_PLUGIN");
        return(successful);
    }

    // initialize the "management" variables.
    m_p_module_mgr        = p_module_mgr;
    m_rotation_properties = rotation_properties;

    FIERY::FITS::MODULE::LOG::MANAGER::LogNormal("COLORANT::MANAGER::Start: RASTER Rotation " + std::string(m_rotation_properties.enabled ? "ENABLED (Degrees = " +
                std::to_string(m_rotation_properties.degrees) + ")" : "DISABLED"));

    // initialize the local variables.
    path_output_plug_in = fits_output_plugin_name;
    
    // check to see if the FITS-OUTPUT "plug-in" is a "relative" PATH -- if so, update the local variables.
    if (path_output_plug_in.is_relative())
    {
        path_output_plug_in = boost::filesystem::current_path() / fits_output_plugin_name;
    }

    try
    {
        // attempt to "load" the FITS-OUTPUT "plug-in".
        m_p_output_plug_in = boost::dll::import_symbol<FIERY::FITS::OUTPUT::PLUG_IN>(path_output_plug_in, FITS_OUTPUT_PLUG_IN_SYMBOLS, boost::dll::load_mode::append_decorations);
    }
    catch (std::exception const & exception)
    {
        FIERY::FITS::MODULE::LOG::MANAGER::LogError("COLORANT::MANAGER::Start: Failed to load FITS-OUTPUT plug-in (File = " + path_output_plug_in.string() + ", Reason = " +
                    exception.what() + ")");
        throw(exception);
    }

    // attempt to "initialize" the FITS-OUTPUT "plug-in".
    if (m_p_output_plug_in->Initialize(m_p_module_mgr->LoggerGet(), colorant_properties.colorant_list, m_output_properties))
    {
        // if successful, attempt to "start" the OUTPUT "manager".
        if (m_transfer_mgr.Start(m_p_module_mgr, m_p_output_plug_in, m_output_properties))
        {
            // if successful, update the status.
            successful = true;

            // attempt to "take" the COLORANT_MAP "mutex".
            std::lock_guard<std::mutex>   lock(m_colorant_mutex);

            // otherwise, loop through the COLORANTs, attempting to "create" the individual COLORANTs.
            for (auto & colorant : colorant_properties.colorant_list)
            {
                // attempt to "find" the "associated" COLORANT.
                if ((iterator = m_colorant_map.find(colorant)) == m_colorant_map.end())
                {
                    // if unsuccessful, attempt to "create" the COLORANT.
                    if ((p_colorant = new FIERY::FITS::MODULE::COLORANT::COLORANT(colorant, colorant_properties, m_p_output_plug_in, m_output_properties, m_rotation_properties)) != nullptr)
                    {
                        // if successful, attempt to "initialize" the COLORANT.
                        if (p_colorant->Initialize())
                        {
                            // if successful, attempt to "add" the COLORANT to the COLORANT_MAP.
                            m_colorant_map[colorant] = p_colorant;
                        }
                        else
                        {
                            // otherwise, update the status.
                            successful = false;
                        }
                    }
                    else
                    {
                        // otherwise, update the status.
                        successful = false;
                    }
                }
            }
        }
    }

    // return the status.
    return(successful);
}

bool MANAGER::Stop(void)
{
    bool   successful;


    // attempt to "un-initialize" the FITS-OUTPUT "plug-in".
    m_p_output_plug_in->Uninitialize();

    // attempt to "take" the COLORANT_MAP "mutex".
    std::lock_guard<std::mutex>   lock(m_colorant_mutex);

    // loop through all COLORANT_MAP entries -- attempting to "delete" all COLORANTs.
    for (auto & entry_colorant : m_colorant_map)
    {
        delete entry_colorant.second;
    }

    // attempt to "delete" COLORANT_MAP.
    m_colorant_map.clear();

    // attempt to "stop" the OUTPUT "manager".
    successful = m_transfer_mgr.Stop();

    // return the status.
    return(successful);
}

bool MANAGER::colorantGet(const std::string & colorant, COLORANT * & p_colorant)
{
    std::map<std::string, COLORANT *>::iterator   iterator;

    bool                                          successful = false;


    // initialize the return variables.
    p_colorant = nullptr;

    // attempt to "take" the COLORANT_MAP "mutex".
    std::lock_guard<std::mutex>   lock(m_colorant_mutex);

    // attempt to "find" the "associated" COLORANT.
    if ((iterator = m_colorant_map.find(colorant)) != m_colorant_map.end())
    {
        // if successful, attempt to update the return variables.
        p_colorant = iterator->second;
        successful = true;
    }

    // return the status.
    return(successful);
}

bool MANAGER::colorantListGet(const std::list<std::string> & colorant_list_valid, std::list<COLORANT *> & colorant_list)
{
    COLORANT *   p_colorant;

    bool         successful = true;


    // initialize the return variables.
    colorant_list.clear();

    // loop through all "valid" COLORANT entries -- attempting to "get" the "required" COLORANT "list".
    for (auto & entry_colorant : colorant_list_valid)
    {
        // attempt to "get" the COLORANT.
        if (colorantGet(entry_colorant, p_colorant))
        {
            // if successful, update the return variables.
            colorant_list.push_back(p_colorant);
        }
    }

    // return the status.
    return(successful);
}

void MANAGER::jobCancelSet(const std::string & job_id)
{
    bool   found = false;


    // attempt to "take" the JOB_CANCELED "mutex".
    std::lock_guard<std::mutex>   lock(m_job_canceled_mutex);

    // loop through the JOB_CANCELED, attempting to "find" the JOB.
    for (auto iterator = m_job_canceled_list.begin(); iterator != m_job_canceled_list.end(); iterator++)
    {
        // check to see if the JOB_ID "matches".
        if (iterator->id == job_id)
        {
            // if so, update the return variables.
            iterator->timestamp = boost::posix_time::microsec_clock::local_time();

            // update the local variables.
            found = true;
        }
    }

    // check to see if we "found" the JOB.
    if (!found)
    {
        JOB_CANCELED   job_canceled(job_id, boost::posix_time::microsec_clock::local_time());


        // if not, attempt to "add" the JOB.
        m_job_canceled_list.push_back(job_canceled);
    }
}
