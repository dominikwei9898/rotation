/**********************************************************************************************************************************************************************************/
/*                                                                                                                                                                                */
/*  Copyright (c) 1990-2023 Fiery, LLC.  All Rights Reserved.                                                                                                                     */
/*                                                                                                                                                                                */
/*  Fiery, LLC ("Fiery") products contain certain trade secrets and confidential and proprietary information of Fiery.  Use, reproduction, disclosure, distribution by any means  */
/*  are prohibited, except pursuant to a written license from Fiery.  Modification, translation, reverse engineering, decompiling, disassembling, and creating derivative works   */
/*  based on this software are prohibited, except pursuant to a written license from Fiery. Use of copyright notice does not imply publication or disclosure.                     */
/*                                                                                                                                                                                */
/**********************************************************************************************************************************************************************************/
#ifndef __BUFFER_MANAGER_H__
#define __BUFFER_MANAGER_H__


/**********************************************************************************************************************************************************************************/
/*                                                                                    includes                                                                                    */
/**********************************************************************************************************************************************************************************/
#include <condition_variable>
#include <cstdint>
#include <list>
#include <mutex>
#include <string>
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
            namespace BUFFER
            {
                // class declarations.
                class BUFFER
                {
                    public:
                    // public "management" variables.
                    std::string   job_id;
                    std::string   image_id;

                    // public "buffer" variables.
                    uint8_t *   p_buffer;
                    uint64_t    buffer_size_in_bytes;

                    // constructors and destructor.
                    BUFFER
                    (
                        const std::string &     job_id,
                        const std::string &     image_id,
                        const uint8_t * const   p_buffer,
                        const uint64_t &        buffer_size_in_bytes
                    ) : job_id(job_id), image_id(image_id), p_buffer(const_cast<uint8_t *>(p_buffer)), buffer_size_in_bytes(buffer_size_in_bytes)
                    {
                        ;
                    };

                    BUFFER()
                    {
                        clear();
                    };

                    BUFFER
                    (
                        const BUFFER &   buffer
                    ) : job_id(buffer.job_id), image_id(buffer.image_id), p_buffer(buffer.p_buffer), buffer_size_in_bytes(buffer.buffer_size_in_bytes)
                    {
                        ;
                    };

                    virtual ~BUFFER()
                    {
                        ;
                    };


                    // "accessor" methods.
                    void   clear()
                    {
                        job_id.erase();
                        image_id.erase();
                        p_buffer             = nullptr;
                        buffer_size_in_bytes = 0;
                    };
                };

                class MANAGER
                {
                    public:
                    // public constructors and destructor.
                    MANAGER(const std::string & colorant, const uint64_t & buffer_pool_size_in_bytes);
                    virtual ~MANAGER();

                    // public "management" methods.
                    bool Initialize(void);

                    // public BUFFER "management" methods.
                    bool BufferCreateFromPool(const std::string & job_id, const std::string & image_id, const std::vector<uint8_t> & image_data, BUFFER & buffer);
                    bool BufferDeleteFromPool(const std::string & job_id, const std::string & image_id);

                    // public JOB "management" methods.
                    bool JobCancel(const std::string & job_id);


                    private:
                    // private "management" methods.
                    bool allocate(const std::string & job_id, const std::string & image_id, const std::vector<uint8_t> & image_data, BUFFER & buffer);
                    bool deallocate(BUFFER & buffer);
                    bool get(const std::string & job_id, const std::string & image_id, BUFFER & buffer);
                    bool poolAllocate(void);
                    bool poolCopy(const uint8_t * p_buffer_source, const uint64_t & number_of_bytes_to_copy, uint8_t * p_buffer_destination);
                    bool poolDeallocate(void);
                    void poolUpdate(void);

                    // private "management" variables.
                    std::string               m_colorant;
                    std::list<BUFFER>         m_buffer_list;
                    bool                      m_initialized;
                    std::mutex                m_mutex;
                    uint8_t *                 m_p_pool_end;
                    uint8_t *                 m_p_pool_start;
                    uint8_t *                 m_p_pool_window_end;
                    uint8_t *                 m_p_pool_window_start;
                    uint64_t                  m_pool_size_in_bytes;
                    uint64_t                  m_pool_size_in_bytes_available_left_of_window;
                    uint64_t                  m_pool_size_in_bytes_available_right_of_window;
                    uint64_t                  m_pool_size_in_bytes_used;
                    std::condition_variable   m_signal_deallocated;
                    bool                      m_throttled;

                    // private static "management" constants.
                    static const uint32_t   SLEEP_IN_MILLISECONDS_CANCEL;
                };
            }
        }
    }
}
#endif // __BUFFER_MANAGER_H__
