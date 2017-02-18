#ifndef SIMPLE_UNORDERED_BIMAP_HDR
#define SIMPLE_UNORDERED_BIMAP_HDR

#include <unordered_map>

template<typename t_key, typename t_val> class t_unordered_bimap {
public:
	typedef  std::unordered_map<t_key, t_val>  t_kv_map;
	typedef  std::unordered_map<t_val, t_key>  t_vk_map;

	t_unordered_bimap(const std::initializer_list<std::pair<const t_key, t_val>> pairs): kv_map(pairs) {
		vk_map.reserve(pairs.size());

		for (const auto& pair: pairs) {
			if (vk_map.find(pair.second) == vk_map.end()) {
				vk_map.insert({pair.second, pair.first});
			}
		}
	}

	const t_kv_map& get_kv_map() const { return kv_map; }
	const t_vk_map& get_vk_map() const { return vk_map; }

private:
	const t_kv_map kv_map;
	      t_vk_map vk_map;
};

#endif

