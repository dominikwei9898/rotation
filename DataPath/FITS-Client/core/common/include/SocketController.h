/**********************************************************************************************************************************************************************************/
/*                                                                                                                                                                                */
/*  Copyright (c) 1990-2026 Fiery, LLC.  All Rights Reserved.                                                                                                                     */
/*                                                                                                                                                                                */
/*  Fiery, LLC ("Fiery") products contain certain trade secrets and confidential and proprietary information of Fiery.  Use, reproduction, disclosure, distribution by any means  */
/*  are prohibited, except pursuant to a written license from Fiery.  Modification, translation, reverse engineering, decompiling, disassembling, and creating derivative works   */
/*  based on this software are prohibited, except pursuant to a written license from Fiery. Use of copyright notice does not imply publication or disclosure.                     */
/*                                                                                                                                                                                */
/**********************************************************************************************************************************************************************************/
#ifndef __SOCKET_CONTROLLER_H__
#define __SOCKET_CONTROLLER_H__


/**********************************************************************************************************************************************************************************/
/*                                                                                    includes                                                                                    */
/**********************************************************************************************************************************************************************************/
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <cstdint>
#include <string>
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
            namespace SOCKET
            {
                namespace CONTROLLER_ERROR
                {
                    typedef enum controller_error
                    {
                        SOCKET_CLOSED    = -3,
                        CONNECTION_ERROR = -2,
                        READ_ERROR       = -1,
                        NONE,
                    } ENUM;
                }

                class CONTROLLER
                {
                    public:
                    // public constructors and destructor.
                    CONTROLLER(const std::string & name);
                    virtual ~CONTROLLER();

                    // public "management" methods.
                    bool Connect(const std::string & ip, const uint32_t & port, const std::string & ip_local_bind, std::string & error);
                    void Run(void);
                    void Stop(void);

                    // public "read/write" methods.
                    uint64_t Read(uint8_t * p_buffer, const uint64_t & buffer_size_in_bytes);
                    void ReadAsync(std::vector<uint8_t> & rec_buf, size_t size_to_receive, std::function<void(int)> callback);
                    void Write(const std::vector<uint8_t> & buffer);

                    // public "accessor" methods.
                    boost::asio::ip::tcp::socket & GetSocket() { return(m_socket); };


                    private:
                    // private "management" methods.
                    void close();

                    // private "management" variables.
                    bool                           m_enabled;
                    boost::asio::io_context        m_io_context;
                    std::string                    m_name;
                    boost::asio::ip::tcp::socket   m_socket;
                };
            }
        }
    }
}
#endif // __SOCKET_CONTROLLER_H__
