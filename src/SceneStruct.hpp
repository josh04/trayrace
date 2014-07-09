#ifndef SCENESTRUCT_HPP
#define SCENESTRUCT_HPP

#include "point3d.hpp"

namespace tr {
	struct SceneStruct {
	public:
		SceneStruct() {

		}

		~SceneStruct() {

		}

		void defaults() {
			CameraLocation = point3d(-30.0, -6.0, 10.0);
			waistRotation = 15;
			headTilt = -15;
			horizontalFov = 90.0;
			width = 1280;
			height = 720;
		}

		point3d CameraLocation;
		int waistRotation;
		int headTilt;
		double horizontalFov;
		unsigned int width, height;
	};
}

#endif