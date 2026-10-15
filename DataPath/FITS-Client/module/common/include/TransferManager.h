/**********************************************************************************************************************************************************************************/
/*                                                                                                                                                                                */
/*  Copyright (c) 1990-2023 Fiery, LLC.  All Rights Reserved.                                                                                                                     */
/*                                                                                                                                                                                */
/*  Fiery, LLC ("Fiery") products contain certain trade secrets and confidential and proprietary information of Fiery.  Use, reproduction, disclosure, distribution by any means  */
/*  are prohibited, except pursuant to a written license from Fiery.  Modification, translation, reverse engineering, decompiling, disassembling, and creating derivative works   */
/*  based on this software are prohibited, except pursuant to a written license from Fiery. Use of copyright notice does not imply publication or disclosure.                     */
/*                                                                                                                                                                                */
/**********************************************************************************************************************************************************************************/
#ifndef __TRANSFER_MANAGER_H__
#define __TRANSFER_MANAGER_H__


/**********************************************************************************************************************************************************************************/
/*                                                                                    includes                                                                                    */
/**********************************************************************************************************************************************************************************/
#include "FITS-OutputApi.h"
#include "ImageManager.h"
#include "ModuleManager.h"

#include <boost/smart_ptr/shared_ptr.hpp>

#include <condition_variable>
#include <cstdint>
#include <list>
#include <mutex>
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
            namespace TRANSFER
            {
                // class declarations.
                class REQUEST
                {
                    public:
                    // public constructors and destructor.
                    REQUEST(const std::string & job_id, const std::string & job_name, const std::string & image_id, const std::list<std::string> & colorant_list);
                    virtual ~REQUEST();

                    // public "management" methods.
                    bool IsColorantRequired(const std::string & colorant);
                    bool Transferred(const std::string & colorant);
                    bool WaitForTransferred(void);

                    // public "accessor" methods.
                    const std::list<std::string> & ColorantListGet(void) const { return(m_colorant_requested_list); };
                    const std::string & ImageIdGet(void) const { return(m_image_id); };
                    const std::string & JobIdGet(void) const { return(m_job_id); };
                    const std::string & JobNameGet(void) const { return(m_job_name); };


                    private:
                    std::list<std::string>    m_colorant_requested_list;
                    uint64_t                  m_count;
                    uint64_t                  m_count_maximum;
                    std::condition_variable   m_count_signal;
                    std::string               m_image_id;
                    std::string               m_job_id;
                    std::string               m_job_name;
                    std::mutex                m_mutex;
                };

                class MANAGER
                {
                    public:
                    // public constructors and destructor.
                    MANAGER();
                    virtual ~MANAGER();

                    // public "management" methods.
                    bool Start(FIERY::FITS::MODULE::MANAGER * p_module_mgr, const boost::shared_ptr<FIERY::FITS::OUTPUT::PLUG_IN> & p_output_plug_in, const FIERY::FITS::OUTPUT::PROPERTIES::OUTPUT & output_properties);
                    bool Stop(void);

                    // public JOB "management" methods.
                    bool JobCancel(const std::string & job_id);

                    // public static REQUEST "management" methods.
                    bool RequestCreate(const std::string & job_id, const std::string & job_name, const std::string & image_id, const std::list<std::string> & colorant_list, REQUEST * & p_request);


                    private:
                    // private "management" methods.
                    bool requestGet(REQUEST * & p_transfer_request);
                    void run(void);
                    bool waitForEjected(REQUEST * p_transfer_request);
                    void waitForStop(void);

                    // private static "management" variables.
                    FIERY::FITS::OUTPUT::PROPERTIES::OUTPUT           m_output_properties;
                    FIERY::FITS::MODULE::MANAGER *                    m_p_module_mgr;
                    boost::shared_ptr<FIERY::FITS::OUTPUT::PLUG_IN>   m_p_output_plug_in;
                    std::list<REQUEST *>                              m_request_list;
                    std::mutex                                        m_request_mutex;
                    std::condition_variable                           m_request_signal;
                    std::thread                                       m_thread;
                    bool                                              m_thread_enabled;
                };
            }
        }
    }
}
#endif // __TRANSFER_MANAGER_H__
