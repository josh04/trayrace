// trayrace 

#include <memory>
#include <iostream>
#include <tiffio.h>

#include <ConfigStruct.hpp>
#include <exports.hpp>

#include "scene.hpp"
#include "viewport8bit.hpp"
#include "viewportFloat.hpp"
#include "tiffOutput.hpp"

using namespace tr;
using std::make_shared;

void doThread(unsigned int width, unsigned int height) {

	// initialise config struct
	hdr::config config;
	config.defaults();

	// width and height
	config.width = width;
	config.height = height;
	// external input
	config.inputEngine = hdr::externalInput;
	// gohdr method
	config.compressEngine = hdr::exrCompress;
	// vlc output
	config.outputEngine = hdr::noOutput;
	// Show the goHDR gui?
	config.show_gui = true;
	// Sim2 preview window
	config.sim2preview = true;
	// pixel format for external input
	config.input_pix_fmt = hdr::float_4channel;

	sprintf(config.filename, "%s", "testRender_1080_");
	int size = sizeof(float);

	// run function in thread
	hdrRunExternalInput(&config, config.width, config.height);
}

int main(int argc, char** argv[])
{
	// load scene data
	SceneStruct sconfig;
	sconfig.defaults();
	//	- list of objects
	Scene scene{};
	scene.init(&sconfig);

	// load viewport
	std::shared_ptr<ViewportFloat> viewport = make_shared<ViewportFloat>(sconfig.width, sconfig.height);

	// fire the rays from the viewport to the scene
	//	- inc. intersection code
//	scene.snap(viewport);

	std::thread * thread = new std::thread(std::bind(&doThread, sconfig.width, sconfig.height));
	int size = 4;
	point3d vec(1, 1, 1);
	point3d vec2 = vec * yRotation(20.0*(M_PI / 180.0));
	point3d vec3 = vec * yRotation(10.0*(M_PI / 180.0)) * yRotation(10.0*(M_PI / 180.0));

	int counter = 0;
	// lock image, copy memory into place
	while (counter < 360) {
		++counter;
		scene.snap(viewport);
		// lock
		unsigned char * in = (unsigned char *) lockInput();
		if (in == nullptr) {
			break;
		}
		// write
		memcpy(in, viewport->ptr(), size * 4 * sconfig.width * sconfig.height);
		// unlock
		unlockInput(); 
		sconfig.waistRotation -= 1;
		sconfig.CameraLocation = sconfig.CameraLocation * yRotation(-1.0*(M_PI/180.0));//+ point3d(0.5, 0, 0.5);
		scene.moveCamera(sconfig.CameraLocation, sconfig.waistRotation, sconfig.headTilt, sconfig.horizontalFov, sconfig.width, sconfig.height);
	}

	// done!



	/*
	tiffOutput TiffOutput{};
	TiffOutput.writeImage("output.tif", viewport, config.width, config.height);

	std::cin.ignore();*/
}
