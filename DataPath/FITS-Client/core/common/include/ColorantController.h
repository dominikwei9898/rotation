/**********************************************************************************************************************************************************************************/
/*                                                                                                                                                                                */
/*  Copyright (c) 1990-2026 Fiery, LLC.  All Rights Reserved.                                                                                                                     */
/*                                                                                                                                                                                */
/*  Fiery, LLC ("Fiery") products contain certain trade secrets and confidential and proprietary information of Fiery.  Use, reproduction, disclosure, distribution by any means  */
/*  are prohibited, except pursuant to a written license from Fiery.  Modification, translation, reverse engineering, decompiling, disassembling, and creating derivative works   */
/*  based on this software are prohibited, except pursuant to a written license from Fiery. Use of copyright notice does not imply publication or disclosure.                     */
/*                                                                                                                                                                                */
/**********************************************************************************************************************************************************************************/
#ifndef __COLORANT_CONTROLLER_H__
#define __COLORANT_CONTROLLER_H__


/**********************************************************************************************************************************************************************************/
/*                                                                                    includes                                                                                    */
/**********************************************************************************************************************************************************************************/
#include "FITS-ModuleApi.h"
#include "SocketController.h"

#include <boost/smart_ptr/shared_ptr.hpp>

#include <cstdint>
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
            namespace COLORANT
            {
                class CONTROLLER
                {
                    public:
                    // public constructors and destructor.
                    CONTROLLER(const std::string & ip, const uint16_t & port, const std::string & ip_local_bind, const std::string & colorant, boost::shared_ptr<FIERY::FITS::MODULE::PLUG_IN> & p_module_plug_in);
                    virtual ~CONTROLLER();

                    // public "management" methods.
                    void Start(void);
                    void Stop(void);
                    void WaitForExit(void);


                    private:
                    // private "management" methods.
                    bool connect(void);
                    void processHeader(const std::vector<uint8_t> & buffer_header);
                    void processHeaderLength(void);
                    void processServerData(const std::vector<uint8_t> & buffer_image_data);
                    void run(void);

                    // private "management" variables.
                    std::vector<uint8_t>                              m_buffer_fits_header_length;
                    std::string                                       m_colorant;
                    bool                                              m_connected;
                    bool                                              m_enabled;
                    std::string                                       m_ip;
                    std::string                                       m_ip_local_bind;
                    boost::shared_ptr<FIERY::FITS::MODULE::PLUG_IN>   m_p_module_plug_in;
                    uint32_t                                          m_port;
                    FIERY::FITS::CORE::SOCKET::CONTROLLER *           m_socket_controller;
                    std::thread                                       m_thread;

                    // private static "management" constants.
                    static const uint32_t   SLEEP_IN_SECONDS_CONNECT;
                };
            }
        }
    }
}
#endif // __COLORANT_CONTROLLER_H__