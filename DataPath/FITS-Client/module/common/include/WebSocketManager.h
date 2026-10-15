/**********************************************************************************************************************************************************************************/
/*                                                                                                                                                                                */
/*  Copyright (c) 1990-2026 Fiery, LLC.  All Rights Reserved.                                                                                                                     */
/*                                                                                                                                                                                */
/*  Fiery, LLC ("Fiery") products contain certain trade secrets and confidential and proprietary information of Fiery.  Use, reproduction, disclosure, distribution by any means  */
/*  are prohibited, except pursuant to a written license from Fiery.  Modification, translation, reverse engineering, decompiling, disassembling, and creating derivative works   */
/*  based on this software are prohibited, except pursuant to a written license from Fiery. Use of copyright notice does not imply publication or disclosure.                     */
/*                                                                                                                                                                                */
/**********************************************************************************************************************************************************************************/
#ifndef __WEB_SOCKET_MANAGER_H__
#define __WEB_SOCKET_MANAGER_H__


/**********************************************************************************************************************************************************************************/
/*                                                                                    includes                                                                                    */
/**********************************************************************************************************************************************************************************/
#include "PropertiesManager.h"

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/websocket/stream.hpp>
#include <boost/property_tree/ptree.hpp>

#include <cstdint>
#include <string>
#include <thread>


/**********************************************************************************************************************************************************************************/
/*                                                            typedefs, structures, enumerations, and related defines                                                             */
/**********************************************************************************************************************************************************************************/
namespace FIERY
{
    namespace FITS
    {
        namespace MODULE
        {
		    // forward declarations.
		    class MANAGER;

            namespace WEB_SOCKET
            {
                // class declarations.
                class MANAGER
                {
                    public:
                    // public constructors and destructor.
                    MANAGER();
                    virtual ~MANAGER();

                    // public "management" methods.
                    bool Start(FIERY::FITS::MODULE::MANAGER * p_module_mgr, const FIERY::FITS::MODULE::PROPERTIES::FIERY_PATH_CLIENT & fiery_path_client);
                    bool Stop(void);

                    // public IMAGE "management" methods.
                    bool ImagePrinted(const std::string & job_id, const std::string & image_id);


                    private:
                    // private "management" methods.
                    bool connect(void);
                    bool disconnect(void);
                    bool processEvent(const boost::property_tree::ptree & json_object);
                    bool processEventAbort(const boost::property_tree::ptree & json_object);
                    bool processEventCleanup(const boost::property_tree::ptree & json_object);
                    bool processEventImageProperties(const boost::property_tree::ptree & json_object);
                    bool processEventPrint(const boost::property_tree::ptree & json_object);
                    void run(void);
                    bool sendConnect(void);
                    void waitForStop(void);

                    // private "management" variables.
                    bool                                                              m_connected;
                    FIERY::FITS::MODULE::PROPERTIES::FIERY_PATH_CLIENT                m_fiery_path_client;
                    boost::asio::io_context                                           m_io_context;
                    std::thread                                                       m_thread;
                    bool                                                              m_thread_enabled;
                    FIERY::FITS::MODULE::MANAGER *                                    m_p_module_mgr;
                    boost::beast::websocket::stream<boost::asio::ip::tcp::socket> *   m_p_web_socket;
                    boost::beast::flat_buffer                                         m_read_buffer;
                    bool                                                              m_started;
                    bool                                                              m_stop;

                    // private static "management" constants.
                    static const std::string   JSON_PROPERTY_IMAGE_ID;
                    static const std::string   JSON_PROPERTY_IMAGE_PROPERTIES;
                    static const std::string   JSON_PROPERTY_INKS;
                    static const std::string   JSON_PROPERTY_JOB_ID;
                    static const std::string   JSON_PROPERTY_JOB_NAME;
                    static const std::string   JSON_PROPERTY_SETTINGS;
                    static const std::string   JSON_PROPERTY_TYPE;
                    static const uint32_t      SLEEP_IN_SECONDS_CONNECT;
                    static const std::string   TYPE_ABORT;
                    static const std::string   TYPE_CLEAN_UP;
                    static const std::string   TYPE_CONNECT;
                    static const std::string   TYPE_IMAGE_PROPERTIES;
                    static const std::string   TYPE_PRINT;
                    static const std::string   TYPE_PRINTED;
                    static const std::string   WEB_SOCKET_VERSION;
                };
            }
        }
    }
}
#endif // __WEB_SOCKET_MANAGER_H__
