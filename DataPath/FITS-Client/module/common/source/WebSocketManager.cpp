/**********************************************************************************************************************************************************************************/
/*                                                                                                                                                                                */
/*  Copyright (c) 1990-2023 Fiery, LLC.  All Rights Reserved.                                                                                                                     */
/*                                                                                                                                                                                */
/*  Fiery, LLC ("Fiery") products contain certain trade secrets and confidential and proprietary information of Fiery.  Use, reproduction, disclosure, distribution by any means  */
/*  are prohibited, except pursuant to a written license from Fiery.  Modification, translation, reverse engineering, decompiling, disassembling, and creating derivative works   */
/*  based on this software are prohibited, except pursuant to a written license from Fiery. Use of copyright notice does not imply publication or disclosure.                     */
/*                                                                                                                                                                                */
/**********************************************************************************************************************************************************************************/
#include "WebSocketManager.h"

#include "LogManager.h"
#include "ModuleManager.h"

#include <boost/asio/connect.hpp>
#include <boost/beast/core/buffers_to_string.hpp>
#include <boost/filesystem.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <chrono>
#include <sstream>
#include <utility>

using namespace FIERY::FITS::MODULE::WEB_SOCKET;


/**********************************************************************************************************************************************************************************/
/*                                                         local typedefs, structures, enumerations, and related defines                                                          */
/**********************************************************************************************************************************************************************************/


/**********************************************************************************************************************************************************************************/
/*                                                                                local variables                                                                                 */
/**********************************************************************************************************************************************************************************/
const std::string   MANAGER::JSON_PROPERTY_IMAGE_ID   = "imageId";
const std::string   MANAGER::JSON_PROPERTY_INKS       = "inks";
const std::string   MANAGER::JSON_PROPERTY_JOB_ID     = "jobId";
const std::string   MANAGER::JSON_PROPERTY_JOB_NAME   = "jobName";
const std::string   MANAGER::JSON_PROPERTY_TYPE       = "type";
const uint32_t      MANAGER::SLEEP_IN_SECONDS_CONNECT = 5;
const std::string   MANAGER::TYPE_CANCEL              = "cancel";
const std::string   MANAGER::TYPE_CONNECT             = "connect";
const std::string   MANAGER::TYPE_PRINT               = "print";
const std::string   MANAGER::TYPE_PRINTED             = "printed";
const std::string   MANAGER::WEB_SOCKET_VERSION       = "FITS-Client.WebSocket/1.0";


/**********************************************************************************************************************************************************************************/
/*                                                                                     methods                                                                                    */
/**********************************************************************************************************************************************************************************/
MANAGER::MANAGER() : m_connected(false), m_fiery_path_server(), m_io_context(), m_thread(), m_thread_enabled(false), m_p_module_mgr(nullptr), m_p_web_socket(nullptr),
            m_read_buffer(), m_started(false)
{
    ;
}

MANAGER::~MANAGER()
{
    delete m_p_web_socket;
}

bool MANAGER::ImagePrinted(const std::string & job_id, const std::string & image_id)
{
    std::ostringstream            buffer;
    boost::beast::error_code      error_code;
    boost::property_tree::ptree   json_object;

    bool                          successful = false;


    // initialize the local variables.
    json_object.put(JSON_PROPERTY_TYPE, TYPE_PRINTED);
    json_object.put(JSON_PROPERTY_JOB_ID, job_id);
    json_object.put(JSON_PROPERTY_IMAGE_ID, image_id);

    // attempt to "get" the JSON "buffer".
    boost::property_tree::write_json(buffer, json_object, false);

    // attempt to "write" the JSON "buffer".
    successful = m_p_web_socket->write(boost::asio::buffer(buffer.str()), error_code) == buffer.str().length();

    // return the status.
    return(successful);
}

bool MANAGER::Start(FIERY::FITS::MODULE::MANAGER * p_module_mgr, const FIERY::FITS::MODULE::PROPERTIES::FIERY_PATH_SERVER & fiery_path_server)
{
    bool   successful = true;


    // check to see if we need to "start" the MANAGER.
    if (!m_started)
    {
        // if so, update the "management" variables.
        m_fiery_path_server = fiery_path_server;
        m_p_module_mgr      = p_module_mgr;
        m_p_web_socket      = new boost::beast::websocket::stream<boost::asio::ip::tcp::socket>(m_io_context);
        m_thread_enabled    = true;

        // attempt to "run" the MANAGER.
        m_thread = std::thread(std::bind(&MANAGER::run, this));
    }

    // return the status.
    return(successful);
}

bool MANAGER::Stop(void)
{
    bool   successful = true;


    // attempt to "wait" for the MANAGER to "stop".
    waitForStop();

    // update the "management" variables.
    m_fiery_path_server.clear();

    FIERY::FITS::MODULE::LOG::MANAGER::LogNormal("FITS-Module[WebSocket]: STOPPED");

    // return the status.
    return(successful);
}

bool MANAGER::connect(void)
{
    boost::asio::ip::tcp::resolver   resolver(m_io_context);

    std::string                      host       = m_fiery_path_server.ip;
    bool                             successful = false;


    try
    {
        // attempt to "connect" the WEB_SOCKET.
        auto end_point = boost::asio::connect(m_p_web_socket->next_layer(), resolver.resolve(m_fiery_path_server.ip, std::to_string(m_fiery_path_server.port)));

        // update the "host" string -- this will provide the value of the Host HTTP header during the WebSocket handshake (See https://tools.ietf.org/html/rfc7230#section-5.4).
        host += ':' + std::to_string(end_point.port());

        // attempt to "set" a decorator to change the User-Agent of the handshake.
        m_p_web_socket->set_option(boost::beast::websocket::stream_base::decorator([](boost::beast::websocket::request_type & request)
        {
            request.set(boost::beast::http::field::user_agent, WEB_SOCKET_VERSION);
        }));

        // attempt a WEB_SOCKET "handshake".
        m_p_web_socket->handshake(host, "/");

        // attempt to "send" the "connect" EVENT.
        if (successful = sendConnect())
        {
            FIERY::FITS::MODULE::LOG::MANAGER::LogNormal("FITS-Module[WebSocket]: CONNECTED (IP = <" + m_fiery_path_server.ip + ">, Port = " +
                        std::to_string(m_fiery_path_server.port) + ")");
        }
    }
    catch (std::exception const & exception)
    {
        std::string   error(exception.what());


        FIERY::FITS::MODULE::LOG::MANAGER::LogError("FITS-Module[WebSocket]: connect FAILED (Error = <" + error + ">)");

        // if unsuccessful, update the return variables.
        successful = false;
    }

    // return the status.
    return(successful);
}

bool MANAGER::disconnect(void)
{
    bool   successful = true;


    try
    {
        // attempt to "connect" the WEB_SOCKET.
        m_p_web_socket->close(boost::beast::websocket::close_code::normal);
    }
    catch (std::exception const & exception)
    {
        std::string   error(exception.what());


        FIERY::FITS::MODULE::LOG::MANAGER::LogError("FITS-Module[WebSocket]: disconnect FAILED (Error = <" + error + ">)");

        // if unsuccessful, update the return variables.
        successful = false;
    }

    // return the status.
    return(successful);
}

bool MANAGER::processEvent(const boost::property_tree::ptree & json_object)
{
    bool          successful = false;
    std::string   type       = json_object.get<std::string>(JSON_PROPERTY_TYPE);


    // attempt to "process" the EVENT, appropriately.
    if (type == TYPE_PRINT)
    {
        successful = processEventPrint(json_object);
    }
    else if (type == TYPE_CANCEL)
    {
        successful = processEventCancel(json_object);
    }
    else
    {
        FIERY::FITS::MODULE::LOG::MANAGER::LogError("FITS-Module[WebSocket]: processEvent UNKNOWN (Type = <" + type + ">)");
    }

    // return the status.
    return(successful);
}

bool MANAGER::processEventCancel(const boost::property_tree::ptree & json_object)
{
    std::string   job_id     = json_object.get<std::string>(JSON_PROPERTY_JOB_ID);
    bool          successful = false;


    // attempt to "cancel" the JOB.
    successful = m_p_module_mgr->JobCancel(job_id);

    // return the status.
    return(successful);
}

bool MANAGER::processEventPrint(const boost::property_tree::ptree & json_object)
{
    std::list<std::string>   ink_list;

    std::string              image_id   = json_object.get<std::string>(JSON_PROPERTY_IMAGE_ID);
    std::string              job_id     = json_object.get<std::string>(JSON_PROPERTY_JOB_ID);
    std::string              job_name   = json_object.get<std::string>(JSON_PROPERTY_JOB_NAME, "");
    bool                     successful = false;


    // loop through the INKS, attempting to "get" every INK.
    for (auto & entry : json_object.get_child(JSON_PROPERTY_INKS))
    {
        ink_list.push_back(entry.second.get_value<std::string>());
    }

    // attempt to "transfer" the IMAGE.
    successful = m_p_module_mgr->ImageTransfer(job_id, job_name, image_id, ink_list);

    // return the status.
    return(successful);
}

void MANAGER::run(void)
{
    boost::beast::error_code   error_code;


    FIERY::FITS::MODULE::LOG::MANAGER::LogNormal("FITS-Module[WebSocket]: RUNNING");

    // loop continuously while "enabled".
    while (m_thread_enabled)
    {
        // check to see if we are "connected" to the FIERY_PATH_SERVER.
        if (!m_connected)
        {
            // if not, attempt to "connect" to the FIERY_PATH_SERVER -- if unsuccessful, attempt to "sleep" for a bit and then try again.
            if (!(m_connected = connect()))
            {
                std::this_thread::sleep_for(std::chrono::seconds(SLEEP_IN_SECONDS_CONNECT));
            }
        }
        else
        {
            try
            {
                // otherwise, attempt to "read" from the FIERY_PATH_SERVER.
                if (m_p_web_socket->read(m_read_buffer, error_code) != 0)
                {
                    boost::property_tree::ptree   json_object;
                    std::istringstream            input_string_stream(boost::beast::buffers_to_string(m_read_buffer.data()));


                    // if successful, attempt to "get" the JSON OBJECT.
                    boost::property_tree::read_json(input_string_stream, json_object);

                    // atempt to "process" the EVENT.
                    processEvent(json_object);
                }
                else
                {
                    FIERY::FITS::MODULE::LOG::MANAGER::LogError("FITS-Module[WebSocket]: read FAILED");

                    // attempt to "disconnect" the WEB_SOCKET.
                    disconnect();

                    // update the "management" variables.
                    m_connected = false;
                }

                // update the "management" variables.
                m_read_buffer.clear();
            }
            catch (std::exception const & exception)
            {
                std::string   error(exception.what());


                FIERY::FITS::MODULE::LOG::MANAGER::LogError("FITS-Module[WebSocket]: read FAILED (Error = <" + error + ">)");
            }
        }
    }
}

bool MANAGER::sendConnect(void)
{
    boost::beast::error_code         error_code;
    boost::property_tree::ptree      json_object;
    boost::property_tree::ptree      json_object_inks;
    std::ostringstream               output_string_stream;

    const std::list<std::string> &   colorant_list = m_p_module_mgr->ColorantListGet();
    bool                             successful    = false;


    // initialize the local variables.
    for (auto & colorant : colorant_list)
    {
        boost::property_tree::ptree   json_object_ink;


        json_object_ink.put("", colorant);
        json_object_inks.push_back(std::make_pair("", json_object_ink));
    }

    // initialize the local variables.
    json_object.put(JSON_PROPERTY_TYPE, TYPE_CONNECT);
    json_object.add_child(JSON_PROPERTY_INKS, json_object_inks);

    // attempt to "get" the JSON "buffer".
    boost::property_tree::write_json(output_string_stream, json_object, false);

    // attempt to "write" the JSON "buffer".
    successful = m_p_web_socket->write(boost::asio::buffer(output_string_stream.str()), error_code) == output_string_stream.str().length();

    // return the status.
    return(successful);
}

void MANAGER::waitForStop(void)
{
    // update the "management" variables.
    m_thread_enabled = false;

    // attempt to "disconnect" the WEB_SOCKET.
    disconnect();

    // check to see if we can "wait" for the THREAD to "exit" -- if so, do it.
    if (m_thread.joinable())
    {
        m_thread.join();
    }
}

