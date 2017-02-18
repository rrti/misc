/*
 * U+00000000 U+0000007F   0xxxxxxx
 * U+00000000              00000000
 *            U+0000007F   01111111
 *
 * U+00000080 U+000007FF   110xxxxx 10xxxxxx
 * U+00000080              11000010 10000000
 *            U+000007FF   11011111 10111111
 *
 * U+00000800 U+0000FFFF   1110xxxx 10xxxxxx 10xxxxxx
 * U+00000800              11000000 10100000 10000000
 *            U+0000FFFF   11001111 10111111 10111111
 *
 * U+00010000 U+001FFFFF   11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
 * U+00010000              11110000 10010000 10000000 10000000
 *            U+001FFFFF   11110111 10111111 10111111 10111111
 *
 * U+00200000 U+03FFFFFF   111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
 * U+00200000              11111000 10001000 10000000 10000000 10000000
 *            U+03FFFFFF   11111011 10111111 10111111 10111111 10111111
 *
 * U+04000000 U+7FFFFFFF   1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
 * U+04000000              11111100 10000100 10000000 10000000 10000000 10000000
 *            U+7FFFFFFF   11111101 10111111 10111111 10111111 10111111 10111111
 */

#include <stddef.h>
#include <string.h>

#define UTF8_CHAR_BYTES 6
#define UTF8_CODE_BYTES 4

static const unsigned char UTF8_RANGE_TABLE[UTF8_CHAR_BYTES][2] = {
	{0x00, 0x7F},
	{0xC2, 0xDF},
	{0xE0, 0xEF},
	{0xF0, 0xF7},
	{0xF8, 0xFB},
	{0xFC, 0xFD},
};

const unsigned char* utf8_getbytes(const unsigned char* str, unsigned char* buf, size_t* buf_size) {
	size_t num_bytes = -1u;
	size_t buf_index = 0;

	const unsigned char* first = (const unsigned char*) str;
	const unsigned char* next = first;

	unsigned char utf8_buf[UTF8_CHAR_BYTES] = {0};

	if ((*first) == 0)
		return NULL;

	// find out how many code-bytes to copy into buffer
	for (buf_index = 0; buf_index < UTF8_CHAR_BYTES; buf_index++) {
		if (((*first) >= UTF8_RANGE_TABLE[buf_index][0]) && ((*first) <= UTF8_RANGE_TABLE[buf_index][1])) {
			num_bytes = buf_index + 1; break;
		}
	}

	if (num_bytes == -1u)
		return NULL;

	for (buf_index = 0, utf8_buf[buf_index++] = *(next++); buf_index < num_bytes; utf8_buf[buf_index++] = *(next++)) {
		unsigned char min = 0x80;
		unsigned char max = 0xBF;

		if (buf_index == 1) {
			switch (num_bytes) {
				case 3: { if (utf8_buf[0] == 0xE0) min = 0xA0; } break;
				case 4: { if (utf8_buf[0] == 0xF0) min = 0x90; } break;
				case 5: { if (utf8_buf[0] == 0xF8) min = 0x88; } break;
				case 6: { if (utf8_buf[0] == 0xFC) min = 0x84; } break;
			}
		}

		if ((*next) < min || max < (*next)) {
			return NULL;
		}
	}

	if (buf != NULL && (*buf_size) >= num_bytes) {
		memcpy(buf, utf8_buf, num_bytes);
		*buf_size = num_bytes;
		return next;
	}

	*buf_size = num_bytes;
	return NULL;
}



const unsigned char* utf8_getcode(const unsigned char* str, unsigned char* code_buf, size_t* buf_size) {
	unsigned char utf8_buf[UTF8_CHAR_BYTES] = {0};
	size_t buf_len = sizeof(utf8_buf);

	const unsigned char* next = utf8_getbytes(str, utf8_buf, &buf_len);

	if (next == NULL)
		return NULL;

	// decode the bytes
	if (code_buf != NULL && (*buf_size) >= UTF8_CODE_BYTES) {
		switch (buf_len) {
			case 1: {
				*(code_buf + UTF8_CODE_BYTES - 1) = (utf8_buf[0] & 0x7F);
				*(code_buf + UTF8_CODE_BYTES - 2) = 0;
				*(code_buf + UTF8_CODE_BYTES - 3) = 0;
				*(code_buf + UTF8_CODE_BYTES - 4) = 0;
			} break;
			case 2: {
				*(code_buf + UTF8_CODE_BYTES - 1) = ((utf8_buf[0] << 6) & 0xC0) | (utf8_buf[1] & 0x3F);
				*(code_buf + UTF8_CODE_BYTES - 2) = ((utf8_buf[0] >> 2) & 0x07);
				*(code_buf + UTF8_CODE_BYTES - 3) = 0;
				*(code_buf + UTF8_CODE_BYTES - 4) = 0;
			} break;
			case 3: {
				*(code_buf + UTF8_CODE_BYTES - 1) = ((utf8_buf[1] << 6) & 0xC0) | ((utf8_buf[2] >> 0) & 0x3F);
				*(code_buf + UTF8_CODE_BYTES - 2) = ((utf8_buf[0] << 4) & 0xF0) | ((utf8_buf[1] >> 2) & 0x0F);
				*(code_buf + UTF8_CODE_BYTES - 3) = 0;
				*(code_buf + UTF8_CODE_BYTES - 4) = 0;
			} break;
			case 4: {
				*(code_buf + UTF8_CODE_BYTES - 1) = ((utf8_buf[2] << 6) & 0xC0) | ((utf8_buf[3] >> 0) & 0x3F);
				*(code_buf + UTF8_CODE_BYTES - 2) = ((utf8_buf[1] << 4) & 0xF0) | ((utf8_buf[2] >> 2) & 0x0F);
				*(code_buf + UTF8_CODE_BYTES - 3) = ((utf8_buf[0] << 2) & 0x1C) | ((utf8_buf[1] >> 4) & 0x03);
				*(code_buf + UTF8_CODE_BYTES - 4) = 0;
			} break;
			case 5: {
				*(code_buf + UTF8_CODE_BYTES - 1) = ((utf8_buf[3] << 6) & 0xC0) | ((utf8_buf[4] >> 0) & 0x3F);
				*(code_buf + UTF8_CODE_BYTES - 2) = ((utf8_buf[2] << 4) & 0xF0) | ((utf8_buf[3] >> 2) & 0x0F);
				*(code_buf + UTF8_CODE_BYTES - 3) = ((utf8_buf[1] << 2) & 0xFC) | ((utf8_buf[2] >> 4) & 0x03);
				*(code_buf + UTF8_CODE_BYTES - 4) = ((utf8_buf[0] << 0) & 0x03);
			} break;
			case 6: {
				*(code_buf + UTF8_CODE_BYTES - 1) = ((utf8_buf[4] << 6) & 0xC0) | ((utf8_buf[5] >> 0) & 0x3F);
				*(code_buf + UTF8_CODE_BYTES - 2) = ((utf8_buf[3] << 4) & 0xF0) | ((utf8_buf[4] >> 2) & 0x0F);
				*(code_buf + UTF8_CODE_BYTES - 3) = ((utf8_buf[2] << 2) & 0xFC) | ((utf8_buf[3] >> 4) & 0x03);
				*(code_buf + UTF8_CODE_BYTES - 4) = ((utf8_buf[0] << 6) & 0x40) | ((utf8_buf[1] >> 0) & 0x3F);
			} break;
		}

		*buf_size = UTF8_CODE_BYTES;
		return next;
	}

	// assume caller is not interested in the details
	*buf_size = buf_len;
	return NULL;
}



int utf8_isstr(const char* str) {
	size_t buf_len = 0;

	const unsigned char* utf8_str = (const unsigned char*) str;
	unsigned char utf8_buf[UTF8_CHAR_BYTES] = {0};

	while ((*utf8_str) != 0) {
		buf_len = sizeof(utf8_buf);

		if ((utf8_str = utf8_getbytes(utf8_str, utf8_buf, &buf_len)) == NULL) {
			return 0;
		}
	}

	return 1;
}

size_t utf8_strlen(const char* str) {
	size_t str_len = 0;
	size_t buf_len = 0;

	const unsigned char* utf8_str = (const unsigned char*) str;
	unsigned char utf8_buf[UTF8_CHAR_BYTES] = {0};

	while ((*utf8_str) != 0) {
		buf_len = sizeof(utf8_buf);

		if ((utf8_str = utf8_getbytes(utf8_str, utf8_buf, &buf_len)) == NULL)
			return 0;

		++str_len;
	}

	return str_len;
}


int utf8_substr(char* dst, size_t* dst_size, const char* src, const size_t start, const size_t end) {
	size_t src_len = 0;
	size_t buf_len = 0;
	size_t sub_len = 0;

	const unsigned char* first = NULL;
	const unsigned char* next = (const unsigned char*) src;

	const unsigned char* sub_start = NULL;
	const unsigned char* sub_end = NULL;

	unsigned char utf8_buf[UTF8_CHAR_BYTES] = {0};

	if (src_len == start || start == 0)
		sub_start = next;

	while ((*next) != 0) {
		buf_len = sizeof(utf8_buf);
		first = next;

		if ((next = utf8_getbytes(first, utf8_buf, &buf_len)) == NULL)
			return 0;

		if (src_len == start) sub_start = first;
		else if (src_len == end) sub_end = first;

		++src_len;
	}

	if (src_len == end || end == 0)
		sub_end = next;

	if (sub_start != NULL && sub_end != NULL && sub_start <= sub_end) {
		sub_len = sub_end - sub_start;

		if (dst != NULL && (*dst_size) >= (sub_len + 1)) {
			memcpy(dst, sub_start, sub_len);

			*(dst + sub_len) = 0;
			*dst_size = sub_len + 1;
		} else {
			*dst_size = sub_len + 1;
			return 0;
		}

		return sub_len;
	}

	return 0;
}

int utf8_strcmp(const char* lhs_str, const char* rhs_str) {
	int ret = 0;

	size_t lhs_buf_size = 0;
	size_t rhs_buf_size = 0;

	unsigned char lhs_code_buf[UTF8_CODE_BYTES] = {0};
	unsigned char rhs_code_buf[UTF8_CODE_BYTES] = {0};

	const unsigned char* utf8_lhs_str = (const unsigned char*) lhs_str;
	const unsigned char* utf8_rhs_str = (const unsigned char*) rhs_str;

	do {
		if ((*utf8_lhs_str) == 0)
			return (((*utf8_rhs_str) == 0)? 0: -1);
		if ((*utf8_rhs_str) == 0)
			return 1;

		lhs_buf_size = sizeof(lhs_code_buf);
		rhs_buf_size = sizeof(rhs_code_buf);
		utf8_lhs_str = utf8_getcode(utf8_lhs_str, lhs_code_buf, &lhs_buf_size);
		utf8_rhs_str = utf8_getcode(utf8_rhs_str, rhs_code_buf, &rhs_buf_size);

		if (utf8_lhs_str == 0)
			return ((utf8_rhs_str == 0)? 0: -1);
		if (utf8_rhs_str == 0)
			return 1;

		// compare 4-byte chunks
		ret = memcmp(lhs_code_buf, rhs_code_buf, UTF8_CODE_BYTES);
	} while (ret == 0);

	return ret;
}

