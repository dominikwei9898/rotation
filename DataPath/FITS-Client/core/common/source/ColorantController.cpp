/**********************************************************************************************************************************************************************************/
/*                                                                                                                                                                                */
/*  Copyright (c) 1990-2026 Fiery, LLC.  All Rights Reserved.                                                                                                                     */
/*                                                                                                                                                                                */
/*  Fiery, LLC ("Fiery") products contain certain trade secrets and confidential and proprietary information of Fiery.  Use, reproduction, disclosure, distribution by any means  */
/*  are prohibited, except pursuant to a written license from Fiery.  Modification, translation, reverse engineering, decompiling, disassembling, and creating derivative works   */
/*  based on this software are prohibited, except pursuant to a written license from Fiery. Use of copyright notice does not imply publication or disclosure.                     */
/*                                                                                                                                                                                */
/**********************************************************************************************************************************************************************************/
#include "ColorantController.h"
#include "LogManager.h"

#include "fieryImageTransferServiceIfc.h"
#include "fieryImageTransferServiceIfc.pb.h"

#include <chrono>
#include <cstring>

using namespace FIERY::FITS::CORE::COLORANT;


/**********************************************************************************************************************************************************************************/
/*                                                                                local variables                                                                                 */
/**********************************************************************************************************************************************************************************/
const uint32_t   CONTROLLER::SLEEP_IN_SECONDS_CONNECT = 5;


/**********************************************************************************************************************************************************************************/
/*                                                                                     methods                                                                                    */
/**********************************************************************************************************************************************************************************/
CONTROLLER::CONTROLLER(const std::string & ip, const uint16_t & port, const std::string & ip_local_bind, const std::string & colorant, boost::shared_ptr<FIERY::FITS::MODULE::PLUG_IN> & p_module_plug_in) :
            m_buffer_fits_header_length(FITS_HEADER_LENGTH), m_colorant(colorant), m_connected(false), m_enabled(true), m_ip(ip), m_ip_local_bind(ip_local_bind),
            m_p_module_plug_in(p_module_plug_in), m_port(port), m_socket_controller(nullptr), m_thread()
{
}

CONTROLLER::~CONTROLLER()
{
    if (m_socket_controller != nullptr)
    {
        delete m_socket_controller;
    }
}

void CONTROLLER::Start(void)
{
    m_socket_controller = new SOCKET::CONTROLLER(m_colorant);
    m_thread = std::thread(std::bind(&CONTROLLER::run, this));
}

void CONTROLLER::Stop(void)
{
    // update the "management" variables.
    m_enabled = false;

    // attempt to "stop" the SOCKET::CONTROLLER.
    m_socket_controller->Stop();
}

void   CONTROLLER::WaitForExit(void)
{
    if (m_thread.joinable())
    {
        m_thread.join();
    }
};

bool CONTROLLER::connect(void)
{
    std::string   error;

    bool          connected               = false;
    bool          connection_error_logged = false;


    // loop continuously, attempting to "connect" to the FITS_SERVER.
    while (!connected)
    {
        // attempt to "connect" the SOCKET::CONTROLLER.
        if (!(connected = m_socket_controller->Connect(m_ip, m_port, m_ip_local_bind, error)))
        {
            // check to see we need to "log" the "error" -- if so, do it.
            if (!connection_error_logged)
            {
                LOG::MANAGER::LogError("FITS-Client[" + m_colorant + "]: connect FAILED (IP = <" + m_ip + ">, Port = " + std::to_string(m_port) + ", Error = <" + error + ">)");
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

    LOG::MANAGER::LogInfo("FITS-Client[" + m_colorant + "]: CONNECTED (IP = <" + m_ip + ">, Port = " + std::to_string(m_port) + ")");

    // return the status.
    return(connected);
}

void CONTROLLER::run()
{
    // loop continuously while "enabled".
    while (m_enabled)
    {
        // check to see if we need to "connect" to the FITS_SERVER "colorant" -- if so, do it.
        if (!m_connected)
        {
            m_connected = connect();
        }

        // check to see if we need to "read" from the FITS_SERVER "colorant".
        if (m_connected)
        {
            // if so, attempt to "read" the FITS_HEADER_LENGTH.
            m_socket_controller->ReadAsync(m_buffer_fits_header_length, FITS_HEADER_LENGTH, [this](int return_status)
            {
                // check to see if the "read" was successful.
                if (return_status == FITS_HEADER_LENGTH)
                {
                    // if so, attempt to "process" the FITS_HEADER_LENGTH.
                    processHeaderLength();
                }
                else if (return_status == FIERY::FITS::CORE::SOCKET::CONTROLLER_ERROR::SOCKET_CLOSED)
                {
                    // update the "management" variables.
                    m_connected = false;
                }
                else if (return_status == FIERY::FITS::CORE::SOCKET::CONTROLLER_ERROR::CONNECTION_ERROR)
                {
                    LOG::MANAGER::LogError("SOCKET[" + m_colorant + "]: CONNECTION_ERROR");

                    // update the "management" variables.
                    m_connected = false;
                }
                else if (return_status == FIERY::FITS::CORE::SOCKET::CONTROLLER_ERROR::READ_ERROR)
                {
                    LOG::MANAGER::LogError("SOCKET[" + m_colorant + "]: READ_ERROR");
                }
                else
                {
                    LOG::MANAGER::LogError("SOCKET[" + m_colorant + "]: ERROR (Status = " + std::to_string(return_status) + ")");
                }
            });
        }
    }
}

void CONTROLLER::processHeader(const std::vector<uint8_t> & buffer_header)
{
    EFI::FITS::CONSTANT::COMMON_HEADER   header;


    // attempt to "parse" the FITS_HEADER.
    if (header.ParseFromArray(buffer_header.data(), static_cast<int>(buffer_header.size())) && header.has_tag() && header.has_version() && header.has_message_type() && \
                header.has_message_size_in_bytes())
    {
        uint32_t                            message_size_in_bytes = header.message_size_in_bytes();
        EFI::FITS::CONSTANT::MESSAGE_TYPE   message_type          = header.message_type();
        EFI::FITS::CONSTANT::TAG            tag                   = header.tag();
        EFI::FITS::CONSTANT::VERSION        version               = header.version();


        // if successful, check to see if there's IMAGE "data" to "read".
        if ((message_type == EFI::FITS::CONSTANT::SERVER_DATA) && (message_size_in_bytes != 0))
        {
            uint64_t               number_of_bytes_read_image_data;
            std::vector<uint8_t>   image_buffer(message_size_in_bytes);


            // if so, attempt to "read" the IMAGE "data".
            if ((number_of_bytes_read_image_data = m_socket_controller->Read(image_buffer.data(), message_size_in_bytes)) == message_size_in_bytes)
            {
                // if successful, attempt to "process" the IMAGE "data".
                processServerData(image_buffer);
            }
            else
            {
                LOG::MANAGER::LogError("IMAGE[" + m_colorant + "]: FAILED (Expected = " + std::to_string(message_size_in_bytes) + ", Result = " +
                            std::to_string(number_of_bytes_read_image_data) + ")");
            }
        }
        else
        {
            LOG::MANAGER::LogError("FITS_HEADER[" + m_colorant + "]: INVALID (Type = " + std::to_string(message_type) + ", Size = " + std::to_string(message_size_in_bytes) + ")");
        }
    }
    else
    {
        LOG::MANAGER::LogError("FITS_HEADER[" + m_colorant + "]: INVALID");
    }
}

void CONTROLLER::processHeaderLength(void)
{
    uint64_t   number_of_bytes_read_header;

    uint32_t   number_of_bytes_read_header_length = 0;


    // if so, attempt to "convert" the BUFFER contents to the FITS_HEADER "size" (little-endian).
    std::memcpy(&(number_of_bytes_read_header_length), m_buffer_fits_header_length.data(), sizeof(uint32_t));

    // attempt to "create" a BUFFER for the FITS_HEADER.
    std::vector<uint8_t>   buffer_header(number_of_bytes_read_header_length);


    // attempt to "read" the FITS_HEADER.
    if ((number_of_bytes_read_header = m_socket_controller->Read(buffer_header.data(), number_of_bytes_read_header_length)) == number_of_bytes_read_header_length)
    {
        // if successful, attempt to "process" the FITS_HEADER.
        processHeader(buffer_header);
    }
    else
    {
        LOG::MANAGER::LogError("HEADER[" + m_colorant + "]: FAILED (Expected = " + std::to_string(number_of_bytes_read_header_length) + ", Result = " +
                    std::to_string(number_of_bytes_read_header) + ")");
    }
}

void CONTROLLER::processServerData(const std::vector<uint8_t> & buffer_image_data)
{
    EFI::FITS::CONSTANT::MESSAGE_SERVER_DATA   message;

    // attempt to "parse" the SERVER_DATA.
    if (message.ParseFromArray(buffer_image_data.data(), static_cast<int>(buffer_image_data.size())) && message.has_job())
    {
        EFI::FITS::CONSTANT::COMMON_JOB   job = message.job();


        // check to see if the JOB is "valid".
        if (job.has_id() && job.has_image())
        {
            EFI::FITS::CONSTANT::COMMON_IMAGE   image  = job.image();
            std::string                         job_id = job.id();


            // check to see if the IMAGE is "valid".
            if (image.has_ink() && image.has_id() && image.has_bits_per_pixel() && image.has_pixels() && image.has_resolution() && image.resolution().has_x() && \
                        image.resolution().has_y() && image.has_size_in_bytes())
            {
                uint64_t               number_of_bytes_read_image;

                std::string            image_id            = image.id();
                uint64_t               image_size_in_bytes = image.size_in_bytes();
                std::string            ink_name            = image.ink();

                std::vector<uint8_t>   image_buffer(image_size_in_bytes);


                // check to see if the COLORANT is "valid" -- if not, attempt to "log" an "error".
                if (ink_name != m_colorant)
                {
                    LOG::MANAGER::LogError("SERVER_DATA[" + m_colorant + "]: INVALID Colorant (Ink = " + ink_name + ")");
                }

                // attempt to "read" the IMAGE.
                if ((number_of_bytes_read_image = m_socket_controller->Read(image_buffer.data(), image_size_in_bytes)) == image_size_in_bytes)
                {
                    FIERY::FITS::MODULE::PROPERTIES::IMAGE   image_properties(job.id(), image.id(), image.bits_per_pixel(), image.bytes_per_scanline(), ink_name,
                                image.pixels().x(), image.pixels().y(), image.resolution().x(), image.resolution().y());

                    // if successful, attempt to "save" the IMAGE.
                    m_p_module_plug_in->ImageSave(image_properties, image_buffer);
                }
                else
                {
                    LOG::MANAGER::LogError("SERVER_DATA[" + m_colorant + "]: FAILED (Expected = " + std::to_string(image_size_in_bytes) +  ", Result = " +
                                std::to_string(number_of_bytes_read_image) + ")");
                }
            }
            else
            {
                LOG::MANAGER::LogError("SERVER_DATA[" + m_colorant + "]: INVALID Image Information");
            }
        }
        else
        {
            LOG::MANAGER::LogError("SERVER_DATA[" + m_colorant + "]: INVALID Job Information");
        }
    }
    else
    {
        LOG::MANAGER::LogError("SERVER_DATA[" + m_colorant + "]: INVALID");
    }
}
