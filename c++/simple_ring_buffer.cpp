#include <cstddef>

template<typename t_cont_type> struct t_ring_buffer {
typedef typename  t_cont_type::const_iterator  m_const_iter_type;
typedef typename  t_cont_type::iterator  m_iter_type;
typedef typename  t_cont_type::value_type  m_elem_type;
public:
	t_ring_buffer(size_t max_size) {
		if (max_size == 0)
			max_size = 1;

		// pre-allocate
		for (size_t n = 0; n < max_size; n++) {
			m_cont.push_back(m_elem_type());
		}

		m_iter = m_cont.begin();
	}

	t_ring_buffer& operator = (const t_ring_buffer& buf) {
		if (buf.size() != size())
			resize(buf.size());

		// insert from the beginning of m_cont
		m_iter = m_cont.begin();

		for (auto it = buf.cbegin(); it != buf.cend(); ++it) {
			push_back(*it);
		}

		return *this;
	}

	bool empty() const { return (m_cont.empty()); }
	size_t size() const { return (m_cont.size()); }

	void push_back(const m_elem_type e) {
		*m_iter = e;
		++m_iter;

		// wrap-around
		if (m_iter == m_cont.end()) {
			m_iter = m_cont.begin();
		}
	}

	void resize(size_t new_size, m_elem_type def_value = m_elem_type()) {
		const size_t dif_size = new_size - m_cont.size();

		if (new_size >= size()) {
			for (size_t n = 0; n < dif_size; n++) {
				m_cont.push_back(def_value);
			}
		} else {
			for (size_t n = 0; n < -dif_size; n++) {
				m_cont.pop_back();
			}
		}

		// write-pos might have been invalidated
		m_iter = m_cont.begin();
	}

	m_const_iter_type cbegin() const { return (m_cont.cbegin()); }
	m_const_iter_type cend() const { return (m_cont.cend()); }

	m_iter_type begin() const { return (m_cont.begin()); }
	m_iter_type end() const { return (m_cont.end()); }
	m_iter_type pos() const { return (m_iter); }

private:
	t_cont_type m_cont;
	m_iter_type m_iter;
};

// typedef t_ring_buffer< std::vector<int> > int_vect_buffer;
// typedef t_ring_buffer< std::list<int> > int_list_buffer;

