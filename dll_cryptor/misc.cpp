/* ========================== */
/*     � SlonoBoyko 2018      */
/* ========================== */

#include "main.h"
#include <iostream>

void Log(const char *text, ...)
{
	va_list ap;
	if (text == NULL)
		return;

	char tmp[512];
	memset(tmp, 0, 512);

	va_start(ap, text);
	vsnprintf(tmp, sizeof(tmp) - 1, text, ap);
	va_end(ap);

	char filename[MAX_PATH];
	sprintf(filename, "dll_cryptor.log");
	FILE* log = fopen(filename, "a");
	fprintf(log, tmp);
	fprintf(log, "\n");
	fclose(log);

}

void xtea_encode(unsigned int num_rounds, uint32_t v[2], uint32_t const key[4]){
	unsigned int i;
	uint32_t v0 = v[0], v1 = v[1], sum = 0, delta = 0x9E3779B9;
	for (i = 0; i < num_rounds; i++){
		v0 += (((v1 << 4) ^ (v1 >> 5)) + v1) ^ (sum + key[sum & 3]);
		sum += delta;
		v1 += (((v0 << 4) ^ (v0 >> 5)) + v0) ^ (sum + key[(sum >> 11) & 3]);
	}
	v[0] = v0; v[1] = v1;
}

void xtea_decode(unsigned int num_rounds, uint32_t v[2], uint32_t const key[4]){
	unsigned int i;
	uint32_t v0 = v[0], v1 = v[1], delta = 0x9E3779B9, sum = delta*num_rounds;
	for (i = 0; i < num_rounds; i++){
		v1 -= (((v0 << 4) ^ (v0 >> 5)) + v0) ^ (sum + key[(sum >> 11) & 3]);
		sum -= delta;
		v0 -= (((v1 << 4) ^ (v1 >> 5)) + v1) ^ (sum + key[sum & 3]);
	}
	v[0] = v0; v[1] = v1;
}

uint32_t *generate_key()
{
	uint32_t buf[0xFF];
	srand(GetTickCount());
	
	for (size_t i = 0; i < 0xFF; i++)
	{
		buf[i] = (rand() % 0xFFFFFFFF) + 0x10000000;
		buf[i] = buf[i] ^ (buf[i] + (rand() % (0xFFFFFFFF - buf[i])) + 0x10000000);

	}
	return buf;
}

static const char b64_table[] = {
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
	'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
	'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
	'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
	'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
	'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
	'w', 'x', 'y', 'z', '0', '1', '2', '3',
	'4', '5', '6', '7', '8', '9', '+', '/'
};

char *b64_encode(const unsigned char *src, size_t len) {
	int i = 0;
	int j = 0;
	char *enc = NULL;
	size_t size = 0;
	unsigned char buf[4];
	unsigned char tmp[3];

	// alloc
	enc = (char *)malloc(1);
	if (NULL == enc) { return NULL; }

	// parse until end of source
	while (len--) {
		// read up to 3 bytes at a time into `tmp'
		tmp[i++] = *(src++);

		// if 3 bytes read then encode into `buf'
		if (3 == i) {
			buf[0] = (tmp[0] & 0xfc) >> 2;
			buf[1] = ((tmp[0] & 0x03) << 4) + ((tmp[1] & 0xf0) >> 4);
			buf[2] = ((tmp[1] & 0x0f) << 2) + ((tmp[2] & 0xc0) >> 6);
			buf[3] = tmp[2] & 0x3f;

			// allocate 4 new byts for `enc` and
			// then translate each encoded buffer
			// part by index from the base 64 index table
			// into `enc' unsigned char array
			enc = (char *)realloc(enc, size + 4);
			for (i = 0; i < 4; ++i) {
				enc[size++] = b64_table[buf[i]];
			}

			// reset index
			i = 0;
		}
	}

	// remainder
	if (i > 0) {
		// fill `tmp' with `\0' at most 3 times
		for (j = i; j < 3; ++j) {
			tmp[j] = '\0';
		}

		// perform same codec as above
		buf[0] = (tmp[0] & 0xfc) >> 2;
		buf[1] = ((tmp[0] & 0x03) << 4) + ((tmp[1] & 0xf0) >> 4);
		buf[2] = ((tmp[1] & 0x0f) << 2) + ((tmp[2] & 0xc0) >> 6);
		buf[3] = tmp[2] & 0x3f;

		// perform same write to `enc` with new allocation
		for (j = 0; (j < i + 1); ++j) {
			enc = (char *)realloc(enc, size + 1);
			enc[size++] = b64_table[buf[j]];
		}

		// while there is still a remainder
		// append `=' to `enc'
		while ((i++ < 3)) {
			enc = (char *)realloc(enc, size + 1);
			enc[size++] = '=';
		}
	}

	// Make sure we have enough space to add '\0' character at end.
	enc = (char *)realloc(enc, size + 1);
	enc[size] = '\0';

	return enc;
}

unsigned char *b64_decode(const char *src, size_t len) {
	int i = 0;
	int j = 0;
	int l = 0;
	size_t size = 0;
	unsigned char *dec = NULL;
	unsigned char buf[3];
	unsigned char tmp[4];

	// alloc
	dec = (unsigned char *)malloc(1);
	if (NULL == dec) { return NULL; }

	// parse until end of source
	while (len--) {
		// break if char is `=' or not base64 char
		if ('=' == src[j]) { break; }
		if (!(isalnum(src[j]) || '+' == src[j] || '/' == src[j])) { break; }

		// read up to 4 bytes at a time into `tmp'
		tmp[i++] = src[j++];

		// if 4 bytes read then decode into `buf'
		if (4 == i) {
			// translate values in `tmp' from table
			for (i = 0; i < 4; ++i) {
				// find translation char in `b64_table'
				for (l = 0; l < 64; ++l) {
					if (tmp[i] == b64_table[l]) {
						tmp[i] = l;
						break;
					}
				}
			}

			// decode
			buf[0] = (tmp[0] << 2) + ((tmp[1] & 0x30) >> 4);
			buf[1] = ((tmp[1] & 0xf) << 4) + ((tmp[2] & 0x3c) >> 2);
			buf[2] = ((tmp[2] & 0x3) << 6) + tmp[3];

			// write decoded buffer to `dec'
			dec = (unsigned char *)realloc(dec, size + 3);
			if (dec != NULL){
				for (i = 0; i < 3; ++i) {
					dec[size++] = buf[i];
				}
			}
			else {
				return NULL;
			}

			// reset
			i = 0;
		}
	}

	// remainder
	if (i > 0) {
		// fill `tmp' with `\0' at most 4 times
		for (j = i; j < 4; ++j) {
			tmp[j] = '\0';
		}

		// translate remainder
		for (j = 0; j < 4; ++j) {
			// find translation char in `b64_table'
			for (l = 0; l < 64; ++l) {
				if (tmp[j] == b64_table[l]) {
					tmp[j] = l;
					break;
				}
			}
		}

		// decode remainder
		buf[0] = (tmp[0] << 2) + ((tmp[1] & 0x30) >> 4);
		buf[1] = ((tmp[1] & 0xf) << 4) + ((tmp[2] & 0x3c) >> 2);
		buf[2] = ((tmp[2] & 0x3) << 6) + tmp[3];

		// write remainer decoded buffer to `dec'
		dec = (unsigned char *)realloc(dec, size + (i - 1));
		if (dec != NULL){
			for (j = 0; (j < i - 1); ++j) {
				dec[size++] = buf[j];
			}
		}
		else {
			return NULL;
		}
	}

	// Make sure we have enough space to add '\0' character at end.
	dec = (unsigned char *)realloc(dec, size + 1);
	if (dec != NULL){
		dec[size] = '\0';
	}
	else {
		return NULL;
	}

	
	return dec;
}
