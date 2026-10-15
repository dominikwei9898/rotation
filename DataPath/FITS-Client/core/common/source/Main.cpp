/**********************************************************************************************************************************************************************************/
/*                                                                                                                                                                                */
/*  Copyright (c) 1990-2026 Fiery, LLC.  All Rights Reserved.                                                                                                                     */
/*                                                                                                                                                                                */
/*  Fiery, LLC ("Fiery") products contain certain trade secrets and confidential and proprietary information of Fiery.  Use, reproduction, disclosure, distribution by any means  */
/*  are prohibited, except pursuant to a written license from Fiery.  Modification, translation, reverse engineering, decompiling, disassembling, and creating derivative works   */
/*  based on this software are prohibited, except pursuant to a written license from Fiery. Use of copyright notice does not imply publication or disclosure.                     */
/*                                                                                                                                                                                */
/**********************************************************************************************************************************************************************************/
#include "FITS-ModuleApi.h"

#include "ConfigController.h"
#include "LogManager.h"

#include <boost/algorithm/string.hpp>
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

const std::string   COMMAND_LINE_ARGUMENT_BIND                 = "-bind";
const std::string   COMMAND_LINE_ARGUMENT_FITS_SERVER_IP       = "-ip_fits_server";
const std::string   COMMAND_LINE_ARGUMENT_FIERY_PATH_CLIENT_IP = "-ip_fiery_path_client";
const std::string   VERSION                                    = "1.6.3";


/**********************************************************************************************************************************************************************************/
/*                                                                                     methods                                                                                    */
/**********************************************************************************************************************************************************************************/
bool configurationGet(FIERY::FITS::MODULE::CONFIGURATION & configuration)
{
    boost::property_tree::ptree   json_object;

    bool                          successful = true;


    // attempt to "read" the JSON "configuration" FILE.
    try
    {
        boost::property_tree::read_json(FIERY::FITS::MODULE::CONSTANT::FILENAME_CONFIGURATION_JSON, json_object);
    }
    catch (...)
    {
        successful = false;
    }

    // check to see if we successfully "read" the JSON "configuration" FILE.
    if (successful)
    {
        // if so, initialize the return variables.
        configuration.fits_properties.ip                   = json_object.get<std::string>("fits_server.ip", FIERY::FITS::MODULE::CONSTANT::FITS_SERVER_IP_DEFAULT);
        configuration.fits_properties.port                 = json_object.get<uint32_t>("fits_server.port", FIERY::FITS::MODULE::CONSTANT::FITS_SERVER_PORT_DEFAULT);
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
    std::string                                       ip_fiery_path_client;
    std::string                                       ip_fits_server;
    FIERY::FITS::MODULE::LOGGER                       logger(FIERY::FITS::CORE::LOG::MANAGER::LogInfo, FIERY::FITS::CORE::LOG::MANAGER::LogWarning, FIERY::FITS::CORE::LOG::MANAGER::LogError, FIERY::FITS::CORE::LOG::MANAGER::LogFatal);
    boost::shared_ptr<FIERY::FITS::MODULE::PLUG_IN>   p_module_plug_in;

    bool                                              bind                = false;
    boost::filesystem::path                           path_module_plug_in = boost::filesystem::current_path() / FITS_MODULE_PLUG_IN_NAME;
    int                                               result              = -1;


    // check to see if there are any "override" command line "arguments".
    if (argc > 1)
    {
        std::string   argument;
        std::string   value;

        bool          invalid = false;


        // loop through all the command line "arguments", attempting to "parse" them.
        for (int index = 1; index < argc; index++)
        {
            // initialize the local variables.
            argument = argv[index];
            boost::algorithm::to_lower(argument);

            // attempt to "process" the command line "argument", appropriately.
            if (argument == COMMAND_LINE_ARGUMENT_BIND)
            {
                bind = true;
            }
            else
            {
                // initialize the local variables.
                index++;

                // check to see if the command line "value" is "valid".
                if (index < argc)
                {
                    // if so, initialize the local variables.
                    value = argv[index];
                    boost::algorithm::to_lower(value);

                    if (argument == COMMAND_LINE_ARGUMENT_FITS_SERVER_IP)
                    {
                        ip_fits_server = value;
                    }
                    else if (argument == COMMAND_LINE_ARGUMENT_FIERY_PATH_CLIENT_IP)
                    {
                        ip_fiery_path_client = value;
                    }
                    else
                    {
                        invalid = true;
                    }
                }
                else
                {
                    invalid = true;
                }
            }

            // check to see if the command line "arguments" are "valid" -- if not, return immediately. 
            if (invalid)
            {
                std::cout << "\nFITS-client [" << COMMAND_LINE_ARGUMENT_BIND << "] [" << COMMAND_LINE_ARGUMENT_FITS_SERVER_IP << " <IP address>] [" <<
                            COMMAND_LINE_ARGUMENT_FIERY_PATH_CLIENT_IP << " <IP address>]" << std::endl;
                return(result);
            }
        }
    }

    // attempt to "get" the CONFIGURATION.
    if (configurationGet(configuration))
    {
        // if successful, attempt to "initialize" the LOGGING.
        FIERY::FITS::CORE::LOG::MANAGER::Initialize(argv[0], L"FITS-Client", configuration.log_properties);
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

        // check to see we need to "override" the FITS-Server "ip" -- if so, do it.
        if (!ip_fits_server.empty())
        {
            FIERY::FITS::CORE::LOG::MANAGER::LogInfo("FITS-Client: OVERRIDING (IP = <" + ip_fits_server + ">)");

            configuration.fits_properties.ip = ip_fits_server;
        }

        // check to see we need to "override" the FITS-Server "bind" -- if so, do it.
        if (bind)
        {
            FIERY::FITS::CORE::LOG::MANAGER::LogInfo("FITS-Client: OVERRIDING (BIND)");

            configuration.fits_properties.bind_to_similar_local_ip = bind;
        }

        FIERY::FITS::CORE::LOG::MANAGER::LogInfo("FITS-Client[FITS-Module]", true);

        try
        {
            // attempt to "load" the FITS-MODULE "plug-in".
            p_module_plug_in = boost::dll::import_symbol<FIERY::FITS::MODULE::PLUG_IN>(path_module_plug_in, FITS_MODULE_PLUG_IN_SYMBOLS, boost::dll::load_mode::append_decorations);
        }
        catch (std::exception const & exception)
        {
            FIERY::FITS::CORE::LOG::MANAGER::LogError("FITS-Client[FITS-Module]: Failed to load FITS-MODULE plug-in (File = " + path_module_plug_in.string() + ", Reason = " +
                        exception.what() + ")");
            throw(exception);
        }

        // attempt to "initialize" the FITS-MODULE "plug-in".
        if (p_module_plug_in->Initialize(logger, ip_fiery_path_client))
        {
            // if successful, check to see if the FITS_PROPERTIES are valid.
            if (configuration.fits_properties.ip.empty())
            {
                FIERY::FITS::CORE::LOG::MANAGER::LogError("FITS-Client[Configuration]: INVALID IP", true);
            }
            else if (configuration.fits_properties.port == 0)
            {
                FIERY::FITS::CORE::LOG::MANAGER::LogError("FITS-Client[Configuration]: INVALID Port", true);
            }
            else
            {
                // if so, attempt to "create" the CONFIG_CONTROLLER.
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
        }
        else
        {
            FIERY::FITS::CORE::LOG::MANAGER::LogError("FITS-Client[Initialize]: FAILED", true);
        }

        FIERY::FITS::CORE::LOG::MANAGER::LogInfo("FITS-Client: STOPPED");
    }

    return(result);
}
