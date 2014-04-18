#ifndef TIFFOUTPUT_HPP
#define TIFFOUTPUT_HPP

#include <string>
#include <memory>

#include <tiffio.h>

#include "viewport8bit.hpp"

namespace tr {
	class tiffOutput {
	public:
		tiffOutput() {

		}


		~tiffOutput() {

		}

		void writeImage(const char * filename, std::shared_ptr<Viewport8bit> viewport, unsigned int width, unsigned int height) {

			TIFF* tiff = TIFFOpen("output.tif" , "w");

			uint32 samples = 4;

			TIFFSetField(tiff, TIFFTAG_IMAGEWIDTH, width);  // set the width of the image
			TIFFSetField(tiff, TIFFTAG_IMAGELENGTH, height);    // set the height of the image
			TIFFSetField(tiff, TIFFTAG_SAMPLESPERPIXEL, samples);   // set number of channels per pixel
			//	TIFFSetField(tiff, TIFFTAG_COMPRESSION, COMPRESSION_LZW);
			TIFFSetField(tiff, TIFFTAG_BITSPERSAMPLE, 8);    // set the size of the channels
			TIFFSetField(tiff, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);    // set the origin of the image.
			//   Some other essential fields to set that you do not have to understand for now.
			TIFFSetField(tiff, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
			TIFFSetField(tiff, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
			TIFFSetField(tiff, TIFFTAG_ROWSPERSTRIP, TIFFDefaultStripSize(tiff, width * samples));

			tsize_t linebytes = samples * width;

			std::unique_ptr<uint8_t []> buf(new uint8_t[linebytes]);

			//	uint8_t* buf = new uint8_t[linebytes];

			for (uint32 row = 0; row < height; ++row) {
				for (uint32 i = 0; i < width; ++i) {
					Viewport8bit::rgb colour = viewport->get(i, row);
					buf[i * samples] = colour.r;
					buf[i * samples + 1] = colour.g;
					buf[i * samples + 2] = colour.b;
					buf[i * samples + 3] = 255;
				}

				if (TIFFWriteScanline(tiff, buf.get(), row, 0) < 0)
					break;
			}

			TIFFClose(tiff);
		}

	private:

	};
}

#endif