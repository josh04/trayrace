#ifndef CUBE_HPP
#define CUBE_HPP

#include "matrix3d.hpp"
#include "square3d.hpp"
#include "Texture.hpp"
#include "shape.hpp"

namespace tr {
	class Cube : public Shape {
	public:
		Cube(point3d location, Texture tex, double length, double theta, double phi) : Shape(location, std::move(tex)), length(length), theta(theta), phi(phi),
        top(length, length/2.0, 0, -90, 0, Light::rgb(0.0, 1.0, 0.0)),
        bottom(length, length/2.0, 0, 90, 0, Light::rgb(0.0, 1.0, 0.0)),
        front(length, length/2.0, 0, 0, 0, texture->clone()),
        back(length, length/2.0, 180, 0, 0, texture->clone()),
        left(length, length/2.0, -90, 0, 0, Light::rgb(0.0, 0.9, 0.5)),
        right(length, length/2.0, 90, 0, 0, Light::rgb(0.0, 0.9, 0.5))
        {
            const yRotation yrot(theta * (M_PI / 180.0));
            const zRotation zrot(phi * (M_PI / 180.0));
            orthonormalUp = point3d(0, 1, 0)*zrot*yrot;
            orthonormalRight = point3d(1, 0, 0)*zrot*yrot;
            orthonormalForward = point3d(0, 0, 1)*zrot*yrot;
            coordTrans = coordTransform(orthonormalRight, orthonormalUp, orthonormalForward);
		}
        
		Cube(point3d Location, Light::rgb colour, double length, double theta, double phi)
        : Cube(Location, std::make_unique<Paint>(colour), length, theta, phi) {}

        
		~Cube() {

		}
        
        void relocate(const point3d addloc, const double addlen, const double addtheta, const double addphi) {
            location = location + addloc;
            length = length + addlen;
            theta = theta + addtheta;
            phi = phi + addphi;
            const yRotation xrot(theta * (M_PI / 180.0));
            const zRotation yrot(phi * (M_PI / 180.0));
            orthonormalUp = point3d(0, 1, 0)*xrot*yrot;
            orthonormalRight = point3d(1, 0, 0)*xrot*yrot;
            orthonormalForward = point3d(0, 0, 1)*xrot*yrot;
            coordTrans = coordTransform(orthonormalRight, orthonormalUp, orthonormalForward);
        }

		virtual bool intersection(const line3d& ray, double &distance, Light::rgb &colour) const {
            const line3d rebaseRay((ray.point-location)*coordTrans, ray.direction*coordTrans);
            //const line3d rebaseRay = ray;
            distance = 1e9;
            double temp_distance = 0.0;
            Light::rgb temp_colour = Light::rgb(0);
            bool hit = false;
            
            if (top.intersection(rebaseRay, temp_distance, temp_colour)) {
                if (temp_distance < distance) {
                    distance = temp_distance;
                    colour = temp_colour;
                    hit = true;
                }
            }
            
            if (bottom.intersection(rebaseRay, temp_distance, temp_colour)) {
                if (temp_distance < distance) {
                    distance = temp_distance;
                    colour = temp_colour;
                    hit = true;
                }
            }
            
            if (front.intersection(rebaseRay, temp_distance, temp_colour)) {
                if (temp_distance < distance) {
                    distance = temp_distance;
                    colour = temp_colour;
                    hit = true;
                }
            }
            
            if (back.intersection(rebaseRay, temp_distance, temp_colour)) {
                if (temp_distance < distance) {
                    distance = temp_distance;
                    colour = temp_colour;
                    hit = true;
                }
            }
            
            if (left.intersection(rebaseRay, temp_distance, temp_colour)) {
                if (temp_distance < distance) {
                    distance = temp_distance;
                    colour = temp_colour;
                    hit = true;
                }
            }
            
            if (right.intersection(rebaseRay, temp_distance, temp_colour)) {
                if (temp_distance < distance) {
                    distance = temp_distance;
                    colour = temp_colour;
                    hit = true;
                }
            }
            
            return hit;
		}

		virtual unit3d normal(const point3d& spot) const {
            const point3d rebaseSport = (spot-location)*coordTrans;
            double xdiff = fabs(fabs(rebaseSport.z) - length/2.0);
            double ydiff = fabs(fabs(rebaseSport.y) - length/2.0);
            double zdiff = fabs(fabs(rebaseSport.x) - length/2.0);
            
            int win;
            if (xdiff < ydiff) {
                if (xdiff < zdiff) {
                    win = 0;
                } else {
                    win = 2;
                }
            } else {
                if (ydiff < zdiff) {
                    win = 1;
                } else {
                    win = 2;
                }
            }
            
            switch (win) {
                default:
                case 0:
                    if (rebaseSport.z > 0.0) {
                        return orthonormalForward;
                    } else {
                        return -orthonormalForward;
                    }
                    break;
                case 1:
                    if (rebaseSport.y > 0.0) {
                        return orthonormalUp;
                    } else {
                        return -orthonormalUp;
                    }
                    break;
                case 2:
                    if (rebaseSport.x > 0.0) {
                        return orthonormalRight;
                    } else {
                        return -orthonormalRight;
                    }
                    break;
            }
        }

	private:
		unit3d norm;
		unit3d orthonormalUp;
        unit3d orthonormalRight;
        unit3d orthonormalForward;
        
        Square3d top;
        Square3d bottom;
        Square3d front;
        Square3d back;
        Square3d left;
        Square3d right;
        
        double theta = 0.0, phi = 0.0, length = 0.0;
        
        coordTransform coordTrans;
	};
}
#endif;