#ifndef UNIT3D_HPP
#define UNIT3D_HPP

#include "point3d.hpp"

namespace tr {
	class unit3d : public point3d {
	public:
		unit3d(double x, double y, double z)
		 : point3d(x, y, z) {
			double m = magnitude();

			if (std::abs(m) < 1e-9) {
				throw std::runtime_error("Zero unit vector");
			}

			x = x / m;
			y = y / m;
			z = z / m;
		}

		unit3d(double theta, double phi) {
			
		}

		unit3d() : point3d(1.0, 0, 0) {
		}

		unit3d(const unit3d& copy) {
			x = copy.x;
			y = copy.y;
			z = copy.z;
		}

		unit3d(const point3d& copy) {
			double m = copy.magnitude();
			x = copy.x / m;
			y = copy.y / m;
			z = copy.z / m;
		}


		unit3d& operator=(const point3d& rhs) {
			double m = rhs.magnitude();
			x = rhs.x/m;
			y = rhs.y/m;
			z = rhs.z/m;
			return *this;
		}

		~unit3d() {

		}

		unit3d unit() { return *this; }

	private:
	};

	unit3d point3d::unit() {
		if (magnitude() > 0) {
			return unit3d(*this);
		} else {
			return unit3d(0.0, -1.0, 0.0);
		}
	}
}

#endif