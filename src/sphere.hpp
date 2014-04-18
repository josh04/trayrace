#ifndef SPHERE_HPP
#define SPHERE_HPP

#include "shape.hpp"
#include "line3d.hpp"
#include "light.hpp"
#include "paint.hpp"

namespace tr {
	class Sphere : public Shape {
	public:
		Sphere(point3d location, double radius, Texture texture)
			: Shape(location, std::move(texture)), radius(radius) {

		}

		Sphere(point3d location, double radius, Light::rgb colour)
			: Sphere(location, radius, std::make_unique<Paint>(colour)) {}

		~Sphere() {}

		bool intersection(const line3d& line, double &distance, Light::rgb &colour) const {
			point3d diff = line.point - location;
			double dot = line.direction.dot(diff);
			double discriminant = dot*dot - diff.dot(diff) + (radius * radius);
			if (discriminant < 0) {
				return false;
			} else if (discriminant > 0) {
				double p1 = -(dot) + sqrt(discriminant);
				double p2 = -(dot) - sqrt(discriminant);
				distance = (p1 < p2) ? p1 : p2;
			} else {
				distance = -(dot);
			}
			colour = texture->colour(0, 0);
			return true;
		}

		unit3d normal(const point3d& pointonsurface) const {
			return unit3d(pointonsurface - location);
		}

	private:
		double radius;
	};
}

#endif