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
#include "SocketController.h"

#include "fieryImageTransferServiceIfc.pb.h"

#include <boost/asio/connect.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>

using namespace FIERY::FITS::CORE::SOCKET;


/**********************************************************************************************************************************************************************************/
/*                                                                                     methods                                                                                    */
/**********************************************************************************************************************************************************************************/
CONTROLLER::CONTROLLER(const std::string & name) : m_enabled(true), m_io_context(), m_name(name), m_socket(m_io_context)
{
    ;
}

CONTROLLER::~CONTROLLER()
{
    // attempt to "stop" the CONTROLLER.
    Stop();
}

bool CONTROLLER::Connect(const std::string & ip, const uint32_t & port, const std::string & ip_local_bind, std::string & error)
{
    boost::asio::ip::tcp::resolver::query   query(ip, std::to_string(port));
    boost::asio::ip::tcp::resolver          resolver(m_io_context);

    bool                                    successful = true;


    // initialize the return variables.
    error.clear();

    try
    {
        // attempt to "connect" the CONTROLLER.
        boost::asio::connect(m_socket, resolver.resolve(query));

        // check to see if the "local bind" IP is "valid".
        if (!ip_local_bind.empty())
        {
            boost::asio::ip::tcp::endpoint   endpoint_local(boost::asio::ip::make_address(ip_local_bind), 0);
            boost::system::error_code        error_code;

            m_socket.bind(endpoint_local, error_code);

            // check to see if everything was successful -- if not, attempt to "log" the "error" and return immediately.
            if (error_code)
            {
                LOG::MANAGER::LogError("CONTROLLER::Connect[" + m_name + "]: FAILED (Error = <" + error_code.message() + ">)");
            }
        }

        // The worker thread will run the m_io_context
        m_io_context.run();
    }
    catch (const boost::system::system_error & system_error)
    {
        // if unsuccessful, update the return variables.
        successful = false;
        error      = system_error.what();
    }

    // return the status.
    return(successful);
}

uint64_t CONTROLLER::Read(uint8_t * p_buffer, const uint64_t & buffer_size_in_bytes)
{
    boost::system::error_code   error_code;
    uint64_t                    number_of_bytes_read;

    uint64_t                    number_of_bytes_remaining = buffer_size_in_bytes;
    uint8_t *                   p_buffer_current          = p_buffer;


    // loop continuously, attempt to "read" the BUFFER.
    while ((number_of_bytes_remaining != 0) && m_socket.is_open())
    {
        // attempt to "read" the BUFFER.
        number_of_bytes_read = boost::asio::read(m_socket, boost::asio::buffer(p_buffer_current, number_of_bytes_remaining), error_code);

        // check to see if everything was successful -- if not, attempt to "log" the "error" and return immediately.
        if (error_code)
        {
            LOG::MANAGER::LogError("CONTROLLER::Read[" + m_name + "]: FAILED (Error = <" + error_code.message() + ">)");
            return(-1);
        }

        // update the local variables.
        number_of_bytes_remaining -= number_of_bytes_read;
        p_buffer_current          += number_of_bytes_read;
    }

    // return the status.
    return(buffer_size_in_bytes);
}

void CONTROLLER::ReadAsync(std::vector<uint8_t>& rec_buf, size_t size_to_receive, std::function<void(int)> callback)
{
    boost::system::error_code   error_code;

    size_t                      total_bytes_received = 0;


    // Until desired size buffer received or error
    while (total_bytes_received < size_to_receive && m_socket.is_open())
    {
        size_t remaining_bytes = size_to_receive - total_bytes_received;
        size_t buffer_offset = total_bytes_received;

        async_read(m_socket, boost::asio::buffer(rec_buf.data() + buffer_offset, remaining_bytes), [&error_code, &total_bytes_received, &rec_buf, callback](const boost::system::error_code & error, std::size_t bytes_received)
        {
            error_code = error;
        });

        Run();

        if (m_enabled)
        {
            if (error_code)
            {
                LOG::MANAGER::LogError("CONTROLLER::ReadAsync[" + m_name + "]: FAILED (Error = <" + error_code.message() + ">)");

                if (error_code == boost::asio::error::operation_aborted || error_code == boost::asio::error::connection_reset || error_code == boost::asio::error::shut_down)
                {
                    callback(CONTROLLER_ERROR::CONNECTION_ERROR);
                    return;
                }
                else
                {
                    callback(CONTROLLER_ERROR::READ_ERROR);
                    return;
                }
            }
        }
        else
        {
            callback(CONTROLLER_ERROR::SOCKET_CLOSED);
            return;
        }

        total_bytes_received += rec_buf.size();
    }

    callback(static_cast<int>(total_bytes_received));
}

void CONTROLLER::Run(void)
{
    m_io_context.restart();
    m_io_context.run();
}

void CONTROLLER::Stop()
{
    m_enabled = false;
    close();
}

void CONTROLLER::Write(const std::vector<uint8_t> & buffer)
{
    boost::asio::write(m_socket, boost::asio::buffer(buffer));
}

void CONTROLLER::close()
{
    boost::system::error_code   error_code;


    // check to see if the SOCKET is "open".
    if (m_socket.is_open())
    {
        // if so, attempt to "shutdown" the SOCKET.
        m_socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, error_code);

        // check to see if there 
        if (error_code)
        {
            LOG::MANAGER::LogInfo("CONTROLLER::close[" + m_name + "]: FAILED (Error = <" + error_code.message() + ">)");
        }

        // attempt to "close" the SOCKET.
        m_socket.close();
    }
}
