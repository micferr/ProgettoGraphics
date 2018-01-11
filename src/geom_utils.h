#ifndef GEOM_UTILS_H
#define GEOM_UTILS_H

#include <vector>

#include "yocto\yocto_gl.h"
#include "poly2tri\poly2tri\poly2tri.h"

namespace rekt {
	const float pi = 3.14159265359f;

	ygl::vec2f centroid(std::vector<ygl::vec2f>& points) {
		// Average of the input points
		ygl::vec2f c = { 0,0 };
		for (auto& p : points) c += p;
		return c / points.size();
	}

	ygl::vec3f centroid(std::vector<ygl::vec3f>& points) {
		ygl::vec3f c = { 0,0,0 };
		for (auto& p : points) c += p;
		return c / points.size();
	}

	/**
	* Discards the y component of 3d points
	*/
	std::vector<ygl::vec2f> to_2d(
		const std::vector<ygl::vec3f>& points
	) {
		std::vector<ygl::vec2f> res;
		for (const auto& p : points) {
			res.push_back({ p.x, p.z });
		}
		return res;
	}

	/**
	 * Transforms 2D points (x,z) to 3D points (x,y,-z), with y in input.
	 * z is flipped by default to preserve clockwiseness
	 */
	std::vector<ygl::vec3f> to_3d(
		const std::vector<ygl::vec2f>& points, 
		float y = 0.f,
		bool flip_z = true
	) {
		std::vector<ygl::vec3f> res;
		for (const auto& p : points) {
			res.push_back({ p.x, y, flip_z ? -p.y : p.y });
		}
		return res;
	}

	/**
	 * Return the points of a 2D polygon.
	 * The first point is set at (radius,0) if the polygon is not aligned,
	 * else it's at radius*(cos(a),sin(a)), a = 2*pi/(2*num_sides)
	 */
	std::vector<ygl::vec2f> make_regular_polygon(
		int num_sides, 
		float radius = 1.f,
		bool xy_aligned = false
	) {
		if (num_sides <= 2) {
			throw std::exception("A polygon must have at least 3 sides.");
		}
		std::vector<ygl::vec2f> res;
		float angle_step = 2 * pi / num_sides;
		float angle0 = xy_aligned ? angle_step/2.f : 0.f;
		for (int i = 0; i < num_sides; i++) {
			float angle = i*angle_step + angle0;
			res.push_back({ cos(angle)*radius, sin(angle)*radius });
		}
		return res;
	}

	/**
	 * Returns a 2D regular polygon in 3D space, with a fixed
	 * value of 0 for the y component of the points.
	 * Face normal is (0,1,0)
	 */
	std::vector<ygl::vec3f> make_regular_polygonXZ(
		int num_sides,
		float radius = 1.f,
		bool xz_aligned = false
	) {
		auto poly = to_3d(make_regular_polygon(num_sides, radius, xz_aligned));
		return poly;
	}

	std::vector<ygl::vec2f> make_quad(float side_length = 1.f) {
		float s = side_length/2.f;
		return { {s,s},{-s,s},{-s,-s},{s,-s} };
	}

	std::vector<ygl::vec3f> make_quadXZ(float side_length = 1.f) {
		auto quad = to_3d(make_quad(side_length));
		return quad;
	}

	/**
	 * Returns the point of a line made of 'steps' consecutive, connected
	 * segments.
	 * 
	 * Arguments:
	 *		start : Position of the first vertex of the line
	 *		steps : Number of segments in the line
	 *		start_alpha : Angle of the first generated segment
	 *		alpha_delta_per_step : Generator of subsequent angle changes
	 *			(the returned angle is a delta to add to the previous value).
	 *		segment_length_per_step : Generator of subsequent segment lengths
	 */
	std::vector<ygl::vec2f> make_segmented_line(
		const ygl::vec2f& start,
		unsigned steps,
		float start_alpha,
		const std::function<float()>& alpha_delta_per_step,
		const std::function<float()>& segment_length_per_step
	) {
		std::vector<ygl::vec2f> points;
		points.push_back(start);
		float alpha = start_alpha;
		for (int i = 0; i < steps; i++) {
			alpha += !i ? 0.f : alpha_delta_per_step();
			const ygl::vec2f& last_pos = points.back();
			float segment_length = segment_length_per_step();
			points.push_back(
				last_pos + ygl::vec2f{cos(alpha), sin(alpha)}*segment_length
			);
		}
		return points;
	}

	/**
	 * Overridden version where the starting angle is also
	 * randomly generated
	 */
	std::vector<ygl::vec2f> make_segmented_line(
		const ygl::vec2f& start,
		unsigned steps,
		const std::function<float()>& alpha_delta_per_step,
		const std::function<float()>& segment_length_per_step
	) {
		return make_segmented_line(
			start,
			steps,
			alpha_delta_per_step(),
			alpha_delta_per_step,
			segment_length_per_step
		);
	}

	/**
	 * Returns a 2d surface obtained from widening the input line.
	 * It is currently assumed that the resulting polygon will be simple.
	 */
	std::tuple<std::vector<ygl::vec4i>, std::vector<ygl::vec3f>> make_wide_line(
		const std::vector<ygl::vec2f>& points,
		float width,
		bool lengthen_ends = false
	) {
		auto half_width = width / 2.f;
		auto _points = points; // Work-around to keep code cleaner :)
		std::vector<ygl::vec2f> vertexes;
		if (lengthen_ends) {
			_points[0] -= ygl::normalize(points[1] - points[0]) * half_width;
			_points.back() += ygl::normalize(points[points.size() - 1] - points[points.size() - 2]) * half_width;
		}
		_points.push_back(2*_points[_points.size()-1] - _points[_points.size()-2]); //p[size] + to(p[last-1], p[last])
		_points.insert(_points.begin(), _points[0] - (_points[1]-_points[0]));
		for (int i = 1; i < _points.size() - 1; i++) {
			const auto& p1 = _points[i - 1];
			const auto& p2 = _points[i];
			const auto& p3 = _points[i + 1];
			auto segment1 = p2 - p1;
			auto segment2 = p3 - p2;
			auto alpha1 = atan2f(segment1.y, segment1.x);
			auto alpha2 = atan2f(segment2.y, segment2.x);
			auto alpha = (alpha1 + alpha2) / 2;
			ygl::vec2f delta = ygl::vec2f{ cos(alpha + pi / 2.f), sin(alpha + pi / 2.f) }*half_width;
			vertexes.push_back(p2 + delta);
			vertexes.push_back(p2 - delta);
		}
		std::vector<ygl::vec4i> quads;
		for (int i = 0; i < vertexes.size()-4; i += 2) {
			quads.push_back({ i,i + 1,i + 3,i + 2 });
		}
		return { quads, to_3d(vertexes,0,false) };
	}

	std::vector<ygl::vec3f> make_wide_line_border(
		const std::vector<ygl::vec2f>& points,
		float width,
		bool lengthen_ends = false
	) {
		auto pos = std::get<1>(make_wide_line(points, width, lengthen_ends));
		std::vector<ygl::vec3f> res;
		for (int i = 0; i < pos.size(); i += 2) res.push_back(pos[i]);
		for (int i = pos.size() - 1; i >= 0; i -= 2) res.push_back(pos[i]);
		return res;
	}

	/**
	 * Triangulates an arbitrary shape.
	 * Holes must be given in clockwise order
	 */
	std::tuple<std::vector<ygl::vec3i>, std::vector<ygl::vec3f>>
		triangulate(
			const std::vector<ygl::vec3f>& border,
			const std::vector<std::vector<ygl::vec3f>>& holes = {}
		) {
		// Code adapted from https://github.com/greenm01/poly2tri/blob/master/testbed/main.cc
		std::vector<p2t::Point*> polyline;
		for (const auto& p : border) polyline.push_back(new p2t::Point(p.x,p.z));

		std::vector<std::vector<p2t::Point*>> p2t_holes;
		for (const auto& hole : holes) {
			p2t_holes.push_back(std::vector<p2t::Point*>());
			for (const auto& p : hole) p2t_holes.back().push_back(new p2t::Point(p.x,p.z));
		}

		p2t::CDT* cdt = new p2t::CDT(polyline);
		for (const auto& hole : p2t_holes) cdt->AddHole(hole);

		cdt->Triangulate();
		auto p2t_triangles = cdt->GetTriangles();
		auto map = cdt->GetMap();

		std::vector<ygl::vec2f> pos;
		std::vector<ygl::vec3i> triangles;
		int i = 0;
		for (auto t : p2t_triangles) {
			auto p1 = *t->GetPoint(0),
				p2 = *t->GetPoint(1),
				p3 = *t->GetPoint(2);
			pos.push_back({ float(p1.x), float(p1.y) });
			pos.push_back({ float(p2.x), float(p2.y) });
			pos.push_back({ float(p3.x), float(p3.y) });
			triangles.push_back({ i,i + 1,i + 2 });
			i += 3;
		}

		//Cleanup
		for (auto p : polyline) delete p;
		for (const auto& hole : p2t_holes) {
			for (auto p : hole) {
				delete p;
			}
		}

		return { triangles, to_3d(pos) };
	}

	/**
	 * Moves all points by the specified value
	 */
	template<typename T>
	void displace(std::vector<T>& points, T& disp) {
		for (auto& p : points) p += disp;
	}

	template<typename T>
	void scale(std::vector<T>& points, float scale) {
		for (auto& p : points) p *= scale;
	}

	/**
	* Rotates all points by 'angle' radiants, counter-clockwise relative to (0,0)
	*
	* The function is quite inefficient, to rotate an in-world instance try changing
	* its frame rather than using this.
	*/
	void rotate(std::vector<ygl::vec2f>& points, float angle) {
		for (auto& p : points) {
			float new_angle = atan2f(p.y, p.x) + angle;
			float length = ygl::length(p);
			p = { length*cos(new_angle), length*sin(new_angle) };
		}
	}

	/**
	 * Merges duplicate points and updates triangles and quads.
	 * eps is the distance under which two points are considered the same.
	 */
	/*void compact_shape(
		std::vector<ygl::vec3i>& triangles,
		std::vector<ygl::vec4i>& quads,
		std::vector<ygl::vec3f>& pos,
		float eps = 1e-6f
	) {
		// Naive, computationally expensive algorithm
		// For each point, check its duplicates and update polygons accordingly
		std::vector<bool> merged(pos.size(), false); // Merged points will be skipped
		for (int i = 0; i < pos.size()-1; i++) {
			if (merged[i]) continue; // The point has already been merged
			auto& p1 = pos[i];
			for (int j = i + 1; j < pos.size(); j++) {
				if (merged[j]) continue;
				auto& p2 = pos[j];
				if (ygl::length(p1 - p2) < eps) { // If the distance between the two points is near zero
					merged[j] = true; // Set point j as merged
					for (auto& t : triangles) {
						for (auto& p : t) {
							if (p == j) p = i; // i == j, we delete j and set p to i
							else if (p > j) p--; // we delete j so all vertexes after it fall back one place
						}
					}
					// Same as for triangles
					for (auto& q : quads) {
						for (auto& p : q) {
							if (p == j) p = i;
							else if (p > j) p--;
						}
					}
				}
			}
		}
		// Delete merged points from pos
		auto pos_copy = pos;
		pos.clear();
		for (int i = 0; i < pos_copy.size(); i++) {
			if (!merged[i]) {
				pos.push_back(pos_copy[i]);
			}
		}
	}*/

	/**
	 * Makes a thick solid from a polygon.
	 *
	 * Thickness is added along the face normal
	 */
	ygl::shape* thicken_polygon(
		const std::vector<ygl::vec3f>& border,
		float thickness,
		const std::vector<std::vector<ygl::vec3f>>& holes = {}
	) {
		auto shape = new ygl::shape();
		shape->pos = border; 
		auto face_norm = ygl::normalize(ygl::cross(border[1] - border[0], border[2] - border[1]));
		face_norm = { 0,1,0 }; // ???
		// Outer walls
		for (const auto& p : border) {
			shape->pos.push_back(p + face_norm*thickness);
		}
		int ps = border.size();
		for (int i = 0; i < border.size() - 1; i++) {
			shape->quads.push_back({ i,i + 1,ps + i + 1,ps + i });
		}
		shape->quads.push_back({ ps - 1,0,ps,2 * ps - 1 });

		// Hole walls
		for (const auto& hole : holes) {
			ps = shape->pos.size();
			int hs = hole.size();
			shape->pos.insert(shape->pos.end(), hole.begin(), hole.end()); // Floor points
			for (const auto& p : hole) shape->pos.push_back(p + face_norm*thickness); // Ceiling points
			for (int i = 0; i < hole.size() - 1; i++) {
				shape->quads.push_back({ ps + i,ps + i + 1,ps + hs + i + 1,ps + hs + i });
			}
		}

		// Floor and ceiling
		ps = shape->pos.size();
		auto triangles_data = triangulate(border, holes);
		const auto& triangles = std::get<0>(triangles_data);
		const auto& triangles_pos = std::get<1>(triangles_data);
		ygl::vec3i psize3 = { ps,ps,ps };
		ygl::vec4i psize4 = { ps,ps,ps,ps };
		shape->pos.insert(shape->pos.end(), triangles_pos.begin(), triangles_pos.end()); // Floor
		for (const auto& p : triangles_pos) shape->pos.push_back(p + face_norm*thickness); // Ceiling
		for (const auto& t : triangles) {
			auto nt = t + psize3;
			shape->triangles.push_back({ nt.z, nt.y, nt.x }); // Original face, seen from the other side
			nt += {int(triangles_pos.size()), int(triangles_pos.size()), int(triangles_pos.size())};
			shape->triangles.push_back(nt); // New face
		}
		//compact_shape(shape->triangles, shape->quads, shape->pos);
		shape->norm = ygl::compute_normals({}, shape->triangles, shape->quads, shape->pos);
		return shape;
	}

/**
 * If origin_center is true, the model is centered on (0,0,0),
 * else its center is (width,height,depth)/2
 */
#define DEFAULT_ORIGIN_CENTER true

	std::tuple<std::vector<ygl::vec4i>, std::vector<ygl::vec3f>> 
		make_parallelepidedon(
			float x, float y, float z,
			float width, float height, float depth,
			bool origin_center = DEFAULT_ORIGIN_CENTER
		) {
		auto t = ygl::make_cube();
		auto w = width / 2.f,
			h = height / 2.f,
			d = depth / 2.f;
		for (auto& p : std::get<1>(t)) {
			p.x *= w;
			p.y *= h;
			p.z *= d;
			if (!origin_center) {
				p += {w, h, d};
			}
			p += {x, y, z};
		}
		return t;
	}

#undef DEFAULT_ORIGIN_CENTER

	/**
	 * Adds equilateral triangles on each side of the given polygon.
	 * 
	 * Arguments:
	 *     - polygon : the base polygon
	 *     - external : whether the fractal must be expanded outside or inside the polygon
	 *                  (assumes vertexes in counter-clockwise order)
	 *     - levels : number of recursion for the fractal expansion
	 */
	std::vector<ygl::vec2f> fractalize_triangle(
		const std::vector<ygl::vec2f>& polygon,
		bool outside = true,
		int levels = 1
	) {
		auto points = polygon;
		while (levels-- > 0) {
			std::vector<ygl::vec2f> newpos;
			for (int i = 0; i < polygon.size(); i++) {
				newpos.push_back(points[i]);
				ygl::vec2f mid1 = (points[i] * 2 / 3) + (points[(i + 1) % points.size()] * 1 / 3);
				ygl::vec2f mid2 = (points[i] * 1 / 3) + (points[(i + 1) % points.size()] * 2 / 3);
				ygl::vec2f mid =  (points[i] + points[(i + 1) % points.size()]) / 2;
				float side_normal_angle = 
					atan2f(mid1.x - mid2.x, mid1.y - mid2.y) + 
					((pi / 2) * (outside?+1.f:-1.f));
				// Height of an equilateral triangle is sqrt(3)/2 times the length of its side
				// sqrt(3)/2 roughly equals 0.866
				float triangle_height = ygl::length(mid2 - mid1)*0.866f;
				newpos.push_back(mid1);
				newpos.push_back(mid + ygl::vec2f{ sin(side_normal_angle)*triangle_height, cos(side_normal_angle)*triangle_height });
				newpos.push_back(mid2);
			}
			points = newpos;
		}
		return points;
	}

	/**
	 * Adds squares on each side of the given polygon.
	 * 
	 * Arguments:
	 *     - polygon : the base polygon
	 *     - external : whether the fractal must be expanded outside or inside the polygon
	 *                  (assumes vertexes in counter-clockwise order)
	 *     - levels : number of recursion for the fractal expansion
	 */
	std::vector<ygl::vec2f> fractalize_square(
		const std::vector<ygl::vec2f>& polygon,
		bool outside = true,
		int levels = 1
	) {
		auto points = polygon;
		while (levels-- > 0) {
			std::vector<ygl::vec2f> newpos;
			for (int i = 0; i < polygon.size(); i++) {
				newpos.push_back(points[i]);
				ygl::vec2f mid1 = (points[i] * 2 / 3) + (points[(i + 1) % points.size()] * 1 / 3);
				ygl::vec2f mid2 = (points[i] * 1 / 3) + (points[(i + 1) % points.size()] * 2 / 3);
				float side_normal_angle = 
					atan2f(mid1.x - mid2.x, mid1.y - mid2.y) + 
					((pi / 2) * (outside ? +1.f : -1.f));
				float quad_height = ygl::length(mid2 - mid1);
				auto add = ygl::vec2f{ sin(side_normal_angle)*quad_height, cos(side_normal_angle)*quad_height };
				newpos.push_back(mid1);
				newpos.push_back(mid1 + add);
				newpos.push_back(mid2 + add);
				newpos.push_back(mid2);
			}
			points = newpos;
		}
		return points;
	}
}

#endif // GEOM_UTILS_H