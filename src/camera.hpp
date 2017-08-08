#ifndef CAMERA_HPP
#define CAMERA_HPP

#define _USE_MATH_DEFINES

#include <vector>
#include <iomanip>
#include <atomic>
#include <math.h>

#include <Mush Core/imageBuffer.hpp>

#include "point3d.hpp"
#include "cell.hpp"
#include "viewport8bit.hpp"
#include "matrix3d.hpp"
#include "profile.hpp"

using std::vector;

namespace tr {
	typedef vector<double> Depths;
	typedef vector<point3d> Normals;
    typedef vector<Light::rgb> Colours;
}

namespace tr {
	class Camera {
	public:
		Camera() {
                
		}

		~Camera() {

		}
        
		virtual void move(const point3d newLocation, const double newTheta, const double newPhi, const double changeFOV) = 0;
        
		virtual void preliminary_snap(shared_ptr<Viewport> depthMap,
			shared_ptr<Viewport> normalMap,
			shared_ptr<Viewport> colourMap,
			Shapes shapes,
			Lights lights) = 0;

		virtual void final_snap(shared_ptr<Viewport> viewport, Shapes shapes, Lights lights) = 0;
        
		virtual void activateAll() = 0;
        
		virtual void activateMap(uint8_t const * map) = 0;
        
	private:

	};
}

#endif