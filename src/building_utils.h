#ifndef BUILDING_UTILS_H
#define BUILDING_UTILS_H

#include "geom_utils.h"
#include "prob_utils.h"
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

	// Parameter enums and structs

	enum class roof_type {
		none = 0,
		crossgabled,
		crosshipped,
		pyramid
	};

	// Roof parameters, all types of roof combined for simplicity.
	// Only relevant params have to be set, the others can be ignored
	struct roof_params {
		roof_type type = roof_type::none;
		ygl::vec3f color1 = { 1,1,1 };
		float roof_angle = rekt::pi / 2.f;

		// CrossGabled
		float thickness = -1.f; // Ignored if negative
		ygl::vec3f color2 = { 1,1,1 };
		float rake_overhang = 0.f;
		float roof_overhang = 0.f;

		// CrossHipped
		float hip_depth = 0.f;

		// Pyramid
		float roof_height; // We can't use angle since it's different for each edge
	};

	// Groups the parameters relative to windows's generation
	struct windows_params {
		std::string name = "";
		float windows_distance = 0.f; // Avg distance between windows
		float windows_distance_from_edges = 0.f;
		ygl::shape* closed_window_shape = nullptr;
		ygl::shape* open_window_shape = nullptr;
		float open_windows_ratio = 0.5f; // Percentage of open windows
		float filled_spots_ratio = 1.f; // Ratio of spots that actually have a window
	};

	enum class building_type {
		main_points = 0,
		border,
		regular
	};

	struct building_params {
		building_type type;

		// Type == Main points
		std::vector<ygl::vec2f> floor_main_points = {};
		float floor_width = 1.f;
		// Type == Border
		std::vector<ygl::vec2f> floor_border = {};
		// Type == Regular
		unsigned num_sides = 3;
		float radius = 1.f;
		float reg_base_angle = 0.f;

		unsigned num_floors = 1;
		float floor_height = 1.f;
		float belt_height = 0.1f;
		float belt_additional_width = 0.1f;
		float width_delta_per_floor = 0.f; // How much to expand or shrink 
		                                   // each consecutive floor,
		                                   // as a polygon offsetting size
		std::string id = "";
		ygl::vec3f color1 = { 1,1,1 };
		ygl::vec3f color2 = { 1,1,1 };
		ygl::rng_pcg32* rng = nullptr;

		// Roof
		roof_params roof_pars;

		// Windows
		windows_params win_pars;

		// Wall
		// -- nothing for now --
	};

	// Roofs

	ygl::shape* make_roof_crossgabled_simple(
		const std::vector<ygl::vec2f>& floor_main_points,
		float floor_width,
		float roof_angle,
		float base_height = 0.f
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
		float base_height = 0.f
	) {
		auto shp = make_roof_crossgabled_simple(
			floor_main_points, floor_width, roof_angle, base_height
		);
		auto fr = 2*floor_main_points.size(); // First roof point
		auto lr = shp->pos.size() - 1; // Last roof point
		shp->pos[fr] += ygl::normalize(shp->pos[fr + 1] - shp->pos[fr])*hip_depth;
		shp->pos[lr] -= ygl::normalize(shp->pos[lr] - shp->pos[lr - 1])*hip_depth;
		shp->norm = ygl::compute_normals({}, shp->triangles, shp->quads, shp->pos);
		return shp;
	}

	ygl::shape* make_roof_pyramid_from_border(
		const std::vector<ygl::vec2f>& border,
		float roof_height,
		float base_height = 0.f
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
		return shp;
	}

	ygl::shape* make_roof_pyramid_from_regular(
		const ygl::vec2f& floor_center,
		const ygl::vec2f& floor_vertex,
		unsigned num_sides,
		float roof_angle,
		float base_height = 0.f
	) {
		auto radius_segment = floor_vertex - floor_center;
		auto base_angle = get_angle(radius_segment);
		auto points = make_regular_polygon(num_sides, ygl::length(radius_segment), base_angle);
		return make_roof_pyramid_from_border(
			points, tanf(roof_angle)*ygl::length(radius_segment), base_height
		);
	}

	// Usually not recommended
	ygl::shape* make_roof_pyramid_from_main_points(
		const std::vector<ygl::vec2f>& floor_main_points,
		float floor_width,
		float roof_height,
		float base_height = 0.f
	) {
		auto border = to_2d(make_wide_line_border(floor_main_points, floor_width));
		return make_roof_pyramid_from_border(
			border, roof_height, base_height
		);
	}

	ygl::shape* make_roof_crossgabled_thickness(
		const std::vector<ygl::vec2f>& floor_main_points,
		float floor_width,
		float roof_angle,
		float thickness,
		float rake_overhang,
		float roof_overhang,
		float base_height = 0.f
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
				shp->quads.push_back({ bi + 7,bi + 10,bi + 11,bi + 8 });
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
		return shp;
	}

	// Floors

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

	std::tuple<ygl::shape*, ygl::shape*> __make_floors_from_border(
		const std::vector<ygl::vec2f>& floor_border,
		unsigned num_floors,
		float floor_height,
		float belt_height,
		float belt_additional_width,
		float width_delta_per_floor = 0.f
	) {
		if (num_floors <= 0 || floor_height <= 0.f || 
			belt_height < 0.f || belt_additional_width <= 0.f) {
			throw std::runtime_error("Invalid arguments");
		}
		auto floor = thicken_polygon(floor_border, floor_height);
		ygl::shape* belt = nullptr;
		if (belt_height <= 0.f) { // Belt is optional
			belt = new ygl::shape();
		}
		else {
			belt = thicken_polygon(
				expand_polygon(floor_border, belt_additional_width), 
				belt_height
			);
			displace(belt->pos, { 0,floor_height,0 });
		}
		auto floor_shp = new ygl::shape();
		auto belt_shp = new ygl::shape();
		merge_shapes(floor_shp, floor);
		for (int i = 1; i < num_floors; i++) {
			merge_shapes(belt_shp, belt);
			if (width_delta_per_floor == 0.f) {
				displace(belt->pos, { 0,floor_height + belt_height,0 });
				displace(floor->pos, { 0,floor_height + belt_height,0 });
			}
			else {
				delete floor;
				delete belt;
				floor = thicken_polygon(
					offset_polygon(floor_border, width_delta_per_floor*i)[0],
					floor_height
				);
				belt = thicken_polygon(
					offset_polygon(
						floor_border,
						belt_additional_width + width_delta_per_floor*i
					)[0],
					belt_height
				);
				displace(floor->pos, { 0,(floor_height + belt_height)*i,0 });
				displace(belt->pos, { 0,(floor_height + belt_height)*i + floor_height,0 });
			}
			merge_shapes(floor_shp, floor);
		}

		delete floor;
		delete belt;
		return { floor_shp, belt_shp };
	}

	std::tuple<ygl::shape*, ygl::shape*> make_floors_from_main_points(
		const std::vector<ygl::vec2f>& floor_main_points,
		float floor_width,
		unsigned num_floors,
		float floor_height,
		float belt_height,
		float belt_additional_width,
		float width_delta_per_floor = 0.f
	) {
		if (floor_width <= 0.f || num_floors <= 0 || floor_height <= 0.f || 
			belt_height < 0.f || belt_additional_width < 0.f) {
			throw std::runtime_error("Invalid arguments");
		}

		// This function is mostly copy-pasted from __make_floors_from_border
		// Will fix later :)

		auto floor_border = to_2d(make_wide_line_border(floor_main_points, floor_width));
		auto floor = thicken_polygon(floor_border, floor_height);
		ygl::shape* belt = nullptr;
		if (belt_height <= 0.f) { // Belt is optional
			belt = new ygl::shape();
		}
		else {
			belt = thicken_polygon(
				expand_polygon(floor_border, belt_additional_width),
				belt_height
			);
			displace(belt->pos, { 0,floor_height,0 });
		}

		auto floor_shp = new ygl::shape();
		auto belt_shp = new ygl::shape();
		merge_shapes(floor_shp, floor);
		for (int i = 1; i < num_floors; i++) {
			merge_shapes(belt_shp, belt);
			if (width_delta_per_floor == 0.f) {
				displace(belt->pos, { 0,floor_height + belt_height,0 });
				displace(floor->pos, { 0,floor_height + belt_height,0 });
			}
			else {
				delete floor;
				delete belt;
				auto floor_border = to_2d(make_wide_line_border(
					floor_main_points, 
					floor_width + width_delta_per_floor*i
				));
				floor = thicken_polygon(
					floor_border,
					floor_height
				);
				belt = thicken_polygon(
					offset_polygon(
						floor_border,
						belt_additional_width
					)[0],
					belt_height
				);
				displace(floor->pos, { 0,(floor_height + belt_height)*i,0 });
				displace(belt->pos, { 0,(floor_height + belt_height)*i + floor_height,0 });
			}
			merge_shapes(floor_shp, floor);
		}

		delete floor;
		delete belt;
		return { floor_shp, belt_shp };
	}

	std::tuple<ygl::shape*, ygl::shape*> make_floors_from_regular(
		unsigned num_sides,
		float radius,
		float base_angle,
		unsigned num_floors,
		float floor_height,
		float belt_height,
		float belt_additional_width,
		float width_delta_per_floor = 0.f
	) {
		auto floor_border = make_regular_polygon(
			num_sides,
			radius,
			base_angle
		);

		// This function is mostly copy-pasted from __make_floors_from_border
		// Will fix later :)

		auto floor = thicken_polygon(floor_border, floor_height);
		ygl::shape* belt = nullptr;
		if (belt_height <= 0.f) { // Belt is optional
			belt = new ygl::shape();
		}
		else {
			belt = thicken_polygon(
				expand_polygon(floor_border, belt_additional_width),
				belt_height
			);
			displace(belt->pos, { 0,floor_height,0 });
		}

		auto floor_shp = new ygl::shape();
		auto belt_shp = new ygl::shape();
		merge_shapes(floor_shp, floor);
		for (int i = 1; i < num_floors; i++) {
			merge_shapes(belt_shp, belt);
			if (width_delta_per_floor == 0.f) {
				displace(belt->pos, { 0,floor_height + belt_height,0 });
				displace(floor->pos, { 0,floor_height + belt_height,0 });
			}
			else {
				delete floor;
				delete belt;
				auto floor_border = make_regular_polygon(
					num_sides, radius+width_delta_per_floor*i, base_angle
				);
				floor = thicken_polygon(
					floor_border,
					floor_height
				);
				belt = thicken_polygon(
					offset_polygon(
						floor_border,
						belt_additional_width
					)[0],
					belt_height
				);
				displace(floor->pos, { 0,(floor_height + belt_height)*i,0 });
				displace(belt->pos, { 0,(floor_height + belt_height)*i + floor_height,0 });
			}
			merge_shapes(floor_shp, floor);
		}

		delete floor;
		delete belt;
		return { floor_shp, belt_shp };
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
		bool closed = false
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
		return shp;
	}

	// Windows

	void check_win_info(const windows_params& win_info) {
		const auto& w = win_info;
		if (w.windows_distance < 0.f ||
			w.closed_window_shape == nullptr || w.open_window_shape == nullptr ||
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
		const building_params& params
	) {
		check_win_info(params.win_pars);

		int win_id = 0; // Windows's unique id for instances' names
		std::vector<ygl::instance*> windows;
		for (int i = 0; i < params.num_floors; i++) {
			std::vector<ygl::vec2f> border;
			switch (params.type) {
			case building_type::main_points:
				border = to_2d(make_wide_line_border(
					params.floor_main_points, 
					params.floor_width + params.width_delta_per_floor*i
					)
				);
				break;
			case building_type::border: 
				border = offset_polygon(params.floor_border, params.width_delta_per_floor*i)[0]; 
				break;
			case building_type::regular:
				border = make_regular_polygon(
					params.num_sides, 
					params.radius + params.width_delta_per_floor*i, 
					params.reg_base_angle
				);
				break;
			default:
				throw std::runtime_error("Invalid building type");
			}
			for_sides(border, [&](const ygl::vec2f& p1, const ygl::vec2f& p2) {
				auto side = p2 - p1;
				auto eps = params.win_pars.windows_distance_from_edges; // Min distance between window and corner
				auto W = ygl::length(side); // Side length

				auto w = get_size(params.win_pars.closed_window_shape).x; // Window's width
				if (get_size(params.win_pars.open_window_shape).x > w)
					w = get_size(params.win_pars.open_window_shape).x;

				auto s = params.win_pars.windows_distance;
				int n; // Number of windows fitting on this side within the given constraints
				if (w + 2 * eps >= W) n = 0;
				else n = int((W - w - 2 * eps) / (w + s)) + 1;
				if (n <= 0) return;

				// Having found the number of windows, distribute them more uniformly
				s = (W - 2 * eps - n*w) / (n - 1);
				if (n == 1) eps = W / 2.f; // Special case: 1 window is on the center

				auto dir = ygl::normalize(side); // Side versor
				for (int j = 0; j < n; j++) { // n windows per size
					auto win_center_xz = p1 + dir*(eps + w / 2.f + (w + s)*j);
					// Keep around f_p_s percent of windows
					if (!bernoulli(params.win_pars.filled_spots_ratio, *params.rng))
						continue;

					auto win_center_y =
						params.floor_height / 2.f +
						(params.floor_height + params.belt_height)*i;

					auto win_inst = new ygl::instance();
					win_inst->name = params.win_pars.name + "_" + std::to_string(win_id++);
					win_inst->shp =
						bernoulli(params.win_pars.open_windows_ratio, *params.rng) ?
						params.win_pars.open_window_shape :
						params.win_pars.closed_window_shape;
					win_inst->frame.o = to_3d(win_center_xz, win_center_y);
					rotate_y(win_inst->frame.x, get_angle(side));
					rotate_y(win_inst->frame.z, get_angle(side));
					windows.push_back(win_inst);
				}
			});
		}
		return windows;
	}

	std::tuple<ygl::shape*, ygl::shape*> make_test_windows(
		const std::string& name_open, const std::string& name_closed
	) {
		auto regwnd_shp = new ygl::shape();
		std::tie(regwnd_shp->quads, regwnd_shp->pos) = make_parallelepidedon(1.6f, 1.f, .10f);
		set_shape_normals(regwnd_shp);
		center_points(regwnd_shp->pos);
		regwnd_shp->mat = make_material(name_open + "_mat", { 0.8f,0.8f,1.f }, nullptr, { 0.8f,0.8f,0.8f });
		regwnd_shp->name = name_open + "_shape";

		auto regwnd_close_shp = new ygl::shape();
		std::tie(regwnd_close_shp->quads, regwnd_close_shp->pos) =
			make_parallelepidedon(1.f, 1.f, .10f);
		center_points(regwnd_close_shp->pos);
		set_shape_normals(regwnd_close_shp);
		regwnd_close_shp->mat = make_material(name_closed + "_mat", { 0.3f,0.1f,0.f }, nullptr, { 0,0,0 });
		regwnd_close_shp->name = name_closed + "_shape";

		return { regwnd_shp, regwnd_close_shp };
	}

	building_params* make_rand_building_params(
		ygl::rng_pcg32& rng,
		ygl::shape *open_window_shape,
		ygl::shape *closed_window_shape,
		std::string id
	) {
		building_params *params = new building_params();
		params->type = choose_random_weighted(
			std::vector<building_type>{
				building_type::main_points,
				building_type::border,
				building_type::regular
			},
			{80.f, 5.f, 20.f},
			rng
		);
		int num_segments = choose_random(std::vector<int>{ 3,4,5,6,7,8 }, rng);
		params->floor_main_points = make_segmented_line(
		{ 0,0 }, num_segments, rekt::pi / 2.f,
			[&rng]() {
				float v = 0.f;
				while (v == 0.f) v = rekt::uniform(rng, -rekt::pi / 3.f, rekt::pi / 3.f);
				return v;
			},
			[&rng]() {return rekt::gaussian(rng, 10.f, 1.f);}
		);
		params->floor_width = rekt::uniform(rng, 5.f, 15.f);
		params->floor_border = { {10,10},{0,5},{-10,10},{-5,0},{-10,-10},{0,-5},{10,-10},{5,0} };
		params->num_sides = ygl::next_rand1i(rng, 2) + 3;
		params->radius = uniform(rng, 5.f, 15.f);
		params->reg_base_angle = uniform(rng, 0.f, pi);

		params->num_floors = ygl::next_rand1i(rng, 6) + 3;
		params->floor_height = rekt::uniform(rng, 2.5f, 5.f);
		params->belt_height = rekt::uniform(rng, 0.25f, 0.45f);
		params->belt_additional_width = rekt::uniform(rng, 0.25f, 0.45f);
		params->id = id + "_building";
		params->color1 = rekt::rand_color3f(rng);
		params->color2 = rekt::rand_color3f(rng);
		params->width_delta_per_floor = uniform(rng, -0.15f, 2.f);
		params->rng = &rng;

		if (params->type == building_type::main_points) {
			params->roof_pars.type = rekt::choose_random_weighted(
				std::vector<roof_type>{ 
					roof_type::crossgabled,
					roof_type::crosshipped,
					roof_type::pyramid,
					roof_type::none
				},
				std::vector<float>{75.f, 10.f, 10.f, 5.f},
				rng
			);
		}
		else {
			params->roof_pars.type = rekt::choose_random_weighted(
				std::vector<roof_type>{roof_type::pyramid, roof_type::none},
				std::vector<float>{85.f, 15.f},
				rng
			);
		}
		params->roof_pars.color1 = rekt::rand_color3f(rng);
		params->roof_pars.roof_angle = uniform(rng, pi / 10.f, pi/3.f);
		params->roof_pars.thickness = uniform(rng, 0.25f, 0.75f);
		params->roof_pars.color2 = rekt::rand_color3f(rng);
		params->roof_pars.rake_overhang = uniform(rng, 0.1f, 2.f);
		params->roof_pars.roof_overhang = uniform(rng, 0.1f, 1.f);
		auto max_hip_depth = std::max<float>(std::min<float>(
			ygl::length(params->floor_main_points[1] - params->floor_main_points[0]),
			ygl::length(
				params->floor_main_points.back() -
				params->floor_main_points[params->floor_main_points.size() - 2]
			)
		)*0.9f, 0.f);
		params->roof_pars.hip_depth = uniform(rng, 0.f, max_hip_depth);
		params->roof_pars.roof_height = uniform(rng, 3.f, 13.f);

		params->win_pars.name = id + "_wnd";
		params->win_pars.windows_distance = uniform(rng, .1f, .5f);
		params->win_pars.windows_distance_from_edges = uniform(rng, .2f, .5f);
		params->win_pars.closed_window_shape = closed_window_shape;
		params->win_pars.open_window_shape = open_window_shape;
		params->win_pars.open_windows_ratio = uniform(rng, 0, 1);
		params->win_pars.filled_spots_ratio = uniform(rng, 0, 1);

		return params;
	}

	// Whole house

	std::tuple<ygl::shape*, ygl::shape*> make_floors_from_params(
		const building_params& params
	) {
		switch (params.type) {
		case building_type::main_points:
			return make_floors_from_main_points(
				params.floor_main_points, params.floor_width, params.num_floors,
				params.floor_height, params.belt_height, params.belt_additional_width,
				params.width_delta_per_floor
			);
		case building_type::border:
			return make_floors_from_border(
				params.floor_border, params.num_floors, params.floor_height,
				params.belt_height, params.belt_additional_width,
				params.width_delta_per_floor
			);
		case building_type::regular:
			return make_floors_from_regular(
				params.num_sides, params.radius, params.reg_base_angle,
				params.num_floors, params.floor_height, params.belt_height,
				params.belt_additional_width, params.width_delta_per_floor
			);
		default:
			throw std::runtime_error("Invalid building type");
		}
	}

	std::tuple<ygl::shape*, ygl::shape*> make_roof_from_params(const building_params& params) {
		const auto& r_pars = params.roof_pars; // Shorter alias
		auto base_height = get_building_height(
			params.num_floors, params.floor_height, params.belt_height
		);
		auto floor_width =
			params.floor_width +
			params.width_delta_per_floor * (params.num_floors - 1);
		
		ygl::shape* r_shp = nullptr; // Main body of the roof
		ygl::shape* t_shp = nullptr; // Roof thickness with overhangs

		switch (r_pars.type) {
		case roof_type::none:
			r_shp = new ygl::shape(); // Empty shape
			break;
		case roof_type::crossgabled: 
			if (params.type != building_type::main_points) {
				throw std::runtime_error("Invalid parameters");
			}
			r_shp = make_roof_crossgabled_simple(
				params.floor_main_points,
				floor_width,
				r_pars.roof_angle,
				base_height
			);
			if (r_pars.thickness > 0.f) {
				t_shp = make_roof_crossgabled_thickness(
					params.floor_main_points, floor_width, r_pars.roof_angle,
					r_pars.thickness, r_pars.rake_overhang, r_pars.roof_overhang,
					base_height
				);
			}
			break;
		case roof_type::crosshipped:
			if (params.type != building_type::main_points) {
				throw std::runtime_error("Invalid parameters");
			}
			r_shp = make_roof_crosshipped_simple(
				params.floor_main_points, floor_width, r_pars.roof_angle,
				r_pars.hip_depth, base_height
			);
			break;
		case roof_type::pyramid:
			switch (params.type) {
			case building_type::main_points:
				r_shp = make_roof_pyramid_from_main_points(
					params.floor_main_points, floor_width, r_pars.roof_height,
					base_height
				);
				break;
			case building_type::border:
				r_shp = make_roof_pyramid_from_border(
					offset_polygon(
						params.floor_border,
						params.width_delta_per_floor * (params.num_floors - 1)
					)[0],
					r_pars.roof_height,
					base_height
				);
				break;
			case building_type::regular:
				r_shp = make_roof_pyramid_from_border(
					make_regular_polygon(
						params.num_sides, 
						params.radius + params.width_delta_per_floor*(params.num_floors-1), 
						params.reg_base_angle
					),
					r_pars.roof_height,
					base_height
				);
				break;
			default:
				throw std::runtime_error("Invalid building type");
			}
			break;
		default:
			throw std::runtime_error("Invalid roof type");
		}
		return { r_shp, t_shp != nullptr ? t_shp : new ygl::shape() };
	}

	std::vector<ygl::instance*> make_building(const building_params& params) {
		std::vector<ygl::instance*> instances;
		
		auto h_shp = make_floors_from_params(params);
		instances += make_instance(
			params.id + "_h1",
			std::get<0>(h_shp),
			make_material("", params.color1)
		);
		instances += make_instance(
			params.id + "_h2",
			std::get<1>(h_shp),
			make_material("", params.color2)
		);

		auto r_shp = make_roof_from_params(params);
		instances += make_instance(
			params.id + "_rr",
			std::get<0>(r_shp),
			make_material("", params.roof_pars.color1)
		);
		instances += make_instance(
			params.id + "_rt",
			std::get<1>(r_shp),
			make_material("", params.roof_pars.color2)
		);
		
		auto w_insts = make_windows(params);
		instances.insert(instances.end(), w_insts.begin(), w_insts.end());

		return instances;
	}
}

#endif // BUILDING_UTILS_H