#ifndef SCENE_HPP
#define SCENE_HPP

#include <vector>
#include <memory>

#include "shape.hpp"
#include "light.hpp"

using std::shared_ptr;
using std::vector;

namespace tr {
	typedef shared_ptr<vector<shared_ptr<const Shape>>> Shapes;
	typedef shared_ptr<vector<shared_ptr<const Light>>> Lights;
}

#include "SceneStruct.hpp"
#include "sphere.hpp"
#include "camera.hpp"
#include "viewport8bit.hpp"
#include "boundBox.hpp"
#include "boundOblong.hpp"
#include "plane3d.hpp"
#include "circle3d.hpp"
#include "square3d.hpp"

#include "paint.hpp"
#include "checkerboard.hpp"

using std::vector;
using std::shared_ptr;
using std::make_shared;

namespace tr {
	class Scene {
	public:
		Scene() {

		}

		void init(SceneStruct * config) {
			shapes = std::make_shared<vector<shared_ptr<const Shape>>>();
			camera = std::make_shared<Camera>(config->CameraLocation, config->waistRotation, config->headTilt, config->horizontalFov, config->width, config->height);
			lights = std::make_shared<vector<shared_ptr<const Light>>>();

			shared_ptr<Shape> redBall = std::static_pointer_cast<Shape>(make_shared<Sphere>(point3d(1, -3.5, 1), 2, std::make_unique<Paint>(Light::rgb(0.5, 0.0, 0.0))));
			redBall->reflective = 0.6;
			shapes->push_back(redBall);
			shared_ptr<Shape> yellowBall = std::static_pointer_cast<Shape>(make_shared<Sphere>(point3d(1, -3.5, -11), 2, std::make_unique<Paint>(Light::rgb(0.5, 0.5, 0.0))));
			yellowBall->reflective = 0.6;
			shapes->push_back(yellowBall);
			shared_ptr<Shape> greenBall = std::static_pointer_cast<Shape>(make_shared<Sphere>(point3d(1, -6.5, -6), 2, std::make_unique<Paint>(Light::rgb(0.0, 0.5, 0.0))));
			greenBall->reflective = 0.6;
			shapes->push_back(greenBall);

			shared_ptr<BoundOblong> grayBox = make_shared<BoundOblong>(point3d(0, 0, -10), 20, 60, std::make_unique<Paint>(Light::rgb(0.5)));
			grayBox->reflective = 0.001;
			grayBox->setDownTexture(std::make_unique<CheckerBoard>(Light::rgb(0.5), Light::rgb(2.0), 6));
			grayBox->setLeftTexture(std::make_unique<CheckerBoard>(Light::rgb(0.5, 0.0, 0.0), Light::rgb(2.0), 6));
			grayBox->setRightTexture(std::make_unique<CheckerBoard>(Light::rgb(0.0, 0.5, 0.0), Light::rgb(2.0), 6));
			shapes->push_back(grayBox);

			shared_ptr<Shape> mirrorBall = std::static_pointer_cast<Shape>(make_shared<Sphere>(point3d(1, 15, -6), 5, std::make_unique<Paint>(Light::rgb(0.75, 0.75, 0.75))));
			mirrorBall->reflective = 1.0;
			shapes->push_back(mirrorBall);

			shared_ptr<Shape> plane = std::static_pointer_cast<Shape>(make_shared<Square3d>(5, 5, 0, -60, std::make_unique<CheckerBoard>(Light::rgb(0, 0.75, 0.75), Light::rgb(2.0, 2.0, 2.0), 1)));
			plane->reflective = 1.0;
			shapes->push_back(plane);

			shared_ptr<Shape> lampBallOne = std::static_pointer_cast<Shape>(make_shared<Sphere>(point3d(6.0, -6.0, 9.0), 0.6, std::make_unique<Paint>(Light::rgb(0.9))));
			lampBallOne->reflective = 0.0;
			shared_ptr<Shape> lampBallTwo = std::static_pointer_cast<Shape>(make_shared<Sphere>(point3d(-9.0, 0.0, 0.0), 0.6, std::make_unique<Paint>(Light::rgb(0.9))));
			lampBallTwo->reflective = 0.0;

			shapes->push_back(lampBallOne);
			shapes->push_back(lampBallTwo);


			shared_ptr<Light> light = make_shared<Light>(point3d(6.0, -6.0, 9.0), Light::rgb(0.4), lampBallOne);
			shared_ptr<Light> light2 = make_shared<Light>(point3d(-9.0, 0.0, 0.0), Light::rgb(0.4), lampBallTwo);
			lights->push_back(light);
			lights->push_back(light2);
		}

		void moveCamera(point3d CameraLocation, int waistRotation, int headTilt, double horizontalFov, unsigned int width, unsigned int height) {
			camera.reset();
			camera = std::make_shared<Camera>(CameraLocation, waistRotation, headTilt, horizontalFov, width, height);
		}

		~Scene() {}

		void snap(shared_ptr<Viewport> viewport) {
			camera->snap(viewport, shapes, lights);
		}

	private:
		Shapes shapes = nullptr;
		Lights lights = nullptr;
		shared_ptr<Camera> camera = nullptr;
	};

}

#endif