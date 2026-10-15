/**********************************************************************************************************************************************************************************/
/*                                                                                                                                                                                */
/*  Copyright (c) 1990-2023 Fiery, LLC.  All Rights Reserved.                                                                                                                     */
/*                                                                                                                                                                                */
/*  Fiery, LLC ("Fiery") products contain certain trade secrets and confidential and proprietary information of Fiery.  Use, reproduction, disclosure, distribution by any means  */
/*  are prohibited, except pursuant to a written license from Fiery.  Modification, translation, reverse engineering, decompiling, disassembling, and creating derivative works   */
/*  based on this software are prohibited, except pursuant to a written license from Fiery. Use of copyright notice does not imply publication or disclosure.                     */
/*                                                                                                                                                                                */
/**********************************************************************************************************************************************************************************/
#ifndef __IMAGE_MANAGER_H__
#define __IMAGE_MANAGER_H__


/**********************************************************************************************************************************************************************************/
/*                                                                                    includes                                                                                    */
/**********************************************************************************************************************************************************************************/
#include "BufferManager.h"

#include "FITS-ModuleApi.h"

#include <condition_variable>
#include <cstdint>
#include <list>
#include <mutex>
#include <string>


/**********************************************************************************************************************************************************************************/
/*                                                            typedefs, structures, enumerations, and related defines                                                             */
/**********************************************************************************************************************************************************************************/
namespace FIERY
{
    namespace FITS
    {
        namespace MODULE
        {
            namespace IMAGE
            {
                // class declarations.
                class IMAGE : public FIERY::FITS::MODULE::PROPERTIES::IMAGE
                {
                    public:
                    // public "buffer" variables.
                    uint8_t *   p_buffer;
                    uint64_t    buffer_size_in_bytes;

                    // public constructors and destructor.
                    IMAGE
                    (
                        const FIERY::FITS::MODULE::PROPERTIES::IMAGE &   image_properties,
                        const uint8_t * const                            p_buffer,
                        const uint64_t &                                 buffer_size_in_bytes
                    ) : FIERY::FITS::MODULE::PROPERTIES::IMAGE(image_properties), p_buffer(const_cast<uint8_t *>(p_buffer)), buffer_size_in_bytes(buffer_size_in_bytes)
                    {
                        ;
                    };

                    IMAGE
                    (
                        const std::string &     job_id,
                        const std::string &     image_id,
                        const uint8_t * const   p_buffer,
                        const uint64_t &        buffer_size_in_bytes,
                        const uint32_t &        bits_per_pixel,
                        const uint32_t &        bytes_per_scanline,
                        const std::string &     colorant,
                        const uint64_t &        number_of_pixels_x,
                        const uint64_t &        number_of_pixels_y,
                        const uint64_t &        resolution_x,
                        const uint64_t &        resolution_y
                    ) : FIERY::FITS::MODULE::PROPERTIES::IMAGE(job_id, image_id, bits_per_pixel, bytes_per_scanline, colorant, number_of_pixels_x, number_of_pixels_y,
                                resolution_x, resolution_y), p_buffer(const_cast<uint8_t *>(p_buffer)), buffer_size_in_bytes(buffer_size_in_bytes)
                    {
                        ;
                    };

                    IMAGE()
                    {
                        clear();
                    };

                    IMAGE
                    (
                        const IMAGE &   image
                    ) : FIERY::FITS::MODULE::PROPERTIES::IMAGE(image.job_id, image.image_id, image.bits_per_pixel, image.bytes_per_scanline, image.colorant,
                                image.number_of_pixels_x, image.number_of_pixels_y, image.resolution_x, image.resolution_y), p_buffer(const_cast<uint8_t *>(image.p_buffer)),
                                buffer_size_in_bytes(image.buffer_size_in_bytes)
                    {
                        ;
                    };

                    virtual ~IMAGE()
                    {
                        ;
                    };


                    // public "accessor" methods.
                    void   clear()
                    {
                        job_id.erase();
                        image_id.erase();
                        colorant.erase();
                        p_buffer             = nullptr;
                        buffer_size_in_bytes = 0;
                        bits_per_pixel       = 0;
                        bytes_per_scanline   = 0;
                        number_of_pixels_x   = 0;
                        number_of_pixels_y   = 0;
                        resolution_x         = 0;
                        resolution_y         = 0;
                    };
                };

                class MANAGER
                {
                    public:
                    // public constructors and destructor.
                    MANAGER(const std::string & colorant);
                    virtual ~MANAGER();

                    // public IMAGE "management" methods.
                    bool ImageCreate(const FIERY::FITS::MODULE::PROPERTIES::IMAGE & image_properties, const FITS::MODULE::BUFFER::BUFFER & buffer);
                    bool ImageDelete(const std::string & job_id, const std::string & image_id);
                    bool ImageGet(const std::string & job_id, const std::string & image_id, IMAGE & image);

                    // public JOB "management" methods.
                    bool JobCancel(const std::string & job_id);


                    private:
                    // private "management" methods.
                    bool get(const std::string & job_id, const std::string & image_id, IMAGE & image);

                    // private "management" variables.
                    std::string               m_colorant;
                    std::list<IMAGE>          m_image_list;
                    std::mutex                m_image_mutex;
                    std::condition_variable   m_image_signal;
                };
            }
        }
    }
}
#endif // __IMAGE_MANAGER_H__
