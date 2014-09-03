#ifndef POINT3D_HPP
#define POINT3D_HPP

#include <vector>
#include <cmath>

using std::vector;

namespace tr {
	class unit3d;
    
	class point3d {
	public:
		point3d(double x = 0, double y = 0, double z = 0)
			: x(x), y(y), z(z) {

		}

		point3d(const point3d& point)
			: x(point.x), y(point.y), z(point.z) {
			
		}

		~point3d() {

		}

		virtual unit3d unit();

		const double magnitude() const {
			return sqrt(x*x + y*y + z*z);
		}

		const double dot(const point3d& rhs) const {
			return x*rhs.x + y*rhs.y + z*rhs.z;
		}


		point3d& operator=(const point3d& rhs) {
			x = rhs.x;
			y = rhs.y;
			z = rhs.z;
			return *this;
		}
		
		point3d& operator+=(const point3d& rhs) {
			x += rhs.x;
			y += rhs.y;
			z += rhs.z;
			return *this;
		}

		point3d& operator-=(const point3d& rhs) {
			x -= rhs.x;
			y -= rhs.y;
			z -= rhs.z;
			return *this;
		}

		const point3d operator-() const {
			return point3d(-x, -y, -z);
		}

		const point3d cross(const point3d& rhs) const {
			return point3d(y*rhs.z - z*rhs.y, z*rhs.x - x*rhs.z, x*rhs.y - y*rhs.x);
		}
		double x, y, z;
	private:
		
	};

	point3d operator+(const point3d& lhs, const point3d& rhs) {
		return point3d(lhs.x+rhs.x, lhs.y+rhs.y, lhs.z+rhs.z);
	}

	point3d operator-(const point3d& lhs, const point3d& rhs) {
		return point3d(lhs.x-rhs.x, lhs.y-rhs.y, lhs.z-rhs.z);
	}

	point3d operator*(const point3d& lhs, const double& rhs) {
		return point3d(lhs.x*rhs, lhs.y*rhs, lhs.z*rhs);
	}

	point3d operator*(const double& lhs, const point3d& rhs) {
		return rhs*lhs;
	}

	point3d operator*(const point3d& lhs, const point3d& rhs) {
		return point3d(lhs.x*rhs.x, lhs.y*rhs.y, lhs.z*rhs.z);
	}

	point3d operator/(point3d lhs, const double rhs) {
		double x = lhs.x / rhs;
		double y = lhs.y / rhs;
		double z = lhs.z / rhs;
		point3d ret = point3d(x, y, z);

		return ret;
	}

}

#endif