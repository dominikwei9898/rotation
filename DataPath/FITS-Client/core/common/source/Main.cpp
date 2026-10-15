/**********************************************************************************************************************************************************************************/
/*                                                                                                                                                                                */
/*  (c) Copyright 1990-2023  Electronics for Imaging, Inc. ("EFI").     All Rights Reserved                                                                                       */
/*                                                                                                                                                                                */
/*  EFI products contain certain trade secrets and confidential and proprietary information of EFI.  Use, reproduction, disclosure, distribution by any means  are prohibited,    */
/*  except pursuant to a written license from EFI. Modification, translation, reverse engineering, decompiling, disassembling, and creating derivative works based on this        */
/*  software are prohibited, except pursuant to a written license from EFI. Use of copyright notice does not imply publication or disclosure.                                     */
/*                                                                                                                                                                                */
/*  Restricted Rights Legend:                                                                                                                                                     */
/*  Use, duplication, or disclosure by the Government is subject to restrictions as set forth in subparagraph (c)(1)(ii) of The Rights in Technical Data and Computer Software    */
/*  clause in DFARS 252.227-7013 or subparagraphs (c)(1) and (2) of the Commercial Computer Software -- Restricted Rights at 48 CFR 52.227-19, as applicable.                     */
/*                                                                                                                                                                                */
/**********************************************************************************************************************************************************************************/
#include "FITS-ModuleApi.h"

#include "ConfigController.h"
#include "LogManager.h"

#include <boost/dll/import.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>

#include <iostream>
#include <string>

#if (defined(_WIN32))
#include <windows.h>
#else
#include <signal.h>
#endif // _WIN32


/**********************************************************************************************************************************************************************************/
/*                                                                                global variables                                                                                */
/**********************************************************************************************************************************************************************************/
FIERY::FITS::CORE::CONFIG::CONTROLLER *   p_config_controller = nullptr;

const std::string   VERSION = "1.0.6";


/**********************************************************************************************************************************************************************************/
/*                                                                                     methods                                                                                    */
/**********************************************************************************************************************************************************************************/
bool configurationGet(FIERY::FITS::MODULE::CONFIGURATION & configuration)
{
    boost::property_tree::ptree   json_object;

    boost::filesystem::path       path_current                 = boost::filesystem::current_path();
    boost::filesystem::path       path_json_configuration_file = path_current / FIERY::FITS::MODULE::CONSTANT::FILENAME_CONFIGURATION_JSON;
    bool                          successful                   = true;


    // attempt to "read" the JSON "configuration" FILE.
    try
    {
        boost::property_tree::read_json(path_json_configuration_file.string(), json_object);
    }
    catch (...)
    {
        successful = false;
    }

    // check to see if we successfully "read" the JSON "configuration" FILE.
    if (successful)
    {
        // if so, initialize the return variables.
        configuration.fits_properties.ip                   = json_object.get<std::string>("fits_server.ip", "");
        configuration.fits_properties.port                 = json_object.get<uint32_t>("fits_server.port", 0);
        configuration.log_properties.enable_console        = json_object.get<bool>("logging.console", false);
        configuration.log_properties.files_max             = json_object.get<uint32_t>("logging.max_files", 20);
        configuration.log_properties.size_max_in_megabytes = json_object.get<uint32_t>("logging.max_size_in_megabytes", 20);
    }

    // return the status.
    return(successful);
}

#if (defined(_WIN32))
BOOL WINAPI windows_ctrl_handler(DWORD event_type)
{
    BOOL   successful = TRUE;


    switch (event_type)
    {
        case CTRL_C_EVENT:
        {
            FIERY::FITS::CORE::LOG::MANAGER::LogInfo("Ctrl-C event received", true);
            break;
        }
        case CTRL_CLOSE_EVENT:
        {
            FIERY::FITS::CORE::LOG::MANAGER::LogInfo("Ctrl-Close event received", true);
            break;
        }
        case CTRL_BREAK_EVENT:
        {
            FIERY::FITS::CORE::LOG::MANAGER::LogInfo("Ctrl-Break event received", true);
            break;
        }
        case CTRL_LOGOFF_EVENT:
        {
            FIERY::FITS::CORE::LOG::MANAGER::LogInfo("Ctrl-Logoff event received", true);
            break;
        }
        case CTRL_SHUTDOWN_EVENT:
        {
            FIERY::FITS::CORE::LOG::MANAGER::LogInfo("Ctrl-Shutdown event received", true);
            break;
        }
        default:
        {
            // update the status and return.
            successful = FALSE;
            return(successful);
        }
    }

    // check to see if everything is "successful".
    if (successful == TRUE)
    {
        // if so, check to see if the CONFIG_CONTROLLER is "valid" -- if so, attempt to "stop" it.
        if (p_config_controller != nullptr)
        {
            p_config_controller->Stop();
        }
    }

    // return the status.
    return(successful);
}
#else
void linux_signal_handler(int signal_id)
{
    FIERY::FITS::CORE::LOG::MANAGER::LogInfo("Signal event received (ID = " + std::to_string(signal_id) + ")", true);

    // check to see if the CONFIG_CONTROLLER is "valid" -- if so, attempt to "stop" it.
    if (p_config_controller != nullptr)
    {
        p_config_controller->Stop();
    }
}

bool linux_signal_handler_set(int signal_id)
{
    struct sigaction   sig_action;

    bool               successful = true;


    sigemptyset(&(sig_action.sa_mask));
    sig_action.sa_flags   = 0;
    sig_action.sa_handler = linux_signal_handler;

    if (sigaction(signal_id, &(sig_action), NULL) == -1)
    {
        successful = false;
    }

    // return the status.
    return(successful);
}
#endif // _WIN32

// Console app
int main(int argc, char* argv[])
{
    FIERY::FITS::MODULE::CONFIGURATION                configuration;
    FIERY::FITS::MODULE::LOGGER                       logger(FIERY::FITS::CORE::LOG::MANAGER::LogInfo, FIERY::FITS::CORE::LOG::MANAGER::LogWarning, FIERY::FITS::CORE::LOG::MANAGER::LogError, FIERY::FITS::CORE::LOG::MANAGER::LogFatal);
    boost::shared_ptr<FIERY::FITS::MODULE::PLUG_IN>   p_module_plug_in;

    boost::filesystem::path                           path_current        = boost::filesystem::current_path();
    boost::filesystem::path                           path_module_plug_in = path_current / FITS_MODULE_PLUG_IN_NAME;
    int                                               result              = -1;


    // attempt to "get" the CONFIGURATION.
    if (configurationGet(configuration))
    {
        // if successful, attempt to "initialize" the LOGGING.
        FIERY::FITS::CORE::LOG::MANAGER::Initialize(argv[0], "FITS-Client", path_current, configuration.log_properties);
        FIERY::FITS::CORE::LOG::MANAGER::LogInfo("FITS-Client: Version " + VERSION);

#if (defined(_WIN32))
        // attempt to "set" the CONSOLE "ctrl" HANDLER.
        if (SetConsoleCtrlHandler(windows_ctrl_handler, TRUE) != TRUE)
        {
            FIERY::FITS::CORE::LOG::MANAGER::LogError("FITS-Client: Unable to set CONSOLE handler");
            return(result);
        }
#else
        // attempt to "set" the CONSOLE "signal" HANDLERs.
        if (!linux_signal_handler_set(SIGINT))
        {
            FIERY::FITS::CORE::LOG::MANAGER::LogError("FITS-Client: Unable to set CONSOLE handler (Signal ID = " + std::to_string(SIGINT) + ")");
            return(result);
        }

        if (!linux_signal_handler_set(SIGTERM))
        {
            FIERY::FITS::CORE::LOG::MANAGER::LogError("FITS-Client: Unable to set CONSOLE handler (Signal ID = " + std::to_string(SIGTERM) + ")");
            return(result);
        }
#endif // _WIN32

        FIERY::FITS::CORE::LOG::MANAGER::LogInfo("FITS-Client[FITS-Module]", true);

        try
        {
            // attempt to "load" the FITS-MODULE "plug-in".
            p_module_plug_in = boost::dll::import_symbol<FIERY::FITS::MODULE::PLUG_IN>(path_module_plug_in, FITS_MODULE_PLUG_IN_SYMBOLS, boost::dll::load_mode::append_decorations);
        }
        catch (std::exception const & exception)
        {
            FIERY::FITS::CORE::LOG::MANAGER::LogError("FITS-Client[FITS-Module]: Failed to load FITS-MODULE plug-in (File = " + path_module_plug_in.string() + ")");
            throw(exception);
        }

        // attempt to "initialize" the FITS-MODULE "plug-in".
        if (p_module_plug_in->Initialize(logger))
        {
            // if successful, attempt to "create" the CONFIG_CONTROLLER.
            if ((p_config_controller = new FIERY::FITS::CORE::CONFIG::CONTROLLER(configuration.fits_properties, p_module_plug_in)) != nullptr)
            {
                FIERY::FITS::CORE::LOG::MANAGER::LogInfo("FITS-Client: STARTED");

                // if successful, attempt to "start" the CONFIG_CONTROLLER.
                result = p_config_controller->Start();
            }
            else
            {
                FIERY::FITS::CORE::LOG::MANAGER::LogError("FITS-Client[Configuration]: FAILED", true);
            }
        }
        else
        {
            FIERY::FITS::CORE::LOG::MANAGER::LogError("FITS-Client[Initialize]: FAILED", true);
        }

        FIERY::FITS::CORE::LOG::MANAGER::LogInfo("FITS-Client: STOPPED");
    }

    return(result);
}
