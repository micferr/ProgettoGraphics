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
		const std::vector<ygl::vec2f>& floor_main_points,
		float floor_width,
		float center_height,
		float base_height = 0.f
	) {
		auto _floor_main_points = to_3d(floor_main_points);
		ygl::shape* shp = new ygl::shape();
		shp->pos = rekt::make_wide_line_border(floor_main_points, floor_width);
		auto mid_point = (shp->pos[0] + shp->pos.back()) / 2.f;
		mid_point.y += center_height;
		shp->pos.push_back(mid_point);
		// i goes along the right side of the widenened segmented line, j is the opposite point
		for (int i = 0, j = shp->pos.size() - 2; i < j-2; i+=1, j-=1) {
			auto mid_point_next = (shp->pos[i+1] + shp->pos[j-1]) / 2.f;
			mid_point_next.y += center_height;
			shp->pos.push_back(mid_point_next);
			shp->quads.push_back({ j,j - 1,i + 1,i });
			shp->quads.push_back({ j - 1,j,int(shp->pos.size() - 2),int(shp->pos.size() - 1) });
			shp->quads.push_back({ i,i + 1,int(shp->pos.size() - 1),int(shp->pos.size() - 2) });
			
			mid_point = mid_point_next;
		}
		int floor_points = 2 * floor_main_points.size();
		shp->triangles.push_back({ 
			0,floor_points,floor_points-1
		});
		shp->triangles.push_back({ 
			floor_points/2, int(shp->pos.size()-1), floor_points/2-1
		});
		if (base_height != 0.f) { // Check purely for performance
			displace(shp->pos, { 0,base_height,0 });
		}
		shp->norm = ygl::compute_normals({}, shp->triangles, shp->quads, shp->pos);
		return shp; 
	}

	/**
	 * Hip depth must be strictly less than both the first and last 
	 * floor's main segments' length.
	 */
	ygl::shape* make_roof_crosshipped_simple(
		const std::vector<ygl::vec2f>& floor_main_points,
		float floor_width,
		float center_height,
		float hip_depth,
		float base_height = 0.f
	) {
		auto shp = make_roof_crossgabled_simple(
			floor_main_points, floor_width, center_height, base_height
		);
		auto fr = 2*floor_main_points.size(); // First roof point
		auto lr = shp->pos.size() - 1; // Last roof point
		shp->pos[fr] += ygl::normalize(shp->pos[fr + 1] - shp->pos[fr])*hip_depth;
		shp->pos[lr] -= ygl::normalize(shp->pos[lr] - shp->pos[lr - 1])*hip_depth;
		return shp;
	}
}

#endif // BUILDING_UTILS_H