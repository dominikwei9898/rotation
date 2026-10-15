/**********************************************************************************************************************************************************************************/
/*                                                                                                                                                                                */
/*  Copyright (c) 1990-2026 Fiery, LLC.  All Rights Reserved.                                                                                                                     */
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

#include <boost/property_tree/ptree.hpp>

#include <condition_variable>
#include <cstdint>
#include <list>
#include <map>
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
                        job_id.clear();
                        image_id.clear();
                        colorant.clear();
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
                    bool Abort(void);
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

                namespace PROPERTIES
                {
                    class MANAGER
                    {
                        public:
                        // public static IMAGE_PROPERTIES "management" methods.
                        static bool ImagePropertiesCreate(const std::string & job_id, const std::string & image_id, const boost::property_tree::ptree & json_object_image_properties);
                        static bool ImagePropertiesDelete(const std::string & job_id, const std::string & image_id);
                        static bool ImagePropertiesGet(const std::string & job_id, const std::string & image_id, boost::property_tree::ptree & json_object_image_properties);

                        // public JOB "management" methods.
                        static bool Abort(void);
                        static bool JobCancel(const std::string & job_id);


                        private:
                        // private constructors and destructor.
                        MANAGER();
                        virtual ~MANAGER();

                        // private typedefs.
                        typedef std::map<std::string, boost::property_tree::ptree>   IMAGE_PROPERTIES_MAP;
                        typedef std::map<std::string, IMAGE_PROPERTIES_MAP>          JOB_IMAGE_PROPERTIES_MAP_MAP;

                        // private static "management" variables.
                        static JOB_IMAGE_PROPERTIES_MAP_MAP   m_job_image_properties_map_map;
                        static std::mutex                     m_job_image_properties_map_map_mutex;
                    };
                }
            }
        }
    }
}
#endif // __IMAGE_MANAGER_H__
