#ifndef MATRIX3D_HPP
#define MATRIX3D_HPP

#include "point3d.hpp"
#include "unit3d.hpp"

namespace tr{
	class matrix3d {
	public:
		matrix3d(point3d x, point3d y, point3d z)
		 : x(x), y(y), z(z) {

		}

		~matrix3d() {

		}
		point3d x, y, z;
	private:
		
	};

	class zRotation : public matrix3d {
	public:
		zRotation(double theta)
		: matrix3d(	unit3d(cos(theta), sin(theta), 0), 
					unit3d(-sin(theta), cos(theta), 0), 
					unit3d(0, 0, 1)
					) {

		}
	private:
	};

	class xRotation : public matrix3d {
	public:
		xRotation(double theta)
		: matrix3d(	unit3d(1, 0, 0),
					unit3d(0, cos(theta), sin(theta)),
					unit3d(0, -sin(theta), cos(theta)) 
					) {

		}
	};

	class yRotation : public matrix3d {
	public:
		yRotation(double theta)
		: matrix3d(	unit3d(cos(theta), 0, -sin(theta)),
					unit3d(0, 1, 0),
					unit3d(sin(theta), 0, cos(theta))
					) {

		}
	};

	class coordTransform : public matrix3d {
	public:
		coordTransform(point3d inDir, point3d upDir, point3d rightDir) 
			: matrix3d(	unit3d(inDir),
						unit3d(upDir),
						unit3d(rightDir)
			){

		}

		coordTransform()
			: matrix3d(	unit3d(1.0, 0.0, 0.0),
						unit3d(0.0, 1.0, 0.0),
						unit3d(0.0, 0.0, 1.0)
			){

		}

	};

	point3d operator*(const point3d& lhs, const matrix3d& rhs) {
		double x = lhs.x*rhs.x.x + lhs.y*rhs.x.y + lhs.z*rhs.x.z;
		double y = lhs.x*rhs.y.x + lhs.y*rhs.y.y + lhs.z*rhs.y.z;
		double z = lhs.x*rhs.z.x + lhs.y*rhs.z.y + lhs.z*rhs.z.z;
		return point3d(x, y, z);
	}
}

#endif
