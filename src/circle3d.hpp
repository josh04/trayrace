#ifndef CIRCLE3D_HPP
#define CIRCLE3D_HPP

#include "plane3d.hpp"

namespace tr {
	class Circle3d : public plane3d {
	public:
		Circle3d(double radius, double distance, double theta, double phi, Light::rgb colour) : plane3d(distance, theta, phi, colour), radius(radius) {

		}

		~Circle3d() {

		}

		virtual bool intersects(const line3d& ray) const {
			double t = -((ray.point - location).dot(norm)) / (ray.direction.dot(norm));
			
			if (t > 0) {
				point3d intersection = ray.getPoint(t);
				if ((location - intersection).magnitude() < radius) {
					return true;
				}
			}
			return false;
		}

	private:
		double radius;
	};

}

#endif