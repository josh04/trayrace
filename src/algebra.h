#ifndef MUSH_ALGEBRA_H
#define MUSH_ALGEBRA_H

// dummy types for correctly sizing data to give opencl

#ifdef _WIN32
#include "CL/cl.h"
#else
#include <OpenCL/cl.h>
#endif

typedef cl_float tr_unit;
typedef cl_float3 tr_point3d;
typedef cl_float4 tr_colour;
typedef cl_float4 tr_packed_sphere;

typedef struct {
	tr_point3d x;
	tr_point3d y;
	tr_point3d z;
} tr_matrix3d;

typedef struct  {
	tr_point3d point;
	tr_point3d direction;
} tr_line3d;

typedef struct {
	tr_point3d location;
	tr_unit radius;
//    float pad;
} tr_sphere;

typedef struct {
    tr_matrix3d coord_transform;
	tr_unit distance;
//    float pad;
} tr_plane;

typedef struct {
    tr_point3d a;
    tr_point3d transformed_a, transformed_b, transformed_c;
    tr_point3d line_a, line_b, line_c;
    tr_matrix3d coord_transform;
} tr_triangle;

#endif
