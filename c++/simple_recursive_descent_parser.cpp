/*
 * PROG:
 *   END
 *   EXPR_LIST END
 * EXPR_LIST:
 *   EXPR PRINT
 *   EXPR PRINT EXPR_LIST
 * EXPR:
 *   EXPR + TERM
 *   EXPR - TERM
 *   TERM
 * TERM:
 *   TERM * PRIM
 *   TERM / PRIM
 *   PRIM
 * PRIM:
 *   VALUE [0-9]*(\.[0-9]*)?
 *   IDENT [a-zA-Z]+
 *   IDENT = EXPR
 *   + PRIM
 *   - PRIM
 *   ( EXPR )
 */

#include <cassert>
#include <cstdio>
// not needed with C++11
// #include <cstdint>

#include <exception>
#include <limits>
#include <iostream>
#include <sstream>
#include <string>

#include <map>

static const char*  input_prompt = "<<< ";
static const char* output_prompt = ">>> ";
 
enum {
	TOKEN_QUIT   =  0,
	TOKEN_NONE   =  1,

	TOKEN_IDENT  =  2, // string literal
	TOKEN_VALUE  =  3, // number literal

	TOKEN_SEPAR  =  4, // uint8_t(';'),
	TOKEN_PRINT  =  5, // uint8_t('\n'),

	TOKEN_SET    =  6, // uint8_t('='),
	TOKEN_MUL    =  7, // uint8_t('*'),
	TOKEN_DIV    =  8, // uint8_t('/'),
	TOKEN_PLUS   =  9, // uint8_t('+'),
	TOKEN_MINUS  = 10, // uint8_t('-'),

	TOKEN_LPAR   = 11, // uint8_t('('),
	TOKEN_RPAR   = 12, // uint8_t(')'),
};



struct t_syntax_exception: public std::exception {
public:
	t_syntax_exception(const char* e): m_str(e) {}
	~t_syntax_exception() throw () {}

	const char* what() const noexcept { return m_str; }

private:
	const char* m_str;
};



struct t_token {
public:
	explicit t_token(uint8_t type = TOKEN_NONE, std::istream* input = NULL): m_type(type) {
		if (input != NULL) {
			switch (type) {
				case TOKEN_IDENT: { extract_ident(input); } break;
				case TOKEN_VALUE: { extract_value(input); } break;
				default: {} break;
			}
		}
	}

	t_token& operator = (const t_token& t) {
		m_type  = t.get_type();
		m_ident = t.get_ident_str();
		m_value = t.get_value_str();
		return *this;
	}

	uint8_t get_type() const { return m_type; }

	const std::string& get_ident_str() const { return m_ident; }
	const std::string& get_value_str() const { return m_value; }

	template<typename value_type> value_type get_value() const {
		value_type v(0);
		std::istringstream s(m_value.c_str());

		if (s.good())
			s >> v;

		return v;
	}


	t_token& extract_value(std::istream* input) {
		// we scan numbers manually to avoid operator >> (std::string&)
		// tokenization issues, e.g. on input "x=1;y=2" the value would
		// become "1;y=2"
		//
		m_value.clear();
		m_value.reserve(8);
		m_value.push_back(input->get());

		char c = 0;

		for (int i = 0; i < 2; i++) {
			// extract and advance
			while (input->get(c) && std::isdigit(c))
				m_value.push_back(c);

			// no longer part of our number
			if (c != '.' || m_value[0] == '.')
				break;

			// grab the decimal-point
			m_value.push_back(c);
		}

		// put back the char that broke the loop
		input->putback(c);
		return *this;
	}

	t_token& extract_ident(std::istream* input) {
		// operator>> only stops at whitespace, so "a*b" would
		// be consumed entirely instead of tokenized as "a * b"
		//
		m_ident.clear();
		m_ident.reserve(8);
		m_ident.push_back(input->get());

		char c = 0;

		for (int i = 0; i < 1; i++) {
			// extract and advance
			while (input->get(c) && std::isalpha(c))
				m_ident.push_back(c);
		}

		// put back the char that broke the loop
		input->putback(c);
		return *this;
	}

private:
	uint8_t m_type;

	std::string m_ident;
	std::string m_value;
};



template<typename t_expr_type> struct t_parser {
public:
	t_parser(std::istream* input_stream, std::ostream* output_stream) {
		m_ident_table["pi"] = t_expr_type(3.141592653);
		m_ident_table[ "e"] = t_expr_type(2.718281828);

		// TODO: output formatting
		m_input_stream = input_stream;
		m_output_stream = output_stream;
	}

	~t_parser() {
		if (m_input_stream != &std::cin)
			delete m_input_stream;

		m_input_stream = NULL;
		m_output_stream = NULL;
	}

private:
	char read_next_char() {
		char c = 0;

		do {
			if (!m_input_stream->get(c) || c == 'Q') {
				return 0;
			}
		} while (c != '\n' && std::isspace(c));

		return c;
	}

	t_token scan_token() {
		// read the next character ignoring whitespace
		const char c = read_next_char();

		switch (c) {
			case '=': { return (t_token(TOKEN_SET  )); } break;
			case '*': { return (t_token(TOKEN_MUL  )); } break;
			case '/': { return (t_token(TOKEN_DIV  )); } break;
			case '+': { return (t_token(TOKEN_PLUS )); } break;
			case '-': { return (t_token(TOKEN_MINUS)); } break;
			case '(': { return (t_token(TOKEN_LPAR )); } break;
			case ')': { return (t_token(TOKEN_RPAR )); } break;

			case  ';': { return (t_token(TOKEN_SEPAR)); } break;
			case '\n': { return (t_token(TOKEN_PRINT)); } break;

			case 0: {
				return (t_token(TOKEN_QUIT));
			} break;

			#if 1
			case '.': {
				// needed for numbers with only fractional parts (".5")
				// hack-ish because this also allows writing "." for 0
				return (t_token(TOKEN_VALUE, &(m_input_stream->putback(c))));
			} break;
			#endif

			default: {
				// any other character
				if (std::isalpha(c))
					return (t_token(TOKEN_IDENT, &(m_input_stream->putback(c))));
				if (std::isdigit(c))
					return (t_token(TOKEN_VALUE, &(m_input_stream->putback(c))));
			} break;
		}

		// interrupt the parser
		return (t_token(TOKEN_NONE));
	}


	t_expr_type prim(bool get_token) {
		if (get_token)
			m_curr_token = scan_token();

		switch (m_curr_token.get_type()) {
			case TOKEN_NONE: {
				throw (t_syntax_exception("[prim] bad token"));
			} break;

			case TOKEN_VALUE: {
				const t_expr_type value = m_curr_token.get_value<t_expr_type>();

				// PRIM -> VALUE
				if ((m_curr_token = scan_token()).get_type() == TOKEN_SEPAR)
					(void) value;

				return value;
			} break;

			case TOKEN_IDENT: {
				// values for new identifiers will be default-constructed to 0
				t_expr_type& value = m_ident_table[m_curr_token.get_ident_str()];

				// PRIM -> IDENT [ = EXPR ]
				if ((m_curr_token = scan_token()).get_type() == TOKEN_SET)
					value = expr(true);

				return value;
			} break;


			case TOKEN_PLUS: {
				// PRIM -> +PRIM
				return (+prim(true));
			} break;
			case TOKEN_MINUS: {
				// PRIM -> -PRIM
				return (-prim(true));
			} break;

			case TOKEN_LPAR: {
				const t_expr_type e = expr(true);

				// PRIM -> ( EXPR )
				if (m_curr_token.get_type() != TOKEN_RPAR)
					throw (t_syntax_exception("[prim] expected \')\'"));

				// consume the TOKEN_RPAR
				m_curr_token = scan_token();
				return e;
			} break;

			default: {
				throw (t_syntax_exception("[prim] bad value or identifier"));
			} break;
		}

		// unreachable
		return (t_expr_type(0));
	}


	t_expr_type term(bool get_token) {
		t_expr_type rhs = prim(get_token);

		for (;;) {
			switch (m_curr_token.get_type()) {
				case TOKEN_NONE: {
					throw (t_syntax_exception("[term] bad token")); break;
				} break;

				case TOKEN_MUL: {
					// TERM -> TERM * PRIM
					rhs *= prim(true);
				} break;
				case TOKEN_DIV: {
					// TERM -> TERM / PRIM
					rhs /= prim(true);
				} break;

				default: {
					// TERM -> PRIM
					return rhs;
				} break;
			}
		}

		// unreachable
		return (t_expr_type(0));
	}


	t_expr_type expr(bool get_token) {
		t_expr_type rhs = term(get_token);

		for (;;) {
			switch (m_curr_token.get_type()) {
				case TOKEN_NONE: {
					throw (t_syntax_exception("[expr] bad token")); break;
				} break;

				case TOKEN_PLUS: {
					// EXPR -> EXPR + TERM
					rhs += term(true);
				} break;
				case TOKEN_MINUS: {
					// EXPR -> EXPR - TERM
					rhs -= term(true);
				} break;

				default: {
					// EXPR -> TERM
					return rhs;
				} break;
			}
		}

		// unreachable
		return (t_expr_type(0));
	}


	void show_prompt() {
		if (m_input_stream == &std::cin) {
			(*m_output_stream) << input_prompt;
		}
	}

	void show_error(const char* error) {
		(*m_output_stream) << "syntax error: " << error;
		(*m_output_stream) << std::endl;

		if (m_curr_token.get_type() != TOKEN_PRINT) {
			// erase the rest of the stream (up to the next newline)
			m_input_stream->ignore(std::numeric_limits<int>::max(), '\n');
		}
	}

	void init_state() { m_curr_token = t_token(TOKEN_PRINT); }
	void read_input() { m_curr_token = scan_token(); }
	void exec_input() {
		// ignore empty input
		if (m_curr_token.get_type() == TOKEN_PRINT)
			return;

		try {
			(*m_output_stream) << output_prompt;
			(*m_output_stream) << expr(false);
			(*m_output_stream) << std::endl;
		} catch (const t_syntax_exception& e) {
			// this propagates via the default case of prim()
			if (m_curr_token.get_type() == TOKEN_QUIT)
				return;

			show_error(e.what());
			// reset state for new expression
			init_state();
		}
	}

public:
	void repl() {
		init_state();

		while (m_curr_token.get_type() != TOKEN_QUIT) {
			if (m_curr_token.get_type() == TOKEN_PRINT)
				show_prompt();

			read_input();
			exec_input();
		}
	}

	void dump() {
		(*m_output_stream) << "[" << __FUNCTION__ << "]";
		(*m_output_stream) << std::endl;

		for (auto it = m_ident_table.begin(); it != m_ident_table.end(); ++it) {
			(*m_output_stream) << "\t";
			(*m_output_stream) << (it->first);
			(*m_output_stream) << " = ";
			(*m_output_stream) << (it->second);
			(*m_output_stream) << std::endl;
		}
	}

private:
	t_token m_curr_token;

	std::istream* m_input_stream;
	std::ostream* m_output_stream;

	std::map<std::string, t_expr_type> m_ident_table;
};



int main(int argc, char** argv) {
	t_parser<double> p((argc == 1)? &std::cin: (new std::istringstream(argv[1])), &std::cout);

	p.repl();
	p.dump();
	return 0;
}

