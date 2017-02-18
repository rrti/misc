// Andrew Kensler's submission for the business-card RT challenge
// (extended with a kd-tree structure and multithreaded rendering)
#include <cassert>
#include <cstdio>
#include <cstring>

#include <algorithm>
#include <thread>

#define ENABLE_KDT 1
#define ENABLE_SMP 1
#define ENABLE_DOF 0

#define MAX_NUM_OBJECTS 1024
#define MAX_OBJECT_COLS 32
#define MAX_OBJECT_ROWS 16



struct t_params {
	unsigned int img_size_x;
	unsigned int img_size_y;
	unsigned int random_seed;
	unsigned int num_threads;
	unsigned int num_rays_pp;
	unsigned int max_bounces;

	float ray_weight;
	float dof_weight;
};

template<typename t> struct t_type3 {
	t_type3<t>(t a = 0, t b = 0, t c = 0) { x = a; y = b; z = c; }

	t_type3<t> operator + (const t_type3<t>& r) const { return (t_type3<t>(x + r.x, y + r.y, z + r.z)); }
	t_type3<t> operator - (const t_type3<t>& r) const { return (t_type3<t>(x - r.x, y - r.y, z - r.z)); }
	t_type3<t> operator ^ (const t_type3<t>& r) const { return (t_type3<t>(y*r.z - z*r.y, z*r.x - x*r.z, x*r.y - y*r.x)); }
	t_type3<t> operator * (t r) const { return (t_type3<t>(x * r, y * r, z * r)); }
	t_type3<t> operator / (t r) const { return ((*this) * (1 / r)); }
	t_type3<t> operator ! () const { return ((*this) * (1 / l())); }

	t_type3<t> neg() const { return (t_type3<t>(         -x,           -y,           -z )); }
	t_type3<t> abs() const { return (t_type3<t>(std::fabs(x), std::fabs(y), std::fabs(z))); }

	t  operator [] (unsigned int i) const { return *(&x + i); }
	t& operator [] (unsigned int i)       { return *(&x + i); }

	t operator % (const t_type3<t>& r) const { return (x*r.x + y*r.y + z*r.z); }

	t sql() const { return ((*this) % (*this)); }
	t l() const { return (std::sqrt(sql())); }

	bool operator < (const t_type3<t>& v) const { return (x < v.x && y < v.y && z < v.z); }

	t x;
	t y;
	t z;
};

typedef t_type3<float> t_type3f;

struct t_ray {
	t_type3f pos;
	t_type3f end;
	t_type3f dir;
};

struct t_hit {
	// intersection mode (HIT_*)
	int m;
	// intersection time
	float t;
	// intersection point
	t_type3f p;
	// intersection normal
	t_type3f n;
};

struct t_object {
	// position
	t_type3f p;
	// radius
	float r;
};

struct t_image {
	unsigned int xsize;
	unsigned int ysize;

	t_type3f* pdata;
};

struct t_block {
	unsigned int xmin;
	unsigned int ymin;
	unsigned int xmax;
	unsigned int ymax;
};

struct t_camera {
	t_type3f  pos;
	t_type3f xdir; // rgt
	t_type3f ydir; // fwd
	t_type3f zdir; // up (!)
	t_type3f  ipo; // image-plane origin (bottom-left corner)

	t_image image;
};

struct t_bbox {
	t_type3f mins;
	t_type3f maxs;
};

struct t_kdtree {
	t_bbox bbox; // node volume; axis-aligned

	t_kdtree* nodes[2]; // child nodes (std::array<*>)
	t_object** objps; // leaf objects (std::vector<*>)
};

struct t_scene {
	t_object* objects;
	t_kdtree* kdtree;
	t_camera camera;
	t_params params;

	t_type3f wlp; // light-position (x,z,y)
	t_type3f wuv; // world-up axis (x,z,y); y is forward
};

enum {
	HIT_NONE   = 0,
	HIT_GROUND = 1,
	HIT_OBJECT = 2,
};
enum {
	AXIS_IDX_X = 0,
	AXIS_IDX_Y = 2,
	AXIS_IDX_Z = 1,
	AXIS_COUNT = 3,
};



static const unsigned int OBJ_MASKS[] = {
	0000000*1, // {0,0,0,0,0,0,  0,0,0,0,0,0,  0,0,0,0,0,0,  0,0,0,0,0}
	7989198*1, // {1,1,1,1,0,0,  1,1,1,1,0,0,  1,1,1,1,1,0,  0,1,1,1,0},
	4526340*1, // {1,0,0,0,1,0,  1,0,0,0,1,0,  0,0,1,0,0,0,  0,0,1,0,0},
	4526340*1, // {1,0,0,0,1,0,  1,0,0,0,1,0,  0,0,1,0,0,0,  0,0,1,0,0},
	7987460*1, // {1,1,1,1,0,0,  1,1,1,1,0,0,  0,0,1,0,0,0,  0,0,1,0,0},
	4526340*1, // {1,0,0,0,1,0,  1,0,0,0,1,0,  0,0,1,0,0,0,  0,0,1,0,0},
	4526340*1, // {1,0,0,0,1,0,  1,0,0,0,1,0,  0,0,1,0,0,0,  0,0,1,0,0},
	4526350*1, // {1,0,0,0,1,0,  1,0,0,0,1,0,  0,0,1,0,0,0,  0,1,1,1,0},
	0000000*0, // {0,0,0,0,0,0,  0,0,0,0,0,0,  0,0,0,0,0,0,  0,0,0,0,0},
	8388607*1,
};

static const t_type3f DEF_COLORS[] = {
	{ 3.0f,  1.0f,  1.0f}, // red tile color
	{ 3.0f,  3.0f,  3.0f}, // white tile color
	{13.0f, 13.0f, 13.0f}, // base pixel color
	{ 0.7f,  0.6f,  1.0f}, // base sky color
};



static unsigned int log2(unsigned int n) {
	unsigned int r = 0;
	while (n > 0) {
		n >>= 1;
		r += 1;
	}
	return r;
}

static unsigned int get_num_obj_rows() { return (sizeof(OBJ_MASKS) / sizeof(OBJ_MASKS[0])); }
static unsigned int get_num_obj_cols() {
	unsigned int r = 0;
	for (unsigned int i = 0; i < get_num_obj_rows(); i++) {
		r = std::max(r, log2(OBJ_MASKS[i]));
	}
	return r;
}

// note: not thread-safe
static float urnd(float a = 0.0f, float b = 1.0f) {
	return (a + ((random() * 1.0f) / RAND_MAX) * (b - a));
}



static void init_camera(t_scene& scene) {
	t_camera& cam = scene.camera;
	t_image& img = cam.image;

	img.xsize = scene.params.img_size_x;
	img.ysize = scene.params.img_size_y;
	img.pdata = new t_type3f[img.xsize * img.ysize];

	// pre-multiply the image aspect-scaling factors so it
	// can be skipped in trace_ray; both must be 1.0/xsize
	// (to handle non-uniform ratios correctly)
	cam.pos  =   t_type3f(19.0f,  19.0f,  8.0f);
	cam.ydir = !(t_type3f(-6.0f, -16.0f,  0.0f));
	cam.xdir = !(scene.wuv ^ cam.ydir) / img.xsize;
	cam.zdir = !( cam.ydir ^ cam.xdir) / img.xsize;
	cam.ipo  = cam.ydir - (cam.xdir * img.xsize * 0.5f + cam.zdir * img.ysize * 0.5f);
}

static unsigned int init_objects(t_scene& scene) {
	t_object* objs = scene.objects;
	t_kdtree* kdt = scene.kdtree;

	t_bbox& bbox = kdt->bbox;

	bbox.mins = t_type3f( 1e6f,  1e6f,  1e6f);
	bbox.maxs = t_type3f(-1e6f, -1e6f, -1e6f);

	const unsigned int num_object_rows = get_num_obj_rows();
	const unsigned int num_object_cols = get_num_obj_cols();

	unsigned int num_objs = 0;
	unsigned int obj_mask = 0;

	assert(num_object_rows <= MAX_OBJECT_ROWS);
	assert(num_object_cols <= MAX_OBJECT_COLS);
	assert((MAX_OBJECT_ROWS * MAX_OBJECT_COLS) < MAX_NUM_OBJECTS);

	for (unsigned int j = 0; j < num_object_rows; j++) {
		#if 0
		obj_mask = 0;

		for (unsigned int k = 0; k < num_object_cols; k++) {
			const unsigned int shl = (num_object_cols - k) - 1;
			const unsigned int bit = OBJ_MASKS[j][k];

			obj_mask += (bit << shl);
		}
		#else
		obj_mask = OBJ_MASKS[j];
		#endif

		for (unsigned int k = 0; k < num_object_cols; k++) {
			const unsigned int shl = (num_object_cols - k) - 1;
			const unsigned int bit = (1 << shl);

			if ((obj_mask & bit) == 0)
				continue;

			t_object& obj = objs[num_objs++];

			obj.p = t_type3f(-2.5f + num_object_cols - k, 0.0f, num_object_rows - j);
			obj.r = 1.0f;

			bbox.mins.x = std::min(bbox.mins.x, obj.p.x - obj.r);
			bbox.mins.y = std::min(bbox.mins.y, obj.p.y - obj.r);
			bbox.mins.z = std::min(bbox.mins.z, obj.p.z - obj.r);
			bbox.maxs.x = std::max(bbox.maxs.x, obj.p.x + obj.r);
			bbox.maxs.y = std::max(bbox.maxs.y, obj.p.y + obj.r);
			bbox.maxs.z = std::max(bbox.maxs.z, obj.p.z + obj.r);
		}
	}

	// sentinel
	objs[num_objs].r = -1.0f;
	return num_objs;
}

static void init_kdtree(
	t_object*   scn_objs,
	t_object**& tmp_objs,
	t_kdtree*   scn_kdt,
	t_kdtree*&  tmp_kdt,
	unsigned int node_axis = 0,
	unsigned int cur_depth = 0,
	unsigned int max_depth = 5
) {
	// never split along z
	node_axis += (node_axis == AXIS_IDX_Z);
	node_axis %= AXIS_COUNT;

	const t_type3f& mins = scn_kdt->bbox.mins;
	const t_type3f& maxs = scn_kdt->bbox.maxs;

	{
		unsigned int num_objs = 0;

		// gather all (pointers to) objects within this node
		for (unsigned int n = 0; (scn_objs[n].r != -1.0f); n++) {
			const t_object& obj = scn_objs[n];
			const t_type3f& pos = obj.p;

			// node volume boundaries can clip objects, account for radius
			if (pos.x < (mins.x - obj.r) || pos.x > (maxs.x + obj.r)) continue;
			if (pos.y < (mins.y - obj.r) || pos.y > (maxs.y + obj.r)) continue;
			if (pos.z < (mins.z - obj.r) || pos.z > (maxs.z + obj.r)) continue;

			tmp_objs[num_objs++] = &scn_objs[n];
		}

		if (num_objs <= 4 || cur_depth == max_depth /*|| (maxs[node_axis] - mins[node_axis]) < 1.0f*/) {
			// leaf, add objects to node
			scn_kdt->nodes[0] = nullptr;
			scn_kdt->nodes[1] = nullptr;

			// make node point to proper chunk of buffer, set sentinel
			scn_kdt->objps = tmp_objs;
			scn_kdt->objps[num_objs] = nullptr;

			tmp_objs += num_objs;
			tmp_objs += 1;
			return;
		}
	}

	// internal node, split BV along axis
	scn_kdt->nodes[0] = tmp_kdt++;
	scn_kdt->nodes[1] = tmp_kdt++;
	scn_kdt->objps    = nullptr;

	// volumes are boxes, objects are spheres
	scn_kdt->nodes[0]->bbox.mins = mins;
	scn_kdt->nodes[0]->bbox.maxs = maxs;
	scn_kdt->nodes[1]->bbox.mins = mins;
	scn_kdt->nodes[1]->bbox.maxs = maxs;

	scn_kdt->nodes[0]->bbox.maxs[node_axis] = (mins[node_axis] + maxs[node_axis]) * 0.5f;
	scn_kdt->nodes[1]->bbox.mins[node_axis] = (mins[node_axis] + maxs[node_axis]) * 0.5f;

	init_kdtree(scn_objs, tmp_objs, scn_kdt->nodes[0], tmp_kdt, (node_axis + 1) % AXIS_COUNT, cur_depth + 1, max_depth);
	init_kdtree(scn_objs, tmp_objs, scn_kdt->nodes[1], tmp_kdt, (node_axis + 1) % AXIS_COUNT, cur_depth + 1, max_depth);
}

static void create_scene(t_scene& scene) {
	scene.wlp = t_type3f(9.0f,  9.0f, 16.0f);
	scene.wuv = t_type3f(0.0f,  0.0f,  1.0f);

	// node and object allocation pools
	static t_kdtree kdts[512];
	static t_object objs[MAX_NUM_OBJECTS];

	scene.objects = &objs[0];
	scene.kdtree = &kdts[0];

	// temporary object-pointer storage
	// note: tmp_objs might be too small since multiple nodes
	// can contain (a pointer to) the same object, must count
	// sentinels as well
	t_object* tmp_objs[MAX_NUM_OBJECTS * 2];
	t_object** tmp_obj = &tmp_objs[0];
	t_kdtree* tmp_kdt = &kdts[1];

	init_objects(scene);
	init_kdtree(scene.objects, tmp_obj, scene.kdtree, tmp_kdt);
	init_camera(scene);

	assert(size_t(tmp_kdt - &kdts[0]) < (sizeof(kdts) / sizeof(kdts[0])));
	srandom(scene.params.random_seed);
}



static bool intersect_sphere(const t_object& obj, const t_ray& ray, t_hit& ray_hit) {
	const t_type3f obj_pos = obj.p;
	const t_type3f dif_vec = ray.pos - obj_pos;

	const float b = dif_vec % ray.dir;
	const float c = dif_vec % dif_vec - (obj.r * obj.r);
	const float q = b * b - c;

	if (q <= 0.0f)
		return false;

	const float t = -b - std::sqrt(q);

	if (t <= 0.01f || t >= ray_hit.t)
		return false;

	ray_hit.t = t;
	ray_hit.p = ray.pos + ray.dir * t;
	ray_hit.n = !(dif_vec + ray.dir * t);
	ray_hit.m = HIT_OBJECT;
	return true;
}

// only called by intersect_kdtree, so no use for a t_hit&
static bool intersect_box(const t_bbox& box, const t_ray& ray) {
	const t_type3f box_hlf_dim = (box.maxs - box.mins) * 0.5f;
	const t_type3f box_mid_pos = (box.mins + box.maxs) * 0.5f;

	// box is in world-space, shift ws-ray by inverse box xform
	const t_type3f ray_pos_os = ray.pos - box_mid_pos;
	const t_type3f end_pos_os = ray.end - box_mid_pos;

	// terminate early in the special case
	// that ray-segment originated within v
	if (ray_pos_os.abs() < box_hlf_dim)
		return true;

	float tn = -1e6f;
	float tf =  1e6f;
	float t0 =  0.0f;
	float t1 =  0.0f;

	// the algorithm needs both an end-position and the direction
	// however the latter would require normalization if only the
	// end-position were given, so it takes both as arguments via
	// t_ray
	//
	// const t_type3f ray_dir = !(end_pos_os - ray_pos_os);

	for (unsigned int n = 0; n < AXIS_COUNT; n++) {
		if (std::fabs(ray.dir[n]) < 0.0001f) {
			if (std::fabs(ray_pos_os[n]) > box_hlf_dim[n])
				return false;
		} else {
			if (ray.dir[n] > 0.0f) {
				t0 = (-box_hlf_dim[n] - ray_pos_os[n]) / ray.dir[n];
				t1 = ( box_hlf_dim[n] - ray_pos_os[n]) / ray.dir[n];
			} else {
				t1 = (-box_hlf_dim[n] - ray_pos_os[n]) / ray.dir[n];
				t0 = ( box_hlf_dim[n] - ray_pos_os[n]) / ray.dir[n];
			}

			if (t0 > t1)
				std::swap(t0, t1);

			tn = std::max(tn, t0);
			tf = std::min(tf, t1);

			if (tn >   tf) return false;
			if (tf < 0.0f) return false;
		}
	}

	// get the intersection points in volume-space
	const t_type3f int_pos_n = ray_pos_os + (ray.dir * tn);
	const t_type3f int_pos_f = ray_pos_os + (ray.dir * tf);

	// get the length of the ray segment in volume-space
	const float seg_len_sq = (end_pos_os - ray_pos_os).sql();
	// get the distances from the start of the ray
	// to the intersection points in volume-space
	const float dst_n_sq = (int_pos_n - ray_pos_os).sql();
	const float dst_f_sq = (int_pos_f - ray_pos_os).sql();

	// if one of the intersection points is closer to p0
	// than the end of the ray segment, the hit is valid
	const int b0 = (dst_n_sq <= seg_len_sq);
	const int b1 = (dst_f_sq <= seg_len_sq);

	return (b0 || b1);
}



static bool intersect_kdtree(const t_kdtree& kdt, const t_ray& ray, t_hit& ray_hit) {
	bool ret = false;

	if (!intersect_box(kdt.bbox, ray))
		return ret;

	if (kdt.nodes[0] == nullptr) {
		assert(kdt.objps != nullptr);

		// leaf node, last element of objps is NULL-sentinel
		// for each object (sphere) inside this bounding volume (node),
		// test for intersection with the ray and save its closest hit
		for (unsigned int i = 0; kdt.objps[i] != nullptr; i++) {
			ret |= intersect_sphere(*kdt.objps[i], ray, ray_hit);
		}

		return ret;
	}

	ret |= intersect_kdtree(*kdt.nodes[0], ray, ray_hit);
	ret |= intersect_kdtree(*kdt.nodes[1], ray, ray_hit);

	return ret;
}



static t_hit intersect_scene(const t_scene& scene, const t_ray& ray) {
	const float t_ground = -ray.pos.z / ray.dir.z;

	t_hit ray_hit;
	ray_hit.m = HIT_NONE;
	ray_hit.t = 1e6f;

	if (t_ground > 0.01f) {
		// ground intersection, not necessarily the closest
		ray_hit.t = t_ground;
		ray_hit.p = ray.pos + ray.dir * t_ground;
		ray_hit.n = scene.wuv;
		ray_hit.m = HIT_GROUND;
	}

	#if (ENABLE_KDT == 1)
	intersect_kdtree(*scene.kdtree, ray, ray_hit);
	#else
	for (unsigned int n = 0; (scene.objects[n].r != -1.0f); n++) {
		intersect_sphere(scene.objects[n], ray, ray_hit);
	}
	#endif

	return ray_hit;
}



static t_type3f shade_pixel(const t_scene& scene, const t_ray& ray, unsigned int num_bounces = 0) {
	if (num_bounces >= scene.params.max_bounces)
		return (t_type3f());

	const t_hit& ray_hit = intersect_scene(scene, ray);

	// sky (i.e. no) intersection
	if (ray_hit.m == HIT_NONE)
		return (DEF_COLORS[3] * std::pow(1.0f - ray.dir.z, 4.0f));

	const t_type3f& lgt_pos = scene.wlp;
	const t_type3f  hit_pos = ray.pos + ray.dir * ray_hit.t;
	const t_type3f  lgt_dir = !(t_type3f(lgt_pos.x + urnd(), lgt_pos.y + urnd(), lgt_pos.z) - hit_pos);
	const t_type3f  ref_dir = ray.dir + ray_hit.n * ((ray_hit.n % ray.dir) * -2.0f);

	const t_ray lgt_ray = {hit_pos, hit_pos + lgt_dir * 1e6f, lgt_dir}; // shadow-ray (light_pos - hit_pos)
	const t_ray ref_ray = {hit_pos, hit_pos + ref_dir * 1e6f, ref_dir}; // reflection-ray

	float n_dot_l = ray_hit.n % lgt_dir;

	if (n_dot_l < 0.0f || intersect_scene(scene, lgt_ray).m != HIT_NONE)
		n_dot_l = 0.0f;

	if (ray_hit.m == HIT_GROUND) {
		const int ihpx = std::ceil(hit_pos.x * 0.2f);
		const int ihpy = std::ceil(hit_pos.y * 0.2f);
		const int cidx = (ihpx + ihpy) & 1;

		// alternate between red or white tile color
		return (DEF_COLORS[cidx] * (n_dot_l * 0.2f + 0.1f) + shade_pixel(scene, ref_ray, num_bounces + 1) * 0.333f);
	}

	// specular light-source reflection
	const float base = (lgt_dir % ref_dir) * (n_dot_l > 0.0f);
	const float lobe = std::pow(base, 99.0f);

	// object intersection
	return (t_type3f(lobe, lobe, lobe) + shade_pixel(scene, ref_ray, num_bounces + 1) * 0.666f);
}

static t_type3f trace_rays(const t_scene& scene, const t_camera& cam, unsigned int x, unsigned int y) {
	t_type3f pxl_col = DEF_COLORS[2];

	#if (ENABLE_DOF == 0)
	const t_type3f raw_dir = cam.ipo + (cam.xdir * x) + (cam.zdir * y);
	const t_type3f pxl_dir = !raw_dir;
	const t_ray pxl_ray = {cam.pos, cam.pos + pxl_dir * 1e6f, pxl_dir};

	return (pxl_col + shade_pixel(scene, pxl_ray) * scene.params.ray_weight);

	#else

	for (unsigned int r = scene.params.num_rays_pp; (r--) != 0; ) {
		const t_type3f dof_xv = cam.xdir * (urnd() - 0.5f) * scene.params.dof_weight;
		const t_type3f dof_zv = cam.zdir * (urnd() - 0.5f) * scene.params.dof_weight;
		const t_type3f dof_xz = (dof_xv + dof_zv);

		// spawn an infinite-length ray
		const t_type3f raw_dir = cam.ipo + cam.xdir * (x + urnd()) + cam.zdir * (y + urnd());
		const t_type3f pxl_dir = !(raw_dir * 16.0f - dof_xz);
		const t_ray    pxl_ray = {cam.pos + dof_xz, cam.pos + dof_xz + pxl_dir * 1e6f, pxl_dir};

		pxl_col = pxl_col + shade_pixel(scene, pxl_ray) * scene.params.ray_weight;
	}
	#endif

	return pxl_col;
}

static void render_image_block(const t_scene& scene, const t_block& block) {
	t_camera& cam = const_cast<t_camera&>(scene.camera);
	t_image& img = cam.image;

	for (unsigned int y = block.ymax; (y--) != block.ymin; ) {
		for (unsigned int x = block.xmax; (x--) != block.xmin; ) {
			img.pdata[y * img.xsize + x] = trace_rays(scene, cam, x, y);
		}
	}
}

static void render_image_mt(const t_scene& scene, unsigned int num_threads_x, unsigned int num_threads_y) {
	const t_camera& cam = scene.camera;
	const t_image& img = cam.image;

	t_block block = {0, 0, img.xsize, img.ysize};

	#if (ENABLE_SMP == 1)
	if (num_threads_y == 0)
	#endif
	{
		render_image_block(scene, block);
		return;
	}

	assert((num_threads_x * num_threads_y) <= 256);
	assert((img.xsize % num_threads_x) == 0);
	assert((img.ysize % num_threads_y) == 0);

	std::thread threads[256];

	for (unsigned int y = 0; y < num_threads_y; y++) {
		for (unsigned int x = 0; x < num_threads_x; x++) {
			block.xmin = (x + 0) * (img.xsize / num_threads_x);
			block.ymin = (y + 0) * (img.ysize / num_threads_y);
			block.xmax = (x + 1) * (img.xsize / num_threads_x);
			block.ymax = (y + 1) * (img.ysize / num_threads_y);

			threads[y * num_threads_x + x] = std::move(std::thread(&render_image_block, std::cref(scene), block));
		}
	}

	for (unsigned int n = 0; n < (num_threads_x * num_threads_y); n++) {
		threads[n].join();
	}
}

static void render_image(const t_scene& scene) {
	switch (scene.params.num_threads) {
		case   1: { render_image_mt(scene,  1,  0); } break;
		case   2: { render_image_mt(scene,  1,  2); } break;
		case   4: { render_image_mt(scene,  2,  2); } break;
		case   8: { render_image_mt(scene,  2,  4); } break;
		case  16: { render_image_mt(scene,  4,  4); } break;
		case  32: { render_image_mt(scene,  4,  8); } break;
		case  64: { render_image_mt(scene,  8,  8); } break;
		case 128: { render_image_mt(scene,  8, 16); } break;
		case 256: { render_image_mt(scene, 16, 16); } break;
		default: { assert(false); } break;
	}
}

static void output_image(const t_scene& scene) {
	const t_camera& cam = scene.camera;
	const t_image& img = cam.image;

	printf("P6 %d %d 255 ", img.xsize, img.ysize);

	for (unsigned int y = img.ysize; (y--) != 0; ) {
		for (unsigned int x = img.xsize; (x--) != 0; ) {
			printf("%c%c%c", int(img.pdata[y * img.xsize + x].x), int(img.pdata[y * img.xsize + x].y), int(img.pdata[y * img.xsize + x].z));
		}
	}
}



void parse_params(int argc, char** argv, t_params& params) {
	argv += (argc > 0);
	argc -= (argc > 0);

	params.img_size_x  = 1600;
	params.img_size_y  = 1200;
	params.random_seed = 0;
	params.num_threads = 16;
	params.num_rays_pp = 64 * ENABLE_DOF;
	params.max_bounces = 3;

	for (int i = 0; i < (argc - 1); i++) {
		if (std::strcmp(argv[i], "--isx") == 0) { params.img_size_x  = std::atoi(argv[i + 1]); continue; }
		if (std::strcmp(argv[i], "--isy") == 0) { params.img_size_y  = std::atoi(argv[i + 1]); continue; }
		if (std::strcmp(argv[i], "--irs") == 0) { params.random_seed = std::atoi(argv[i + 1]); continue; }
		if (std::strcmp(argv[i], "--nts") == 0) { params.num_threads = std::atoi(argv[i + 1]); continue; }
		if (std::strcmp(argv[i], "--rpp") == 0) { params.num_rays_pp = std::atoi(argv[i + 1]); continue; }
		if (std::strcmp(argv[i], "--nrb") == 0) { params.max_bounces = std::atoi(argv[i + 1]); continue; }

	}

	params.ray_weight = 224.0f / std::max(params.num_rays_pp, 1u);
	params.dof_weight = 100.0f;
}

int main(int argc, char** argv) {
	t_scene scene;

	parse_params(argc, argv, scene.params);
	create_scene(scene);
	render_image(scene);
	output_image(scene);
	return 0;
}

