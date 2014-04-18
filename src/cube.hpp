#ifndef CUBE_HPP
#define CUBE_HPP

#include "shape.hpp"

namespace tr {
	class Cube : public Shape {
	public:
		Cube(point3d Location, Light::rgb colour, double theta, double phi) : Shape(Location), col(colour) {

		}

		~Cube() {

		}

		virtual bool intersection(const line3d& ray, double &distance, Light::rgb &colour) const {

		}

		virtual unit3d normal(const point3d& location) const = 0;

	private:
		Light::rgb col;
	};
}
#endif;