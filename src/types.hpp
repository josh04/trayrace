//
//  types.hpp
//  trayrace
//
//  Created by Josh McNamee on 12/08/2014.
//  Copyright (c) 2014 Josh McNamee. All rights reserved.
//

#ifndef trayrace_types_hpp
#define trayrace_types_hpp

#include <memory>
#include <vector>

#include "point3d.hpp"
#include "matrix3d.hpp"

#include "unit3d.hpp"
#include "line3d.hpp"

#include "motion3d.hpp"

using std::shared_ptr;
using std::vector;

namespace tr {
    class Shape;
    class Light;
    
	typedef shared_ptr<vector<shared_ptr<Shape>>> Shapes;
	typedef shared_ptr<vector<shared_ptr<Light>>> Lights;
    
}

#include "shape.hpp"
#include "light.hpp"

#endif
