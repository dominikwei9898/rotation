/**********************************************************************************************************************************************************************************/
/*                                                                                                                                                                                */
/*  Copyright (c) 1990-2026 Fiery, LLC.  All Rights Reserved.                                                                                                                     */
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

#include <string>


/**********************************************************************************************************************************************************************************/
/*                                                            typedefs, structures, enumerations, and related defines                                                             */
/**********************************************************************************************************************************************************************************/
namespace FIERY
{
    namespace FITS
    {
        namespace MODULE
        {
            namespace LOG
            {
                // class declarations.
                class MANAGER
                {
                    public:
                    // public static "management" methods.
                    static bool Initialize(const FIERY::FITS::MODULE::LOGGER & logger);

                    // public static LOG methods.
                    static void LogError(const std::string & message, const bool & console_log = false);
                    static void LogFatal(const std::string & message, const bool & console_log = false);
                    static void LogNormal(const std::string & message, const bool & console_log = false);
                    static void LogWarning(const std::string & message, const bool & console_log = false);

                    private:
                    // private constructors and destructor.
                    MANAGER();
                    virtual ~MANAGER();

                    // private static "management" variables.
                    static FIERY::FITS::MODULE::LOGGER::CALLBACK_ERROR     m_callback_error;
                    static FIERY::FITS::MODULE::LOGGER::CALLBACK_FATAL     m_callback_fatal;
                    static FIERY::FITS::MODULE::LOGGER::CALLBACK_NORMAL    m_callback_normal;
                    static FIERY::FITS::MODULE::LOGGER::CALLBACK_WARNING   m_callback_warning;
                    static bool                                            m_initialized;
                };
            }
        }
    }
}
#endif // __LOG_MANAGER_H__
