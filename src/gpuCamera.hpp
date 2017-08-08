#ifndef GPUCAMERA_HPP
#define GPUCAMERA_HPP

#define _USE_MATH_DEFINES

#include <vector>
#include <iomanip>
#include <atomic>
#include <math.h>

#include <Mush Core/imageBuffer.hpp>

#include <Mush Core/opencl.hpp>

#include "point3d.hpp"
#include "cell.hpp"
#include "viewport8bit.hpp"
#include "matrix3d.hpp"
#include "profile.hpp"

#include "viewportFloat.hpp"

#include "sphere.hpp"
#include "boundOblong.hpp"
#include "triangle3d.hpp"

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
			/*ray_directions = (tr_point3d *)context->hostReadBuffer(height * width* sizeof(tr_point3d));

			tr_point3d ray_direction;
			ray_direction.s[0] = 0.0;
			ray_direction.s[1] = 0.0;
			ray_direction.s[2] = 0.0;

			for (uint32 j = 0; j < height; ++j) {
				for (uint32 i = 0; i < width; ++i) {
					ray_directions[j*width+i] = ray_direction;
				}
			}*/


            output_image = context->floatImage(width, height);
            geom_image = context->floatImage(width, height);
            depth_image = context->floatImage(width, height);

			tr_ray_directions = context->buffer(sizeof(tr_point3d)*width*height);

			fire_primary = context->getKernel("tr_fire_primary");
            gen_rays = context->getKernel("tr_gen_rays");
            gen_triangles = context->getKernel("tr_gen_triangles");
                
            queue = context->getQueue();
                
            move(location, theta, phi, horizontalFOV);
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

            gen_rays->setArg(0, width);
            gen_rays->setArg(1, height);
            gen_rays->setArg(2, movePoint(gazen));
            gen_rays->setArg(3, movePoint(vratio));
            gen_rays->setArg(4, movePoint(u));
            gen_rays->setArg(5, *tr_ray_directions);
            cl::Event event;
            
            queue->enqueueNDRangeKernel(*gen_rays, cl::NullRange, cl::NDRange(width, height), cl::NullRange, NULL, &event);
            event.wait();
        }

		virtual void preliminary_snap(shared_ptr<Viewport> depthMap,
			shared_ptr<Viewport> normalMap,
			shared_ptr<Viewport> colourMap,
			Shapes shapes,
			Lights lights) {
            
            tr_packed_sphere * spheres = nullptr;
            tr_plane * planes = nullptr;
            
            tr_point3d * triangles_a = nullptr;
            tr_point3d * triangles_b = nullptr;
            tr_point3d * triangles_c = nullptr;
            
            cl::Buffer * tr_spheres = nullptr;
            cl::Buffer * tr_planes = nullptr;
            
            cl::Buffer * tr_triangles_a = nullptr;
            cl::Buffer * tr_triangles_b = nullptr;
            cl::Buffer * tr_triangles_c = nullptr;
            
            cl::Buffer * tr_triangles = nullptr;
            
            
            int64_t spheres_size = 0, planes_size = 0, triangles_size = 0;

            for (int64_t i = 0; i < shapes->size(); ++i) {
                auto sphere = std::dynamic_pointer_cast<Sphere>((*shapes)[i]);
                auto bound = std::dynamic_pointer_cast<BoundOblong>((*shapes)[i]);
                auto triangle = std::dynamic_pointer_cast<Triangle3d>((*shapes)[i]);
                auto plane = std::dynamic_pointer_cast<plane3d>((*shapes)[i]);
                if (sphere != nullptr) {
                    ++spheres_size;
                }
                if (bound != nullptr) {
                    planes_size += 6;
                }
                if (triangle != nullptr) {
                    ++triangles_size;
                }
                if (plane != nullptr) {
//                    ++planes_size;
                }
            }
            
            spheres = (tr_packed_sphere *)context->hostReadBuffer(sizeof(tr_packed_sphere) * spheres_size);
            planes = (tr_plane *)context->hostReadBuffer(sizeof(tr_plane) * planes_size);
            triangles_a = (tr_point3d *)context->hostReadBuffer(sizeof(point3d) * triangles_size);
            triangles_b = (tr_point3d *)context->hostReadBuffer(sizeof(point3d) * triangles_size);
            triangles_c = (tr_point3d *)context->hostReadBuffer(sizeof(point3d) * triangles_size);
            
            int64_t spheres_cnt = 0, planes_cnt = 0, triangles_cnt = 0;
            
			for (int64_t i = 0; i < shapes->size(); ++i) {
				auto sphere = std::dynamic_pointer_cast<Sphere>((*shapes)[i]);
				if (sphere != nullptr) {
					tr_packed_sphere s;
					s.s[0] = sphere->getLocation().x;
					s.s[1] = sphere->getLocation().y;
					s.s[2] = sphere->getLocation().z;

					s.s[3] = sphere->getRadius();

                    spheres[spheres_cnt] = s;
                    ++spheres_cnt;
                    continue;
				}

				auto bound = std::dynamic_pointer_cast<BoundOblong>((*shapes)[i]);
				if (bound != nullptr) {
                    tr_plane up, down, front, back, left, right;
                    
					double endDimension = bound->getEndDimension();
					double sideDimension = bound->getSideDimension();

					up.distance = endDimension;
					down.distance = endDimension;
					front.distance = sideDimension;
					back.distance = sideDimension;
					left.distance = sideDimension;
					right.distance = sideDimension;

					moveMatrix(up, bound->up);
					moveMatrix(down, bound->down);
					moveMatrix(front, bound->front);
					moveMatrix(back, bound->back);
					moveMatrix(left, bound->left);
					moveMatrix(right, bound->right);
                    
                    planes[planes_cnt] = up;
                    planes[planes_cnt+1] = down;
                    planes[planes_cnt+2] = front;
                    planes[planes_cnt+3] = back;
                    planes[planes_cnt+4] = left;
                    planes[planes_cnt+5] = right;
                    
                    planes_cnt += 6;
                    continue;
				}
                
                auto triangle = std::dynamic_pointer_cast<Triangle3d>((*shapes)[i]);
                if (triangle != nullptr) {
                    tr_point3d a, b, c;
                    
                    a.s[0] = triangle->a.x;
                    a.s[1] = triangle->a.y;
                    a.s[2] = triangle->a.z;
                    
                    b.s[0] = triangle->b.x;
                    b.s[1] = triangle->b.y;
                    b.s[2] = triangle->b.z;
                
                    c.s[0] = triangle->c.x;
                    c.s[1] = triangle->c.y;
                    c.s[2] = triangle->c.z;
                    
                    triangles_a[triangles_cnt] = a;
                    triangles_b[triangles_cnt] = b;
                    triangles_c[triangles_cnt] = c;
                    ++triangles_cnt;
                    continue;
                }
                
                auto plane = std::dynamic_pointer_cast<plane3d>((*shapes)[i]);
                if (plane != nullptr) {
                    /*tr_plane p;
                    
                    p.distance = plane->getDistance();
                    
                    moveMatrix(p, *plane);
                    
                    planes[planes_cnt] = p;
                    
                    ++planes_cnt;*/
                    continue;
                }
            }
            
            cl::Event event;

            tr_spheres = context->buffer(sizeof(tr_packed_sphere)*spheres_size);
            tr_planes = context->buffer(sizeof(tr_plane)*planes_size);
            tr_triangles_a = context->buffer(sizeof(tr_point3d)*triangles_size);
            tr_triangles_b = context->buffer(sizeof(tr_point3d)*triangles_size);
            tr_triangles_c = context->buffer(sizeof(tr_point3d)*triangles_size);
            tr_triangles = context->buffer(sizeof(tr_triangle)*triangles_size);

            gen_triangles->setArg(0, *tr_triangles_a);
            gen_triangles->setArg(1, *tr_triangles_b);
            gen_triangles->setArg(2, *tr_triangles_c);
            gen_triangles->setArg(3, *tr_triangles);
            
            queue->enqueueWriteBuffer(*tr_triangles_a, CL_TRUE, 0, sizeof(tr_point3d)*triangles_size, triangles_a, NULL, &event);
            event.wait();
            queue->enqueueWriteBuffer(*tr_triangles_b, CL_TRUE, 0, sizeof(tr_point3d)*triangles_size, triangles_b, NULL, &event);
            event.wait();
            queue->enqueueWriteBuffer(*tr_triangles_c, CL_TRUE, 0, sizeof(tr_point3d)*triangles_size, triangles_c, NULL, &event);
            event.wait();
            
            queue->enqueueNDRangeKernel(*gen_triangles, cl::NullRange, cl::NDRange(triangles_size, 1), cl::NullRange, NULL, &event);
            event.wait();
            
            context->deleteCL(triangles_a);
            context->deleteCL(triangles_b);
            context->deleteCL(triangles_c);
            context->deleteCL(tr_triangles_a);
            context->deleteCL(tr_triangles_b);
            context->deleteCL(tr_triangles_c);
            
            fire_primary->setArg(0, *output_image);
            fire_primary->setArg(1, *depth_image);
            fire_primary->setArg(2, *geom_image);
            fire_primary->setArg(3, spheres_size);
            fire_primary->setArg(4, *tr_spheres);
            fire_primary->setArg(5, planes_size);
            fire_primary->setArg(6, *tr_planes);
            fire_primary->setArg(7, triangles_size);
            fire_primary->setArg(8, *tr_triangles);
			fire_primary->setArg(9, location);
            fire_primary->setArg(10, *tr_ray_directions);
            //fire_primary->setArg(9, frameCount);
			
            int o = sizeof(tr_packed_sphere);
			queue->enqueueWriteBuffer(*tr_spheres, CL_TRUE, 0, sizeof(tr_packed_sphere)*spheres_size, spheres, NULL, &event);
            event.wait();
            queue->enqueueWriteBuffer(*tr_planes, CL_TRUE, 0, sizeof(tr_plane)*planes_size, planes, NULL, &event);
            event.wait();

			queue->enqueueNDRangeKernel(*fire_primary, cl::NullRange, cl::NDRange(width, height), cl::NullRange, NULL, &event);
            event.wait();
            

			cl::size_t<3> origin, region;
			origin[0] = 0; origin[1] = 0; origin[2] = 0;
			region[0] = width; region[1] = height; region[2] = 1;

            auto col = std::dynamic_pointer_cast<ViewportFloat>(colourMap);
            auto geo = std::dynamic_pointer_cast<ViewportFloat>(normalMap);
            auto dpt = std::dynamic_pointer_cast<ViewportFloat>(depthMap);

			queue->enqueueReadImage(*output_image, CL_TRUE, origin, region, 0, 0, col->ptr(), NULL, &event);
            event.wait();
            queue->enqueueReadImage(*geom_image, CL_TRUE, origin, region, 0, 0, geo->ptr(), NULL, &event);
            event.wait();
            queue->enqueueReadImage(*depth_image, CL_TRUE, origin, region, 0, 0, dpt->ptr(), NULL, &event);
            event.wait();
			++frameCount;
			std::cout << "Done frame (" << frameCount << ")                                              \r";
            
            
            if (tr_spheres != nullptr) {
                context->deleteCL(tr_spheres);
            }
            
            if (tr_planes != nullptr) {
                context->deleteCL(tr_planes);
            }
            
            if (tr_triangles != nullptr) {
                context->deleteCL(tr_triangles);
            }
            
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
        
        tr_point3d movePoint(point3d p) {
            tr_point3d t;
            t.s[0] = p.x;
            t.s[1] = p.y;
            t.s[2] = p.z;
            return t;
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
        
        cl::Image2D * output_image = nullptr;
        cl::Image2D * geom_image = nullptr;
        cl::Image2D * depth_image = nullptr;
        
		cl::Buffer * tr_ray_directions = nullptr;
        

		cl::Kernel * fire_primary = nullptr;
        cl::Kernel * gen_rays = nullptr;
        cl::Kernel * gen_triangles = nullptr;
		cl::CommandQueue * queue = nullptr;
		std::shared_ptr<mush::opencl> context = nullptr;
	};
}

#endif