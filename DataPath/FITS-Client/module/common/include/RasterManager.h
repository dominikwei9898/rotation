/**********************************************************************************************************************************************************************************/
/*                                                                                                                                                                                */
/*  Copyright (c) 1990-2026 Fiery, LLC.  All Rights Reserved.                                                                                                                     */
/*                                                                                                                                                                                */
/*  Fiery, LLC ("Fiery") products contain certain trade secrets and confidential and proprietary information of Fiery.  Use, reproduction, disclosure, distribution by any means  */
/*  are prohibited, except pursuant to a written license from Fiery.  Modification, translation, reverse engineering, decompiling, disassembling, and creating derivative works   */
/*  based on this software are prohibited, except pursuant to a written license from Fiery. Use of copyright notice does not imply publication or disclosure.                     */
/*                                                                                                                                                                                */
/**********************************************************************************************************************************************************************************/
#ifndef __RASTER_MANAGER_H__
#define __RASTER_MANAGER_H__


/**********************************************************************************************************************************************************************************/
/*                                                                                    includes                                                                                    */
/**********************************************************************************************************************************************************************************/
#include "FITS-ModuleApi.h"

#include <cstdint>
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
            namespace RASTER
            {
                // class declarations.
                //
                // NOTE: this "utility" rotates a SINGLE COLORANT "plane" (i.e. the buffer for one ink) by 0/90/180/270 degrees (CLOCKWISE).  It is applied identically, and
                // independently, to every COLORANT of a given IMAGE so all "planes" remain in registration.  A 90 or 270 degree rotation swaps the "width" and "height" of the
                // IMAGE, as well as the "x" and "y" RESOLUTION (this matters whenever RESOLUTION is "anisotropic", i.e. resolution_x != resolution_y) -- callers MUST use the
                // returned (rotated) IMAGE "properties" (not the original) for all further "processing", "buffering", and "transfer" of the IMAGE "data", since the "buffer"
                // layout (bytes_per_scanline, number_of_pixels_x, number_of_pixels_y) will no longer match the original IMAGE "properties" otherwise.
                class MANAGER
                {
                    public:
                    // public static "management" methods.
                    static bool Rotate
                    (
                        const FIERY::FITS::MODULE::PROPERTIES::IMAGE &   image_properties_input,
                        const std::vector<uint8_t> &                     image_data_input,
                        const uint32_t &                                 degrees,
                        FIERY::FITS::MODULE::PROPERTIES::IMAGE &         image_properties_output,
                        std::vector<uint8_t> &                           image_data_output
                    );


                    private:
                    // private constructors and destructor.
                    MANAGER();
                    virtual ~MANAGER();

                    // private static "helper" methods.
                    static uint64_t   bytesPerScanlineGet(const uint64_t & number_of_pixels_x, const uint32_t & bits_per_pixel);
                    static bool       bitsPerPixelSupported(const uint32_t & bits_per_pixel);
                    static uint32_t   pixelGet(const uint8_t * const p_scanline, const uint64_t & x, const uint32_t & bits_per_pixel);
                    static void       pixelSet(uint8_t * const p_scanline, const uint64_t & x, const uint32_t & bits_per_pixel, const uint32_t & value);

                    // private static "management" constants.
                    static const uint32_t   DEGREES_090;
                    static const uint32_t   DEGREES_180;
                    static const uint32_t   DEGREES_270;
                };
            }
        }
    }
}
#endif // __RASTER_MANAGER_H__
