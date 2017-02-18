#include <cassert>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

// std::function introduces major I-cache bloat, but is OK for splitting
static const std::function<int(int)> is_space = [](int c) { return (std::isspace(c)); };
static const std::function<bool(int)> is_alpha = [](int c) { return (std::isalpha(c)); };
static const std::function<bool(int)> is_punct = [](int c) { return (std::ispunct(c)); };

static size_t split_string(
	const std::string& s,
	const std::function<int(int)>& f,
	std::vector<std::string>& v
) {
	// start and end markers
	size_t i = 0;
	size_t j = 0;

	for (size_t n = 0; n < s.size(); n++) {
		if (f(s[n])) {
			// unwanted char, get last-tracked substring (if any)
			if (j > i)
				v.emplace_back(s.substr(i, j - i));

			i = n + 1;
			j = n + 1;
			continue;
		}

		j++;
	}

	if (j > i)
		v.emplace_back(s.substr(i, j - i));

	return (v.size());
}



template<typename T> static T clamp(T v, T vmin, T vmax) {
	return (std::max(vmin, std::min(v, vmax)));
}

template<typename T, typename F> static size_t bfind(const std::vector<T>& vec, const T& val, const F& cmp) {
	size_t min_idx =          0; // incl.
	size_t max_idx = vec.size(); // excl.

	constexpr size_t nil_idx = -1lu;

	if (max_idx == 0)
		return nil_idx;

	while (max_idx > min_idx && max_idx != nil_idx) {
		const size_t cur_idx = (min_idx + max_idx) >> 1;

		assert(cur_idx <  vec.size());
		assert(max_idx <= vec.size());

		switch (cmp(vec[cur_idx], val)) {
			case  0: { min_idx = cur_idx    ; max_idx = min_idx; } break; // cur == val; exit loop
			case  1: { max_idx = cur_idx - 1;                    } break; // cur >  val; update max
			case -1: { min_idx = cur_idx + 1;                    } break; // cur <  val; update min

			default: {
				assert(false);
			} break;
		}
	}

	// final check; loop might not have exited via case 0
	if (cmp(vec[min_idx], val) != 0)
		return nil_idx;

	return min_idx;
}


struct trie_node_t {
public:
	trie_node_t(int8_t key_val = 0, int8_t is_word = 0) {
		m_key_val = key_val;
		m_is_word = is_word;
	}

	trie_node_t(const trie_node_t& n) { copy(n); }
	trie_node_t(trie_node_t&& n) { swap(std::move(n)); }

	trie_node_t& operator = (const trie_node_t& n) { return (copy(n)); }
	trie_node_t& operator = (trie_node_t&& n) { return (swap(std::move(n))); }


	trie_node_t& copy(const trie_node_t& n) {
		m_children = n.children();

		m_key_val = n.key_val();
		m_is_word = n.is_word();
		return *this;
	}
	trie_node_t& swap(trie_node_t&& n) {
		m_children.swap(n.children());

		m_key_val = n.key_val();
		m_is_word = n.is_word();
		return *this;
	}


	const std::vector<trie_node_t>& children() const { return m_children; }
	      std::vector<trie_node_t>& children()       { return m_children; }

	int8_t key_val() const { return m_key_val; }
	int8_t is_word() const { return m_is_word; }

	static int8_t cmp(const trie_node_t& a, const trie_node_t& b) {
		return (clamp(int8_t(a.key_val() - b.key_val()), int8_t(-1), int8_t(1)));
	}

	// upper-case vs. lower-case ordering is handled implicitly (65-91 < 97-123)
	bool operator < (const trie_node_t& n) const { return (m_key_val < n.key_val()); }
	bool operator > (const trie_node_t& n) const { return (m_key_val > n.key_val()); }


	const trie_node_t* find_child_node_const(int8_t key_val) const {
		// NOTE: always requires a temporary node
		const size_t key_idx = bfind(m_children, trie_node_t(key_val), &cmp);

		if (key_idx == -1lu)
			return nullptr;

		return &m_children[key_idx];
	}

	trie_node_t* find_child_node(int8_t key_val) {
		return (const_cast<trie_node_t*>(find_child_node_const(key_val)));
	}


	trie_node_t* add_child_node(int8_t key_val, int8_t is_word) {
		// caller should ensure no duplicates are added
		m_children.reserve(4);
		m_children.emplace_back(key_val, is_word);

		if (m_children.size() == 1)
			return &m_children[0];

		// put the newly added node in its proper place (O(N))
		// this assumes a *sparse* distribution of child keys
		size_t i = m_children.size() - 2;
		size_t j = m_children.size() - 1;

		while (j > 0 && m_children[i] > m_children[j]) {
			std::swap(m_children[i], m_children[j]);

			i--;
			j--;
		}

		assert(m_children[j].key_val() == key_val);
		return &m_children[j];
	}


	const trie_node_t* find_node(const std::string& word, size_t indx = 0) const {
		if (indx == word.size())
			return this;

		const trie_node_t* node = find_child_node_const(word[indx]);

		if (node == nullptr)
			return nullptr;

		return (node->find_node(word, indx + 1));
	}


	void insert_word(const std::string& word) {
		trie_node_t* cur_node = this;
		trie_node_t* nxt_node = nullptr;

		size_t k = word.size() - 1;

		for (size_t n = 0; n < word.size(); n++) {
			// update index of final non-punctuation character
			// (such that the node still gets tagged if a word
			// ends in non-alpha chars)
			k -= (!is_alpha(word[k]));

			// do not create nodes for punctuation, etc.
			if (!is_alpha(word[n]))
				continue;

			if ((nxt_node = cur_node->find_child_node(word[n])) == nullptr)
				nxt_node = cur_node->add_child_node(word[n], n == k);

			cur_node = nxt_node;
		}
	}

	// insert every word in <text_string>
	bool insert_all_words(const std::string& text_string, const std::function<int(int)>& split_func) {
		std::vector<std::string> words;

		if (split_string(text_string, split_func, words) == 0)
			return false;

		for (const std::string& word: words) {
			insert_word(word);
		}

		return true;
	}


	// find all complete continuations (in alphabetical order) of prefix <word>
	size_t find_all_words(const std::string& word, std::vector<std::string>& words) const {
		const trie_node_t* node = find_node(word);

		if (node == nullptr)
			return 0;

		return (node->find_all_words_ext(words));
	}

	// helper
	size_t find_all_words_ext(std::vector<std::string>& words, std::string word = "") const {
		if (m_is_word != 0)
			words.emplace_back(word);

		for (const trie_node_t& n: m_children) {
			n.find_all_words_ext(words, word + char(n.key_val()));
		}

		return (words.size());
	}


	// call after all words have been inserted
	void shrink_to_fit() {
		m_children.shrink_to_fit();

		for (trie_node_t& n: m_children) {
			n.shrink_to_fit();
		}
	}

	void debug_print(size_t depth = 0, std::string word = "") const {
		for (size_t n = 0; n < depth; n++)
			printf("\t");

		printf("%c\n", m_key_val);

		for (const trie_node_t& n: m_children) {
			n.debug_print(depth + 1, word + char(n.key_val()));
		}
	}

private:
	std::vector<trie_node_t> m_children;

	int8_t m_key_val;
	int8_t m_is_word;
};



int main(int argc, char** argv) {
	trie_node_t trie;
	trie.insert_all_words(((argc > 1)? argv[1]: ""), is_space);
	trie.shrink_to_fit();
	trie.debug_print();
	return 0;
}

