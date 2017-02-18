#include <cstdio>

template<char c> struct TChar {
public:
    TChar(): m_char(c) {}
private:
    const char m_char;
};



template<int n> struct TSpaces {
private:
	TChar<' '> m_chars[n];
};
// specialization for compilers that do not like empty arrays
template<> struct TSpaces<0> {};



template<int n, char c> struct TDiamondLine: TSpaces<n> {
	TChar<c> c1;
	TSpaces<(c - 'A') * 2 - 1> c2;
	TChar<c> c3;
	TChar<'\n'> c4;
};
// base case handles leading spaces
template<int n> struct TDiamondLine<n, 'A'>: TSpaces<n>, TChar<'A'>, TChar<'\n'> {};



// recursive cases
template<int n, char c> struct TDiamondUpperHalf: TDiamondUpperHalf<n + 1, c - 1>, TDiamondLine<n, c> {};
template<int n, char c> struct TDiamondLowerHalf: TDiamondLine<n, c>, TDiamondLowerHalf<n + 1, c - 1> {};
// base cases
template<int n> struct TDiamondUpperHalf<n, 'A'>: TDiamondLine<n, 'A'> {};
template<int n> struct TDiamondLowerHalf<n, 'A'>: TDiamondLine<n, 'A'> {};



// recursive case
template<char c> struct Diamond: TDiamondUpperHalf<0, c>, TDiamondLowerHalf<1, c - 1>, TChar<0> {
    const char* s() const { return (reinterpret_cast<const char*>(this)); }
};
// base case
template<> struct Diamond<'A'>: TChar<'A'>, TChar<'\n'>, TChar<0> {
	const char* s() const { return (reinterpret_cast<const char*>(this)); }
};



int main() {
    Diamond<'Z'> p;

	printf("%s\n", p.s());
    return 0;
}

