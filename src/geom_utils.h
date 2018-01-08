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
	 * points[0] must not be repeated at the end of the input.
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
	 * value of 0 for the y component of the points.
	 * Face normal is (0,1,0)
	 */
	std::vector<ygl::vec3f> make_regular_polygonXZ(
		int num_sides,
		float radius = 1.f,
		bool xz_aligned = false
	) {
		auto poly = to_3d(make_regular_polygon(num_sides, radius, xz_aligned));
		std::reverse(poly.begin(), poly.end());
		return poly;
	}

	std::vector<ygl::vec2f> make_quad(float side_length = 1.f) {
		float s = side_length/2.f;
		return { {s,s},{-s,s},{-s,-s},{s,-s} };
	}

	std::vector<ygl::vec3f> make_quadXZ(float side_length = 1.f) {
		auto quad = to_3d(make_quad(side_length));
		std::reverse(quad.begin(), quad.end());
		return quad;
	}

	/**
	 * Makes a thick solid from a polygon.
	 *
	 * Thickness is added along the vertexes' normals; if norm is empty,
	 * it is assumed that all vertexes lie on the same plane and the face 
	 * normal is used instead.
	 *
	 * It is not assumed that all points in the vector lie on the perimeter of 
	 * the polygon (i.e. a quad represented as pos = {(-1,-1), (1,-1), (1,1), (-1,1), (0,0)},
	 * triangles = {(0,1,4),(1,2,4),(2,3,4),(3,0,4)} is fine).
	 * Note that two internal faces, hidden inside the polygon, will be created for each
	 * internal side
	 */
	ygl::shape* thicken_polygon(
		const std::vector<ygl::vec3i>& triangles,
		const std::vector<ygl::vec4i>& quads,
		const std::vector<ygl::vec3f>& pos,
		const std::vector<ygl::vec3f>& norm,
		float thickness
	) {
		auto shape = new ygl::shape();
		shape->pos = pos;
		if (norm.size() != 0) {
			for (int i = 0; i < pos.size(); i++) {
				shape->pos.push_back(pos[i] + norm[i] * thickness);
			}
		}
		else {
			ygl::vec3f p1, p2, p3;
			if (triangles.size() != 0) {
				p1 = pos[triangles[0].x],
				p2 = pos[triangles[0].y],
				p3 = pos[triangles[0].z];
			}
			else { // quads.size() != 0
				p1 = pos[quads[0].x],
				p2 = pos[quads[0].y],
				p3 = pos[quads[0].z];
			}
			auto face_norm = ygl::normalize(ygl::cross(p2 - p1, p3 - p2));
			for (auto p : pos) {
				shape->pos.push_back(p + face_norm*thickness);
			}
		}
		shape->triangles.reserve(triangles.size() * 2); // Each triangle will be duplicated
		shape->quads.reserve(
			quads.size() * 2 +
			triangles.size() * 3 + 
			quads.size() * 4
		);
		int ps = pos.size();
		ygl::vec3i psize3 = { ps,ps,ps };
		ygl::vec4i psize4 = { ps,ps,ps,ps };
		for (const auto& t : triangles) {
			shape->triangles.push_back({ t.z, t.y, t.x }); // Original face, seen from the other side
			const auto nt = t + psize3;
			shape->triangles.push_back(nt); // New face
			// Sides
			shape->quads.push_back({ t.z,t.y,nt.y,nt.x });
			shape->quads.push_back({ t.y,t.x,nt.z,nt.y });
			shape->quads.push_back({ t.x,t.z,nt.x,nt.z });
		}
		for (const auto& q : quads) {
			shape->quads.push_back({ q.w,q.z,q.y,q.x });
			const auto nq = q + psize4;
			shape->quads.push_back(nq);
			// Sides
			shape->quads.push_back({ q.w,q.z,nq.y,nq.x });
			shape->quads.push_back({ q.z,q.y,nq.z,nq.y });
			shape->quads.push_back({ q.y,q.x,nq.w,nq.z });
			shape->quads.push_back({ q.x,q.w,nq.w,nq.x });
		}
		ygl::compute_normals({}, shape->triangles, shape->quads, shape->pos);
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