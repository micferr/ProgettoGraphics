#ifndef GEOM_UTILS_H
#define GEOM_UTILS_H

#include <vector>

#include "yocto\yocto_gl.h"

namespace rekt {
	const float pi = 3.14159265359f;

	ygl::vec2f centroid(std::vector<ygl::vec2f>& points) {
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
	 * Adds mid-points for each side of the input polygon.
	 *
	 * points[0] must non be repeated at the end of the input.
	 */
	std::vector<ygl::vec3f> tesselate_shape(
		const std::vector<ygl::vec3f>& points,
		int num_segments = 2
	) {
		std::vector<ygl::vec3f> res;
		for (int i = 0; i < points.size(); i++) {
			res.push_back(points[i]);
			for (int j = 0; j < num_segments - 1; j++) {
				res.push_back(points[i] * j / num_segments + points[(i+1)%points.size()] * (num_segments - j) / num_segments);
			}
		}
		return res;
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
	 * Transforms 2D points (x,z) to 3D points (x,y,z), with y in input
	 */
	std::vector<ygl::vec3f> to_3d(
		const std::vector<ygl::vec2f>& points, 
		float y = 0.f
	) {
		std::vector<ygl::vec3f> res;
		for (const auto& p : points) {
			res.push_back({ p.x, y, p.y });
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
	 * value of 0 for the y component of the points
	 */
	std::vector<ygl::vec3f> make_regular_polygonXZ(
		int num_sides,
		float radius = 1.f,
		bool xz_aligned = false
	) {
		return to_3d(make_regular_polygon(num_sides, radius, xz_aligned));
	}

	std::vector<ygl::vec2f> make_quad(float side_length = 1.f) {
		float s = side_length/2.f;
		return { {s,s},{-s,s},{-s,-s},{s,-s} };
	}

	std::vector<ygl::vec3f> make_quadXZ(float side_length = 1.f) {
		return to_3d(make_quad(side_length));
	}

	template<typename T>
	std::vector<T> displace(const std::vector<T>& points, T& disp) {
		std::vector<T> res;
		std::for_each(points.begin(), points.end(), [&res, &disp](const T& p){
			res.push_back(p+disp);
		});
		return res;
	}

	template<typename T>
	std::vector<T> scale(const std::vector<T>& points, float scale) {
		std::vector<T> res;
		std::for_each(points.begin(), points.end(), [&res, scale](const T& p) {
			res.push_back(p * scale);
		});
		return res;
	}

	/**
	 * Rotates all points by 'angle' radiants, counter-clockwise relative to (0,0)
	 *
	 * The function is quite inefficient, to rotate an in-world instance try changing
	 * its frame rather than using this.
	 */
	std::vector<ygl::vec2f> rotate(const std::vector<ygl::vec2f>& points, float angle) {
		std::vector<ygl::vec2f> res;
		std::for_each(points.begin(), points.end(), [&res, angle](const ygl::vec2f& p) {
			float new_angle = atan2f(p.y, p.x) + angle;
			float length = ygl::length(p);
			res.push_back({ length*cos(new_angle), length*sin(new_angle) });
		});
		return res;
	}

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
					((pi / 2) * (outside?-1.f:1.f));
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
					((pi / 2) * (outside ? -1.f : 1.f));
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