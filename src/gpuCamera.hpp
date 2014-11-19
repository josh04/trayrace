#ifndef GPUCAMERA_HPP
#define GPUCAMERA_HPP

#define _USE_MATH_DEFINES

#include <vector>
#include <iomanip>
#include <atomic>
#include <math.h>

#include <Video Mush/imageBuffer.hpp>

#include <Video Mush/opencl.hpp>

#include "point3d.hpp"
#include "cell.hpp"
#include "viewport8bit.hpp"
#include "matrix3d.hpp"
#include "profile.hpp"

#include "viewportFloat.hpp"

#include "sphere.hpp"
#include "boundOblong.hpp"

#include "algebra.h"

using std::vector;

namespace tr {
	typedef vector<double> Depths;
	typedef vector<point3d> Normals;
	typedef vector<Light::rgb> Colours;
}

namespace tr {
	class gpuCamera : public Camera {
	public:
		gpuCamera(std::shared_ptr<mush::opencl> context, point3d location, double theta, double phi, double horizontalFOV, unsigned int width, unsigned int height)
			: Camera(), location({ { static_cast<cl_float>(location.x), static_cast<cl_float>(location.y), static_cast<cl_float>(location.z) } }), width(width), height(height), horizontalFOV(horizontalFOV) {
			this->context = context;
			ray_directions = (tr_point3d *)context->hostReadBuffer(height * width* sizeof(tr_point3d));

			tr_point3d ray_direction;
			ray_direction.s[0] = 0.0;
			ray_direction.s[1] = 0.0;
			ray_direction.s[2] = 0.0;

			for (uint32 j = 0; j < height; ++j) {
				for (uint32 i = 0; i < width; ++i) {
					ray_directions[j*width+i] = ray_direction;
				}
			}

			move(location, theta, phi, horizontalFOV);

			output_image = context->floatImage(width, height);

			tr_ray_directions = context->buffer(sizeof(tr_point3d)*width*height);

			fire_primary = context->getKernel("fire_primary");
			queue = context->getQueue();
		}

		~gpuCamera() {

		}

		virtual void move(const point3d newLocation, const double newTheta, const double newPhi, const double changeFOV) {
			horizontalFOV = changeFOV;
			//motion = motion3d(location, yRotate.inverse()*zRotate.inverse());

			this->location.s[0] = newLocation.x;
			this->location.s[1] = newLocation.y;
			this->location.s[2] = newLocation.z;

			zRotate = zRotation(newPhi*(M_PI / 180.0));
			yRotate = yRotation(newTheta*(M_PI / 180.0));

			//motion = motion3d(move, (zRotation(addPhi*(M_PI/180.0))*yRotation(addTheta*(M_PI/180.0))).inverse());

			const unit3d gaze(point3d(1, 0, 0) * zRotate * yRotate);
			const unit3d up(point3d(0, 1, 0) * zRotate * yRotate);

			n = tan(horizontalFOV / 2.0*(M_PI / 180.0));

			const unit3d w = -gaze;
			const unit3d u = up.cross(w);
			const unit3d v = w.cross(u);

			const point3d gazen = gaze*n;

			const point3d vratio = v * static_cast<double>(height) / static_cast<double>(width);

			tr_point3d ray_direction;

			for (uint32 j = 0; j < height; ++j) {
				for (uint32 i = 0; i < width; ++i) {
					unit3d dir = gazen + vratio*(-1.0 + 2.0*((0.5 + j) / (height))) + u*(-1.0 + 2.0*((0.5 + i) / (width)));

					ray_direction.s[0] = dir.x;
					ray_direction.s[1] = dir.y;
					ray_direction.s[2] = dir.z;

					ray_directions[j*width + i] = ray_direction;
				}
			}
		}

		virtual void preliminary_snap(shared_ptr<Viewport> depthMap,
			shared_ptr<Viewport> normalMap,
			shared_ptr<Viewport> colourMap,
			Shapes shapes,
			Lights lights) {

			if (tr_objects != nullptr) {
				delete tr_objects;
			}

			int64_t objects_size = 0;
			std::vector<tr_sphere> tr_spheres;
			std::vector<tr_bound> tr_bounds;

			for (int64_t i = 0; i < shapes->size(); ++i) {
				auto sphere = std::dynamic_pointer_cast<Sphere>((*shapes)[i]);
				if (sphere != nullptr) {
					tr_sphere s;
					s.location.s[0] = sphere->getLocation().x;
					s.location.s[1] = sphere->getLocation().y;
					s.location.s[2] = sphere->getLocation().z;

					s.radius = sphere->getRadius();

					s.type = tr_object_sphere;
					tr_spheres.push_back(s);
				}

				auto bound = std::dynamic_pointer_cast<BoundOblong>((*shapes)[i]);
				if (bound != nullptr) {
					tr_bound b;
					double endDimension = bound->getEndDimension();
					double sideDimension = bound->getSideDimension();

					b.up.type = tr_object_plane;
					b.down.type = tr_object_plane;
					b.front.type = tr_object_plane;
					b.back.type = tr_object_plane;
					b.left.type = tr_object_plane;
					b.right.type = tr_object_plane;

					b.up.distance = endDimension;
					b.down.distance = endDimension;
					b.front.distance = sideDimension;
					b.back.distance = sideDimension;
					b.left.distance = sideDimension;
					b.right.distance = sideDimension;

					moveMatrix(b.up, bound->up);
					moveMatrix(b.down, bound->down);
					moveMatrix(b.front, bound->front);
					moveMatrix(b.back, bound->back);
					moveMatrix(b.left, bound->left);
					moveMatrix(b.right, bound->right);

					b.type = tr_object_bound;
					tr_bounds.push_back(b);
				}
			}

			objects_size = tr_spheres.size() + tr_bounds.size();

			objects = (tr_object *)context->hostReadBuffer(sizeof(tr_object)*objects_size);

			for (int j = 0; j < tr_spheres.size(); ++j) {
				objects[j].sphere = tr_spheres[j];
			}

			for (int j = 0; j < tr_bounds.size(); ++j) {
				objects[tr_spheres.size() + j].bound = tr_bounds[j];
			}


			tr_objects = context->buffer(sizeof(tr_object)*objects_size);

			fire_primary->setArg(0, *output_image);
			fire_primary->setArg(1, objects_size);
			fire_primary->setArg(2, *tr_objects);
			fire_primary->setArg(3, location);
			fire_primary->setArg(4, *tr_ray_directions);

			cl::Event event;

			queue->enqueueWriteBuffer(*tr_ray_directions, CL_TRUE, 0, sizeof(tr_point3d)*width*height, ray_directions, NULL, &event);
			event.wait();
			queue->enqueueWriteBuffer(*tr_objects, CL_TRUE, 0, sizeof(tr_object)*objects_size, objects, NULL, &event);
			event.wait();

			queue->enqueueNDRangeKernel(*fire_primary, cl::NullRange, cl::NDRange(width, height), cl::NullRange, NULL, &event);
			event.wait();

			cl::size_t<3> origin, region;
			origin[0] = 0; origin[1] = 0;  origin[2] = 0;
			region[0] = width; region[1] = height;  region[2] = 1;

			auto dpt = std::dynamic_pointer_cast<ViewportFloat>(normalMap);

			queue->enqueueReadImage(*output_image, CL_TRUE, origin, region, 0, 0, dpt->ptr(), NULL, &event);
			event.wait();
			memset(dpt->ptr(), 200, 10000);
			++frameCount;
			std::cout << "Done frame (" << frameCount << ")                                              \r";
		}

		virtual void final_snap(shared_ptr<Viewport> viewport, Shapes shapes, Lights lights) {

		}

		virtual void activateAll() {
		
		}

		virtual void activateMap(uint8_t const * map) {
		
		}

	private:

		void moveMatrix(tr_plane& p, plane3d& r) {
			matrix3d q = r.getCoordMatrix();
			p.coord_transform.x.s[0] = q.x.x;
			p.coord_transform.x.s[1] = q.x.y;
			p.coord_transform.x.s[2] = q.x.z;

			p.coord_transform.y.s[0] = q.y.x;
			p.coord_transform.y.s[1] = q.y.y;
			p.coord_transform.y.s[2] = q.y.z;

			p.coord_transform.z.s[0] = q.z.x;
			p.coord_transform.z.s[1] = q.z.y;
			p.coord_transform.z.s[2] = q.z.z;
		}

		uint8_t const * map = nullptr;

		double n = 0;
		double horizontalFOV = 0;
		tr_point3d location;
		const unsigned int width = 0, height = 0;

		int64_t frameCount = 0;

		matrix3d yRotate;
		matrix3d zRotate;

		motion3d motion;

		tr_point3d * ray_directions = nullptr;
		tr_object * objects = nullptr;

		cl::Image2D * output_image = nullptr;
		cl::Buffer * tr_ray_directions = nullptr;
		cl::Buffer * tr_objects = nullptr;

		cl::Kernel * fire_primary = nullptr;
		cl::CommandQueue * queue = nullptr;
		std::shared_ptr<mush::opencl> context = nullptr;
	};
}

#endif