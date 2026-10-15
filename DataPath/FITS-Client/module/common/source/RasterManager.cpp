/**********************************************************************************************************************************************************************************/
/*                                                                                                                                                                                */
/*  Copyright (c) 1990-2026 Fiery, LLC.  All Rights Reserved.                                                                                                                     */
/*                                                                                                                                                                                */
/*  Fiery, LLC ("Fiery") products contain certain trade secrets and confidential and proprietary information of Fiery.  Use, reproduction, disclosure, distribution by any means  */
/*  are prohibited, except pursuant to a written license from Fiery.  Modification, translation, reverse engineering, decompiling, disassembling, and creating derivative works   */
/*  based on this software are prohibited, except pursuant to a written license from Fiery. Use of copyright notice does not imply publication or disclosure.                     */
/*                                                                                                                                                                                */
/**********************************************************************************************************************************************************************************/
#include "RasterManager.h"

#include "LogManager.h"

using namespace FIERY::FITS::MODULE::RASTER;


/**********************************************************************************************************************************************************************************/
/*                                                                                local variables                                                                                 */
/**********************************************************************************************************************************************************************************/
const uint32_t   MANAGER::DEGREES_090 = 90;
const uint32_t   MANAGER::DEGREES_180 = 180;
const uint32_t   MANAGER::DEGREES_270 = 270;


/**********************************************************************************************************************************************************************************/
/*                                                                                     methods                                                                                    */
/**********************************************************************************************************************************************************************************/
MANAGER::MANAGER()
{
    ;
}

MANAGER::~MANAGER()
{
    ;
}

bool MANAGER::Rotate(const FIERY::FITS::MODULE::PROPERTIES::IMAGE & image_properties_input, const std::vector<uint8_t> & image_data_input, const uint32_t & degrees,
            FIERY::FITS::MODULE::PROPERTIES::IMAGE & image_properties_output, std::vector<uint8_t> & image_data_output)
{
    uint64_t   number_of_pixels_x_input;
    uint64_t   number_of_pixels_x_output;
    uint64_t   number_of_pixels_y_input;
    uint64_t   number_of_pixels_y_output;
    uint64_t   bytes_per_scanline_output;

    bool       successful = false;


    // initialize the return variables.
    image_properties_output = image_properties_input;
    image_data_output.clear();

    // check to see if the requested "rotation" is a "supported" value -- 0/90/180/270 degrees CLOCKWISE only.
    if ((degrees != 0) && (degrees != DEGREES_090) && (degrees != DEGREES_180) && (degrees != DEGREES_270))
    {
        FIERY::FITS::MODULE::LOG::MANAGER::LogError("FITS-Module[Rotate]: INVALID Degrees (Degrees = " + std::to_string(degrees) + ") -- MUST be 0, 90, 180, or 270");
        return(successful);
    }

    // check to see if the IMAGE "bits-per-pixel" is a "supported" value.
    if (!bitsPerPixelSupported(image_properties_input.bits_per_pixel))
    {
        FIERY::FITS::MODULE::LOG::MANAGER::LogError("FITS-Module[Rotate]: UNSUPPORTED BitsPerPixel (BitsPerPixel = " +
                    std::to_string(image_properties_input.bits_per_pixel) + ")");
        return(successful);
    }

    // check to see if a "rotation" is even required -- if not, simply "pass-through" the ORIGINAL IMAGE "properties" and "data" unchanged.
    if (degrees == 0)
    {
        image_data_output = image_data_input;

        // update the status and return.
        successful = true;
        return(successful);
    }

    // initialize the local variables.
    number_of_pixels_x_input  = image_properties_input.number_of_pixels_x;
    number_of_pixels_y_input  = image_properties_input.number_of_pixels_y;

    // a 90 or 270 degree rotation "swaps" the "width" and "height" of the IMAGE -- a 180 degree rotation does not.
    number_of_pixels_x_output = (degrees == DEGREES_180) ? number_of_pixels_x_input  : number_of_pixels_y_input;
    number_of_pixels_y_output = (degrees == DEGREES_180) ? number_of_pixels_y_input  : number_of_pixels_x_input;
    bytes_per_scanline_output = bytesPerScanlineGet(number_of_pixels_x_output, image_properties_input.bits_per_pixel);

    // update the return variables -- the ROTATED IMAGE "properties" MUST be used by the caller for all further "processing" of the IMAGE "data".
    image_properties_output.number_of_pixels_x = number_of_pixels_x_output;
    image_properties_output.number_of_pixels_y = number_of_pixels_y_output;
    image_properties_output.bytes_per_scanline = bytes_per_scanline_output;

    // a 90 or 270 degree rotation also "swaps" which physical axis the "x" and "y" RESOLUTION (dots-per-inch) values describe -- this matters whenever the RESOLUTION is
    // "anisotropic" (resolution_x != resolution_y).  a 180 degree rotation does not change which axis is which, so RESOLUTION is left unchanged in that case.
    if (degrees != DEGREES_180)
    {
        image_properties_output.resolution_x = image_properties_input.resolution_y;
        image_properties_output.resolution_y = image_properties_input.resolution_x;
    }

    // attempt to "allocate" the OUTPUT "buffer" -- zero-initialized so that any "padding" bits/bytes beyond the "active" pixels of a scanline remain "0".
    image_data_output.assign(bytes_per_scanline_output * number_of_pixels_y_output, 0);

    // loop through every "row" of the INPUT IMAGE.
    for (uint64_t y = 0; y < number_of_pixels_y_input; y++)
    {
        const uint8_t *   p_scanline_input  = image_data_input.data() + (y * image_properties_input.bytes_per_scanline);

        // loop through every "pixel" of the current "row".
        for (uint64_t x = 0; x < number_of_pixels_x_input; x++)
        {
            uint64_t   x_output;
            uint64_t   y_output;
            uint32_t   value = pixelGet(p_scanline_input, x, image_properties_input.bits_per_pixel);


            // determine where this PIXEL "lands" in the OUTPUT IMAGE, based on the requested (CLOCKWISE) "rotation".
            if (degrees == DEGREES_090)
            {
                x_output = number_of_pixels_y_input - 1 - y;
                y_output = x;
            }
            else if (degrees == DEGREES_180)
            {
                x_output = number_of_pixels_x_input - 1 - x;
                y_output = number_of_pixels_y_input - 1 - y;
            }
            else // DEGREES_270
            {
                x_output = y;
                y_output = number_of_pixels_x_input - 1 - x;
            }

            // attempt to "set" the PIXEL in the OUTPUT "buffer".
            pixelSet(image_data_output.data() + (y_output * bytes_per_scanline_output), x_output, image_properties_input.bits_per_pixel, value);
        }
    }

    // update the status and return.
    successful = true;
    return(successful);
}

bool MANAGER::bitsPerPixelSupported(const uint32_t & bits_per_pixel)
{
    // supports "sub-byte" (bit-packed, MSB-first) depths commonly used for 1-bit halftone RASTER data, as well as "byte-aligned" depths commonly used for contone/grayscale
    // RASTER data.  any other depth is considered "unsupported" by this reference implementation.
    return((bits_per_pixel == 1) || (bits_per_pixel == 2) || (bits_per_pixel == 4) || (bits_per_pixel == 8) || (bits_per_pixel == 16) || (bits_per_pixel == 24) ||
                (bits_per_pixel == 32));
}

uint64_t MANAGER::bytesPerScanlineGet(const uint64_t & number_of_pixels_x, const uint32_t & bits_per_pixel)
{
    // round "up" to the nearest whole BYTE.
    return(((number_of_pixels_x * bits_per_pixel) + 7) / 8);
}

uint32_t MANAGER::pixelGet(const uint8_t * const p_scanline, const uint64_t & x, const uint32_t & bits_per_pixel)
{
    uint32_t   value = 0;


    // check to see if this is a "sub-byte" (bit-packed) PIXEL depth.
    if (bits_per_pixel < 8)
    {
        uint64_t   bit_offset = x * bits_per_pixel;
        uint64_t   byte_index = bit_offset / 8;
        uint32_t   bit_shift  = 8 - bits_per_pixel - static_cast<uint32_t>(bit_offset % 8);
        uint8_t    mask       = static_cast<uint8_t>((1 << bits_per_pixel) - 1);


        // extract the PIXEL -- packed MSB-first within the BYTE.
        value = (p_scanline[byte_index] >> bit_shift) & mask;
    }
    else
    {
        uint32_t   bytes_per_pixel = bits_per_pixel / 8;
        uint64_t   byte_index      = x * bytes_per_pixel;


        // copy the PIXEL's "bytes" (in their original order) into the low-order BYTEs of the return VALUE -- this is purely an "opaque" BYTE-for-BYTE copy (not a numeric
        // interpretation), so it is safe regardless of the actual multi-BYTE "endianness" of the underlying RASTER data.
        for (uint32_t byte_offset = 0; byte_offset < bytes_per_pixel; byte_offset++)
        {
            value = (value << 8) | p_scanline[byte_index + byte_offset];
        }
    }

    // return the VALUE.
    return(value);
}

void MANAGER::pixelSet(uint8_t * const p_scanline, const uint64_t & x, const uint32_t & bits_per_pixel, const uint32_t & value)
{
    // check to see if this is a "sub-byte" (bit-packed) PIXEL depth.
    if (bits_per_pixel < 8)
    {
        uint64_t   bit_offset = x * bits_per_pixel;
        uint64_t   byte_index = bit_offset / 8;
        uint32_t   bit_shift  = 8 - bits_per_pixel - static_cast<uint32_t>(bit_offset % 8);
        uint8_t    mask       = static_cast<uint8_t>((1 << bits_per_pixel) - 1);


        // "clear" the target BITs and "OR" in the new VALUE -- packed MSB-first within the BYTE.
        p_scanline[byte_index] = static_cast<uint8_t>((p_scanline[byte_index] & ~(mask << bit_shift)) | ((value & mask) << bit_shift));
    }
    else
    {
        uint32_t   bytes_per_pixel = bits_per_pixel / 8;
        uint64_t   byte_index      = x * bytes_per_pixel;


        // "write" the PIXEL's "bytes" back out in the same order they were "read" by pixelGet() -- see NOTE in pixelGet() regarding "endianness".
        for (uint32_t byte_offset = 0; byte_offset < bytes_per_pixel; byte_offset++)
        {
            p_scanline[byte_index + byte_offset] = static_cast<uint8_t>(value >> (8 * (bytes_per_pixel - 1 - byte_offset)));
        }
    }
}
