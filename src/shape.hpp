#ifndef SHAPE_HPP
#define SHAPE_HPP

#include <utility>

#include "types.hpp"
#include "texture.hpp"

namespace tr {
	class Shape {
	public:
		Shape(point3d location, Texture texture) 
        : location(location), texture(std::move(texture)), movement(point3d(0), matrix3d()) {
		}

		~Shape() {}

		virtual bool intersection(const line3d&, double &distance, Light::rgb &colour) const = 0;
		virtual unit3d normal(const point3d& location) const = 0;
        
        virtual void move(const point3d& newLocation) = 0;
        
        virtual motion3d getMotion() const {
            return movement;
        }

		virtual void setTexture(Texture tex) {
			texture.reset(tex.release());
		}
        
        virtual const point3d getLocation() const {
            return location;
        }

		double reflective = false;
	protected:
		point3d location;
		Texture texture;
        motion3d movement;
	private:
	};

}

#endif