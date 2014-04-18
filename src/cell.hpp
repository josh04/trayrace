#ifndef CELL_HPP
#define CELL_HPP

#define _EPSILON 1e-5
#define _REFLECTIONS 2

#include <vector>
#include <thread>
#include <typeinfo>

#include "point3d.hpp"
#include "shape.hpp"
#include "line3d.hpp"
#include "light.hpp"

using std::vector;
using std::shared_ptr;

namespace tr {
	class Cell {
	public:
		Cell(line3d ray, unsigned int x, unsigned int y, unsigned int w, unsigned int h, point3d u, point3d v)
			: ray(ray), x(x), y(y), w(w), h(h), u(u), v(v) {

		}

		~Cell() {

		}

		static Light::rgb fire(const Shapes& shapes, const Lights& lights, const line3d& ray, uint64_t count = 0) {
			double least_distance = 10000.0;
			Light::rgb colour(0.0);
			std::shared_ptr<const Shape> intersectedShape = nullptr;
			
			Light::rgb shapeColour(0);
			Light::rgb shapeTemp(0);
			double distance = 1e9;

			int shape_count = shapes->size();
			for (int i = 0; i < shape_count; ++i) {
				shared_ptr<const Shape>& shape = std::ref((*shapes)[i]);
				if (shape->intersection(ray, distance, shapeTemp)) {
					if (distance < least_distance && distance > _EPSILON) {
						least_distance = distance;
						intersectedShape = shape;
						shapeColour = shapeTemp;
					}
				}
			}
			if (intersectedShape == nullptr) {
				return Light::rgb(0.0);
			}

			point3d pointOnShape = ray.getPoint(least_distance);
			point3d normal = intersectedShape->normal(pointOnShape);

			Light::rgb highlight = Light::rgb(0);
			int lights_count = lights->size();
			for (int j = 0; j < lights_count; ++j) {
				shared_ptr<const Light>& light = std::ref((*lights)[j]);
				point3d di = light->location - pointOnShape;
				double light_distance = di.magnitude();
				line3d lray(pointOnShape, di.unit());
				bool blocked = false;

				double id = 1e9;
				Light::rgb temp(0);
				for (int i = 0; i < shape_count; ++i) {
					shared_ptr<const Shape>& lshape = std::ref((*shapes)[i]);
					if (lshape.get() != light->shape.get()) {
						if (lshape->intersection(lray, id, temp)) {
							if (id < light_distance && id > _EPSILON) {
								blocked = true;
							}
						}
					}
					if (blocked) {
						break;
					}
				}


				Light::rgb whiteMod = 0.001; //make everything bright2white
				Light::rgb amb = 0.01; //ambient term

				colour += ((whiteMod + shapeColour)*(amb));

				if (!blocked && (intersectedShape.get() != light->shape.get())) {
					// diffuse lighting
#ifdef _DEBUG
					std::string s = typeid(*intersectedShape).name();
					if (s == "BoundOblong") {
						int i = 1 + 2;
					}
#endif

					double nm = lray.direction.dot(normal);
					if (nm < 0) {
						nm = 0;
					}
					colour += (1.0 - (intersectedShape->reflective)) * ((whiteMod + shapeColour)*((light->intensity*nm)));
					
					// phong highlights
					unit3d& ldi = lray.direction;
					unit3d rdi = -(ray.direction);
					unit3d h = (ldi + rdi).unit();
					highlight = light->intensity*(pow(h.dot(normal), 128.0));
					
					colour += (1.0 - (intersectedShape->reflective)) * highlight;

				}
			}

			// specular reflections
			// find direction
			Light::rgb reflection = Light::rgb(0);
			if (intersectedShape->reflective > 0) {
				if (count < _REFLECTIONS) {// && intersectedShape->specular) {
					point3d pt = ray.direction - 2 * (ray.direction.dot(normal)) * normal;
					reflection = Cell::fire(shapes, lights, line3d(pointOnShape, pt), count + 1);
					reflection = intersectedShape->reflective*reflection + (1.0 - intersectedShape->reflective)*reflection*shapeColour;
				}
			}

			colour += intersectedShape->reflective * reflection;


//			point3d bounded = ray.getPoint(least_distance);

			return colour;
		}

		const Light::rgb fire(Shapes shapes, Lights lights) const {
			return Cell::fire(shapes, lights, ray, 0);
		}

		const Light::rgb fire4(Shapes shapes, Lights lights) const {
			const line3d ray1(ray.point, ray.direction + (-0.25 / h)*u + (-0.25 / w)*v);
			const line3d ray2(ray.point, ray.direction + (-0.25 / h)*u + (+0.25 / w)*v);
			const line3d ray3(ray.point, ray.direction + (+0.25 / h)*u + (-0.25 / w)*v);
			const line3d ray4(ray.point, ray.direction + (+0.25 / h)*u + (+0.25 / w)*v);

			Light::rgb colour1 = fire(std::ref(shapes), std::ref(lights), ray1);
			Light::rgb colour2 = fire(std::ref(shapes), std::ref(lights), ray2);
			Light::rgb colour3 = fire(std::ref(shapes), std::ref(lights), ray3);
			Light::rgb colour4 = fire(std::ref(shapes), std::ref(lights), ray4);

			Light::rgb colour;
			colour.r = (colour1.r + colour2.r + colour3.r + colour4.r) / 4;
			colour.g = (colour1.g + colour2.g + colour3.g + colour4.g) / 4;
			colour.b = (colour1.b + colour2.b + colour3.b + colour4.b) / 4;

			return colour;
		}

	private:
		line3d ray;
		unsigned int x, y, w, h;
		point3d u, v;
	};
}

#endif