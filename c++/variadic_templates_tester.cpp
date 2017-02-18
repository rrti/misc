#include <cstddef>
#include <array>
#include <tuple>
#include <iostream>




template<class Type, class... Tail, class Elem = typename std::decay<Type>::type>
std::array<Elem, 1 + sizeof...(Tail)>
make_std_array(Type&& head, Tail&&... tail) {
	return {std::forward<Type>(head), std::forward<Tail>(tail)...};
}

// in code
// const auto std_array = make_array(1, 2, 3, 4, 5);




// std::array version
template<std::size_t size, typename type> struct array_vector {
public:
	typedef std::array<type, size> array_type;

	array_vector() {
		for (size_t n = 0; n < size; n++) {
			m_values[n] = type(0);
		}
	}

	// generic variable-size constructors
	array_vector(const type values[size]) {
		for (size_t n = 0; n < size; n++) {
			m_values[n] = values[n];
		}
	}
	array_vector(const array_type& values) {
		for (size_t n = 0; n < size; n++) {
			m_values[n] = values[n];
		}
	}

	void print() const {
		for (size_t n = 0; n < size; n++) {
			std::cout << "[array_vector::" << __FUNCTION__ << "] " << n << " " << m_values[n] << std::endl;
		}
	}
 
private:
	type m_values[size];
};


typedef array_vector<4, float> array_vector4f;

// specialization for float-arrays
template<> array_vector4f::array_vector(const array_vector4f::array_type& array) {
	for (size_t n = 0; n < 4; n++) {
		m_values[n] = array[n];
	}
}




// variadiac-template version
template<std::size_t size, typename type,  typename... ctor_arg_types> struct templ_vector {
public:
	templ_vector(ctor_arg_types... args);
	void print() const {
		for (size_t n = 0; n < size; n++) {
			std::cout << "[templ_vector::" << __FUNCTION__ << "] " << n << " " << m_values[n] << std::endl;
		}
	}

private:
	type m_values[size];
};


// size, type, ...args...
typedef templ_vector<4, float,  float, float, float, float> templ_vector4f;

// specialize the ctor ...args...
template<> templ_vector4f::templ_vector(float x, float y, float z, float w) {
	m_values[0] = x; m_values[1] = y;
	m_values[2] = z; m_values[3] = w;
}




template <class THead, class ... TTail> struct templ_struct {
public:
	templ_struct() {}
	#if 1
	templ_struct(THead arg, TTail... args);
	#else
	templ_struct(THead arg, TTail... args) {
		std::cout << arg << std::endl;
		std::cout << sizeof...(args) << std::endl;
	}
	#endif

	// tuple head|tail signature must match our ctor
	void print(const std::tuple<THead, const TTail...>& tup) const {
		std::cout << "[templ_struct::" << __FUNCTION__ << "]" << std::get<0>(tup) << std::endl;
		std::cout << "[templ_struct::" << __FUNCTION__ << "]" << std::get<1>(tup) << std::endl;
		std::cout << "[templ_struct::" << __FUNCTION__ << "]" << std::get<2>(tup) << std::endl;
	}
};

// ctor specialization
template<> templ_struct<int, char, bool>::templ_struct(int arg0, char arg1, bool arg2) {
	std::cout << "<templ_struct::" << __FUNCTION__ << "]" << arg0 << " " << arg1 << " " << arg2 << std::endl;
}




int main() {
	templ_vector4f tv(  1.0f, 2.0f, 3.0f, 4.0f  );
	array_vector4f av({{1.0f, 2.0f, 3.0f, 4.0f}});

	templ_struct<int,  char, bool> ts(3, 'a', true);
	std::tuple<int,  char, bool> at(3, 'a', true);

	ts.print(at);
	tv.print();
	av.print();
	return 0;
}

