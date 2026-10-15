/**********************************************************************************************************************************************************************************/
/*                                                                                                                                                                                */
/*  Copyright (c) 1990-2023 Fiery, LLC.  All Rights Reserved.                                                                                                                     */
/*                                                                                                                                                                                */
/*  Fiery, LLC ("Fiery") products contain certain trade secrets and confidential and proprietary information of Fiery.  Use, reproduction, disclosure, distribution by any means  */
/*  are prohibited, except pursuant to a written license from Fiery.  Modification, translation, reverse engineering, decompiling, disassembling, and creating derivative works   */
/*  based on this software are prohibited, except pursuant to a written license from Fiery. Use of copyright notice does not imply publication or disclosure.                     */
/*                                                                                                                                                                                */
/**********************************************************************************************************************************************************************************/
#ifndef __MODULE_MANAGER_H__
#define __MODULE_MANAGER_H__


/**********************************************************************************************************************************************************************************/
/*                                                                                    includes                                                                                    */
/**********************************************************************************************************************************************************************************/
#include "FITS-ModuleApi.h"

#include "PropertiesManager.h"
#include "WebSocketManager.h"

#include <cstdint>
#include <list>
#include <string>
#include <vector>


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
            // class declarations.
            class MANAGER
            {
                public:
                // public constructors and destructor.
                MANAGER();
                virtual ~MANAGER();

                // public "management" methods.
                bool Initialize(const FIERY::FITS::MODULE::LOGGER & logger);

                // public CONNECTION "management" methods.
                bool Start(const std::list<std::string> & colorant_list);
                bool Stop(void);

                // public IMAGE "management" methods.
                bool ImagePrinted(const std::string & job_id, const std::string & image_id);
                bool ImageSave(const FIERY::FITS::MODULE::PROPERTIES::IMAGE & image_properties, const std::vector<uint8_t> & image_data);
                bool ImageTransfer(const std::string & job_id, const std::string & job_name, const std::string & image_id, const std::list<std::string> & colorant_list_valid);

                // public JOB "management" methods.
                bool JobCancel(const std::string & job_id);

                // public "accessor" methods.
                const std::list<std::string> & ColorantListGet(void) { return(m_colorant_properties.colorant_list); };
                const FIERY::FITS::MODULE::LOGGER & LoggerGet(void) { return(m_logger); };


                private:
                // private "helper" methods.
                bool configurationGet(void);

                // private "management" variables.
                FIERY::FITS::MODULE::PROPERTIES::COLORANT            m_colorant_properties;
                FIERY::FITS::MODULE::PROPERTIES::FIERY_PATH_SERVER   m_fiery_path_server_properties;
                std::string                                          m_fits_output_plugin_name;
                FIERY::FITS::MODULE::LOGGER                          m_logger;
                FIERY::FITS::MODULE::WEB_SOCKET::MANAGER             m_web_socket_mgr;
            };
        }
    }
}
#endif   // __MODULE_MANAGER_H__
