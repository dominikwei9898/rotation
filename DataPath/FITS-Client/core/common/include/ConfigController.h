/**********************************************************************************************************************************************************************************/
/*                                                                                                                                                                                */
/*  Copyright (c) 1990-2026 Fiery, LLC.  All Rights Reserved.                                                                                                                     */
/*                                                                                                                                                                                */
/*  Fiery, LLC ("Fiery") products contain certain trade secrets and confidential and proprietary information of Fiery.  Use, reproduction, disclosure, distribution by any means  */
/*  are prohibited, except pursuant to a written license from Fiery.  Modification, translation, reverse engineering, decompiling, disassembling, and creating derivative works   */
/*  based on this software are prohibited, except pursuant to a written license from Fiery. Use of copyright notice does not imply publication or disclosure.                     */
/*                                                                                                                                                                                */
/**********************************************************************************************************************************************************************************/
#ifndef __CONFIG_CONTROLLER_H__
#define __CONFIG_CONTROLLER_H__


/**********************************************************************************************************************************************************************************/
/*                                                                                    includes                                                                                    */
/**********************************************************************************************************************************************************************************/
#include "FITS-ModuleApi.h"
#include "ColorantController.h"
#include "SocketController.h"

#include <boost/asio/io_context.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>

#include <condition_variable>
#include <cstdint>
#include <list>
#include <mutex>
#include <string>
#include <thread>
#include <vector>


/**********************************************************************************************************************************************************************************/
/*                                                            typedefs, structures, enumerations, and related defines                                                             */
/**********************************************************************************************************************************************************************************/
namespace FIERY
{
    namespace FITS
    {
        namespace CORE
        {
            namespace CONFIG
            {
                class CONTROLLER
                {
                    public:
                    // public constructors and destructor.
                    CONTROLLER(const FIERY::FITS::MODULE::PROPERTIES::FITS_SERVER & fits_server_properties, boost::shared_ptr<FIERY::FITS::MODULE::PLUG_IN> & p_module_plug_in);
                    virtual ~CONTROLLER();

                    // public "management" methods.
                    int Start(void);
                    void Stop(void);


                    private:
                    // private "management" methods.
                    bool connect(void);
                    void connectColorants(void);
                    bool getIpLocalBind(void);
                    bool getIpLocalList(std::vector<std::string> & ip_list);
                    void processServerConfiguration(const std::vector<uint8_t> & buffer_server_configuration);
                    void run(void);
                    void sendClientConnected(void);
                    void waitForExit(void);
                    void waitForStop(void);

                    // private "management" variables.
                    std::list<FIERY::FITS::CORE::COLORANT::CONTROLLER *>   m_colorant_controller_list;
                    bool                                                   m_connected;
                    bool                                                   m_enabled;
                    FIERY::FITS::MODULE::PROPERTIES::FITS_SERVER           m_fits_server_properties;
                    boost::asio::io_context                                m_io_context;
                    std::string                                            m_ip_local_bind;
                    std::string                                            m_name;
                    boost::shared_ptr<FIERY::FITS::MODULE::PLUG_IN>        m_p_module_plug_in;
                    FIERY::FITS::CORE::SOCKET::CONTROLLER *                m_socket_controller;
                    std::thread                                            m_thread;
                    std::mutex                                             m_thread_mutex;
                    std::condition_variable                                m_thread_signal;

                    // private static "management" constants.
                    static const uint32_t      CONNECT_ATTEMPTS_WATERMARK;
                    static const std::string   IP_DEFAULT;
                    static const std::string   NAME;
                    static const uint32_t      PORT_DEFAULT;
                    static const uint32_t      SLEEP_IN_SECONDS_CONNECT;
                    static const uint32_t      SLEEP_IN_SECONDS_IDLE;
                };
            }
        }
    }
}
#endif // __CONFIG_CONTROLLER_H__