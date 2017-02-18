#include <limits>
#include <vector>

#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>

#ifdef USE_BOOST_HASH_MAP
#include <boost/unordered_map.hpp>
//
// "not possible to partially or explicitly specialize an alias template"
// (we would need to know the template argument types at the point of the
// alias declaration); note also that "there is no difference between a
// type alias declaration and typedef declaration"
//
// template<> using hash_map_t = (boost|std)::unordered_map<>;
//
// more subversive (global, not a type)
// #define hash_map_t (boost|std)::unordered_map
//
namespace lib {
	// using namespace boost;
	using boost::unordered_map;
};

#else

#include <unordered_map>
namespace lib {
	// using namespace std;
	using std::unordered_map;
};

#endif



template<typename type> inline type min3(type a, type b, type c) { return (std::min<type>(a, std::min<type>(b, c))); }
template<typename type> inline type clamp(type v, type vmin, type vmax) { return (std::max<type>(vmin, std::min<type>(vmax, v))); }

template<typename type, size_t size = 3> struct tuple_t {
public:
	tuple_t<type, size>(type _x = type(0), type _y = type(0), type _z = type(0)) {
		x() = _x;
		y() = _y;
		z() = _z;
	}

	tuple_t<type, size>(const tuple_t<type, size>& t) {
		x() = t.x();
		y() = t.y();
		z() = t.z();
	}

	// bitwise-{in}equality tests
	bool operator == (const tuple_t<type, size>& t) const { return (x() == t.x() || y() == t.y() || z() == t.z()); }
	bool operator != (const tuple_t<type, size>& t) const { return (x() != t.x() || y() != t.y() || z() != t.z()); }

	tuple_t<type, size> operator + (const tuple_t<type, size> t) const { return (tuple_t<type, size>(x() + t.x(), y() + t.y(), z() + t.z())); }
	tuple_t<type, size> operator - (const tuple_t<type, size> t) const { return (tuple_t<type, size>(x() - t.x(), y() - t.y(), z() - t.z())); }
	tuple_t<type, size> operator * (const tuple_t<type, size> t) const { return (tuple_t<type, size>(x() * t.x(), y() * t.y(), z() * t.z())); }

	tuple_t<type, size> operator * (type s) const { return (tuple_t<type, size>(x() * s, y() * s, z() * s)); }
	tuple_t<type, size> operator / (type s) const { return (tuple_t<type, size>(x() / s, y() / s, z() / s)); }

	type sq_len() const { return (x() * x() + y() * y() + z() * z()); }
	type    len() const { return (std::sqrt/*<type>*/(sq_len())); }

	type  operator [] (const size_t idx) const { assert(idx < size); return m_array[idx]; }
	type& operator [] (const size_t idx)       { assert(idx < size); return m_array[idx]; }

	type  x() const { return (*this)[0]; }
	type  y() const { return (*this)[1]; }
	type  z() const { return (*this)[2]; }
	type& x()       { return (*this)[0]; }
	type& y()       { return (*this)[1]; }
	type& z()       { return (*this)[2]; }

private:
	type m_array[size];
};

typedef tuple_t< int64_t, 3> coor3i_t;
typedef tuple_t<uint64_t, 3> coor3u_t;
typedef tuple_t<   float, 3> coor3f_t;

static const coor3i_t zero_coor3i = { 0,  0,  0};
static const coor3i_t ones_coor3i = { 1,  1,  1};
static const coor3i_t  err_coor3i = {-1, -1, -1};

static const coor3f_t zero_coor3f = { 0.0f,  0.0f,  0.0f};
static const coor3f_t ones_coor3f = { 1.0f,  1.0f,  1.0f};
static const coor3f_t  err_coor3f = {-1.0f, -1.0f, -1.0f};

static const coor3f_t  eps_coor3f = {
	std::numeric_limits<float>::min(),
	std::numeric_limits<float>::min(),
	std::numeric_limits<float>::min(),
};
static const coor3f_t  inf_coor3f = {
	std::numeric_limits<float>::infinity(),
	std::numeric_limits<float>::infinity(),
	std::numeric_limits<float>::infinity(),
};



struct ray_t {
public:
	ray_t() {}
	ray_t(const coor3f_t pos, const coor3f_t dir) {
		set_pos(pos);
		set_dir(dir);
	}

	void set_pos(const coor3f_t pos) { m_pos = pos; }
	void set_dir(const coor3f_t dir) { m_dir = dir; }

	const coor3f_t& get_pos() const { return m_pos; }
	const coor3f_t& get_dir() const { return m_dir; }

	coor3f_t& get_pos() { return m_pos; }
	coor3f_t& get_dir() { return m_dir; }

private:
	coor3f_t m_pos;
	coor3f_t m_dir;
};



struct obj_t {
public:
	// TODO
	bool trace_ray(const ray_t ray, coor3f_t& hit) const {
		(void) ray;
		(void) hit;
		return false;
	}

private:
	uint64_t m_uid;
};



// basic building block (note: this name causes an internal compiler
// error using g++ 4.8.4 if the -g flag is set; possibly similar to
// "struct node" in at least one gcc version)
struct small_block_t {
public:
	auto& get_objects() const { return m_objects; }
	auto& get_objects()       { return m_objects; }

	bool trace_ray(const ray_t ray, obj_t& obj) const {
		coor3f_t hit = err_coor3f;
		coor3f_t dst = err_coor3f; // .x := current, .y := previous

		for (const obj_t& object: m_objects) {
			if (!object.trace_ray(ray, hit))
				continue;

			dst.x() = (hit - ray.get_pos()).sq_len();

			if (dst.y() > -1.0f && dst.y() < dst.x())
				continue;

			// save distance of closest hit
			dst.y() = dst.x();
			// save reference to closest object
			obj = object;
		}

		return (dst.x() != -1.0f);
	}

private:
	std::vector<obj_t> m_objects;
};



static const coor3i_t SMALL_BLOCK_SIZE = coor3i_t( 1,  1,  1);
static const coor3i_t NUM_SMALL_BLOCKS = coor3i_t(16, 16, 16);
static const coor3i_t NUM_LARGE_BLOCKS = coor3i_t( 4,  4,  4);
static const coor3i_t GRID_SIZE_TABLE[3] = {
	SMALL_BLOCK_SIZE,
	SMALL_BLOCK_SIZE * NUM_SMALL_BLOCKS,
	SMALL_BLOCK_SIZE * NUM_SMALL_BLOCKS * NUM_LARGE_BLOCKS,
};



// layered hash-grid of block_t's
//
// sparse representation compared to standard uniform grid
// keys are globally unique and based on block coordinates
// (no collisions)
//
//   L2 block_grid=world_t   [                                         ]
//   L1 block_grid=large_t   [                 ] ... [                 ]
//   L0 block     =small_t   [     ] ... [     ] ... [     ] ... [     ]
//
// coor3i_t's represent grid-local indices
// coor3f_t's represent world-global coordinates
//
template<typename block_type, size_t grid_level> struct block_grid_t {
public:
	block_grid_t() {}
	block_grid_t(const coor3i_t coor, const coor3i_t size) {
		m_coor = coor;
		m_size = size;
	}

	coor3i_t get_size() const { return m_size; }
	coor3i_t get_coor() const { return m_coor; }


	// dimensions in world-units of blocks in this (caller's) grid
	coor3i_t get_block_dims_wu() const {
		assert(grid_level > 0);
		assert(grid_level < 3);
		return GRID_SIZE_TABLE[grid_level - 1];
	}
	// dimensions in world-units of this (caller's) grid
	coor3i_t get_local_dims_wu() const {
		assert(grid_level > 0);
		assert(grid_level < 3);
		return GRID_SIZE_TABLE[grid_level];
	}

	// origin coordinates (in world-units) of this grid
	coor3f_t get_base_coor_wu(const coor3i_t offset = zero_coor3i) const {
		const coor3i_t coor_i = (m_coor + offset) * get_local_dims_wu();
		const coor3f_t coor_f = coor3f_t(coor_i.x(), coor_i.y(), coor_i.z());
		return coor_f;
	}


	// world-to-block coordinate transform
	coor3i_t calc_block_coors(const coor3f_t world_coor) const {
		if (!inside_grid(world_coor))
			return err_coor3i;

		const coor3f_t base_coor = get_base_coor_wu();
		const coor3i_t blck_dims = get_block_dims_wu();

		coor3i_t blck_coor;

		// when grid_level=2, base=<0,0,0> and does not influence the result
		blck_coor.x() = (world_coor.x() - base_coor.x()) / blck_dims.x();
		blck_coor.y() = (world_coor.y() - base_coor.y()) / blck_dims.y();
		blck_coor.z() = (world_coor.z() - base_coor.z()) / blck_dims.z();

		return blck_coor;
	}

	// block-to-world coordinate transform
	coor3f_t calc_world_coors(const coor3i_t block_coor) const {
		if (!inside_grid(block_coor))
			return err_coor3f;

		// when grid_level=2, m_coor=<0,0,0> and does not influence the result
		const coor3i_t block_offset_coor = block_coor * get_block_dims_wu();
		const coor3i_t world_offset_coor =     m_coor * get_local_dims_wu();

		coor3f_t world_coor;

		world_coor.x() = block_offset_coor.x() + world_offset_coor.x();
		world_coor.y() = block_offset_coor.y() + world_offset_coor.y();
		world_coor.z() = block_offset_coor.z() + world_offset_coor.z();

		return world_coor;
	}


	coor3f_t calc_block_edge_dists(const ray_t ray, const coor3f_t eps) const {
		// ensured by caller
		// assert(inside_grid(ray.get_pos()));

		// first convert ray position to block-relative coors
		const coor3f_t world_pos = ray.get_pos();
		const coor3f_t world_dir = ray.get_dir();
		const coor3i_t block_dim = get_block_dims_wu();

		// find the global index and extrema of the block containing <ray>
		// this does *not* use calc_block_coors; we want absolute indices
		const coor3i_t blck_indx(world_pos.x() / block_dim.x(), world_pos.y() / block_dim.y(), world_pos.z() / block_dim.z());
		const coor3f_t blck_mins((blck_indx.x()    ) * block_dim.x(), (blck_indx.y()    ) * block_dim.y(), (blck_indx.z()    ) * block_dim.z());
		const coor3f_t blck_maxs((blck_indx.x() + 1) * block_dim.x(), (blck_indx.y() + 1) * block_dim.y(), (blck_indx.z() + 1) * block_dim.z());

		// find the ray's xyz-distance to each edge; min_dsts are negative
		const coor3f_t min_edge_dsts = blck_mins - world_pos;
		const coor3f_t max_edge_dsts = blck_maxs - world_pos;

		// scale the direction components s.t. most non-zero values become either -1 or 1;
		// convert dir-signs to indices (-1 to ref[0]=min, 1 to ref[2]=max, 0 to ref[1]=inf)
		//
		// if_world_dir[xyz] < 0 return min_edge_dsts[xyz]/world_dir[xyz]
		// if world_dir[xyz] > 0 return max_edge_dsts[xyz]/world_dir[xyz]
		// otherwise             return infinity
		//
		// const coor3i_t dir_signs(clamp(world_dir.x() * 1000000.0f, -1.0f, 1.0f), clamp(world_dir.y() * 1000000.0f, -1.0f, 1.0f), clamp(world_dir.z() * 1000000.0f, -1.0f, 1.0f));
		// const coor3i_t dir_index(dir_signs.x() + 1, dir_signs.y() + 1, dir_signs.z() + 1);

		coor3f_t dists;

		for (unsigned int n = 0; n < 3; n++) {
			switch (int(clamp(world_dir[n] * 1000000.0f, -1.0f, 1.0f))) {
				case -1: { dists[n] = (min_edge_dsts[n] / world_dir[n]) + eps[n]; } break;
				case +1: { dists[n] = (max_edge_dsts[n] / world_dir[n]) + eps[n]; } break;
				case  0: { dists[n] =                              inf_coor3f[n]; } break;
				default: {                                         assert(false); } break;
			}
		}

		return dists;
	}


	// linearly map coordinates of a block (x,y,z) to a unique index
	uint64_t calc_coors_hash(const coor3i_t block_coor) const {
		const uint64_t X = block_coor.x() * 1;
		const uint64_t Y = block_coor.y() * m_size.x();
		const uint64_t Z = block_coor.z() * m_size.y() * m_size.z();
		return (X + Y + Z);
	}


	// no return-type deduction in C++11 for normal functions
	// needs decltype(...) to explicitly specify it in C++11;
	// use "-std=c++1y" to surpress the warning
	const auto get_block_iter(const coor3f_t world_coor) const {
		return (m_blocks.find(calc_coors_hash(calc_block_coors(world_coor))));
	}

	const auto get_block_iter(const coor3i_t block_coor) const {
		return (m_blocks.find(calc_coors_hash(block_coor)));
	}


	const block_type* get_block(const coor3f_t world_coor) const {
		return (get_block(calc_block_coors(world_coor)));
	}

	const block_type* get_block(const coor3i_t block_coor) const {
		const auto iter = get_block_iter(block_coor);

		if (iter != m_blocks.end())
			return &(iter->second);

		return nullptr;
	}


	size_t get_blocks(const coor3i_t mins, const coor3i_t maxs, std::vector<block_type*>& blocks) const {
		assert(maxs.x() >= mins.x());
		assert(maxs.y() >= mins.y());
		assert(maxs.z() >= mins.z());
		blocks.reserve((maxs.x() - mins.x()) * (maxs.y() - mins.y()) * (maxs.z() - mins.z()));

		for (int64_t z = mins.z(); z <= maxs.z(); z++) {
			for (int64_t y = mins.y(); y <= maxs.y(); y++) {
				for (int64_t x = mins.x(); x <= maxs.x(); x++) {
					// note:
					//   mins or maxs can technically be out of bounds, but
					//   that will simply result in null-ptrs here since the
					//   table only explicitly stores blocks with coordinates
					//   inside the MAX_*_SIZE range --> no need to clamp
					const block_type* ngb_block = get_block(coor3i_t(x, y, z));

					if (ngb_block == nullptr)
						continue;

					blocks.push_back(const_cast<block_type*>(ngb_block));
				}
			}
		}

		return (blocks.size());
	}

	size_t get_block_ngbs(const coor3i_t block_coors, const coor3i_t block_range, std::vector<block_type*>& blocks) const {
		assert(block_range.x() >= 1);
		assert(block_range.y() >= 1);
		assert(block_range.z() >= 1);

		const coor3i_t mins = block_coors - block_range;
		const coor3i_t maxs = block_coors + block_range;

		return (get_blocks(mins, maxs, blocks));
	}


	bool has_neighbor(const coor3i_t block_coor, const coor3i_t block_delta) const {
		return ((get_block(block_coor) != nullptr) && (get_block(block_coor + block_delta) != nullptr));
	}


	bool insert_block(const coor3i_t block_coor, const block_type& block) {
		// ensure that inserted blocks reside within our bounds
		if (!inside_grid(block_coor))
			return false;

		// do not allow replacing blocks (better to update by reference)
		if (get_block_iter(block_coor) != m_blocks.end())
			return false;

		// uses pair's template constructor
		// m_blocks.emplace(calc_coors_hash(coor), block);
		// uses pair's (converting) move constructor, faster
		m_blocks.emplace(std::make_pair(calc_coors_hash(block_coor), block));
		return true;
	}

	bool remove_block(const coor3i_t block_coor) {
		const auto iter = get_block_iter(block_coor);

		if (iter == m_blocks.end())
			return false;

		m_blocks.erase(iter);
		return true;
	}


	bool inside_grid(const coor3i_t block_coor) const {
		// index-based test
		coor3i_t mask;

		mask.x() = (block_coor.x() < 0 || block_coor.x() >= m_size.x());
		mask.y() = (block_coor.y() < 0 || block_coor.y() >= m_size.y());
		mask.z() = (block_coor.z() < 0 || block_coor.z() >= m_size.z());

		return ((mask.x() == 0) && (mask.y() == 0) && (mask.z() == 0));
	}

	bool inside_grid(const coor3f_t world_coor) const {
		// coordinate-based test
		const coor3f_t mins = get_base_coor_wu(           );
		const coor3f_t maxs = get_base_coor_wu(ones_coor3i);

		// when grid_level=2, m_coor=<0,0,0> so the range reduces to [0*ws, 1*ws]
		coor3i_t mask;

		mask.x() = (world_coor.x() < mins.x() || world_coor.x() >= maxs.x());
		mask.y() = (world_coor.y() < mins.y() || world_coor.y() >= maxs.y());
		mask.z() = (world_coor.z() < mins.z() || world_coor.z() >= maxs.z());

		return ((mask.x() == 0) && (mask.y() == 0) && (mask.z() == 0));
	}


	// trace a ray through the grid until it hits a non-empty cell or exits
	// if cell is non-empty and non-leaf, recurse into it and see if we hit
	// a leaf there
	bool trace_ray(ray_t& ray, obj_t& obj) const {
		assert(ray.get_dir() != zero_coor3f);
		assert(grid_level > 0);

		const block_type* cur_block = nullptr;
		const block_type* prv_block = nullptr;

		// make sure we always advance to a new block
		while ((cur_block == nullptr) || (cur_block != prv_block)) {
			prv_block = cur_block;
			cur_block = get_block(ray.get_pos());

			// recursively descend into all sub-grid blocks
			if (cur_block != nullptr && cur_block->trace_ray(ray, obj))
				return true;
			// check if ray exited grid at a lower level
			if (!inside_grid(ray.get_pos()))
				break;

			const coor3f_t edge_dst = calc_block_edge_dists(ray, eps_coor3f);
			const coor3f_t edge_vec = ray.get_dir() * min3(edge_dst.x(), edge_dst.y(), edge_dst.z());

			// advance ray to edge of current block
			ray.set_pos(ray.get_pos() + edge_vec);
		}

		return false;
	}

private:
	lib::unordered_map<uint64_t, block_type> m_blocks;

	// location of grid within parent grid (if L1)
	coor3i_t m_coor;
	// maximum number of blocks within our bounds
	coor3i_t m_size;
};



// note: world_block_t is recursively defined (as a grid of grids)
typedef block_grid_t<small_block_t, 1> large_block_t;
typedef block_grid_t<large_block_t, 2> world_block_t;



int main() {
	small_block_t sb;
	large_block_t lb(coor3i_t(0, 0, 0), NUM_SMALL_BLOCKS);
	world_block_t wb(coor3i_t(0, 0, 0), NUM_LARGE_BLOCKS);

	const coor3i_t lbs = lb.get_size();
	const coor3i_t wbs = wb.get_size();

	// fill a single large_block_t with small_block_t's
	for (int64_t zs = 0; zs < lbs.z(); zs++) {
		for (int64_t ys = 0; ys < lbs.y(); ys++) {
			for (int64_t xs = 0; xs < lbs.x(); xs++) {
				lb.insert_block(coor3i_t(xs, ys, zs), sb);
			}
		}
	}

	#if 1
	// fill the world with large_block_t's
	for (int64_t zl = 0; zl < wbs.z(); zl++) {
		for (int64_t yl = 0; yl < wbs.y(); yl++) {
			for (int64_t xl = 0; xl < wbs.x(); xl++) {
				wb.insert_block(coor3i_t(xl, yl, zl), lb);
			}
		}
	}
	#endif

	wb.insert_block(lb.get_coor(), lb);
	wb.calc_world_coors(coor3i_t(1, 2, 3));
	wb.get_base_coor_wu();

	ray_t ray = {coor3f_t(0.25f, 0.0f, 0.0f), coor3f_t(1.0f, 0.5f, 0.0f)};
	obj_t obj;
	wb.trace_ray(ray, obj);
	return 0;
}

