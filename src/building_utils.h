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
		ygl::vec4f color = { 1,1,1,1 };
		float roof_angle = rekt::pi / 2.f;

		// CrossGabled
		float thickness = -1.f; // Ignored if negative
		ygl::vec4f color2 = { 1,1,1,1 };
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
		std::string id = "";
		ygl::vec4f color1 = { 1,1,1,1 };
		ygl::vec4f color2 = { 1,1,1,1 };
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
		shp->color = std::vector<ygl::vec4f>(shp->pos.size(), color);
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
		std::vector<ygl::vec2f> border;
		switch (params.type) {
		case building_type::main_points: 
			border = to_2d(make_wide_line_border(params.floor_main_points, params.floor_width));
			break;
		case building_type::border: border = params.floor_border; break;
		case building_type::regular: 
			border = make_regular_polygon(params.num_sides, params.radius, params.reg_base_angle);
			break;
		default:
			throw std::runtime_error("Invalid building type");
		}
		for_sides(
			border,  
			[&params, &win_id, &windows](const ygl::vec2f& p1, const ygl::vec2f& p2) 
		{
			auto side = p2 - p1;
			auto eps = params.win_pars.windows_distance_from_edges; // Min distance between window and corner
			auto W = ygl::length(side); // Side length
			
			auto w = get_size(params.win_pars.closed_window_shape).x; // Window's width
			if (get_size(params.win_pars.open_window_shape).x > w) 
				w = get_size(params.win_pars.open_window_shape).x;

			auto s = params.win_pars.windows_distance;
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
				for (int j = 0; j < params.num_floors; j++) { // replicate vertically
					// Keep around f_p_s percent of windows
					if (!bernoulli(params.win_pars.filled_spots_ratio, *params.rng))
						continue;

					auto win_center_y = 
						params.floor_height / 2.f + 
						(params.floor_height + params.belt_height)*j;
					
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
			}
		});
		return windows;
	}

	std::tuple<ygl::shape*, ygl::shape*> make_test_windows(
		const std::string& name_open, const std::string& name_closed
	) {
		auto regwnd_shp = new ygl::shape();
		std::tie(regwnd_shp->quads, regwnd_shp->pos) = make_parallelepidedon(1.6f, 1.f, .10f);
		set_shape_normals(regwnd_shp);
		set_shape_color(regwnd_shp, { 0.8f,0.8f,1.f,1.f });
		center_points(regwnd_shp->pos);
		regwnd_shp->mat = make_material(name_open + "_mat", { 1,1,1 }, nullptr, { 0.8f,0.8f,0.8f });
		regwnd_shp->name = name_open + "_shape";

		auto regwnd_close_shp = new ygl::shape();
		std::tie(regwnd_close_shp->quads, regwnd_close_shp->pos) =
			make_parallelepidedon(1.f, 1.f, .10f);
		center_points(regwnd_close_shp->pos);
		set_shape_normals(regwnd_close_shp);
		set_shape_color(regwnd_close_shp, { 0.3f,0.1f,0.f,1.f });
		regwnd_close_shp->mat = make_material(name_closed + "_mat", { 1,1,1 }, nullptr, { 0,0,0 });
		regwnd_close_shp->name = name_closed + "_shape";

		return { regwnd_shp, regwnd_close_shp };
	}

	enum class balcony_type {
		none = 0,
		each_window, // each window has its own balcony
		whole_side, // one balcony for each side of the building (at least one window)
		all_around // a single balcony which goes all around the floor
	};

	/*void merge_balconies(
		const std::vector<ygl::vec3f>& floor_border,
		
	) */

	// Whole house

	ygl::shape* make_roof_from_params(const building_params& params) {
		const auto& r_pars = params.roof_pars; // Shorter alias
		auto base_height = get_building_height(
			params.num_floors, params.floor_height, params.belt_height
		);
		
		switch (r_pars.type) {
		case roof_type::none:
			return new ygl::shape(); // Empty shape
		case roof_type::crossgabled: {
			if (params.type != building_type::main_points) {
				throw std::runtime_error("Invalid parameters");
			}
			auto r_shp = make_roof_crossgabled_simple(
				params.floor_main_points,
				params.floor_width,
				r_pars.roof_angle,
				base_height,
				r_pars.color
			);
			if (r_pars.thickness > 0.f) {
				auto thickness_shp = make_roof_crossgabled_thickness(
					params.floor_main_points, params.floor_width, r_pars.roof_angle,
					r_pars.thickness, r_pars.rake_overhang, r_pars.roof_overhang,
					base_height, r_pars.color2
				);
				merge_shapes(r_shp, thickness_shp);
				delete thickness_shp;
			}
			return r_shp;
		}
		case roof_type::crosshipped:
			if (params.type != building_type::main_points) {
				throw std::runtime_error("Invalid parameters");
			}
			return make_roof_crosshipped_simple(
				params.floor_main_points, params.floor_width, r_pars.roof_angle,
				r_pars.hip_depth, base_height, r_pars.color
			);
		case roof_type::pyramid:
			switch (params.type) {
			case building_type::main_points:
				return make_roof_pyramid_from_main_points(
					params.floor_main_points, params.floor_width, r_pars.roof_height,
					base_height, r_pars.color
				);
			case building_type::border:
				return make_roof_pyramid_from_border(
					params.floor_border, r_pars.roof_height, base_height, r_pars.color
				);
			case building_type::regular:
				return make_roof_pyramid_from_border(
					make_regular_polygon(params.num_sides, params.radius, params.reg_base_angle),
					r_pars.roof_height,
					base_height,
					r_pars.color
				);
			default:
				throw std::runtime_error("Invalid building type");
			}
		default:
			throw std::runtime_error("Invalid roof type");
		}
	}

	std::vector<ygl::instance*> make_building(const building_params& params) {
		std::vector<ygl::instance*> instances;
		
		auto h_inst = new ygl::instance();
		h_inst->name = params.id + "_inst";
		switch (params.type) {
		case building_type::main_points:
			h_inst->shp = make_floors_from_main_points(
				params.floor_main_points, params.floor_width, params.num_floors,
				params.floor_height, params.belt_height, params.belt_additional_width,
				params.color1, params.color2
			);
			break;
		case building_type::border:
			h_inst->shp = make_floors_from_border(
				params.floor_border, params.num_floors, params.floor_height,
				params.belt_height, params.belt_additional_width,
				params.color1, params.color2
			);
			break;
		case building_type::regular:
			h_inst->shp = make_floors_from_border(
				make_regular_polygon(params.num_sides, params.radius, params.reg_base_angle),
				params.num_floors, params.floor_height, params.belt_height,
				params.belt_additional_width, params.color1, params.color2
			);
			break;
		default:
			throw std::runtime_error("Invalid building type");
		}
		h_inst->shp->name = params.id + "_shp";
		auto roof_shp = make_roof_from_params(params);
		merge_shapes(h_inst->shp, roof_shp);
		delete roof_shp;
		h_inst->shp->mat = make_material(params.id + "_mat", { 1,1,1 }, nullptr, { 0,0,0 });
		instances += h_inst;
		
		auto w_insts = make_windows(params);
		instances.insert(instances.end(), w_insts.begin(), w_insts.end());

		return instances;
	}
}

#endif // BUILDING_UTILS_H