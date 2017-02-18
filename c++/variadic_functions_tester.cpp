#include <cassert>
#include <iostream>
#include <vector>
#include <boost/any.hpp>

// #define USE_ARGS_LIST_DUMMY_START
//
// an arg-list always needs to be bootstrapped
// arguments ultimately end up on the heap (in
// memory managed by a vector), this isn't too
// ideal
//
// note the double parentheses
#ifdef USE_ARGS_LIST_DUMMY_START
#define MAKE_ARGS_LIST(...) ((t_args_list_dummy_start(), __VA_ARGS__))
#else
#define MAKE_ARGS_LIST(...) ((t_args_list_start(), __VA_ARGS__))
#endif

class t_args_list {
public:
	size_t size() const { return m_args.size(); }

	const std::vector<boost::any>& args() const { return m_args; }
	const boost::any& operator [] (size_t i) const { return m_args[i]; }

	template<class t_arg_type> t_args_list& append_arg(const t_arg_type& t) {
		m_args.push_back(boost::any(t));
		return *this;
	}

private:
	std::vector<boost::any> m_args;
};

template<class t_arg_type>
t_args_list& operator , (t_args_list& list, const t_arg_type& arg) {
    return list.append_arg(arg);
}



#ifdef USE_ARGS_LIST_DUMMY_START
struct t_args_list_dummy_start {
};
#endif



struct t_args_list_start {
public:
	operator const t_args_list& () const { return m_list; }
	operator       t_args_list& ()       { return m_list; }

	// NOTE the return-types here!
	const t_args_list& list() const { return (*this); }
	      t_args_list& list()       { return (*this); }

private:
	t_args_list m_list;
};

#ifdef USE_ARGS_LIST_DUMMY_START
template<class t_arg_type>
t_args_list_start operator , (const t_args_list_dummy_start&, const t_arg_type& arg) {
	t_args_list_start args_list_start; (args_list_start.list()).append_arg(arg); return args_list_start;
}
#else
template<class t_arg_type>
// t_args_list_start operator , (t_args_list_start args_list_start, const t_arg_type& arg) {
t_args_list_start& operator , (t_args_list_start& args_list_start, const t_arg_type& arg) {
	(args_list_start.list()).append_arg(arg); return args_list_start;
}
#endif



void vararg_func(const t_args_list& args) {
	assert(args.size() == 3);
	assert(args[0].type() == typeid(  int));
	assert(args[1].type() == typeid(float));
	assert(args[2].type() == typeid( char));

	std::cout << "[" << __FUNCTION__ << "] args[0]=" << boost::any_cast<  int>(args[0]) << std::endl;
	std::cout << "[" << __FUNCTION__ << "] args[1]=" << boost::any_cast<float>(args[1]) << std::endl;
	std::cout << "[" << __FUNCTION__ << "] args[2]=" << boost::any_cast< char>(args[2]) << std::endl;
}

int main(void) {
	const t_args_list args = MAKE_ARGS_LIST(1, 3.14159f, 'x');
	const std::function<void(const t_args_list&)> func = vararg_func;

	vararg_func(MAKE_ARGS_LIST(1, 3.14159f, 'x'));
	func(args);
	return 0;
}

