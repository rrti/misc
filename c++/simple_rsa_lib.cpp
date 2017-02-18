#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cmath>

#include <sstream>
#include <vector>

#define NUM_DECIMAL_DIGITS(n) (t_uint32(blog(10, n)) + 1)
#define NUM_BINARY_DIGITS(n) (t_uint32(blog(2, n)) + 1)

typedef unsigned char t_uint8;
typedef unsigned int t_uint32;
typedef unsigned long long int t_uint64;

static const double RAND_MAX_D = (double) RAND_MAX;
static const t_uint32 MAX_MSG_LEN = 8092;

static const t_uint8 NUM_CHARS = 2 + 37;
static const t_uint8 CHAR_SIZE = 2;

// used to reduce the numerical range of ASCII characters
// so we can encrypt larger blocks (worst-case 255255255
// vs. 383838 when block-size is eg. 3), starts at 2 because
// encrypting 'A' or 'AA' or 'AAA' or ... would produce the
// same ciphertext as plaintext character if 'A' was mapped
// to 0 or to 1
//
// the maximum number of decimal digits needed to represent
// an index into CHAR_TABLE (ie. one reduced-alphabet encoded
// character) is given by CHAR_SIZE
static t_uint8 CHAR_TABLE[NUM_CHARS] = {
	'-', '-',
	'A', 'B', 'C', 'D', 'E', 'F',
	'G', 'H', 'I', 'J', 'K', 'L',
	'M', 'N', 'O', 'P', 'Q', 'R',
	'S', 'T', 'U', 'V', 'W', 'X',
	'Y', 'Z', '0', '1', '2', '3',
	'4', '5', '6', '7', '8', '9',
	' '
};



template<typename Type> inline static std::string type2str(Type t) {
	std::stringstream ss; ss << t;
	std::string s; ss >> s;
	return s;
}

template<typename Type> inline static Type str2type(const std::string& s) {
	std::stringstream ss; ss << s;
	Type r; ss >> r;
	return r;
}



struct t_rsa_key {
	t_rsa_key() {}
	t_rsa_key(t_uint32 mod, t_uint32 exp, t_uint32 s) {
		m_modulus = mod;
		m_exponent = exp;
		m_dec_size = s;
	}

	std::string to_string() const {
		const std::string mod_str = type2str<t_uint32>(m_modulus);
		const std::string exp_str = type2str<t_uint32>(m_exponent);
		const std::string ret_str = "(" + mod_str + ", " + exp_str + ")";
		return ret_str;
	}

	t_uint32 get_modulus () const { return m_modulus; }
	t_uint32 get_exponent() const { return m_exponent; }
	t_uint32 get_dec_size() const { return m_dec_size; }

private:
	t_uint32 m_modulus;  // n = pq
	t_uint32 m_exponent; // d or e
	t_uint32 m_dec_size; // number of decimals in n
};

struct t_rsa_keypair {
	t_rsa_keypair(const t_rsa_key& pub, const t_rsa_key& pri) {
		m_pub_key = pub;
		m_pri_key = pri;
	}

	const t_rsa_key& get_public_key() const { return m_pub_key; }
	const t_rsa_key& get_private_key() const { return m_pri_key; }

	std::string to_string() const {
		const std::string s1 = "\tpublic key: (n, e) = " + m_pub_key.to_string();
		const std::string s2 = "\tprivate key: (n, d) = " + m_pri_key.to_string();
		return (s1 + "\n" + s2);
	}

private:
	t_rsa_key m_pub_key; // (n, e)
	t_rsa_key m_pri_key; // (n, d)
};




// returns the base-<b> logarithm of <n>
static inline double blog(t_uint32 b, double n) {
	return (log(n) / log(b));
}
// returns <b> raised to the power of <e>
static inline t_uint32 uint32_pow(t_uint32 b, t_uint32 e) {
	t_uint32 r = 1;

	while (e != 0) {
		r *= b;
		e--;
	}

	return r;
}



// writes every uint to string such that it consists
// of <size> ASCII characters (ie. as many as number
// of decimals in modulus), we depend on this during
// decryption
static void write_encrypted_message(const t_uint32* buf, const t_rsa_key* key) {
	std::string s;
	std::string u;

	for (t_uint32 i = 0; i < MAX_MSG_LEN && buf[i] != 0; i++) {
		u = type2str<t_uint32>(buf[i]);

		// use printf("%0nd") where n is size
		while (u.size() < key->get_dec_size()) {
			u = "0" + u;
		}

		s += u;
	}

	printf("[%s] ciphertext: %s\n", __FUNCTION__, s.c_str());
}

static void write_decrypted_message(const t_uint8* buf, const t_rsa_key*) {
	std::string s;

	for (t_uint32 i = 0; i < MAX_MSG_LEN && buf[i] != 0; i++) {
		s += CHAR_TABLE[buf[i]];
	}

	printf("[%s] plaintext: %s\n", __FUNCTION__, s.c_str());
}



// takes an ASCII array of characters and converts
// each to an integer that indexes into CHAR_TABLE,
// then writes those indices to buf
static void rsa_encode_message(const char* s, t_uint8* buf) {
	t_uint32 i = 0;
	t_uint32 j = 0;

	for (i = 0; i < MAX_MSG_LEN && s[i] != 0; i++) {
		const char c = s[i];

		if (isalpha(c)) {
			// map both upper- and lower-case chars to [2, 27]
			if (isupper(c)) { buf[j++] = c - 63; }
			if (islower(c)) { buf[j++] = c - 95; }
		}
		if (isdigit(c)) {
			// map digits to [28, 37]
			buf[j++] = c - 20;
		}
		if (isspace(c)) {
			buf[j++] = c + 6;
		}
	}

	buf[j + 1] = 0;
}

// takes an ASCII array of characters representing
// encrypted uint blocks and fills buf with uints
static void rsa_decode_message(const t_rsa_key* pri_key, const char* s, t_uint32* buf) {
	t_uint32 j = 0;
	t_uint32 k = 0;

	std::string t;

	for (t_uint32 i = 0; i < (MAX_MSG_LEN - pri_key->get_dec_size()) && s[i] != 0; i += pri_key->get_dec_size()) {
		t = "";
		j = i;

		while (t.size() < pri_key->get_dec_size()) {
			t += s[j++];
		}

		buf[k++] = str2type<t_uint32>(t);
	}
}




// encrypts and decrypts plaintext and ciphertext blocks
// <m> using modular exponentiation-by-squaring (ie will
// compute (m ^ k) % n where <k> is the public or private
// exponent <e> or <d> and <n> is modulus)
//
// m * m can overflow an t_uint32, so use an t_uint64 to
// store the product (still requires m to be < (1 << 32))
//
static inline t_uint32 mod_exp_v0(t_uint64 m, t_uint32 k, t_uint32 n) {
	t_uint32 r = 1;

	while (k > 1) {
		if ((k & 1) == 1) {
			r = ((r * m) % n);
			k -= 1;
		}
		m = ((m * m) % n);
		k = (k >> 1);
	}

	return ((r * m) % n);
}

static inline t_uint32 mod_exp_v1(t_uint64 m, t_uint32 k, t_uint32 n) {
	t_uint32 r = 1;

	while (k > 0) {
		if ((k & 1) == 1)
			r = (r * m) % n;

		m = (m * m) % n;
		k = (k >> 1);
	}

	return r;
}

static inline t_uint32 mod_exp_v2(t_uint64 m, t_uint32 k, t_uint32 n) {
	t_uint32 r = 1;

	while (k != 0) {
		if ((k & 1) == 0) {
			m = ((m * m) % n);
			k = (k >> 1);
		}
		r = ((r * m) % n);
		k -= 1;
	}

	return r;
}

// very slow, do not use!
static inline t_uint32 mod_exp_v3(t_uint64 m, t_uint32 k, t_uint32 n) {
	t_uint32 r = 1;

	for (t_uint32 i = 1; i <= k; i++) {
		r = ((r * m) % n);
	}

	return r;
}




// NOTE: use radix of "sizeof"(CHAR_TABLE) instead of 10?
static inline t_uint32 make_block(t_uint32 i, const t_uint8* msg, t_uint32 block_size) {
	t_uint32 block = 0;
	t_uint32 shift = uint32_pow(10, CHAR_SIZE);
	t_uint32  mult = 1;

	for (int j = block_size - 1; j >= 0; j--) {
		block += (msg[i + j] * mult);
		mult *= shift;
	}

	return block;
}

static inline void split_block(t_uint32 block, t_uint32 block_size, t_uint32 i, t_uint8* buf) {
	t_uint32 shift = uint32_pow(10, CHAR_SIZE);
	t_uint32 r = 0;

	for (int j = block_size - 1; j >= 0; j--) {
		r = block % shift;
		block = (block - r) / shift;

		buf[i + j] = r;
	}
}


// encrypt message <msg> to buffer <buf>,
// <s> encoded chars from <msg> at a time
// (where <s> is determined by number of
// decimal digits in modulus)
//
// <msg> is a plaintext of bytes, AFTER
// conversion from chars (where each byte
// is index into CHAR_TABLE) via alphabet
// reduction
static void rsa_encrypt_message(const t_rsa_key* pub_key, const t_uint8* msg, t_uint32* buf) {
	t_uint32 bidx = 0;
	t_uint32 size = pub_key->get_dec_size() / CHAR_SIZE;

	for (t_uint32 i = 0; i < (MAX_MSG_LEN - size) && msg[i] != 0; i += size) {
		buf[bidx++] = mod_exp_v2(make_block(i, msg, size), pub_key->get_exponent(), pub_key->get_modulus());
	}

	buf[bidx] = 0;
}

// decrypt message <msg> to buffer <buf>,
// <s> encoded chars from <msg> at a time
// (where <s> is determined by number of
// decimal digits in modulus)
//
// <msg> is ciphertext of uints read from
// command-line, AFTER conversion from chars
// to uints (ie. equal to output of function
// rsa_encrypt_message()
static void rsa_decrypt_message(const t_rsa_key* pri_key, const uint* msg, t_uint8* buf) {
	t_uint32 bidx = 0;
	t_uint32 size = pri_key->get_dec_size() / CHAR_SIZE;

	for (t_uint32 i = 0; i < MAX_MSG_LEN && msg[i] != 0; i++) {
		split_block(mod_exp_v2(msg[i], pri_key->get_exponent(), pri_key->get_modulus()), size, bidx, buf);

		bidx += size;
	}
}



static bool fermat_primality_test(t_uint32 n, t_uint32 k) {
	if ((n & 1) == 0)
		return false;

	while (k-- != 0) {
		 // choose random number witness in range [1, n - 1]
		const t_uint32 a = 1 + t_uint32((rand() / RAND_MAX_D) * (n - 2));
		const t_uint32 e = n - 1;

		if (mod_exp_v2(a, e, n) != 1) {
			return false;
		}
	}

	return true;
}

static size_t fermat_prime_number_sieve(t_uint32 n, t_uint32 k, std::vector<t_uint32>& w) {
	w.reserve(n / blog(10, n));

	// skip the smallest primes so they won't get picked as keys
	for (unsigned int i = 11; i < n; i += 2) {
		if (fermat_primality_test(i, k)) {
			w.push_back(i);
		}
	}

	return (w.size());
}


// the prime-number Sieve of Eratosthenes
static size_t eratosthenes_prime_number_sieve(t_uint32 n, std::vector<t_uint32>& w) {
	std::vector<bool> v(n, true);

	t_uint32 max = int(pow(n, 0.5) + 1);
	t_uint32 mul = 2;

	// reserve space for O(n / log(n)) primes
	w.reserve(n / blog(10, n));

	for (t_uint32 ctr = 2; ctr <= max; ctr++) {
		while ((ctr * mul) <= n) {
			v[((ctr * mul) - 1)] = false;
			mul += 1;
		}

		mul = 2;
	}

	for (t_uint32 idx = 0; idx < n; idx++) {
		if (!v[idx])
			continue;

		// skip the smallest primes so they won't get picked as keys
		if ((idx + 1) > 10) {
			w.push_back(idx + 1);
		}
	}

	return (w.size());
}


// the Euclidean algorithm (finds the
// greatest common divisor of a and b)
static inline t_uint32 gcd(t_uint32 a, t_uint32 b) {
	t_uint32 gcd;

	while (b != 0) {
		gcd = b;
		b = a % b;
		a = gcd;
	}

	return gcd;
}

// the Extended Euclidean algorithm (finds
// p and q such that ap + bq = gcd(a, b))
static inline t_uint32 ext_gcd(t_uint32 a, t_uint32 b) {
	t_uint32 t1 = 0; t_uint32 t2 = 0; t_uint32 t3 = 0;
	t_uint32 u1 = 1; t_uint32 u2 = 0; t_uint32 u3 = b;
	t_uint32 v1 = 0; t_uint32 v2 = 1; t_uint32 v3 = a;
	t_uint32 q;

	while ((u3 % v3) != 0) {
		q = u3 / v3;

		t1 = (u1 - (q * v1));
		t2 = (u2 - (q * v2));
		t3 = (u3 - (q * v3));

		u1 = v1; u2 = v2; u3 = v3;
		v1 = t1; v2 = t2; v3 = t3;
	}

	return ((v2 + b) % b);
}


static inline t_uint32 max_char_sequence_val(t_uint32 b) {
	const t_uint32 shift = uint32_pow(10, CHAR_SIZE);

	t_uint32 h = 0;
	t_uint32 m = 1;
	t_uint32 k = NUM_CHARS - 1;

	for (int j = b - 1; j >= 0; j--) {
		h += (k * m);
		m *= shift;
	}

	return h;
}




static t_rsa_keypair generate_rsa_keys(bool probabilistic_prime_generation) {
	std::vector<t_uint32> w;

	if (probabilistic_prime_generation) {
		fermat_prime_number_sieve(65536, 20, w);
	} else {
		eratosthenes_prime_number_sieve(65536, w);
	}

	t_uint32 p = 0, q = 0, n = 0;
	t_uint32 s = 0, b = 0, h = 1;
	t_uint32 m = 0, e = 0, d = 0;

	t_uint32 x = 0, y = 0;
	t_uint32 z = w.size() - 1;

	// block-size <b> is (#decimal digits in modulus) / CHAR_SIZE,
	// modulus <n> must always be greater than maximum value <h>
	// that can be formed from sequence of <b> encoded chars
	//
	// <n> must always be smaller than (1 << 32) == UINT_MAX, otherwise
	// n * n can overflow t_uint64 data-type in rsa*crypt_block() (this
	// also means p * q may never require more than sizeof(uint) == 32
	// bits to store, which is satisfied because p and q each require
	// at most 16)
	while (n <= h) {
		x = t_uint32((rand() / RAND_MAX_D) * z); // random index
		y = t_uint32((rand() / RAND_MAX_D) * z); // random index
		p = w[x];                                // first prime
		q = w[y];                                // second prime
		n = p * q;                               // modulus
		s = NUM_DECIMAL_DIGITS(n);               // number of decimals in n
		b = s / CHAR_SIZE;                       // #(encoded chars) encrypted at once, given <s>
		h = max_char_sequence_val(b);            // max. decimal val. of seq. of <b> encoded chars
	}

	x = t_uint32((rand() / RAND_MAX_D) * z);     // random index
	m = (p - 1) * (q - 1);                       // Euler's totient
	e = w[x];                                    // public exponent
	d = 0;                                       // private exponent

	while (e >= m || e < 3 || gcd(e, m) != 1 || e == p || e == q) {
		x = t_uint32((rand() / RAND_MAX_D) * z);
		e = w[x];
	}

	// calculate the multiplicative inverse of e mod m
	d = ext_gcd(e, m);

	const t_rsa_key pub_key(n, e, s);
	const t_rsa_key pri_key(n, d, s);

	return (t_rsa_keypair(pub_key, pri_key));
}



static bool encrypt_message(const int argc, const char** argv) {
	if (argc != 5) {
		printf("[%s] usage: %s --enc <\"message\"> <n> <e>\n", __FUNCTION__, argv[0]);
		return false;
	}

	const std::string& msg = argv[2];
	const std::string&  ns = argv[3];
	const std::string&  es = argv[4];

	t_uint8 encoded_msg[MAX_MSG_LEN + 1] = {0};
	t_uint32 cipher_txt[MAX_MSG_LEN + 1] = {0};

	const t_uint32 n = str2type<t_uint32>(ns);
	const t_uint32 e = str2type<t_uint32>(es);
	const t_uint32 s = NUM_DECIMAL_DIGITS(n);

	const t_rsa_key pub_key(n, e, s);

	rsa_encode_message(msg.c_str(), encoded_msg);
	rsa_encrypt_message(&pub_key, encoded_msg, cipher_txt);

	// show the encoded plaintext message,
	// then output the encrypted message
	write_decrypted_message(encoded_msg, &pub_key);
	write_encrypted_message(cipher_txt, &pub_key);
	return true;
}

static bool decrypt_message(const int argc, const char** argv) {
	if (argc != 5) {
		printf("[%s] usage: %s --dec <\"message\"> <n> <d>\n", __FUNCTION__, argv[0]);
		return false;
	}

	const std::string& msg = argv[2];
	const std::string&  ns = argv[3];
	const std::string&  ds = argv[4];

	t_uint32 decoded_msg[MAX_MSG_LEN + 1] = {0};
	t_uint8 plaintxt_msg[MAX_MSG_LEN + 1] = {0};

	const t_uint32 n = str2type<t_uint32>(ns);
	const t_uint32 d = str2type<t_uint32>(ds);
	const t_uint32 s = NUM_DECIMAL_DIGITS(n);

	const t_rsa_key pri_key(n, d, s);

	rsa_decode_message(&pri_key, msg.c_str(), decoded_msg);
	rsa_decrypt_message(&pri_key, decoded_msg, plaintxt_msg);

	// show the decoded ciphertext message,
	// then output the decrypted message
	write_encrypted_message(decoded_msg, &pri_key);
	write_decrypted_message(plaintxt_msg, &pri_key);
	return true;
}

static bool sign_message(const int argc, const char** argv) {
	if (argc != 5) {
		printf("[%s] usage: %s --sig <\"message\"> <n> <d>\n", __FUNCTION__, argv[0]);
		return false;
	}

	const std::string& msg = argv[2];
	const std::string&  ns = argv[3];
	const std::string&  ds = argv[4];

	t_uint8 encoded_msg[MAX_MSG_LEN + 1] = {0};
	t_uint32 cipher_txt[MAX_MSG_LEN + 1] = {0};

	const t_uint32 n = str2type<t_uint32>(ns);
	const t_uint32 d = str2type<t_uint32>(ds);
	const t_uint32 s = NUM_DECIMAL_DIGITS(n);

	const t_rsa_key pri_key(n, d, s);

	rsa_encode_message(msg.c_str(), encoded_msg);
	rsa_encrypt_message(&pri_key, encoded_msg, cipher_txt);

	// output the ciphertext signature
	write_encrypted_message(cipher_txt, &pri_key);
	return true;
}

static bool auth_message(const int argc, const char** argv) {
	if (argc != 5) {
		printf("[%s] usage: %s --aut <\"message\"> <n> <e>\n", __FUNCTION__, argv[0]);
		return false;
	}

	const std::string& msg = argv[2];
	const std::string&  ns = argv[3];
	const std::string&  es = argv[4];

	t_uint32 decoded_msg[MAX_MSG_LEN + 1] = {0};
	t_uint8 plaintxt_msg[MAX_MSG_LEN + 1] = {0};

	const t_uint32 n = str2type<t_uint32>(ns);
	const t_uint32 e = str2type<t_uint32>(es);
	const t_uint32 s = NUM_DECIMAL_DIGITS(n);

	const t_rsa_key pub_key(n, e, s);

	rsa_decode_message(&pub_key, msg.c_str(), decoded_msg);
	rsa_decrypt_message(&pub_key, decoded_msg, plaintxt_msg);

	// output the plaintext signature
	write_decrypted_message(plaintxt_msg, &pub_key);
	return true;
}

static bool gen_keypair(const int argc, const char** argv) {
	if (argc != 2) {
		printf("[%s] usage: %s --gen\n", __FUNCTION__, argv[0]);
		return false;
	}

	// generate a key-pair
	const t_rsa_keypair kp = generate_rsa_keys(false);
	const std::string& kps = kp.to_string();

	printf("[%s]\n%s\n", __FUNCTION__, kps.c_str());
	return true;
}



static bool run_unit_test(const int argc, const char** argv) {
	if (argc != 3) {
		printf("[%s] usage: %s --tst <N>\n", __FUNCTION__, argv[0]);
		return false;
	}

	for (unsigned int i = atoi(argv[2]); i > 0; i--) {
		const t_uint32 m = random(); // 985743564
		const t_uint32 k = random(); //    385746
		const t_uint32 n = random(); //   2054687

		const t_uint32 v0 = mod_exp_v0(m, k, n);
		const t_uint32 v1 = mod_exp_v1(m, k, n);
		const t_uint32 v2 = mod_exp_v2(m, k, n);
		const t_uint32 v3 = mod_exp_v3(m, k, n);

		printf("[%u] %u^%u mod %u=%u\n", i, m, k, n, v0);
		printf("[%u] %u^%u mod %u=%u\n", i, m, k, n, v1);
		printf("[%u] %u^%u mod %u=%u\n", i, m, k, n, v2);
		printf("[%u] %u^%u mod %u=%u\n", i, m, k, n, v3);

		assert(v0 == v1);
		assert(v1 == v2);
		assert(v2 == v3);
	}

	return true;
}

static bool parse_args(const int argc, const char** argv) {
	if (argc <= 1) {
		printf("[%s] usage: %s <--enc | --dec | --sig | --aut | --gen | --tst>\n", __FUNCTION__, argv[0]);
		return false;
	}

	if (strcmp(argv[1], "--enc") == 0) {
		// encrypt a plaintext message with public key
		return (encrypt_message(argc, argv));
	}
	if (strcmp(argv[1], "--dec") == 0) {
		// decrypt a ciphertext message with private key
		return (decrypt_message(argc, argv));
	}

	if (strcmp(argv[1], "--sig") == 0) {
		// sign a plaintext message with private key
		return (sign_message(argc, argv));
	}
	if (strcmp(argv[1], "--aut") == 0) {
		// authenticate a ciphertext message with public key
		return (auth_message(argc, argv));
	}

	if (strcmp(argv[1], "--gen") == 0) {
		// generate a (public, private) pair of RSA keys
		return (gen_keypair(argc, argv));
	}

	if (strcmp(argv[1], "--tst") == 0) {
		return (run_unit_test(argc, argv));
	}

	return false;
}

int main(const int argc, const char** argv) {
	srand(time(NULL));

	if (!parse_args(argc, argv))
		return -1;

	return 0;
}

