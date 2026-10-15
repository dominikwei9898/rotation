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
#ifndef __CONFIG_CONTROLLER_H__
#define __CONFIG_CONTROLLER_H__


/**********************************************************************************************************************************************************************************/
/*                                                                                    includes                                                                                    */
/**********************************************************************************************************************************************************************************/
#include "FITS-ModuleApi.h"
#include "ColorantController.h"
#include "SocketController.h"

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
                    std::string                                            m_name;
                    boost::shared_ptr<FIERY::FITS::MODULE::PLUG_IN>        m_p_module_plug_in;
                    FIERY::FITS::CORE::SOCKET::CONTROLLER *                m_socket_controller;
                    std::thread                                            m_thread;
                    std::mutex                                             m_thread_mutex;
                    std::condition_variable                                m_thread_signal;

                    // private static "management" constants.
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