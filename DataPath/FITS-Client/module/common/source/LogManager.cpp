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

using namespace FIERY::FITS::MODULE::LOG;


/**********************************************************************************************************************************************************************************/
/*                                                                                local variables                                                                                 */
/**********************************************************************************************************************************************************************************/
FIERY::FITS::MODULE::LOGGER::CALLBACK_ERROR     MANAGER::m_callback_error   = nullptr;
FIERY::FITS::MODULE::LOGGER::CALLBACK_FATAL     MANAGER::m_callback_fatal   = nullptr;
FIERY::FITS::MODULE::LOGGER::CALLBACK_NORMAL    MANAGER::m_callback_normal  = nullptr;
FIERY::FITS::MODULE::LOGGER::CALLBACK_WARNING   MANAGER::m_callback_warning = nullptr;
bool                                            MANAGER::m_initialized      = false;


/**********************************************************************************************************************************************************************************/
/*                                                                                     methods                                                                                    */
/**********************************************************************************************************************************************************************************/
bool MANAGER::Initialize(const FIERY::FITS::MODULE::LOGGER & logger)
{
    // check to see if we need to "initialize" LOGGING.
    if (!m_initialized)
    {
        // if so, initialize the "management" variables.
        m_callback_error   = logger.callback_error;
        m_callback_fatal   = logger.callback_fatal;
        m_callback_normal  = logger.callback_normal;
        m_callback_warning = logger.callback_warning;
        m_initialized      = true;
    }

    // return the status.
    return(m_initialized);
}

void MANAGER::LogError(const std::string & message, const bool & console_log)
{
    // check to see if LOGGING is "initialized" -- if so, attempt to "log" the "error" MESSAGE.
    if (m_initialized && (m_callback_error != nullptr))
    {
        m_callback_error(message, console_log);
    }
}

void MANAGER::LogFatal(const std::string & message, const bool & console_log)
{
    // check to see if LOGGING is "initialized" -- if so, attempt to "log" the "fatal" MESSAGE.
    if (m_initialized && (m_callback_fatal != nullptr))
    {
        m_callback_fatal(message, console_log);
    }
}

void MANAGER::LogNormal(const std::string & message, const bool & console_log)
{
    // check to see if LOGGING is "initialized" -- if so, attempt to "log" the "normal" MESSAGE.
    if (m_initialized && (m_callback_normal != nullptr))
    {
        m_callback_normal(message, console_log);
    }
}

void MANAGER::LogWarning(const std::string & message, const bool & console_log)
{
    // check to see if LOGGING is "initialized" -- if so, attempt to "log" the "warning" MESSAGE.
    if (m_initialized && (m_callback_warning != nullptr))
    {
        m_callback_warning(message, console_log);
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
