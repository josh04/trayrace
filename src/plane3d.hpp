#ifndef PLANE3D_HPP
#define PLANE3D_HPP

#include "line3d.hpp"
#include "shape.hpp"
#include "matrix3d.hpp"
#include "paint.hpp"

namespace tr {
	class plane3d : public Shape {
	public:
		plane3d(double distance, double theta, double phi, Texture texture) : Shape(distance*((point3d(1, 0, 0)*zRotation(phi*(M_PI / 180.0))*yRotation(theta*(M_PI / 180.0)))), std::move(texture)),
			norm((point3d(1, 0, 0)*zRotation(phi*(M_PI / 180.0))*yRotation(theta*(M_PI / 180.0)))),
            distance(distance),
            theta(theta),
            phi(phi) {/*
			point3d Temp = location * zRotation(M_PI-1);
			orthonormalUp = Temp.cross(location);
			orthonormalRight = orthonormalUp.cross(location);
			coordTrans = coordTransform(orthonormalRight, orthonormalUp, norm);*/
            orthonormalUp = point3d(0, 1, 0)*zRotation(phi*(M_PI / 180.0))*yRotation(theta*(M_PI / 180.0));
            orthonormalRight = point3d(1, 0, 0)*zRotation(phi*(M_PI / 180.0))*yRotation(theta*(M_PI / 180.0));
            orthonormalForward = point3d(0, 0, 1)*zRotation(phi*(M_PI / 180.0))*yRotation(theta*(M_PI / 180.0));
            coordTrans = coordTransform(orthonormalRight, orthonormalUp, orthonormalForward);
		}

		plane3d(double distance, double theta, double phi, Light::rgb colour)
			: plane3d(distance, theta, phi, std::make_unique<Paint>(colour)) {}

		~plane3d() {

		}

		virtual bool intersection(const line3d& ray, double &distance, Light::rgb &colour) const {
            
			const double t = -((ray.point - location).dot(norm)) / (ray.direction.dot(norm));
            
			if (t < 0) {
				return false;
			}
            
			distance = t;
			
			point3d spot = ray.getPoint(t)*coordTrans;

			colour = texture->colour(spot.z, spot.y);

			return true;
		}

		virtual unit3d normal(const point3d& location) const {
			return norm;
		}
        
        void move(const point3d& newLocation) {
            move();
        }
        
        void move(const double newDistance) {
            distance = newDistance;
            move();
        }
        
        void move(const double newPhi, const double newTheta) {
            phi = newPhi;
            theta = newTheta;
            move();
        }
        
        void move(const double newDistance, const double newPhi, const double newTheta) {
            distance = +newDistance;
            phi = +newPhi;
            theta = +newTheta;
            move();
        }

	protected:
        void move() {
            location = distance*((point3d(1, 0, 0)*zRotation(phi*(M_PI / 180.0))*yRotation(theta*(M_PI / 180.0))));
            norm = (point3d(1, 0, 0)*zRotation(phi*(M_PI / 180.0))*yRotation(theta*(M_PI / 180.0)));
            orthonormalUp = point3d(0, 1, 0)*zRotation(phi*(M_PI / 180.0))*yRotation(theta*(M_PI / 180.0));
            orthonormalRight = point3d(1, 0, 0)*zRotation(phi*(M_PI / 180.0))*yRotation(theta*(M_PI / 180.0));
            orthonormalForward = point3d(0, 0, 1)*zRotation(phi*(M_PI / 180.0))*yRotation(theta*(M_PI / 180.0));
            coordTrans = coordTransform(orthonormalRight, orthonormalUp, orthonormalForward);
        }
        
        double distance = 0.0, phi = 0.0, theta = 0.0;
		unit3d norm;
		unit3d orthonormalUp;
		unit3d orthonormalRight;
		unit3d orthonormalForward;
		coordTransform coordTrans;
	};
}
#endif