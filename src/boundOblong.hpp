#ifndef BOUNDOBLONG_HPP
#define BOUNDOBLONG_HPP

#include "shape.hpp"
#include "plane3d.hpp"
#include "paint.hpp"

namespace tr {
	class BoundOblong : public Shape {
	public:
		BoundOblong(point3d location, double endDimension, double lengthDimension, Texture tex)
			: Shape(location, std::move(tex)), endDimension(endDimension), lengthDimension(lengthDimension),
			up(endDimension, 0, 90, texture->clone()), down(endDimension, 0, -90, texture->clone()),
			front(lengthDimension, 0, 0, texture->clone()), back(lengthDimension, 180, 0, texture->clone()),
			left(lengthDimension, 90, 0, texture->clone()), right(lengthDimension, -90, 0, texture->clone()) {

		}

		BoundOblong(point3d location, double endDimension, double lengthDimension, Light::rgb color)
			: BoundOblong(location, endDimension, lengthDimension, std::make_unique<Paint>(color)) {}

		~BoundOblong() {

		}

		bool intersection(const line3d& line, double &distance, Light::rgb &colour) const {

			double intersect = 0, xintp = 0, yintp = 0, zintp = 0;
			Light::rgb xCol, yCol, zCol;
			if (line.direction.x > 0) {
				front.intersection(line, xintp, xCol);
			} else {
				back.intersection(line, xintp, xCol);
			}

			if (line.direction.y < 0) {
				up.intersection(line, yintp, yCol);
			} else {
				down.intersection(line, yintp, yCol);
			}

			if (line.direction.z < 0) {
				right.intersection(line, zintp, zCol);
			} else {
				left.intersection(line, zintp, zCol);
			}
            
            if (xintp < 1e-5) {
                xintp = 1e9;
            }
            
            if (yintp < 1e-5) {
                yintp = 1e9;
            }
            
            if (zintp < 1e-5) {
                zintp = 1e9;
            }

			if (xintp > yintp) {
				intersect = yintp;
				colour = yCol;
			} else {
				intersect = xintp;
				colour = xCol;
			}

			if (intersect > zintp) {
				intersect = zintp;
				colour = zCol;
			}

			distance = intersect;
			return true;
		}

		unit3d normal(const point3d& loc) const {
			if (std::abs(loc.x - endDimension) < 1e-5) {
				return (unit3d(-1.0, 0.0, 0.0));
			}
			if (std::abs(loc.y - endDimension) < 1e-5) {
				return (unit3d(0.0, -1.0, 0.0));
			}
			if (std::abs(loc.z - lengthDimension) < 1e-5) {
				return (unit3d(0.0, 0.0, -1.0));
			}

			if (std::abs(loc.x + endDimension) < 1e-5) {
				return (unit3d(1.0, 0.0, 0.0));
			}
			if (std::abs(loc.y + endDimension) < 1e-5) {
				return (unit3d(0.0, 1.0, 0.0));
			}
			if (std::abs(loc.z + lengthDimension) < 1e-5) {
				return (unit3d(0.0, 0.0, 1.0));
			}

			return unit3d(1.0, 0.0, 0.0);
		}

		void setUpTexture(Texture tex) {
			up.setTexture(std::move(tex));
		}

		void setDownTexture(Texture tex) {
			down.setTexture(std::move(tex));
		}

		void setRightTexture(Texture tex) {
			right.setTexture(std::move(tex));
		}

		void setLeftTexture(Texture tex) {
			left.setTexture(std::move(tex));
		}

		void setFrontTexture(Texture tex) {
			front.setTexture(std::move(tex));
		}

		void setBackTexture(Texture tex) {
			back.setTexture(std::move(tex));
		}
        
        virtual void move(const point3d& newLocation) {
            location += newLocation;
        }

        void move(const point3d& newLocation, const double endDimension, const double lengthDimension) {
            move(newLocation);
            up.move(endDimension);
            down.move(endDimension);
            front.move(lengthDimension);
            back.move(lengthDimension);
            left.move(lengthDimension);
            right.move(lengthDimension);
        }
	private:
		plane3d up, down, front, back, left, right;
		double endDimension;
		double lengthDimension;
		Light::rgb col;
	};
}

#endif