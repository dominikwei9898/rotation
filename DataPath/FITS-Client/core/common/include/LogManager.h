/**********************************************************************************************************************************************************************************/
/*                                                                                                                                                                                */
/*  Copyright (c) 1990-2023 Fiery, LLC.  All Rights Reserved.                                                                                                                     */
/*                                                                                                                                                                                */
/*  Fiery, LLC ("Fiery") products contain certain trade secrets and confidential and proprietary information of Fiery.  Use, reproduction, disclosure, distribution by any means  */
/*  are prohibited, except pursuant to a written license from Fiery.  Modification, translation, reverse engineering, decompiling, disassembling, and creating derivative works   */
/*  based on this software are prohibited, except pursuant to a written license from Fiery. Use of copyright notice does not imply publication or disclosure.                     */
/*                                                                                                                                                                                */
/**********************************************************************************************************************************************************************************/
#ifndef __LOG_MANAGER_H__
#define __LOG_MANAGER_H__


/**********************************************************************************************************************************************************************************/
/*                                                                                    includes                                                                                    */
/**********************************************************************************************************************************************************************************/
#include "FITS-ModuleApi.h"

#include <boost/filesystem/path.hpp>

#include <chrono>
#include <cstdint>
#include <mutex>
#include <string>


/**********************************************************************************************************************************************************************************/
/*                                                            typedefs, structures, enumerations, and related defines                                                             */
/**********************************************************************************************************************************************************************************/
namespace FIERY
{
    namespace FITS
    {
        namespace CORE
        {
            namespace LOG
            {
                class MANAGER
                {
                    public:
                    // public static "management" methods.
                    static bool Initialize(const std::string & application, const std::string & filename_base, const boost::filesystem::path & path_base, const FIERY::FITS::MODULE::PROPERTIES::LOG & log_properties);

                    // public static "accessor" methods.
                    static void LogError(const std::string & message, const bool & console_log = false);
                    static void LogFatal(const std::string & message, const bool & console_log = false);
                    static void LogInfo(const std::string & message, const bool & console_log = false);
                    static void LogWarning(const std::string & message, const bool & console_log = false);

                    private:
                    // private constructors and destructor.
                    MANAGER();
                    virtual ~MANAGER();

                    // private static "accessor" methods.
                    static void LogConsole(const std::string & message);

                    // private static "helper" methods.
                    static void DirectoryCleanup(void);
                    static void DirectoryCleanupFiles(const std::string & filename_base);

                    // private static "management" variables.
                    static std::mutex                                       m_directory_cleanup_mutex;
                    static std::chrono::high_resolution_clock::time_point   m_directory_cleanup_time_point_previous;
                    static bool                                             m_initialized;
                    static std::string                                      m_filename_base_error;
                    static std::string                                      m_filename_base_fatal;
                    static std::string                                      m_filename_base_info;
                    static std::string                                      m_filename_base_warning;
                    static std::string                                      m_filepath_directory;
                    static FIERY::FITS::MODULE::PROPERTIES::LOG             m_log_properties;

                    // private static "management" const.
                    static const double_t      DIRECTORY_CLEANUP_PERIOD_IN_SECONDS;
                    static const std::string   DIRECTORY_LOG;
                    static const uint32_t      FILE_SIZE_MAX_IN_MEGABYTES;
                };
            }
        }
    }
}
#endif // __LOG_MANAGER_H__
