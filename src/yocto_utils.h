#ifndef YOCTO_UTILS_H
#define YOCTO_UTILS_H

#include "yocto\yocto_gl.h"

namespace rekt {

	ygl::material* make_material(const std::string& name, const ygl::vec3f& kd,
		ygl::texture* kd_txt = nullptr, const ygl::vec3f& ks = { 0.2f, 0.2f, 0.2f },
		float rs = 0.01f) {
		ygl::material* m = new ygl::material();
		m->name = name;
		m->kd = kd;
		m->kd_txt.txt = kd_txt;
		m->ks = ks;
		m->rs = rs;
		return m;
	}

	void add_light(ygl::scene* scn, const ygl::vec3f& pos, const ygl::vec3f& ke, const std::string& name) {
		ygl::shape* lshp = new ygl::shape{ name + "_shape" };
		lshp->pos.push_back(pos);
		lshp->points.push_back(0);
		lshp->color = { {1,1,1,1} };
		ygl::light* light = new ygl::light();
		ygl::instance* llinst = new ygl::instance();
		llinst->frame = ygl::identity_frame3f;
		llinst->name = name + "_instance";
		llinst->shp = lshp;
		ygl::environment* lenv = new ygl::environment();
		lenv->name = name + "_environment";
		lenv->frame = ygl::identity_frame3f;
		lenv->ke = { 1.f,1.f,1.f };
		auto lmat = new ygl::material();
		lmat->name = name+"_material";
		lmat->ke = ke;
		light->env = lenv;
		light->ist = llinst;
		light->ist->shp->mat = lmat;
		scn->materials.push_back(lmat);
		scn->shapes.push_back(lshp);
		scn->instances.push_back(llinst);
		scn->environments.push_back(lenv);
		scn->lights.push_back(light);
	}

	void set_shape_color(ygl::shape* shp, const ygl::vec4f& color) {
		shp->color = std::vector<ygl::vec4f>(shp->pos.size(), color);
	}

	void set_shape_color(ygl::shape* shp, const ygl::vec3f& color) {
		set_shape_color(shp, { color.x, color.y, color.z, 1.f });
	}

}

#endif // YOCTO_UTILS_H
