/**********************************************************************************************************************************************************************************/
/*                                                                                                                                                                                */
/*  Copyright (c) 1990-2023 Fiery, LLC.  All Rights Reserved.                                                                                                                     */
/*                                                                                                                                                                                */
/*  Fiery, LLC ("Fiery") products contain certain trade secrets and confidential and proprietary information of Fiery.  Use, reproduction, disclosure, distribution by any means  */
/*  are prohibited, except pursuant to a written license from Fiery.  Modification, translation, reverse engineering, decompiling, disassembling, and creating derivative works   */
/*  based on this software are prohibited, except pursuant to a written license from Fiery. Use of copyright notice does not imply publication or disclosure.                     */
/*                                                                                                                                                                                */
/**********************************************************************************************************************************************************************************/
#ifndef __PROPERTIES_MANAGER_H__
#define __PROPERTIES_MANAGER_H__


/**********************************************************************************************************************************************************************************/
/*                                                                                    includes                                                                                    */
/**********************************************************************************************************************************************************************************/
#include <cstdint>
#include <list>
#include <string>


/**********************************************************************************************************************************************************************************/
/*                                                                              compile-time defines                                                                              */
/**********************************************************************************************************************************************************************************/


/**********************************************************************************************************************************************************************************/
/*                                                            typedefs, structures, enumerations, and related defines                                                             */
/**********************************************************************************************************************************************************************************/
namespace FIERY
{
    namespace FITS
    {
        namespace MODULE
        {
            namespace PROPERTIES
            {
                // class declarations.
                class COLORANT
                {
                    public:
                    // public "management" variables.
                    uint64_t                 buffer_pool_size_in_bytes;
                    std::list<std::string>   colorant_list;


                    // public constructors and destructor.
                    COLORANT
                    (
                        const uint64_t &                 buffer_pool_size_in_bytes,
                        const std::list<std::string> &   colorant_list
                    ) : buffer_pool_size_in_bytes(buffer_pool_size_in_bytes), colorant_list(colorant_list)
                    {
                        ;
                    };

                    COLORANT()
                    {
                        clear();
                    };

                    COLORANT
                    (
                        const COLORANT &   colorant
                    ) : buffer_pool_size_in_bytes(colorant.buffer_pool_size_in_bytes), colorant_list(colorant.colorant_list)
                    {
                        ;
                    };

                    virtual ~COLORANT()
                    {
                        ;
                    };


                    // public "helper" methods.
                    void clear(void)
                    {
                        colorant_list.clear();
                        buffer_pool_size_in_bytes = 0;
                    };
                };

                class FIERY_PATH_SERVER
                {
                    public:
                    // public "management" variables.
                    std::string   ip;
                    uint32_t      port;


                    // public constructors and destructor.
                    FIERY_PATH_SERVER
                    (
                        const std::string &   ip,
                        const uint32_t &      port
                    ) : ip(ip), port(port)
                    {
                        ;
                    };

                    FIERY_PATH_SERVER()
                    {
                        clear();
                    };

                    FIERY_PATH_SERVER
                    (
                        const FIERY_PATH_SERVER &   fiery_path
                    ) : ip(fiery_path.ip), port(fiery_path.port)
                    {
                        ;
                    };

                    virtual ~FIERY_PATH_SERVER()
                    {
                        ;
                    };


                    // public "helper" methods.
                    void clear(void)
                    {
                        ip.clear();
                        port = 0;
                    };
                };
            }
        }
    }
}
#endif   // __PROPERTIES_MANAGER_H__
