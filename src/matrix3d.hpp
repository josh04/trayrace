#ifndef MATRIX3D_HPP
#define MATRIX3D_HPP

#include "point3d.hpp"
#include "unit3d.hpp"

namespace tr{
    class matrix3d;
    matrix3d operator*(const matrix3d& lhs, const matrix3d& rhs);
    
	class matrix3d {
	public:
		matrix3d(point3d x, point3d y, point3d z)
		 : x(x), y(y), z(z) {

		}
        
        matrix3d(const matrix3d& rhs) : x(rhs.x), y(rhs.y), z(rhs.z) {
            
        }
        
        matrix3d()
         : x(1, 0, 0), y(0, 1, 0), z(0, 0 ,1) {
            
        }

		~matrix3d() {

		}
        
        const matrix3d inverse() const {
            const double oneoverdeterminant = 1 / ( x.x*(y.y*z.z-z.y*y.z)
                                                   -y.x*(x.y*z.z-z.y*x.z)
                                                   +z.x*(x.y*y.z-y.y*x.z) );
            
            const point3d firstcol((y.y*z.z-z.y*y.z),
                                   -(y.x*z.z-z.x*y.z),
                                   (y.x*z.y-z.x*y.y));
            const point3d secondcol(-(x.y*z.z-z.y*x.z),
                                    (x.x*z.z-z.x*x.z),
                                    -(x.x*z.y-z.x*x.y));
            const point3d thirdcol((x.y*y.z-y.y*x.z),
                                    -(x.x*y.z-y.x*x.z),
             (x.x*y.y-y.x*x.y));
            
            return matrix3d(firstcol*oneoverdeterminant, secondcol*oneoverdeterminant, thirdcol*oneoverdeterminant).transpose();
        }
        
        const matrix3d transpose() const {
            return matrix3d(point3d(x.x, y.x, z.x), point3d(x.y, y.y, z.y), point3d(x.z, y.z, z.z));
        }
  /*
        bool test() const {
            matrix3d i = inverse();
            matrix3d temp{};
            i *= (*this);
            
            return i.y.x == temp.y.x;
        }
        */
        matrix3d& operator+=(const matrix3d& rhs) {
			x += rhs.x;
			y += rhs.y;
			z += rhs.z;
			return *this;
		}
        
        matrix3d& operator-=(const matrix3d& rhs) {
			x -= rhs.x;
			y -= rhs.y;
			z -= rhs.z;
			return *this;
		}
        
        matrix3d& operator*=(const matrix3d& rhs) {
			*this = (*this) * rhs;
			return *this;
		}
        
		point3d x, y, z;
	private:
		
	};

    matrix3d operator+(matrix3d lhs, const matrix3d& rhs) {
		lhs += rhs;
		return lhs;
	}
    
    
	matrix3d operator-(matrix3d lhs, const matrix3d& rhs) {
		lhs -= rhs;
		return lhs;
	}
    
	matrix3d operator*(const matrix3d& lhs, const double& rhs) {
		return matrix3d(lhs.x*rhs, lhs.y*rhs, lhs.z*rhs);
	}
    
    matrix3d operator*(const matrix3d& lhs, const matrix3d& rhs) {
        const point3d x = point3d(lhs.x.x*rhs.x.x + lhs.y.x*rhs.x.y + lhs.z.x*rhs.x.z,
                    lhs.x.y*rhs.x.x + lhs.y.y*rhs.x.y + lhs.z.y*rhs.x.z,
                    lhs.x.z*rhs.x.x + lhs.y.z*rhs.x.y + lhs.z.z*rhs.x.z);
        const point3d y = point3d(lhs.x.x*rhs.y.x + lhs.y.x*rhs.y.y + lhs.z.x*rhs.y.z,
                    lhs.x.y*rhs.y.x + lhs.y.y*rhs.y.y + lhs.z.y*rhs.y.z,
                    lhs.x.z*rhs.y.x + lhs.y.z*rhs.y.y + lhs.z.z*rhs.y.z);
        const point3d z = point3d(lhs.x.x*rhs.z.x + lhs.y.x*rhs.z.y + lhs.z.x*rhs.z.z,
                    lhs.x.y*rhs.z.x + lhs.y.y*rhs.z.y + lhs.z.y*rhs.z.z,
                    lhs.x.z*rhs.z.x + lhs.y.z*rhs.z.y + lhs.z.z*rhs.z.z);
        
		return matrix3d(x, y, z);
	}
    
	point3d operator*(const point3d& lhs, const matrix3d& rhs) {
		double x = lhs.x*rhs.x.x + lhs.y*rhs.x.y + lhs.z*rhs.x.z;
		double y = lhs.x*rhs.y.x + lhs.y*rhs.y.y + lhs.z*rhs.y.z;
		double z = lhs.x*rhs.z.x + lhs.y*rhs.z.y + lhs.z*rhs.z.z;
		return point3d(x, y, z);
	}
    
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
}

#endif
