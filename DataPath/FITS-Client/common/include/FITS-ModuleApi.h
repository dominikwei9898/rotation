/**********************************************************************************************************************************************************************************/
/*                                                                                                                                                                                */
/*  Copyright (c) 1990-2023 Fiery, LLC.  All Rights Reserved.                                                                                                                     */
/*                                                                                                                                                                                */
/*  Fiery, LLC ("Fiery") products contain certain trade secrets and confidential and proprietary information of Fiery.  Use, reproduction, disclosure, distribution by any means  */
/*  are prohibited, except pursuant to a written license from Fiery.  Modification, translation, reverse engineering, decompiling, disassembling, and creating derivative works   */
/*  based on this software are prohibited, except pursuant to a written license from Fiery. Use of copyright notice does not imply publication or disclosure.                     */
/*                                                                                                                                                                                */
/**********************************************************************************************************************************************************************************/
#ifndef __FITS_MODULE_API_H__
#define __FITS_MODULE_API_H__


/**********************************************************************************************************************************************************************************/
/*                                                                                    includes                                                                                    */
/**********************************************************************************************************************************************************************************/
#include <cstdint>
#include <list>
#include <string>
#include <vector>


/**********************************************************************************************************************************************************************************/
/*                                                                              compile-time defines                                                                              */
/**********************************************************************************************************************************************************************************/
#ifdef __cplusplus
#define EXTERN_C extern "C" 
#else
#define EXTERN_C extern
#endif //__cplusplus

#define FITS_MODULE_PLUG_IN_NAME      "FITS-Module"
#define FITS_MODULE_PLUG_IN_SYMBOLS   "FITS_Module"


/**********************************************************************************************************************************************************************************/
/*                                                            typedefs, structures, enumerations, and related defines                                                             */
/**********************************************************************************************************************************************************************************/
namespace FIERY
{
    namespace FITS
    {
        namespace MODULE
        {
            namespace CONSTANT
            {
                // constant declarations.
                const std::string   FILENAME_CONFIGURATION_JSON = "FITS-Module.json";
            }

            namespace PROPERTIES
            {
                // class declarations.
                class FITS_SERVER
                {
                    public:
                    // public "management" variables.
                    std::string   ip;
                    uint32_t      port;


                    // public constructors and destructor.
                    FITS_SERVER
                    (
                        const std::string &   ip,
                        const uint32_t &      port
                    ) : ip(ip), port(port)
                    {
                        ;
                    };

                    FITS_SERVER()
                    {
                        clear();
                    };

                    FITS_SERVER
                    (
                        const FITS_SERVER &   fits_server
                    ) : ip(fits_server.ip), port(fits_server.port)
                    {
                        ;
                    };

                    virtual ~FITS_SERVER()
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

                class IMAGE
                {
                    public:
                    // public "management" variables.
                    std::string   job_id;
                    std::string   image_id;

                    // public "property" variables.
                    uint32_t      bits_per_pixel;
                    uint32_t      bytes_per_scanline;
                    std::string   colorant;
                    uint64_t      number_of_pixels_x;
                    uint64_t      number_of_pixels_y;
                    uint64_t      resolution_x;
                    uint64_t      resolution_y;

                    // public constructors and destructor.
                    IMAGE
                    (
                        const std::string &   job_id,
                        const std::string &   image_id,
                        const uint32_t &      bits_per_pixel,
                        const uint32_t &      bytes_per_scanline,
                        const std::string &   colorant,
                        const uint64_t &      number_of_pixels_x,
                        const uint64_t &      number_of_pixels_y,
                        const uint64_t &      resolution_x,
                        const uint64_t &      resolution_y
                    ) : job_id(job_id), image_id(image_id), bits_per_pixel(bits_per_pixel), bytes_per_scanline(bytes_per_scanline), colorant(colorant),
                                number_of_pixels_x(number_of_pixels_x), number_of_pixels_y(number_of_pixels_y), resolution_x(resolution_x), resolution_y(resolution_y)
                    {
                        ;
                    };

                    IMAGE()
                    {
                        clear();
                    };

                    IMAGE
                    (
                        const IMAGE &   image_properties
                    ) : job_id(image_properties.job_id), image_id(image_properties.image_id), bits_per_pixel(image_properties.bits_per_pixel),
                                bytes_per_scanline(image_properties.bytes_per_scanline), colorant(image_properties.colorant), number_of_pixels_x(image_properties.number_of_pixels_x),
                                number_of_pixels_y(image_properties.number_of_pixels_y), resolution_x(image_properties.resolution_x), resolution_y(image_properties.resolution_y)
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
                        bits_per_pixel     = 0;
                        bytes_per_scanline = 0;
                        number_of_pixels_x = 0;
                        number_of_pixels_y = 0;
                        resolution_x       = 0;
                        resolution_y       = 0;
                    };
                };

                class LOG
                {
                    public:
                    // public "management" variables.
                    bool       enable_console;
                    uint32_t   files_max;
                    uint32_t   size_max_in_megabytes;


                    // public constructors and destructor.
                    LOG
                    (
                        const bool &       enable_console,
                        const uint32_t &   files_max,
                        const uint32_t &   size_max_in_megabytes
                    ) : enable_console(enable_console), files_max(files_max), size_max_in_megabytes(size_max_in_megabytes)
                    {
                        ;
                    };

                    LOG()
                    {
                        clear();
                    };

                    LOG
                    (
                        const LOG &   log_properties
                    ) : enable_console(log_properties.enable_console), files_max(log_properties.files_max), size_max_in_megabytes(log_properties.size_max_in_megabytes)
                    {
                        ;
                    };

                    LOG & operator=(const LOG & rhs)
                    {
                        if (this != &(rhs))
                        {
                            enable_console        = rhs.enable_console;
                            files_max             = rhs.files_max;
                            size_max_in_megabytes = rhs.size_max_in_megabytes;
                        }

                        return(*this);
                    }

                    virtual ~LOG()
                    {
                        ;
                    };


                    // public "helper" methods.
                    void clear(void)
                    {
                        enable_console        = false;
                        files_max             = 0;
                        size_max_in_megabytes = 0;
                    };
                };
            }

            class CONFIGURATION
            {
                public:
                // public "management" variables.
                FIERY::FITS::MODULE::PROPERTIES::FITS_SERVER   fits_properties;
                FIERY::FITS::MODULE::PROPERTIES::LOG           log_properties;


                // public constructors and destructor.
                CONFIGURATION
                (
                    const FIERY::FITS::MODULE::PROPERTIES::FITS_SERVER &   fits_properties,
                    const FIERY::FITS::MODULE::PROPERTIES::LOG &           log_properties
                ) : fits_properties(fits_properties), log_properties(log_properties)
                {
                    ;
                };

                CONFIGURATION()
                {
                    clear();
                };

                CONFIGURATION
                (
                    const CONFIGURATION &   configuration
                ) : fits_properties(configuration.fits_properties), log_properties(configuration.log_properties)
                {
                    ;
                };

                virtual ~CONFIGURATION()
                {
                    ;
                };


                // public "helper" methods.
                void clear(void)
                {
                    fits_properties.clear();
                    log_properties.clear();
                };
            };

            class LOGGER
            {
                public:
                // typedefs.
                typedef void   (*CALLBACK_ERROR)(const std::string & message, const bool & console_log);
                typedef void   (*CALLBACK_FATAL)(const std::string & message, const bool & console_log);
                typedef void   (*CALLBACK_NORMAL)(const std::string & message, const bool & console_log);
                typedef void   (*CALLBACK_WARNING)(const std::string & message, const bool & console_log);

                // public "management" variables.
                CALLBACK_ERROR     callback_error;
                CALLBACK_FATAL     callback_fatal;
                CALLBACK_NORMAL    callback_normal;
                CALLBACK_WARNING   callback_warning;


                // public constructors and destructor.
                LOGGER
                (
                    const CALLBACK_NORMAL &    callback_normal,
                    const CALLBACK_WARNING &   callback_warning,
                    const CALLBACK_ERROR &     callback_error,
                    const CALLBACK_FATAL &     callback_fatal
                ) : callback_normal(callback_normal), callback_warning(callback_warning), callback_error(callback_error), callback_fatal(callback_fatal)
                {
                    ;
                };

                LOGGER()
                {
                    clear();
                };

                LOGGER
                (
                    const LOGGER &   logger
                ) : callback_normal(logger.callback_normal), callback_warning(logger.callback_warning), callback_error(logger.callback_error), callback_fatal(logger.callback_fatal)
                {
                    ;
                };

                virtual ~LOGGER()
                {
                    ;
                };


                // public "helper" methods.
                void clear(void)
                {
                    callback_error   = nullptr;
                    callback_fatal   = nullptr;
                    callback_normal  = nullptr;
                    callback_warning = nullptr;
                };
            };

            class PLUG_IN
            {
                public:
                // public pure virtual "management" methods.
                virtual bool Initialize(const FIERY::FITS::MODULE::LOGGER & logger) = 0;

                // public pure virtual CONNECTION "management" methods.
                virtual bool Start(const std::list<std::string> & colorant_list) = 0;
                virtual bool Stop(void) = 0;

                // public pure virtual IMAGE "management" methods.
                virtual bool ImageSave(const FIERY::FITS::MODULE::PROPERTIES::IMAGE & image_properties, const std::vector<uint8_t> & image_data) = 0;
            };
        }
    }
}
#endif   // __FITS_MODULE_API_H__