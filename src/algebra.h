#ifndef MUSH_ALGEBRA_H
#define MUSH_ALGEBRA_H

// dummy types for correctly sizing data to give opencl

#include "CL/cl.h"

typedef cl_float tr_unit;
typedef cl_float3 tr_point3d;
typedef cl_float4 tr_colour;

typedef struct {
	tr_point3d x;
	tr_point3d y;
	tr_point3d z;
} tr_matrix3d;

typedef struct  {
	tr_point3d point;
	tr_point3d direction;
} tr_line3d;

typedef enum {
	tr_object_sphere,
	tr_object_plane,
	tr_object_square,
	tr_object_cube,
	tr_object_triangle,
	tr_object_bound
} tr_object_type;

typedef struct {
	tr_object_type type;
	tr_point3d location;
	tr_unit radius;
} tr_sphere;

typedef struct {
	tr_object_type type;
	tr_unit distance;
	tr_matrix3d coord_transform;
} tr_plane;

typedef struct {
	tr_object_type type;
	tr_plane up, down, front, back, left, right;
} tr_bound;

typedef union {
	tr_object_type type;
	tr_sphere sphere;
	tr_plane plane;
	tr_bound bound;
} tr_object;

#endif
