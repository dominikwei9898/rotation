/**********************************************************************************************************************************************************************************/
/*                                                                                                                                                                                */
/*  Copyright (c) 1990-2026 Fiery, LLC.  All Rights Reserved.                                                                                                                     */
/*                                                                                                                                                                                */
/*  Fiery, LLC ("Fiery") products contain certain trade secrets and confidential and proprietary information of Fiery.  Use, reproduction, disclosure, distribution by any means  */
/*  are prohibited, except pursuant to a written license from Fiery.  Modification, translation, reverse engineering, decompiling, disassembling, and creating derivative works   */
/*  based on this software are prohibited, except pursuant to a written license from Fiery. Use of copyright notice does not imply publication or disclosure.                     */
/*                                                                                                                                                                                */
/**********************************************************************************************************************************************************************************/
#ifndef __COLORANT_MANAGER_H__
#define __COLORANT_MANAGER_H__


/**********************************************************************************************************************************************************************************/
/*                                                                                    includes                                                                                    */
/**********************************************************************************************************************************************************************************/
#include "BufferManager.h"
#include "ImageManager.h"
#include "ModuleManager.h"
#include "PropertiesManager.h"
#include "TransferManager.h"

#include "FITS-ModuleApi.h"
#include "FITS-OutputApi.h"

#include "boost/date_time/posix_time/posix_time.hpp"
#include <boost/smart_ptr/shared_ptr.hpp>

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
        namespace MODULE
        {
            namespace COLORANT
            {
                // class declarations.
                class COLORANT
                {
                    public:
                    // public constructors and destructor.
                    COLORANT(const std::string & colorant, const FIERY::FITS::MODULE::PROPERTIES::COLORANT & colorant_properties, const boost::shared_ptr<FIERY::FITS::OUTPUT::PLUG_IN> & p_output_plug_in, const FIERY::FITS::OUTPUT::PROPERTIES::OUTPUT & output_properties, const FIERY::FITS::MODULE::PROPERTIES::ROTATION & rotation_properties);
                    virtual ~COLORANT();

                    // public "management" methods.
                    bool Initialize(void);

                    // public IMAGE "management" methods.
                    bool ImageCreate(const FIERY::FITS::MODULE::PROPERTIES::IMAGE & image_properties, const std::vector<uint8_t> & image_data);
                    bool ImageDelete(const std::string & job_id, const std::string & image_id);
                    bool ImageGet(const std::string & job_id, const std::string & image_id, FITS::MODULE::IMAGE::IMAGE & image);
                    bool ImageTransfer(FIERY::FITS::MODULE::TRANSFER::REQUEST * p_transfer_request);

                    // public JOB "management" methods.
                    bool Abort(void);
                    bool JobCancel(const std::string & job_id);

                    // public JOB "helper" methods.
                    const std::string & NameGet(void) { return(m_colorant); };


                    private:
                    // private "management" methods.
                    bool get(FIERY::FITS::MODULE::TRANSFER::REQUEST * & p_transfer_request);
                    bool output(FIERY::FITS::MODULE::TRANSFER::REQUEST * p_transfer_request);
                    void run(void);
                    void waitForStop(void);

                    // private "management" variables.
                    FITS::MODULE::BUFFER::MANAGER                         m_buffer_mgr;
                    std::string                                           m_colorant;
                    FITS::MODULE::IMAGE::MANAGER                          m_image_mgr;
                    bool                                                  m_initialized;
                    FIERY::FITS::OUTPUT::PROPERTIES::OUTPUT               m_output_properties;
                    boost::shared_ptr<FIERY::FITS::OUTPUT::PLUG_IN>       m_p_output_plug_in;
                    FIERY::FITS::MODULE::PROPERTIES::ROTATION             m_rotation_properties;
                    std::list<FIERY::FITS::MODULE::TRANSFER::REQUEST *>   m_transfer_request_list;
                    std::mutex                                            m_transfer_request_mutex;
                    std::condition_variable                               m_transfer_request_signal;
                    std::thread                                           m_thread;
                    bool                                                  m_thread_enabled;
                };

                class MANAGER
                {
                    public:
                    // public static "management" methods.
                    static bool Start(FIERY::FITS::MODULE::MANAGER * p_module_mgr, const FIERY::FITS::MODULE::PROPERTIES::COLORANT & colorant_properties, const std::string & fits_output_plugin_name, const FIERY::FITS::MODULE::PROPERTIES::ROTATION & rotation_properties);
                    static bool Stop(void);

                    // public static IMAGE "management" methods.
                    static bool ImageCreate(const FIERY::FITS::MODULE::PROPERTIES::IMAGE & image_properties, const std::vector<uint8_t> & image_data);
                    static bool ImageTransfer(const std::string & job_id, const std::string & job_name, const std::string & image_id, const std::list<std::string> & colorant_list_valid, const boost::property_tree::ptree & json_object_settings);

                    // public static JOB "management" methods.
                    static bool Abort(void);
                    static bool IsJobCanceled(const std::string & job_id);
                    static bool JobCancel(const std::string & job_id);


                    private:
    		        // private class declarations.
                    class JOB_CANCELED
                    {
                        public:
                        // public "management" variables.
                        std::string                id;
                        boost::posix_time::ptime   timestamp;

                        // public constructors and destructor.
                        JOB_CANCELED
                        (
                            const std::string &                id,
                            const boost::posix_time::ptime &   timestamp
                        ) : id(id), timestamp(timestamp)
                        {
                            ;
                        };

                        JOB_CANCELED() : id(), timestamp()
                        {
                            ;
                        };

                        JOB_CANCELED
                        (
                            const JOB_CANCELED &   job_canceled
                        ) : id(job_canceled.id), timestamp(job_canceled.timestamp)
                        {
                            ;
                        };

                        virtual ~JOB_CANCELED()
                        {
                            ;
                        };
                    };

                    // private constructors and destructor.
                    MANAGER();
                    virtual ~MANAGER();

                    // private static "helper" methods.
                    static bool colorantGet(const std::string & colorant, COLORANT * & p_colorant);
                    static bool colorantListGet(const std::list<std::string> & colorant_list_valid, std::list<COLORANT *> & colorant_list_required);
                    static void jobCancelSet(const std::string & job_id);

                    // private static "management" variables.
                    static std::map<std::string, COLORANT *>                 m_colorant_map;
                    static std::mutex                                        m_colorant_mutex;
                    static std::list<JOB_CANCELED>                           m_job_canceled_list;
                    static std::mutex                                        m_job_canceled_mutex;
                    static FIERY::FITS::OUTPUT::PROPERTIES::OUTPUT           m_output_properties;
                    static FIERY::FITS::MODULE::MANAGER *                    m_p_module_mgr;
                    static boost::shared_ptr<FIERY::FITS::OUTPUT::PLUG_IN>   m_p_output_plug_in;
                    static FIERY::FITS::MODULE::PROPERTIES::ROTATION         m_rotation_properties;
                    static FIERY::FITS::MODULE::TRANSFER::MANAGER            m_transfer_mgr;

                    // private static "management" constants.
                    static const uint32_t   TIMEOUT_IN_MILLISECONDS_CANCEL;
                };
            }
        }
    }
}
#endif // __COLORANT_MANAGER_H__
