#ifndef SHAPE_HPP
#define SHAPE_HPP

#include "unit3d.hpp"
#include "line3d.hpp"
#include "light.hpp"
#include "texture.hpp"

namespace tr {
	class Shape {
	public:
		Shape(point3d location, Texture texture) 
			: location(location), texture(std::move(texture)) {
		}

		~Shape() {}

		virtual bool intersection(const line3d&, double &distance, Light::rgb &colour) const = 0;
		virtual unit3d normal(const point3d& location) const = 0;

		virtual void setTexture(Texture tex) {
			texture.reset(tex.release());
		}

		double reflective = false;
	protected:
		point3d location;
		Texture texture;
	private:
	};

}

#endif