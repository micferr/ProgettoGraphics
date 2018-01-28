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
 *
 * A building's floor can also be described by a (center, arbitrary vertex, number of sides)
 * triple (these are referred to as regular buildings), or directly from all its vertexes. 
 * These mode may however offer less customizability.
 */

namespace rekt {

	ygl::shape* make_roof_crossgabled_simple(
		const std::vector<ygl::vec2f>& floor_main_points,
		float floor_width,
		float roof_angle,
		float base_height = 0.f,
		const ygl::vec4f& color = {1,1,1,1}
	) {
		if (roof_angle <= 0.f || roof_angle >= rekt::pi / 2.f) {
			throw std::runtime_error("Invalid roof angle");
		}
		float center_height = tanf(roof_angle)*floor_width / 2.f;
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
		shp->color = std::vector<ygl::vec4f>(shp->pos.size(), color);
		return shp; 
	}

	/**
	 * Hip depth must be strictly less than both the first and last 
	 * floor's main segments' length + floor_width/2 (floor_width/2 because of
	 * lengthened ends)
	 */
	ygl::shape* make_roof_crosshipped_simple(
		const std::vector<ygl::vec2f>& floor_main_points,
		float floor_width,
		float roof_angle,
		float hip_depth,
		float base_height = 0.f,
		const ygl::vec4f& color = {1,1,1,1}
	) {
		auto shp = make_roof_crossgabled_simple(
			floor_main_points, floor_width, roof_angle, base_height
		);
		auto fr = 2*floor_main_points.size(); // First roof point
		auto lr = shp->pos.size() - 1; // Last roof point
		shp->pos[fr] += ygl::normalize(shp->pos[fr + 1] - shp->pos[fr])*hip_depth;
		shp->pos[lr] -= ygl::normalize(shp->pos[lr] - shp->pos[lr - 1])*hip_depth;
		shp->norm = ygl::compute_normals({}, shp->triangles, shp->quads, shp->pos);
		shp->color = std::vector<ygl::vec4f>(shp->pos.size(), color);
		return shp;
	}

	ygl::shape* make_roof_pyramid_from_border(
		const std::vector<ygl::vec2f>& border,
		float roof_height,
		float base_height = 0.f,
		const ygl::vec4f& color = { 1,1,1,1 }
	) {
		auto shp = new ygl::shape();
		auto t = triangulate_opposite(to_3d(border), {});
		shp->triangles = std::get<0>(t);
		shp->pos = std::get<1>(t);
		auto c = centroid(border);
		auto top = to_3d(c, roof_height);
		shp->pos.push_back(top);
		int sz = shp->pos.size();
		shp->pos.push_back(to_3d(border[0]));
		for (int i = 0; i < border.size()-1; i++) {
			shp->pos.push_back(to_3d(border[i + 1]));
			shp->triangles.push_back({ sz + i, sz + i + 1, sz - 1 });
		}
		shp->triangles.push_back({ sz + int(border.size()) - 1, sz, sz - 1 });
		if (base_height > 0.f) displace(shp->pos, { 0,base_height,0 });
		merge_same_points(shp);
		set_shape_normals(shp);
		set_shape_color(shp, color);
		return shp;
	}

	ygl::shape* make_roof_pyramid_from_regular(
		const ygl::vec2f& floor_center,
		const ygl::vec2f& floor_vertex,
		unsigned num_sides,
		float roof_angle,
		float base_height = 0.f,
		const ygl::vec4f& color = { 1,1,1,1 }
	) {
		auto radius_segment = floor_vertex - floor_center;
		auto base_angle = get_angle(radius_segment);
		auto points = make_regular_polygon(num_sides, ygl::length(radius_segment), base_angle);
		return make_roof_pyramid_from_border(
			points, tanf(roof_angle)*ygl::length(radius_segment), base_height, color
		);
	}

	// Usually not recommended
	ygl::shape* make_roof_pyramid_from_main_points(
		const std::vector<ygl::vec2f>& floor_main_points,
		float floor_width,
		float roof_height,
		float base_height = 0.f,
		const ygl::vec4f& color = { 1,1,1,1 }
	) {
		auto border = to_2d(make_wide_line_border(floor_main_points, floor_width));
		return make_roof_pyramid_from_border(
			border, roof_height, base_height, color
		);
	}

	ygl::shape* make_roof_crossgabled_thickness(
		const std::vector<ygl::vec2f>& floor_main_points,
		float floor_width,
		float roof_angle,
		float thickness,
		float rake_overhang,
		float roof_overhang,
		float base_height = 0.f,
		const ygl::vec4f& color = {1,1,1,1}
	) {
		if (rake_overhang < 0.f || roof_overhang < 0.f) {
			throw std::runtime_error("Invalid arguments.");
		}
		float center_height = tanf(roof_angle)*floor_width / 2.f;
		auto floor_border = make_wide_line_border(floor_main_points, floor_width);
		auto shp = new ygl::shape();
		
		// Law of sines: 
		//		a/sin(A) = b/sin(B) = c/sin(C), where a,b and c are the lengths
		//		of the sides and A,B and C are the opposite angles
		// We calculate the thickened roof's top height as follows:
		//		Let ABC be a triangle with:
		//		- a : thickness segment
		//		- b : height segment
		//		- c : segment between a and b
		//		Then:
		//		- C = roof_angle
		//		- B = pi/2
		//		- A = pi - C - A = pi - pi/2 - roof_angle = pi/2 - roof_angle
		//		For the law of sines, a/sin(A) = b/sin(B) = b/sin(pi/2) = b
		//		-> b = a/sin(A) = thickness / sin(pi/2 - roof_angle)
		auto thick_height = thickness / sinf(rekt::pi / 2.f - roof_angle);
		auto thick_width = thickness / sinf(roof_angle); // Same calculations

		// For each main point we generate six vertexes:
		// - The original right, left and top, with overhangs
		// - As above, but including thickness
		shp->pos.push_back(floor_border[0]);
		shp->pos.push_back(floor_border.back());
		shp->pos.push_back(
			(floor_border[0] + floor_border.back()) / 2.f + 
			ygl::vec3f{0, center_height, 0}
		);
		auto to_right = ygl::normalize(floor_border[0] - floor_border.back());
		shp->pos.push_back(shp->pos[0] + to_right*thick_width);
		shp->pos.push_back(shp->pos[1] - to_right*thick_width);
		shp->pos.push_back(shp->pos[2] + ygl::vec3f{0, thick_height, 0});

		for (int i = 0, j = floor_border.size()-1; i < j-2; i++, j--) {
			// Six new points again, for the next main point
			auto midpoint_next = (floor_border[i + 1] + floor_border[j - 1]) / 2.f;
			midpoint_next += {0, center_height, 0};
			auto ps = shp->pos.size(); // For readability
			shp->pos.push_back(floor_border[i+1]);
			shp->pos.push_back(floor_border[j-1]);
			shp->pos.push_back(midpoint_next);
			to_right = ygl::normalize(floor_border[i + 1] - floor_border[j - 1]);
			shp->pos.push_back(shp->pos[ps] + to_right*thick_width);
			shp->pos.push_back(shp->pos[ps + 1] - to_right*thick_width);
			shp->pos.push_back(shp->pos[ps + 2] + ygl::vec3f{0, thick_height, 0});

			//Faces
			int bi = ps - 6; // First index to put in the following quads
			                 // floor_border[i]
			// Internal quads (they're seen from below)
			shp->quads.push_back({ bi,bi + 2,bi + 8,bi + 6 });
			shp->quads.push_back({ bi + 1,bi + 2,bi + 8,bi + 7 });
			// Upper quads (seen from above)
			shp->quads.push_back({ bi + 3, bi + 9, bi + 11, bi + 5 });
			shp->quads.push_back({ bi + 4,bi + 5,bi + 11,bi + 10 });
			// Bottom flat quads (seen from below)
			shp->quads.push_back({ bi + 1,bi + 4,bi + 10,bi + 7 });
			shp->quads.push_back({ bi,bi + 6,bi + 9,bi + 3 });
			// Vertical, front-facing
			if (i == 0) { // Skip hidden faces
				shp->quads.push_back({ bi + 1,bi + 2,bi + 5,bi + 4 });
				shp->quads.push_back({ bi,bi + 3,bi + 5,bi + 2 });
			}
			// Vertical, rear-facing
			if (i == j - 3) {
				shp->quads.push_back({ bi + 6,bi + 8,bi + 11,bi + 9 });
				shp->quads.push_back({ bi + 7,bi + 8,bi + 11,bi + 10 });
			}
		}
		if (base_height != 0.f) {
			rekt::displace(shp->pos, { 0,base_height,0 });
		}
		if (rake_overhang > 0.f) {
			auto dir = ygl::normalize(shp->pos[6] - shp->pos[0]);
			for (int i = 0; i < 6; i++) {
				shp->pos[i] -= dir*rake_overhang;
			}
			dir = ygl::normalize(shp->pos[shp->pos.size() - 1] - shp->pos[shp->pos.size() - 7]);
			for (int i = shp->pos.size() - 6; i < shp->pos.size(); i++) {
				shp->pos[i] += dir*rake_overhang;
			}
		}
		if (roof_overhang > 0.f) {
			for (int i = 0; i < shp->pos.size(); i+=6) {
				auto to_top = ygl::normalize(shp->pos[i + 2] - shp->pos[i]);
				//Law of sines again
				auto length = roof_overhang / sinf(rekt::pi / 2.f - roof_angle);
				shp->pos[i] -= to_top*length;
				shp->pos[i + 3] -= to_top*length;
				to_top = { -to_top.x, to_top.y, -to_top.z };
				shp->pos[i + 1] -= to_top*length;
				shp->pos[i + 4] -= to_top*length;
			}
		}
		shp->norm = ygl::compute_normals({}, {}, shp->quads, shp->pos);
		shp->color = std::vector<ygl::vec4f>(shp->pos.size(), color);
		return shp;
	}

	float get_building_height(unsigned num_floors, float floor_height, float belt_height) {
		return num_floors*floor_height + (num_floors - 1)*belt_height;
	}

	std::vector<ygl::vec2f> __make_floor_border_from_main_points(
		const std::vector<ygl::vec2f>& floor_main_points,
		float floor_width
	) {
		return to_2d(make_wide_line_border(floor_main_points, floor_width));
	}

	std::vector<ygl::vec2f> __make_floor_border_from_regular(
		const ygl::vec2f& floor_center,
		const ygl::vec2f& floor_vertex,
		unsigned num_sides
	) {
		if (num_sides < 3) {
			throw std::runtime_error("Invalid arguments");
		}
		auto segment = floor_vertex - floor_center;
		auto angle = get_angle(segment);
		auto radius = ygl::length(segment);
		std::vector<ygl::vec2f> points;
		for (int i = 0; i < num_sides; i++) {
			auto a = angle + 2.f*pi / num_sides*i;
			points.push_back(ygl::vec2f{ cos(a),sin(a) }*radius + floor_center);
		}
		return points;
	}

	// Just for consistency :)
	std::vector<ygl::vec2f> __make_floor_border_from_border(
		const std::vector<ygl::vec2f>& border
	) {
		return border;
	}

	ygl::shape* __make_floors_from_border(
		const std::vector<ygl::vec2f>& floor_border,
		unsigned num_floors,
		float floor_height,
		float belt_height,
		float belt_additional_width,
		const ygl::vec4f& floor_color = { 1,1,1,1 },
		const ygl::vec4f& belt_color = { 1,1,1,1 }
	) {
		if (num_floors <= 0 || floor_height <= 0.f || 
			belt_height < 0.f || belt_additional_width <= 0.f) {
			throw std::runtime_error("Invalid arguments");
		}
		auto floor = thicken_polygon(floor_border, floor_height);
		set_shape_color(floor, floor_color);
		ygl::shape* belt = nullptr;
		if (belt_height <= 0.f) { // Belt is optional
			belt = new ygl::shape();
		}
		else {
			std::vector<ygl::vec2f> belt_border =
				expand_polygon(floor_border, belt_additional_width);
			belt = thicken_polygon(belt_border, belt_height);
			set_shape_color(belt, belt_color);
			displace(belt->pos, { 0,floor_height,0 });
		}
		auto shp = new ygl::shape();
		merge_shapes(shp, floor);
		for (int i = 1; i < num_floors; i++) {
			merge_shapes(shp, belt);
			displace(belt->pos, { 0,floor_height + belt_height,0 });
			displace(floor->pos, { 0,floor_height + belt_height,0 });
			merge_shapes(shp, floor);
		}

		delete floor;
		delete belt;
		return shp;
	}

	ygl::shape* make_floors_from_main_points(
		const std::vector<ygl::vec2f>& floor_main_points,
		float floor_width,
		unsigned num_floors,
		float floor_height,
		float belt_height,
		float belt_additional_width,
		const ygl::vec4f& floor_color = { 1,1,1,1 },
		const ygl::vec4f& belt_color = { 1,1,1,1 }
	) {
		if (floor_width <= 0.f || num_floors <= 0 || floor_height <= 0.f || 
			belt_height < 0.f || belt_additional_width < 0.f) {
			throw std::runtime_error("Invalid arguments");
		}
		auto floor_border = to_2d(make_wide_line_border(floor_main_points, floor_width));
		return __make_floors_from_border(
			floor_border, num_floors, floor_height,
			belt_height, belt_additional_width,
			floor_color, belt_color
		);
	}

	ygl::shape* make_floors_from_regular(
		const ygl::vec2f& floor_center,
		const ygl::vec2f& floor_vertex,
		unsigned num_sides,
		unsigned num_floors,
		float floor_height,
		float belt_height,
		float belt_additional_width,
		const ygl::vec4f& floor_color = { 1,1,1,1 },
		const ygl::vec4f& belt_color = { 1,1,1,1 }
	) {
		auto border = make_regular_polygon(
			num_sides,
			ygl::length(floor_vertex - floor_center),
			get_angle(floor_vertex - floor_center)
		);
		displace(border, floor_center);
		return __make_floors_from_border(
			border,
			num_floors, floor_height,
			belt_height, belt_additional_width,
			floor_color, belt_color
		);
	}

	auto& make_floors_from_border = __make_floors_from_border;

	// Walls

	/**
	 * Makes a wall.
	 *
	 * If closed is true, the last input point is connected to the first.
	 */
	ygl::shape* make_wall(
		const std::vector<ygl::vec2f> points,
		float thickness,
		float height,
		bool closed = false,
		const ygl::vec4f& color = {1,1,1,1}
	) {
		ygl::shape* shp;
		if (!closed) {
			shp = thicken_polygon(
				make_wide_line_border(points, thickness),
				height
			);
		}
		else {
			auto ext_border = expand_polygon(points, thickness / 2.f);
			// Assuming only one output polygon
			auto int_border = offset_polygon(points, -thickness / 2.f)[0];
			std::reverse(int_border.begin(), int_border.end());
			shp = thicken_polygon(
				ext_border, height, { int_border }
			);
		}
		set_shape_color(shp, color);
		return shp;
	}

	// Windows

	// Groups the parameters relative to windows's generation
	struct windows_info {
		std::vector<ygl::vec2f>* floor_border = nullptr;
		float floor_height = 1.f;
		float belt_height = 0.1f;
		unsigned num_floors = 2;
		float windows_distance = 0.f; // Avg distance between windows
		float windows_distance_from_edges = 0.f;
		ygl::shape* closed_window_shp = nullptr;
		ygl::shape* open_window_shp = nullptr;
		std::string name = "";
		ygl::rng_pcg32* rng = nullptr; 	
		float open_windows_ratio = 0.5f; // Percentage of open windows
		float filled_spots_ratio = 1.f; // Ratio of spots that actually have a window
		int door_side = -1;
	};

	void check_win_info(const windows_info& win_info) {
		const auto& w = win_info;
		if (w.door_side < -1 || w.door_side > int(w.floor_border->size()) ||
			w.floor_height <= 0.f || w.belt_height < 0.f || w.num_floors < 1 ||
			w.windows_distance < 0.f ||
			w.closed_window_shp == nullptr || w.open_window_shp == nullptr ||
			w.open_windows_ratio < 0.f || w.open_windows_ratio > 1.f ||
			w.filled_spots_ratio < 0.f || w.filled_spots_ratio > 1.f
			) {
			throw std::runtime_error("Invalid windows parameters");
		}
	}

	/**
	 * Makes all the windows for a building.
	 *
	 * Since it needs a different material than the rest of the building,
	 * the shape (material included) is taken as input and a vector of instances
	 * (one for each window) is returned. The shapes are assumed to be centered around
	 * {0,0,0} (on all three dimensions!).
	 */
	std::vector<ygl::instance*> make_windows(
		const windows_info& win_info
	) {
		check_win_info(win_info);

		int win_id = 0; // Windows's unique id for instances' names
		std::vector<ygl::instance*> windows;
		for_sides(
			*win_info.floor_border,  
			[&win_info, &win_id, &windows](const ygl::vec2f& p1, const ygl::vec2f& p2) 
		{
			auto side = p2 - p1;
			auto eps = win_info.windows_distance_from_edges; // Min distance between window and corner
			auto W = ygl::length(side); // Side length
			auto w = get_size(win_info.closed_window_shp).x; // Window's width
			if (get_size(win_info.open_window_shp).x > w) w = get_size(win_info.open_window_shp).x;
			auto s = win_info.windows_distance;
			int n; // Number of windows fitting on this side within the given constraints
			if (w + 2*eps >= W) n = 0;
			else n = int((W - w - 2 * eps) / (w + s)) + 1;
			if (n <= 0) return;

			// Having found the number of windows, distribute them more uniformly
			s = (W - 2 * eps - n*w) / (n - 1);
			if (n == 1) eps = W / 2.f; // Special case: 1 window is on the center
			
			auto dir = ygl::normalize(side); // Side versor
			for (int i = 0; i < n; i++) { // n windows per size
				auto win_center_xz = p1 + dir*(eps + w / 2.f + (w + s)*i);
				for (int j = 0; j < win_info.num_floors; j++) { // replicate vertically
					// Keep around f_p_s percent of windows
					if (!bernoulli(win_info.filled_spots_ratio, *win_info.rng))
						continue;

					auto win_center_y = 
						win_info.floor_height / 2.f + 
						(win_info.floor_height + win_info.belt_height)*j;
					
					auto win_inst = new ygl::instance();
					win_inst->name = win_info.name + "_" + std::to_string(win_id++);
					win_inst->shp =
						bernoulli(win_info.open_windows_ratio, *win_info.rng) ? 
						win_info.open_window_shp : 
						win_info.closed_window_shp;
					win_inst->frame.o = 
						{ win_center_xz.x, win_center_y, win_center_xz.y };
					rotate_y(win_inst->frame.x, -get_angle(side));
					rotate_y(win_inst->frame.z, -get_angle(side));
					windows.push_back(win_inst);
				}
			}
		});
		return windows;
	}
}

#endif // BUILDING_UTILS_H