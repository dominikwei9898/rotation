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
#include "ConfigController.h"
#include "LogManager.h"

#include "fieryImageTransferServiceIfc.h"
#include "fieryImageTransferServiceIfc.pb.h"

#include <chrono>

using namespace FIERY::FITS::CORE::CONFIG;


/**********************************************************************************************************************************************************************************/
/*                                                                                local variables                                                                                 */
/**********************************************************************************************************************************************************************************/
const std::string   CONTROLLER::NAME                     = "Configuration";
const uint32_t      CONTROLLER::PORT_DEFAULT             = 9210;
const uint32_t      CONTROLLER::SLEEP_IN_SECONDS_CONNECT = 5;
const uint32_t      CONTROLLER::SLEEP_IN_SECONDS_IDLE    = 10;


/**********************************************************************************************************************************************************************************/
/*                                                                                     methods                                                                                    */
/**********************************************************************************************************************************************************************************/
CONTROLLER::CONTROLLER(const FIERY::FITS::MODULE::PROPERTIES::FITS_SERVER & fits_server_properties, boost::shared_ptr<FIERY::FITS::MODULE::PLUG_IN> & p_module_plug_in) :
            m_colorant_controller_list(), m_connected(false), m_enabled(true), m_fits_server_properties(fits_server_properties), m_name(CONTROLLER::NAME),
            m_p_module_plug_in(p_module_plug_in), m_socket_controller(nullptr), m_thread(), m_thread_mutex(), m_thread_signal()
{
    m_socket_controller = new SOCKET::CONTROLLER(m_name);

    // check to see if the FITS "port" is valid -- if not, update the "management" variables.
    if (m_fits_server_properties.port == 0)
    {
        m_fits_server_properties.port = PORT_DEFAULT;
    }
}

CONTROLLER::~CONTROLLER()
{
    if (m_socket_controller != nullptr)
    {
        delete m_socket_controller;
    }
}

int CONTROLLER::Start()
{
    int   result = -1;


    // check to see if the IP is "valid" -- if not, return immediately.
    if (m_fits_server_properties.ip.empty() || (m_fits_server_properties.ip == ""))
    {
        FIERY::FITS::CORE::LOG::MANAGER::LogError("FITS-Client[" + m_name + "]: INVALID IP (Value = <" + m_fits_server_properties.ip + ">)", true);
        return(result);
    }

    // attempt to "run" the CONTROLLER.
    m_thread = std::thread(std::bind(&CONTROLLER::run, this));

    // attempt to "wait" for the CONTROLLER to "exit".
    waitForExit();

    // update the status and return.
    result = 0;
    return(result);
}

void CONTROLLER::Stop()
{
    // update the "management" variables.
    m_enabled = false;

    // loop through all COLORANT_CONTROLLERs, attempting to "stop" them.
    for (auto p_colorant_controller : m_colorant_controller_list)
    {
        p_colorant_controller->Stop();
    }

    // attempt to "wait" for the CONTROLLER to "stop".
    waitForStop();

    LOG::MANAGER::LogInfo("FITS-Client[" + m_name + "]: STOPPED");
}

bool CONTROLLER::connect(void)
{
    std::string   error;

    bool          connected               = false;
    bool          connection_error_logged = false;


    // loop continuously, attempting to "connect" to the FITS_SERVER.
    while (!connected)
    {
        // attempt to "connect" the SOCKET::CONTROLLER.
        if (!(connected = m_socket_controller->Connect(m_fits_server_properties.ip, m_fits_server_properties.port, error)))
        {
            // check to see we need to "log" the "error" -- if so, do it.
            if (!connection_error_logged)
            {
                LOG::MANAGER::LogError("FITS-Client[" + m_name + "]: connect FAILED (IP = <" + m_fits_server_properties.ip + ">, Port = " +
                            std::to_string(m_fits_server_properties.port) + ", Error = <" + error + ">)", true);
                connection_error_logged = true;
            }

            // check to see if everything is still "enabled" -- if not, return immediately.
            if (!m_enabled)
            {
                return(connected);
            }

            // otherwise, attempt to "sleep" for a bit and then try again.
            std::this_thread::sleep_for(std::chrono::seconds(SLEEP_IN_SECONDS_CONNECT));
        }
    }

    LOG::MANAGER::LogInfo("FITS-Client[" + m_name + "]: CONNECTED (IP = <" + m_fits_server_properties.ip + ">, Port = " + std::to_string(m_fits_server_properties.port) + ")",
                true);

    // return the status.
    return(connected);
}

void CONTROLLER::connectColorants(void)
{
    uint64_t   number_of_bytes_read;

    uint32_t   number_of_bytes_fits_header = 0;


    // attempt to "send" the CLIENT_CONNECTED "message".
    sendClientConnected();

    // attempt to "read" the FITS_HEADER_LENGTH.
    if ((number_of_bytes_read = m_socket_controller->Read(reinterpret_cast<uint8_t *>(&(number_of_bytes_fits_header)), FITS_HEADER_LENGTH)) == FITS_HEADER_LENGTH)
    {
        // if successful, attempt to "create" a BUFFER for the FITS_HEADER.
        std::vector<uint8_t>   buffer_header(number_of_bytes_fits_header);

        // attempt to "read" the FITS_HEADER.
        if ((number_of_bytes_read = m_socket_controller->Read(buffer_header.data(), number_of_bytes_fits_header)) == number_of_bytes_fits_header)
        {
            EFI::FITS::CONSTANT::COMMON_HEADER   common_header;


            // if successful, attempt to "parse" the FITS_HEADER.
            if (common_header.ParseFromArray(buffer_header.data(), static_cast<int>(number_of_bytes_fits_header)) && common_header.has_tag() && common_header.has_version() &&
                        common_header.has_message_type() && common_header.has_message_size_in_bytes())
            {
                uint32_t                            message_size_in_bytes = common_header.message_size_in_bytes();
                EFI::FITS::CONSTANT::MESSAGE_TYPE   message_type          = static_cast<EFI::FITS::CONSTANT::MESSAGE_TYPE>(common_header.message_type());


                // check to see if the FITS_HEADER contains a SERVER_CONFIG "message".
                if ((message_size_in_bytes != 0) && (message_type == EFI::FITS::CONSTANT::SERVER_CONFIG))
                {
                    // if so, attempt to "create" a BUFFER for the SERVER_CONFIG.
                    std::vector<uint8_t>   buffer_server_configuration(message_size_in_bytes);

                    // attempt to "read" the SERVER_CONFIG.
                    if ((number_of_bytes_read = m_socket_controller->Read(buffer_server_configuration.data(), message_size_in_bytes)) == message_size_in_bytes)
                    {
                        // if successful, attempt to "process" the SERVER_CONFIG.
                        processServerConfiguration(buffer_server_configuration);
                    }
                    else
                    {
                        LOG::MANAGER::LogError("SERVER_CONFIG: FAILED (Expected = " + std::to_string(message_size_in_bytes) + ", Result = " + std::to_string(number_of_bytes_read) +
                                    ")");
                    }
                }
                else
                {
                    LOG::MANAGER::LogError("FITS_HEADER: INVALID (Type = " + std::to_string(message_type) + ", Size = " + std::to_string(message_size_in_bytes) + ")");
                }
            }
            else
            {
                LOG::MANAGER::LogError("FITS_HEADER: INVALID");
            }
        }
    }
}

// Process the received server configuration data and create Colorant Controllers
void CONTROLLER::processServerConfiguration(const std::vector<uint8_t> & buffer_server_configuration)
{
    std::list<std::string>                       colorant_list;
    EFI::FITS::CONSTANT::MESSAGE_SERVER_CONFIG   received_message;

    bool                                         successful = true;


    if (received_message.ParseFromArray(buffer_server_configuration.data(), static_cast<int>(buffer_server_configuration.size())) && received_message.has_inks())
    {
        EFI::FITS::CONSTANT::COMMON_MAPPINGS   mappings = received_message.inks();


        // check to see if the NUMBER_OF_INKS is "valid".
        if (mappings.has_number_of_inks())
        {
            // if so, loop through each INK/COLORANT, attempting to "create" a COLORANT_CONTROLLER for each.
            for (uint32_t index = 0; index < mappings.number_of_inks(); index++)
            {
                FIERY::FITS::CORE::COLORANT::CONTROLLER *   p_colorant_controller = nullptr;
                EFI::FITS::CONSTANT::COMMON_PORTS           ports                 = mappings.ink_ports(index);


                // attempt to "create" a COLORANT_CONTROLLER.
                if ((p_colorant_controller = new FIERY::FITS::CORE::COLORANT::CONTROLLER(m_fits_server_properties.ip, ports.port(), ports.ink(), m_p_module_plug_in)) != nullptr)
                {
                    // if successful, attempt to "start" the COLORANT_CONTROLLER.
                    p_colorant_controller->Start();

                    // update the "management" variables.
                    m_colorant_controller_list.push_back(p_colorant_controller);

                    // update the local variables.
                    colorant_list.push_back(ports.ink());
                }
            }

            // attempt to "start" the FITS-Client "module" MANAGER -- if unsuccessful, update the local variables.
            if (!m_p_module_plug_in->Start(colorant_list))
            {
                successful = false;
            }
        }
        else
        {
            LOG::MANAGER::LogError("SERVER_CONFIG: INVALID inks/colorants count");

            // otherwise, update the local variables.
            successful = false;
        }
    }
    else
    {
        LOG::MANAGER::LogError("SERVER_CONFIG: MISSING inks/colorants");

        // otherwise, update the local variables.
        successful = false;
    }

    // check to see if everything was successful -- if not, attempt to "stop" the CONTROLLER.
    if (!successful)
    {
        Stop();
    }
}

void CONTROLLER::run(void)
{
    LOG::MANAGER::LogInfo("FITS-Client[" + m_name + "]: RUNNING");

    // loop continuously while "enabled".
    while (m_enabled)
    {
        // check to see if we need to "connect" to the FITS_SERVER -- if so, do it.
        if (!m_connected)
        {
            m_connected = connect();
        }

        // check to see if we need to "connect" the COLORANTs -- if so, do it.
        if (m_connected && m_colorant_controller_list.empty())
        {
            connectColorants();
        }

        // attempt to "sleep" for a bit (depending on if everything is "initialized" or not).
        std::this_thread::sleep_for(std::chrono::seconds((m_connected && !m_colorant_controller_list.empty()) ? SLEEP_IN_SECONDS_IDLE : SLEEP_IN_SECONDS_CONNECT));
    }
}

void CONTROLLER::sendClientConnected(void)
{
    EFI::FITS::CONSTANT::COMMON_HEADER   header;
    int                                  header_size;


    // Create a COMMON_HEADER message and set its values
    header.set_tag(EFI::FITS::CONSTANT::FITS);
    header.set_version(EFI::FITS::CONSTANT::VERSION_1);
    header.set_message_type(EFI::FITS::CONSTANT::CLIENT_CONNECTED);
    header.set_message_size_in_bytes(0);
    header_size = header.ByteSize();

    // Serialize the header to an array
    std::vector<uint8_t>   buffer_header(header_size);
    header.SerializeToArray(buffer_header.data(), header_size);

    // Send CLIENT_CONNECTED message to server
    m_socket_controller->Write(buffer_header);
}

void CONTROLLER::waitForExit(void)
{
    // attempt to "take" the THREAD "mutex".
    std::unique_lock<std::mutex>   lock(m_thread_mutex);

    // attempt to "wait" for the THREAD "signal".
    m_thread_signal.wait(lock);
}

void CONTROLLER::waitForStop(void)
{
    if (m_thread.joinable())
    {
        m_thread.join();
    }

    for (auto p_colorant_controller : m_colorant_controller_list)
    {
        p_colorant_controller->WaitForExit();
        delete p_colorant_controller;
    }

    m_colorant_controller_list.clear();

    // attempt to "stop" the FITS-Client "module" MANAGER.
    m_p_module_plug_in->Stop();

    // attempt to "set" the THREAD "signal".
    m_thread_signal.notify_all();
}
