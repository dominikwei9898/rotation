/**********************************************************************************************************************************************************************************/
/*                                                                                                                                                                                */
/*  Copyright (c) 1990-2026 Fiery, LLC.  All Rights Reserved.                                                                                                                     */
/*                                                                                                                                                                                */
/*  Fiery, LLC ("Fiery") products contain certain trade secrets and confidential and proprietary information of Fiery.  Use, reproduction, disclosure, distribution by any means  */
/*  are prohibited, except pursuant to a written license from Fiery.  Modification, translation, reverse engineering, decompiling, disassembling, and creating derivative works   */
/*  based on this software are prohibited, except pursuant to a written license from Fiery. Use of copyright notice does not imply publication or disclosure.                     */
/*                                                                                                                                                                                */
/**********************************************************************************************************************************************************************************/
#include "LogManager.h"

#include <boost/filesystem.hpp>

#include <iostream>
#include <map>

// GOOGLE_PREDICT_TRUE is defined by both google protobuf and google log.  Since each assumes it was never defined, we get a compile warning when both are included.  So, we
// undefine it here if, and only if, it has already been defined.
#ifdef GOOGLE_PREDICT_TRUE
#undef GOOGLE_PREDICT_TRUE
#endif
#define GLOG_NO_ABBREVIATED_SEVERITIES
#define GLOG_USE_GLOG_EXPORT

#include <glog/logging.h>

using namespace FIERY::FITS::CORE::LOG;


/**********************************************************************************************************************************************************************************/
/*                                                                                local variables                                                                                 */
/**********************************************************************************************************************************************************************************/
std::mutex                                       MANAGER::m_directory_cleanup_mutex;
std::chrono::high_resolution_clock::time_point   MANAGER::m_directory_cleanup_time_point_previous;
std::wstring                                     MANAGER::m_filename_base_error;
std::wstring                                     MANAGER::m_filename_base_fatal;
std::wstring                                     MANAGER::m_filename_base_info;
std::wstring                                     MANAGER::m_filename_base_warning;
std::wstring                                     MANAGER::m_filepath_directory;
FIERY::FITS::MODULE::PROPERTIES::LOG             MANAGER::m_log_properties;

bool   MANAGER::m_initialized = false;

const double_t      MANAGER::DIRECTORY_CLEANUP_PERIOD_IN_SECONDS = 60;
const std::string   MANAGER::DIRECTORY_LOG                       = "logs";
const uint32_t      MANAGER::FILE_SIZE_MAX_IN_MEGABYTES          = 10;


/**********************************************************************************************************************************************************************************/
/*                                                                                     methods                                                                                    */
/**********************************************************************************************************************************************************************************/
void MANAGER::DirectoryCleanup(void)
{
    std::chrono::duration<double_t>   duration;
    double_t                          duration_in_seconds;


    // attempt to "take" the "mutex".
    std::lock_guard<std::mutex>   lock(m_directory_cleanup_mutex);

    // initialize the local variables.
    duration            = std::chrono::duration_cast<std::chrono::duration<double_t>>(std::chrono::high_resolution_clock::now() - m_directory_cleanup_time_point_previous);
    duration_in_seconds = duration.count();

    // check to see if it's time to "clean-up" the LOGGING "files".
    if (duration_in_seconds > DIRECTORY_CLEANUP_PERIOD_IN_SECONDS)
    {
        // if so, attempt to "clean-up" the various LOGGING "files".
        DirectoryCleanupFiles(m_filename_base_error);
        DirectoryCleanupFiles(m_filename_base_fatal);
        DirectoryCleanupFiles(m_filename_base_info);
        DirectoryCleanupFiles(m_filename_base_warning);

        // update the "management" variables.
        m_directory_cleanup_time_point_previous = std::chrono::high_resolution_clock::now();
    }
}

void MANAGER::DirectoryCleanupFiles(const std::wstring & filename_base)
{
    // check the arguments.
    if (!filename_base.empty())
    {
        // if valid, check to see if "clean-up" is enabled.
        if (m_log_properties.files_max > 0)
        {
            // if so, initialize the local variables.
            std::map<std::time_t, boost::filesystem::path>   file_map;

            // loop through all the "files" in the "directory", attempting to "process" them.
            for (auto & file : boost::filesystem::recursive_directory_iterator(m_filepath_directory))
            {
                // check to see if the "file" contains the given "base" name.
                if (file.path().wstring().find(filename_base) != std::wstring::npos)
                {
                    // if so, attempt to "add" the "file" to the "set".
                    file_map[boost::filesystem::last_write_time(file.path())] = file.path();
                }
            }

            // loop continuously (until the number of "files" no longer exceeds the "maximum"), attempting to "delete" the "older" files.
            while (file_map.size() > m_log_properties.files_max)
            {
                auto iterator = file_map.begin();

                boost::filesystem::remove(iterator->second);
                file_map.erase(iterator);
            }
        }
    }
}

bool MANAGER::Initialize(const std::string & application, const std::wstring & filename_base, const FIERY::FITS::MODULE::PROPERTIES::LOG & log_properties)
{
    // check to see if we need to "initialize" LOGGING.
    if (!m_initialized)
    {
        boost::filesystem::path   path_filename_base_error;
        boost::filesystem::path   path_filename_base_fatal;
        boost::filesystem::path   path_filename_base_info;
        boost::filesystem::path   path_filename_base_warning;

        boost::filesystem::path   path_directory = DIRECTORY_LOG;


        // if so, initialize the "management" variables.
        m_filename_base_error   = filename_base + L".ERROR.";
	    m_filename_base_fatal   = filename_base + L".FATAL.";
	    m_filename_base_info    = filename_base + L".INFO.";
	    m_filename_base_warning = filename_base + L".WARNING.";
        m_filepath_directory    = path_directory.wstring();
        m_log_properties        = log_properties;

        // check to see if the LOGGING "configuration" is valid -- if not, update the "management" variables.
        if (m_log_properties.size_max_in_megabytes == 0)
        {
            m_log_properties.size_max_in_megabytes = FILE_SIZE_MAX_IN_MEGABYTES;
        }

        // update the local variables.
        path_filename_base_error   = path_directory / m_filename_base_error;
        path_filename_base_fatal   = path_directory / m_filename_base_fatal;
        path_filename_base_info    = path_directory / m_filename_base_info;
        path_filename_base_warning = path_directory / m_filename_base_warning;
        
        // check to see if we need to "create" the LOGGING "directory" -- if so, do it.
        if (!(boost::filesystem::exists(path_directory.wstring())))
        {
            boost::filesystem::create_directory(path_directory.wstring());
        }

        // initialize the LOGGING "flags" variables.
	    FLAGS_max_log_size    = m_log_properties.size_max_in_megabytes;
	    FLAGS_minloglevel     = google::GLOG_INFO;
	    FLAGS_log_dir         = path_directory.string();
	    FLAGS_logbufsecs      = 0;
	    FLAGS_logbuflevel     = google::GLOG_INFO;
        FLAGS_stderrthreshold = google::GLOG_FATAL;
	    FLAGS_v			      = 0;

	    // initialize the LOGGING.
	    google::InitGoogleLogging(application.c_str());
        google::SetLogDestination(google::GLOG_ERROR, path_filename_base_error.string().c_str());
        google::SetLogDestination(google::GLOG_FATAL, path_filename_base_fatal.string().c_str());
        google::SetLogDestination(google::GLOG_INFO, path_filename_base_info.string().c_str());
        google::SetLogDestination(google::GLOG_WARNING, path_filename_base_warning.string().c_str());

        // check to see if the "number" of FILEs is valid -- if so, attempt to "clean-up" the LOGGING "directory".
        if (m_log_properties.files_max != 0)
        {
            DirectoryCleanup();
        }

        // update the "management" variables.
        m_initialized = true;
    }

    // return the status.
    return(m_initialized);
}

void MANAGER::LogConsole(const std::string & message)
{
    if (m_log_properties.enable_console)
    {
        std::cout << message << std::endl;
    }
}

void MANAGER::LogError(const std::string & message, const bool & console_log)
{
    // check to see if LOGGING is "initialized"
    if (m_initialized)
    {
        // if so, attempt to "log" the MESSAGE.
        LOG(ERROR) << message;

        // check to see if we need to "log" the MESSAGE to the CONSOLE -- if so, do it.
        if (console_log)
        {
            LogConsole(message);
        }

        // attempt to "clean-up" LOGGING "directory".
        DirectoryCleanup();
    }
}

void MANAGER::LogFatal(const std::string & message, const bool & console_log)
{
    // check to see if LOGGING is "initialized"
    if (m_initialized)
    {
        // if so, attempt to "log" the MESSAGE.
        LOG(FATAL) << message;

        // check to see if we need to "log" the MESSAGE to the CONSOLE -- if so, do it.
        if (console_log)
        {
            LogConsole(message);
        }

        // attempt to "clean-up" LOGGING "directory".
        DirectoryCleanup();
    }
}

void MANAGER::LogInfo(const std::string & message, const bool & console_log)
{
    // check to see if LOGGING is "initialized"
    if (m_initialized)
    {
        // if so, attempt to "log" the MESSAGE.
        LOG(INFO) << message;

        // check to see if we need to "log" the MESSAGE to the CONSOLE -- if so, do it.
        if (console_log)
        {
            LogConsole(message);
        }

        // attempt to "clean-up" LOGGING "directory".
        DirectoryCleanup();
    }
}

void MANAGER::LogWarning(const std::string & message, const bool & console_log)
{
    // check to see if LOGGING is "initialized"
    if (m_initialized)
    {
        // if so, attempt to "log" the MESSAGE.
        LOG(WARNING) << message;

        // check to see if we need to "log" the MESSAGE to the CONSOLE -- if so, do it.
        if (console_log)
        {
            LogConsole(message);
        }

        // attempt to "clean-up" LOGGING "directory".
        DirectoryCleanup();
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
