#ifndef SIMPLE_VECTOR_QUEUE_HDR
#define SIMPLE_VECTOR_QUEUE_HDR

#include <cassert>
#include <vector>

template<typename type> struct t_vector_queue {
public:
	t_vector_queue() { clear(); }
	t_vector_queue(size_t n) {
		clear();
		reserve(n);
	}

	t_vector_queue(const t_vector_queue& q) { *this = q; }
	t_vector_queue(t_vector_queue&& q) { *this = std::move(q); }

	t_vector_queue& operator = (const t_vector_queue& q) {
		m_elems = q.elems();

		m_indcs.first  = q.head();
		m_indcs.second = q.tail();
		return *this;
	}
	t_vector_queue& operator = (t_vector_queue&& q) {
		m_elems = std::move(q.elems());

		m_indcs.first  = q.head();
		m_indcs.second = q.tail();

		q.clear();
		return *this;
	}

	// note: queue growth will normally be unbounded, be sure to call clear() when empty()
	type& push_back(const type& t) { m_elems.push_back(t); return m_elems[get_inc_tail()]; }
	type& pop_front() { assert(!empty()); return m_elems[get_inc_head()]; }

	const type& front() const { assert(!empty()); return m_elems[head()]; }
	const type&  back() const { assert(!empty()); return m_elems[tail()]; }
		  type& front()       { assert(!empty()); return m_elems[head()]; }
		  type&  back()       { assert(!empty()); return m_elems[tail()]; }

	bool empty() const { return (size() == 0); }
	bool valid() const { return (tail() >= head()); }

	size_t size() const { return (tail() - head()); }
	size_t head() const { return (m_indcs.first ); }
	size_t tail() const { return (m_indcs.second); }

	void reserve(size_t n) { m_elems.reserve(n); }
	void clear() {
		m_elems.clear();

		m_indcs.first  = 0;
		m_indcs.second = 0;

		assert(empty());
	}

private:
	const std::vector<type>& elems() const { return m_elems; }
	      std::vector<type>& elems()       { return m_elems; }

	size_t get_inc_head() { return (m_indcs.first ++); }
	size_t get_inc_tail() { return (m_indcs.second++); }

private:
	// .first=head, .second=tail
	std::pair<size_t, size_t> m_indcs;
	std::vector<type> m_elems;
};

#endif

