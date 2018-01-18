#ifndef BUILDING_UTILS_H
#define BUILDING_UTILS_H

#include "geom_utils.h"
#include "yocto_utils.h"

/**
 * A collection of utilities to deal with procedural building generation.
 *
 * It contains functions to generate most common architectural elements (e.g roofs,
 * belt courses, railings, ...)
 *
 * Many routines use the notion of a building's floor's main point: for buildings
 * with floor shape generated from widening a segmented line, the line's vertexes
 * are that building's floor's main points.
 * See make_wide_line and make_wide_line_border functions in geom_utils.h for further
 * information.
 */

namespace rekt {

	ygl::shape* make_roof_crossgabled_simple(
		const std::vector<ygl::vec3f>& floor_main_points,
		float floor_width,
		float center_height,
		float base_height = 0.f
	) {
		throw std::runtime_error("not implemented");
	}

}

#endif // BUILDING_UTILS_H