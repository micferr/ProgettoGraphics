#ifndef YOCTO_UTILS
#define YOCTO_UTILS

#include "yocto\yocto_gl.h"

namespace rekt {

	ygl::material* make_material(const std::string& name, const ygl::vec3f& kd,
		const std::string& kd_txt, const ygl::vec3f& ks = { 0.2f, 0.2f, 0.2f },
		float rs = 0.01f) {
		ygl::material* m = new ygl::material();
		m->name = name;
		m->kd = kd;
		m->kd_txt = ygl::texture_info{ nullptr };
		m->ks = ks;
		m->rs = rs;
		return m;
	}

}

#endif // YOCTO_UTILS