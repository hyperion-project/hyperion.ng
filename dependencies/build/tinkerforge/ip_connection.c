/*
 * Copyright (C) 2012-2016, 2019-2020 Matthias Bolte <matthias@tinkerforge.com>
 * Copyright (C) 2011 Olaf Lüke <olaf@tinkerforge.com>
 *
 * Redistribution and use in source and binary forms of this file,
 * with or without modification, are permitted. See the Creative
 * Commons Zero (CC0 1.0) License for more details.
 */

#ifndef _WIN32
	#ifndef _BSD_SOURCE
		#define _BSD_SOURCE // for usleep from unistd.h
	#endif
	#ifndef _GNU_SOURCE
		#define _GNU_SOURCE
	#endif
	#ifndef _DEFAULT_SOURCE
		#define _DEFAULT_SOURCE
	#endif
#endif

#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
	#include <winsock2.h>
	#include <ws2tcpip.h>
	#include <wincrypt.h>
	#include <process.h>
#else
	#include <fcntl.h>
	#include <unistd.h>
	#include <sys/types.h>
	#include <sys/socket.h> // connect
	#include <sys/select.h>
	#include <sys/stat.h>
	#include <netinet/tcp.h> // TCP_NO_DELAY
	#include <netdb.h> // gethostbyname
	#include <netinet/in.h> // struct sockaddr_in
#endif

#ifdef _MSC_VER
	// replace getpid with GetCurrentProcessId
	#define getpid GetCurrentProcessId

	// avoid warning from MSVC about deprecated POSIX name
	#define strdup _strdup
#else
	#include <sys/time.h> // gettimeofday
#endif

#if defined _MSC_VER && _MSC_VER < 1900
	// snprintf is not available in older MSVC versions
	#define snprintf _snprintf
#endif

#define IPCON_EXPOSE_INTERNALS
#define IPCON_EXPOSE_MILLISLEEP

#include "ip_connection.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined _MSC_VER || defined __BORLANDC__
	#pragma pack(push)
	#pragma pack(1)
	#define ATTRIBUTE_PACKED
#elif defined __GNUC__
	#ifdef _WIN32
		// workaround struct packing bug in GCC 4.7 on Windows
		// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=52991
		#define ATTRIBUTE_PACKED __attribute__((gcc_struct, packed))
	#else
		#define ATTRIBUTE_PACKED __attribute__((packed))
	#endif
#else
	#error unknown compiler, do not know how to enable struct packing
#endif

typedef struct {
	PacketHeader header;
} ATTRIBUTE_PACKED DeviceEnumerate_Broadcast;

typedef struct {
	PacketHeader header;
	char uid[8];
	char connected_uid[8];
	char position;
	uint8_t hardware_version[3];
	uint8_t firmware_version[3];
	uint16_t device_identifier;
	uint8_t enumeration_type;
} ATTRIBUTE_PACKED DeviceEnumerate_Callback;

typedef struct {
	PacketHeader header;
} ATTRIBUTE_PACKED DeviceGetIdentity_Request;

typedef struct {
	PacketHeader header;
	char uid[8];
	char connected_uid[8];
	char position;
	uint8_t hardware_version[3];
	uint8_t firmware_version[3];
	uint16_t device_identifier;
} ATTRIBUTE_PACKED DeviceGetIdentity_Response;

typedef struct {
	PacketHeader header;
} ATTRIBUTE_PACKED BrickDaemonGetAuthenticationNonce_Request;

typedef struct {
	PacketHeader header;
	uint8_t server_nonce[4];
} ATTRIBUTE_PACKED BrickDaemonGetAuthenticationNonce_Response;

typedef struct {
	PacketHeader header;
	uint8_t client_nonce[4];
	uint8_t digest[20];
} ATTRIBUTE_PACKED BrickDaemonAuthenticate_Request;

#if defined _MSC_VER || defined __BORLANDC__
	#pragma pack(pop)
#endif
#undef ATTRIBUTE_PACKED

#ifndef __cplusplus
	#ifdef __GNUC__
		#ifndef __GNUC_PREREQ
			#define __GNUC_PREREQ(major, minor) \
				((((__GNUC__) << 16) + (__GNUC_MINOR__)) >= (((major) << 16) + (minor)))
		#endif
		#if __GNUC_PREREQ(4, 6)
			#define STATIC_ASSERT(condition, message) \
				_Static_assert(condition, message);
		#else
			#define STATIC_ASSERT(condition, message) // FIXME
		#endif
	#else
		#define STATIC_ASSERT(condition, message) // FIXME
	#endif

	STATIC_ASSERT(sizeof(PacketHeader) == 8, "PacketHeader has invalid size")
	STATIC_ASSERT(sizeof(Packet) == 80, "Packet has invalid size")
	STATIC_ASSERT(sizeof(DeviceEnumerate_Broadcast) == 8, "DeviceEnumerate_Broadcast has invalid size")
	STATIC_ASSERT(sizeof(DeviceEnumerate_Callback) == 34, "DeviceEnumerate_Callback has invalid size")
	STATIC_ASSERT(sizeof(DeviceGetIdentity_Request) == 8, "DeviceGetIdentity_Request has invalid size")
	STATIC_ASSERT(sizeof(DeviceGetIdentity_Response) == 33, "DeviceGetIdentity_Response has invalid size")
	STATIC_ASSERT(sizeof(BrickDaemonGetAuthenticationNonce_Request) == 8, "BrickDaemonGetAuthenticationNonce_Request has invalid size")
	STATIC_ASSERT(sizeof(BrickDaemonGetAuthenticationNonce_Response) == 12, "BrickDaemonGetAuthenticationNonce_Response has invalid size")
	STATIC_ASSERT(sizeof(BrickDaemonAuthenticate_Request) == 32, "BrickDaemonAuthenticate_Request has invalid size")
#endif

void millisleep(uint32_t msec) {
#ifdef _WIN32
	Sleep(msec);
#else
	if (msec >= 1000) {
		sleep(msec / 1000);

		msec %= 1000;
	}

	usleep(msec * 1000);
#endif
}

/*****************************************************************************
 *
 *                                 SHA1
 *
 *****************************************************************************/

/*
 * Based on the SHA-1 C implementation by Steve Reid <steve@edmweb.com>
 * 100% Public Domain
 *
 * Test Vectors (from FIPS PUB 180-1)
 * "abc"
 *   A9993E36 4706816A BA3E2571 7850C26C 9CD0D89D
 * "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"
 *   84983E44 1C3BD26E BAAE4AA1 F95129E5 E54670F1
 * A million repetitions of "a"
 *   34AA973C D4C4DAA4 F61EEB2B DBAD2731 6534016F
 */

#define SHA1_BLOCK_LENGTH 64
#define SHA1_DIGEST_LENGTH 20

typedef struct {
	uint32_t state[5];
	uint64_t count;
	uint8_t buffer[SHA1_BLOCK_LENGTH];
} SHA1;

#define rol(value, bits) (((value) << (bits)) | ((value) >> (32 - (bits))))

// blk0() and blk() perform the initial expand. blk0() deals with host endianess
#define blk0(i) (block[i] = htonl(block[i]))
#define blk(i) (block[i&15] = rol(block[(i+13)&15]^block[(i+8)&15]^block[(i+2)&15]^block[i&15],1))

// (R0+R1), R2, R3, R4 are the different operations (rounds) used in SHA1
#define R0(v,w,x,y,z,i) z+=((w&(x^y))^y)+blk0(i)+0x5A827999+rol(v,5);w=rol(w,30)
#define R1(v,w,x,y,z,i) z+=((w&(x^y))^y)+blk(i)+0x5A827999+rol(v,5);w=rol(w,30)
#define R2(v,w,x,y,z,i) z+=(w^x^y)+blk(i)+0x6ED9EBA1+rol(v,5);w=rol(w,30)
#define R3(v,w,x,y,z,i) z+=(((w|x)&y)|(w&x))+blk(i)+0x8F1BBCDC+rol(v,5);w=rol(w,30)
#define R4(v,w,x,y,z,i) z+=(w^x^y)+blk(i)+0xCA62C1D6+rol(v,5);w=rol(w,30)

// hash a single 512-bit block. this is the core of the algorithm
static uint32_t sha1_transform(SHA1 *sha1, const uint8_t buffer[SHA1_BLOCK_LENGTH]) {
	uint32_t a, b, c, d, e;
	uint32_t block[SHA1_BLOCK_LENGTH / 4];

	memcpy(&block, buffer, SHA1_BLOCK_LENGTH);

	// copy sha1->state[] to working variables
	a = sha1->state[0];
	b = sha1->state[1];
	c = sha1->state[2];
	d = sha1->state[3];
	e = sha1->state[4];

	// 4 rounds of 20 operations each (loop unrolled)
	R0(a,b,c,d,e, 0); R0(e,a,b,c,d, 1); R0(d,e,a,b,c, 2); R0(c,d,e,a,b, 3);
	R0(b,c,d,e,a, 4); R0(a,b,c,d,e, 5); R0(e,a,b,c,d, 6); R0(d,e,a,b,c, 7);
	R0(c,d,e,a,b, 8); R0(b,c,d,e,a, 9); R0(a,b,c,d,e,10); R0(e,a,b,c,d,11);
	R0(d,e,a,b,c,12); R0(c,d,e,a,b,13); R0(b,c,d,e,a,14); R0(a,b,c,d,e,15);
	R1(e,a,b,c,d,16); R1(d,e,a,b,c,17); R1(c,d,e,a,b,18); R1(b,c,d,e,a,19);

	R2(a,b,c,d,e,20); R2(e,a,b,c,d,21); R2(d,e,a,b,c,22); R2(c,d,e,a,b,23);
	R2(b,c,d,e,a,24); R2(a,b,c,d,e,25); R2(e,a,b,c,d,26); R2(d,e,a,b,c,27);
	R2(c,d,e,a,b,28); R2(b,c,d,e,a,29); R2(a,b,c,d,e,30); R2(e,a,b,c,d,31);
	R2(d,e,a,b,c,32); R2(c,d,e,a,b,33); R2(b,c,d,e,a,34); R2(a,b,c,d,e,35);
	R2(e,a,b,c,d,36); R2(d,e,a,b,c,37); R2(c,d,e,a,b,38); R2(b,c,d,e,a,39);

	R3(a,b,c,d,e,40); R3(e,a,b,c,d,41); R3(d,e,a,b,c,42); R3(c,d,e,a,b,43);
	R3(b,c,d,e,a,44); R3(a,b,c,d,e,45); R3(e,a,b,c,d,46); R3(d,e,a,b,c,47);
	R3(c,d,e,a,b,48); R3(b,c,d,e,a,49); R3(a,b,c,d,e,50); R3(e,a,b,c,d,51);
	R3(d,e,a,b,c,52); R3(c,d,e,a,b,53); R3(b,c,d,e,a,54); R3(a,b,c,d,e,55);
	R3(e,a,b,c,d,56); R3(d,e,a,b,c,57); R3(c,d,e,a,b,58); R3(b,c,d,e,a,59);

	R4(a,b,c,d,e,60); R4(e,a,b,c,d,61); R4(d,e,a,b,c,62); R4(c,d,e,a,b,63);
	R4(b,c,d,e,a,64); R4(a,b,c,d,e,65); R4(e,a,b,c,d,66); R4(d,e,a,b,c,67);
	R4(c,d,e,a,b,68); R4(b,c,d,e,a,69); R4(a,b,c,d,e,70); R4(e,a,b,c,d,71);
	R4(d,e,a,b,c,72); R4(c,d,e,a,b,73); R4(b,c,d,e,a,74); R4(a,b,c,d,e,75);
	R4(e,a,b,c,d,76); R4(d,e,a,b,c,77); R4(c,d,e,a,b,78); R4(b,c,d,e,a,79);

	// add the working variables back into sha1->state[]
	sha1->state[0] += a;
	sha1->state[1] += b;
	sha1->state[2] += c;
	sha1->state[3] += d;
	sha1->state[4] += e;

	// wipe variables
	a = b = c = d = e = 0;

	return a + b + c + d + e; // return to avoid dead-store warning from clang static analyzer
}

static void sha1_init(SHA1 *sha1) {
	sha1->state[0] = 0x67452301;
	sha1->state[1] = 0xEFCDAB89;
	sha1->state[2] = 0x98BADCFE;
	sha1->state[3] = 0x10325476;
	sha1->state[4] = 0xC3D2E1F0;
	sha1->count = 0;
}

static void sha1_update(SHA1 *sha1, const uint8_t *data, size_t length) {
	size_t i, j;

	j = (size_t)((sha1->count >> 3) & 63);
	sha1->count += (uint64_t)length << 3;

	if ((j + length) > 63) {
		i = 64 - j;

		memcpy(&sha1->buffer[j], data, i);
		sha1_transform(sha1, sha1->buffer);

		for (; i + 63 < length; i += 64) {
			sha1_transform(sha1, &data[i]);
		}

		j = 0;
	} else {
		i = 0;
	}

	memcpy(&sha1->buffer[j], &data[i], length - i);
}

static void sha1_final(SHA1 *sha1, uint8_t digest[SHA1_DIGEST_LENGTH]) {
	uint32_t i;
	uint8_t count[8];

	for (i = 0; i < 8; i++) {
		// this is endian independent
		count[i] = (uint8_t)((sha1->count >> ((7 - (i & 7)) * 8)) & 255);
	}

	sha1_update(sha1, (uint8_t *)"\200", 1);

	while ((sha1->count & 504) != 448) {
		sha1_update(sha1, (uint8_t *)"\0", 1);
	}

	sha1_update(sha1, count, 8);

	for (i = 0; i < SHA1_DIGEST_LENGTH; i++) {
		digest[i] = (uint8_t)((sha1->state[i >> 2] >> ((3 - (i & 3)) * 8)) & 255);
	}

	memset(sha1, 0, sizeof(*sha1));
}

#undef rol
#undef blk0
#undef blk
#undef R0
#undef R1
#undef R2
#undef R3
#undef R4

/*****************************************************************************
 *
 *                                 Utils
 *
 *****************************************************************************/

static int string_length(const char *s, int max_length) {
	const char *p = s;
	int n = 0;

	while (*p != '\0' && n < max_length) {
		++p;
		++n;
	}

	return n;
}

#ifdef _MSC_VER

// difference between Unix epoch and January 1, 1601 in 100-nanoseconds
#define DELTA_EPOCH 116444736000000000ULL

typedef void (WINAPI *GETSYSTEMTIMEPRECISEASFILETIME)(LPFILETIME);

// implement gettimeofday based on GetSystemTime(Precise)AsFileTime
static int gettimeofday(struct timeval *tv, struct timezone *tz) {
	GETSYSTEMTIMEPRECISEASFILETIME ptr_GetSystemTimePreciseAsFileTime = NULL;
	FILETIME ft;
	uint64_t t;

	(void)tz;

	if (tv != NULL) {
#pragma warning(push)
#pragma warning(disable: 4191) // stop MSVC from warning about casting FARPROC
		ptr_GetSystemTimePreciseAsFileTime =
		  (GETSYSTEMTIMEPRECISEASFILETIME)GetProcAddress(GetModuleHandleA("kernel32"),
														 "GetSystemTimePreciseAsFileTime");
#pragma warning(pop)

		if (ptr_GetSystemTimePreciseAsFileTime != NULL) {
			ptr_GetSystemTimePreciseAsFileTime(&ft);
		} else {
			GetSystemTimeAsFileTime(&ft);
		}

		t = ((uint64_t)ft.dwHighDateTime << 32) | (uint64_t)ft.dwLowDateTime;
		t = (t - DELTA_EPOCH) / 10; // 100-nanoseconds to microseconds

		tv->tv_sec = (long)(t / 1000000UL);
		tv->tv_usec = (long)(t % 1000000UL);
	}

	return 0;
}

#endif

#ifndef _WIN32

static int read_uint32_non_blocking(const char *filename, uint32_t *value) {
	int fd = open(filename, O_NONBLOCK);
	int rc;

	if (fd < 0) {
		return -1;
	}

	rc = (int)read(fd, value, sizeof(uint32_t));

	close(fd);

	return rc != sizeof(uint32_t) ? -1 : 0;
}

#endif

// this function is not meant to be called often,
// this function is meant to provide a good random seed value
static uint32_t get_random_uint32(void) {
	uint32_t r = 0;
	struct timeval tv;
	uint32_t seconds;
	uint32_t microseconds;
#ifdef _WIN32
	HCRYPTPROV hprovider;

	if (!CryptAcquireContext(&hprovider, NULL, NULL, PROV_RSA_FULL,
							 CRYPT_VERIFYCONTEXT | CRYPT_SILENT)) {
		goto fallback;
	}

	if (!CryptGenRandom(hprovider, sizeof(r), (BYTE *)&r)) {
		CryptReleaseContext(hprovider, 0);

		goto fallback;
	}

	CryptReleaseContext(hprovider, 0);
#else
	// try /dev/urandom first, if not available or a read would
	// block then fall back to /dev/random
	if (read_uint32_non_blocking("/dev/urandom", &r) < 0) {
		if (read_uint32_non_blocking("/dev/random", &r) < 0) {
			goto fallback;
		}
	}
#endif

	return r;

fallback:
	// if no other random source is available fall back to the current time
	if (gettimeofday(&tv, NULL) < 0) {
		seconds = (uint32_t)time(NULL);
		microseconds = 0;
	} else {
		seconds = (uint32_t)tv.tv_sec;
		microseconds = tv.tv_usec;
	}

	return (seconds << 26 | seconds >> 6) + microseconds + getpid(); // overflow is intended
}

static void hmac_sha1(uint8_t *secret, int secret_length,
					  uint8_t *data, int data_length,
					  uint8_t digest[SHA1_DIGEST_LENGTH]) {
	SHA1 sha1;
	uint8_t secret_digest[SHA1_DIGEST_LENGTH];
	uint8_t inner_digest[SHA1_DIGEST_LENGTH];
	uint8_t ipad[SHA1_BLOCK_LENGTH];
	uint8_t opad[SHA1_BLOCK_LENGTH];
	int i;

	if (secret_length > SHA1_BLOCK_LENGTH) {
		sha1_init(&sha1);
		sha1_update(&sha1, secret, secret_length);
		sha1_final(&sha1, secret_digest);

		secret = secret_digest;
		secret_length = SHA1_DIGEST_LENGTH;
	}

	// inner digest
	for (i = 0; i < secret_length; ++i) {
		ipad[i] = secret[i] ^ 0x36;
	}

	for (i = secret_length; i < SHA1_BLOCK_LENGTH; ++i) {
		ipad[i] = 0x36;
	}

	sha1_init(&sha1);
	sha1_update(&sha1, ipad, SHA1_BLOCK_LENGTH);
	sha1_update(&sha1, data, data_length);
	sha1_final(&sha1, inner_digest);

	// outer digest
	for (i = 0; i < secret_length; ++i) {
		opad[i] = secret[i] ^ 0x5C;
	}

	for (i = secret_length; i < SHA1_BLOCK_LENGTH; ++i) {
		opad[i] = 0x5C;
	}

	sha1_init(&sha1);
	sha1_update(&sha1, opad, SHA1_BLOCK_LENGTH);
	sha1_update(&sha1, inner_digest, SHA1_DIGEST_LENGTH);
	sha1_final(&sha1, digest);
}

/*****************************************************************************
 *
 *                                 BASE58
 *
 *****************************************************************************/

static const char BASE58_ALPHABET[] = \
	"123456789abcdefghijkmnopqrstuvwxyzABCDEFGHJKLMNPQRSTUVWXYZ";

#if 0

#define BASE58_MAX_STR_SIZE 13

static void base58_encode(uint64_t value, char *str) {
	uint32_t mod;
	char reverse_str[BASE58_MAX_STR_SIZE] = {'\0'};
	int i = 0;
	int k = 0;

	while (value >= 58) {
		mod = value % 58;
		reverse_str[i] = BASE58_ALPHABET[mod];
		value = value / 58;
		++i;
	}

	reverse_str[i] = BASE58_ALPHABET[value];

	for (k = 0; k <= i; k++) {
		str[k] = reverse_str[i - k];
	}

	for (; k < BASE58_MAX_STR_SIZE; k++) {
		str[k] = '\0';
	}
}

#endif

// https://www.fefe.de/intof.html
static bool uint64_add(uint64_t a, uint64_t b, uint64_t *c) {
	if (UINT64_MAX - a < b) {
		return false;
	}

	*c = a + b;

	return true;
}

static bool uint64_multiply(uint64_t a, uint64_t b, uint64_t *c) {
	uint64_t a0 = a & UINT32_MAX;
	uint64_t a1 = a >> 32;
	uint64_t b0 = b & UINT32_MAX;
	uint64_t b1 = b >> 32;
	uint64_t c0;
	uint64_t c1;

	if (a1 > 0 && b1 > 0) {
		return false;
	}

	c1 = a1 * b0 + a0 * b1;

	if (c1 > UINT32_MAX) {
		return false;
	}

	c0 = a0 * b0;
	c1 <<= 32;

	return uint64_add(c1, c0, c);
}

static bool base58_decode(const char *str, uint64_t *ret_value) {
	int i = strlen(str) - 1;
	int k;
	uint64_t next;
	uint64_t value = 0;
	uint64_t base = 1;

	*ret_value = 0;

	for (; i >= 0; --i) {
		for (k = 0; k < 58; ++k) {
			if (BASE58_ALPHABET[k] == str[i]) {
				break;
			}
		}

		if (k == 58) {
			return false; // invalid char
		}

		if (!uint64_multiply(k, base, &next))  {
			return false; // overflow
		}

		if (!uint64_add(value, next, &value))  {
			return false; // overflow
		}

		if (i > 0 && !uint64_multiply(base, 58, &base))  {
			return false; // overflow
		}
	}

	*ret_value = value;

	return true;
}

/*****************************************************************************
 *
 *                                 Socket
 *
 *****************************************************************************/

struct _Socket {
#ifdef _WIN32
	SOCKET handle;
#else
	int handle;
#endif
	Mutex send_mutex; // used to serialize socket_send calls
};

#ifdef _WIN32

static int socket_create(Socket *socket_, int domain, int type, int protocol) {
	BOOL flag = 1;

	socket_->handle = socket(domain, type, protocol);

	if (socket_->handle == INVALID_SOCKET) {
		return -1;
	}

	if (setsockopt(socket_->handle, IPPROTO_TCP, TCP_NODELAY,
				   (const char *)&flag, sizeof(flag)) == SOCKET_ERROR) {
		closesocket(socket_->handle);

		return -1;
	}

	mutex_create(&socket_->send_mutex);

	return 0;
}

static void socket_destroy(Socket *socket) {
	mutex_destroy(&socket->send_mutex);

	closesocket(socket->handle);
}

static int socket_connect(Socket *socket, struct sockaddr *address, int length) {
	return connect(socket->handle, address, length) == SOCKET_ERROR ? -1 : 0;
}

static void socket_shutdown(Socket *socket) {
	shutdown(socket->handle, SD_BOTH);
}

static int socket_receive(Socket *socket, void *buffer, int length) {
	length = recv(socket->handle, (char *)buffer, length, 0);

	if (length == SOCKET_ERROR) {
		length = -1;

		if (WSAGetLastError() == WSAEINTR) {
			errno = EINTR;
		} else {
			errno = EFAULT;
		}
	}

	return length;
}

static int socket_send(Socket *socket, const void *buffer, int length) {
	mutex_lock(&socket->send_mutex);

	length = send(socket->handle, (const char *)buffer, length, 0);

	mutex_unlock(&socket->send_mutex);

	if (length == SOCKET_ERROR) {
		length = -1;
	}

	return length;
}

#else

static int socket_create(Socket *socket_, int domain, int type, int protocol) {
	int flag = 1;

	socket_->handle = socket(domain, type, protocol);

	if (socket_->handle < 0) {
		return -1;
	}

	if (setsockopt(socket_->handle, IPPROTO_TCP, TCP_NODELAY, (void *)&flag,
				   sizeof(flag)) < 0) {
		close(socket_->handle);

		return -1;
	}

	mutex_create(&socket_->send_mutex);

	return 0;
}

static void socket_destroy(Socket *socket) {
	mutex_destroy(&socket->send_mutex);

	close(socket->handle);
}

static int socket_connect(Socket *socket, struct sockaddr *address, int length) {
	return connect(socket->handle, address, length);
}

static void socket_shutdown(Socket *socket) {
	shutdown(socket->handle, SHUT_RDWR);
}

static int socket_receive(Socket *socket, void *buffer, int length) {
	return (int)recv(socket->handle, buffer, length, 0);
}

static int socket_send(Socket *socket, const void *buffer, int length) {
	int rc;

	mutex_lock(&socket->send_mutex);

	rc = (int)send(socket->handle, buffer, length, 0);

	mutex_unlock(&socket->send_mutex);

	return rc;
}

#endif

/*****************************************************************************
 *
 *                                 Mutex
 *
 *****************************************************************************/

#ifdef _WIN32

void mutex_create(Mutex *mutex) {
	InitializeCriticalSection(&mutex->handle);
}

void mutex_destroy(Mutex *mutex) {
	DeleteCriticalSection(&mutex->handle);
}

void mutex_lock(Mutex *mutex) {
	EnterCriticalSection(&mutex->handle);
}

void mutex_unlock(Mutex *mutex) {
	LeaveCriticalSection(&mutex->handle);
}

#else

void mutex_create(Mutex *mutex) {
	pthread_mutex_init(&mutex->handle, NULL);
}

void mutex_destroy(Mutex *mutex) {
	pthread_mutex_destroy(&mutex->handle);
}

void mutex_lock(Mutex *mutex) {
	pthread_mutex_lock(&mutex->handle);
}

void mutex_unlock(Mutex *mutex) {
	pthread_mutex_unlock(&mutex->handle);
}
#endif

/*****************************************************************************
 *
 *                                 Event
 *
 *****************************************************************************/

#ifdef _WIN32

static void event_create(Event *event) {
	event->handle = CreateEvent(NULL, TRUE, FALSE, NULL);
}

static void event_destroy(Event *event) {
	CloseHandle(event->handle);
}

static void event_set(Event *event) {
	SetEvent(event->handle);
}

static void event_reset(Event *event) {
	ResetEvent(event->handle);
}

static int event_wait(Event *event, uint32_t timeout) { // in msec
	return WaitForSingleObject(event->handle, timeout) == WAIT_OBJECT_0 ? 0 : -1;
}

#else

static void event_create(Event *event) {
	pthread_mutex_init(&event->mutex, NULL);
	pthread_cond_init(&event->condition, NULL);

	event->flag = false;
}

static void event_destroy(Event *event) {
	pthread_mutex_destroy(&event->mutex);
	pthread_cond_destroy(&event->condition);
}

static void event_set(Event *event) {
	pthread_mutex_lock(&event->mutex);

	event->flag = true;

	pthread_cond_broadcast(&event->condition);
	pthread_mutex_unlock(&event->mutex);
}

static void event_reset(Event *event) {
	pthread_mutex_lock(&event->mutex);

	event->flag = false;

	pthread_mutex_unlock(&event->mutex);
}

static int event_wait(Event *event, uint32_t timeout) { // in msec
	struct timeval tp;
	struct timespec ts;
	int ret = E_OK;

	gettimeofday(&tp, NULL);

	ts.tv_sec = tp.tv_sec + timeout / 1000;
	ts.tv_nsec = (tp.tv_usec + (timeout % 1000) * 1000) * 1000;

	while (ts.tv_nsec >= 1000000000L) {
		ts.tv_sec += 1;
		ts.tv_nsec -= 1000000000L;
	}

	pthread_mutex_lock(&event->mutex);

	while (!event->flag) {
		ret = pthread_cond_timedwait(&event->condition, &event->mutex, &ts);

		if (ret != 0) {
			ret = E_TIMEOUT;
			break;
		}
	}

	pthread_mutex_unlock(&event->mutex);

	return ret;
}

#endif

/*****************************************************************************
 *
 *                                 Semaphore
 *
 *****************************************************************************/

#ifdef _WIN32

static void semaphore_create(Semaphore *semaphore) {
	semaphore->handle = CreateSemaphore(NULL, 0, INT32_MAX, NULL);
}

static void semaphore_destroy(Semaphore *semaphore) {
	CloseHandle(semaphore->handle);
}

static int semaphore_acquire(Semaphore *semaphore) {
	return WaitForSingleObject(semaphore->handle, INFINITE) != WAIT_OBJECT_0 ? -1 : 0;
}

static void semaphore_release(Semaphore *semaphore) {
	ReleaseSemaphore(semaphore->handle, 1, NULL);
}

#else

static void semaphore_create(Semaphore *semaphore) {
#ifdef __APPLE__
	// Mac OS X does not support unnamed semaphores, so we fake them. Unlink
	// first to ensure that there is no existing semaphore with that name.
	// Then open the semaphore to create a new one. Finally unlink it again to
	// avoid leaking the name. The semaphore will work fine without a name.
	char name[100];

	snprintf(name, sizeof(name), "tf-ipcon-%p", semaphore);

	sem_unlink(name);
	semaphore->pointer = sem_open(name, O_CREAT | O_EXCL, S_IRWXU, 0);
	sem_unlink(name);
#else
	semaphore->pointer = &semaphore->object;

	sem_init(semaphore->pointer, 0, 0);
#endif
}

static void semaphore_destroy(Semaphore *semaphore) {
#ifdef __APPLE__
	sem_close(semaphore->pointer);
#else
	sem_destroy(semaphore->pointer);
#endif
}

static int semaphore_acquire(Semaphore *semaphore) {
	return sem_wait(semaphore->pointer) < 0 ? -1 : 0;
}

static void semaphore_release(Semaphore *semaphore) {
	sem_post(semaphore->pointer);
}

#endif

/*****************************************************************************
 *
 *                                 Thread
 *
 *****************************************************************************/

#ifdef _WIN32

static DWORD WINAPI thread_wrapper(void *opaque) {
	Thread *thread = (Thread *)opaque;

	thread->function(thread->opaque);

	return 0;
}

static int thread_create(Thread *thread, ThreadFunction function, void *opaque) {
	thread->function = function;
	thread->opaque = opaque;

	thread->handle = CreateThread(NULL, 0, thread_wrapper, thread, 0, &thread->id);

	return thread->handle == NULL ? -1 : 0;
}

static void thread_destroy(Thread *thread) {
	CloseHandle(thread->handle);
}

static bool thread_is_current(Thread *thread) {
	return thread->id == GetCurrentThreadId();
}

static void thread_join(Thread *thread) {
	WaitForSingleObject(thread->handle, INFINITE);
}

#else

static void *thread_wrapper(void *opaque) {
	Thread *thread = (Thread *)opaque;

	thread->function(thread->opaque);

	return NULL;
}

static int thread_create(Thread *thread, ThreadFunction function, void *opaque) {
	thread->function = function;
	thread->opaque = opaque;

	return pthread_create(&thread->handle, NULL, thread_wrapper, thread);
}

static void thread_destroy(Thread *thread) {
	(void)thread;
}

static bool thread_is_current(Thread *thread) {
	return pthread_equal(thread->handle, pthread_self()) ? true : false;
}

static void thread_join(Thread *thread) {
	pthread_join(thread->handle, NULL);
}

#endif

/*****************************************************************************
 *
 *                                 Table
 *
 *****************************************************************************/

static void table_create(Table *table) {
	mutex_create(&table->mutex);

	table->used = 0;
	table->allocated = 16;
	table->keys = (uint32_t *)malloc(sizeof(uint32_t) * table->allocated);
	table->values = (void **)malloc(sizeof(void *) * table->allocated);
}

static void table_destroy(Table *table) {
	free(table->keys);
	free(table->values);

	mutex_destroy(&table->mutex);
}

static void *table_insert(Table *table, uint32_t key, void *value) {
	int i;
	void *replaced_value;

	mutex_lock(&table->mutex);

	for (i = 0; i < table->used; ++i) {
		if (table->keys[i] == key) {
			replaced_value = table->values[i];
			table->values[i] = value;

			mutex_unlock(&table->mutex);

			return replaced_value;
		}
	}

	if (table->allocated <= table->used) {
		table->allocated += 16;
		table->keys = (uint32_t *)realloc(table->keys, sizeof(uint32_t) * table->allocated);
		table->values = (void **)realloc(table->values, sizeof(void *) * table->allocated);
	}

	table->keys[table->used] = key;
	table->values[table->used] = value;

	++table->used;

	mutex_unlock(&table->mutex);

	return NULL;
}

static void table_remove(Table *table, uint32_t key) {
	int i;
	int tail;

	mutex_lock(&table->mutex);

	for (i = 0; i < table->used; ++i) {
		if (table->keys[i] == key) {
			tail = table->used - i - 1;

			if (tail > 0) {
				memmove(table->keys + i, table->keys + i + 1, sizeof(uint32_t) * tail);
				memmove(table->values + i, table->values + i + 1, sizeof(void *) * tail);
			}

			--table->used;

			break;
		}
	}

	mutex_unlock(&table->mutex);
}

static void *table_get(Table *table, uint32_t key) {
	int i;
	void *value = NULL;

	mutex_lock(&table->mutex);

	for (i = 0; i < table->used; ++i) {
		if (table->keys[i] == key) {
			value = table->values[i];

			break;
		}
	}

	mutex_unlock(&table->mutex);

	return value;
}

/*****************************************************************************
 *
 *                                 Queue
 *
 *****************************************************************************/

enum {
	QUEUE_KIND_EXIT = 0,
	QUEUE_KIND_DESTROY_AND_EXIT,
	QUEUE_KIND_META,
	QUEUE_KIND_PACKET
};

typedef struct {
	uint8_t function_id;
	uint8_t parameter;
	uint64_t socket_id;
} Meta;

static void queue_create(Queue *queue) {
	queue->head = NULL;
	queue->tail = NULL;

	mutex_create(&queue->mutex);
	semaphore_create(&queue->semaphore);
}

static void queue_destroy(Queue *queue) {
	QueueItem *item = queue->head;
	QueueItem *next;

	while (item != NULL) {
		next = item->next;

		free(item->data);
		free(item);

		item = next;
	}

	mutex_destroy(&queue->mutex);
	semaphore_destroy(&queue->semaphore);
}

static void queue_put(Queue *queue, int kind, void *data) {
	QueueItem *item = (QueueItem *)malloc(sizeof(QueueItem));

	item->next = NULL;
	item->kind = kind;
	item->data = data;

	mutex_lock(&queue->mutex);

	if (queue->tail == NULL) {
		queue->head = item;
		queue->tail = item;
	} else {
		queue->tail->next = item;
		queue->tail = item;
	}

	mutex_unlock(&queue->mutex);
	semaphore_release(&queue->semaphore);
}

static int queue_get(Queue *queue, int *kind, void **data) {
	QueueItem *item;

	if (semaphore_acquire(&queue->semaphore) < 0) {
		return -1;
	}

	mutex_lock(&queue->mutex);

	if (queue->head == NULL) {
		mutex_unlock(&queue->mutex);

		return -1;
	}

	item = queue->head;
	queue->head = item->next;
	item->next = NULL;

	if (queue->tail == item) {
		queue->head = NULL;
		queue->tail = NULL;
	}

	mutex_unlock(&queue->mutex);

	*kind = item->kind;
	*data = item->data;

	free(item);

	return 0;
}

/*****************************************************************************
 *
 *                                 Device
 *
 *****************************************************************************/

enum {
	DEVICE_FUNCTION_ENUMERATE = 254,
	DEVICE_FUNCTION_GET_IDENTITY = 255
};

static int ipcon_send_request(IPConnectionPrivate *ipcon_p, Packet *request);

// NOTE: assumes device_p->ref_count == 0
static void device_destroy(DevicePrivate *device_p) {
	int i;

	if (!device_p->replaced && device_p->uid_valid) {
		table_remove(&device_p->ipcon_p->devices, device_p->uid);
	}

	for (i = 0; i < DEVICE_NUM_FUNCTION_IDS; i++) {
		free(device_p->high_level_callbacks[i].data);
	}

	mutex_destroy(&device_p->stream_mutex);

	event_destroy(&device_p->response_event);

	mutex_destroy(&device_p->response_mutex);

	mutex_destroy(&device_p->request_mutex);

	mutex_destroy(&device_p->device_identifier_mutex);

	free(device_p);
}

void device_create(Device *device, const char *uid_str,
				   IPConnectionPrivate *ipcon_p, uint8_t api_version_major,
				   uint8_t api_version_minor, uint8_t api_version_release,
				   uint16_t device_identifier) {
	DevicePrivate *device_p;
	uint64_t uid;
	uint32_t value1;
	uint32_t value2;
	int i;

	device_p = (DevicePrivate *)malloc(sizeof(DevicePrivate));
	device->p = device_p;

	device_p->replaced = false;

	device_p->uid_valid = base58_decode(uid_str, &uid);

	if (device_p->uid_valid && uid > 0xFFFFFFFF) {
		// convert from 64bit to 32bit
		value1 = uid & 0xFFFFFFFF;
		value2 = (uid >> 32) & 0xFFFFFFFF;

		uid  = (value1 & 0x00000FFF);
		uid |= (value1 & 0x0F000000) >> 12;
		uid |= (value2 & 0x0000003F) << 16;
		uid |= (value2 & 0x000F0000) << 6;
		uid |= (value2 & 0x3F000000) << 2;
	}

	if (uid == 0) {
		device_p->uid_valid = false; // broadcast UID is forbidden
	}

	device_p->ref_count = 1;

	device_p->uid = (uint32_t)uid;

	device_p->ipcon_p = ipcon_p;

	device_p->api_version[0] = api_version_major;
	device_p->api_version[1] = api_version_minor;
	device_p->api_version[2] = api_version_release;

	// device identifier
	device_p->device_identifier = device_identifier;

	mutex_create(&device_p->device_identifier_mutex);

	device_p->device_identifier_check = DEVICE_IDENTIFIER_CHECK_PENDING;

	// request
	mutex_create(&device_p->request_mutex);

	// response
	device_p->expected_response_function_id = 0;
	device_p->expected_response_sequence_number = 0;

	mutex_create(&device_p->response_mutex);

	memset(&device_p->response_packet, 0, sizeof(Packet));

	event_create(&device_p->response_event);

	for (i = 0; i < DEVICE_NUM_FUNCTION_IDS; i++) {
		device_p->response_expected[i] = DEVICE_RESPONSE_EXPECTED_INVALID_FUNCTION_ID;
	}

	// stream
	mutex_create(&device_p->stream_mutex);

	// callbacks
	for (i = 0; i < DEVICE_NUM_FUNCTION_IDS * 2; i++) {
		device_p->registered_callbacks[i] = NULL;
		device_p->registered_callback_user_data[i] = NULL;
	}

	for (i = 0; i < DEVICE_NUM_FUNCTION_IDS; i++) {
		device_p->callback_wrappers[i] = NULL;
		device_p->high_level_callbacks[i].exists = false;
		device_p->high_level_callbacks[i].data = NULL;
		device_p->high_level_callbacks[i].length = 0;
	}
}

void device_release(DevicePrivate *device_p) {
	IPConnectionPrivate *ipcon_p = device_p->ipcon_p;

	mutex_lock(&ipcon_p->devices_ref_mutex);

	--device_p->ref_count;

	if (device_p->ref_count == 0) {
		device_destroy(device_p);
	}

	mutex_unlock(&ipcon_p->devices_ref_mutex);
}

int device_get_response_expected(DevicePrivate *device_p, uint8_t function_id,
								 bool *ret_response_expected) {
	int flag = device_p->response_expected[function_id];

	if (flag == DEVICE_RESPONSE_EXPECTED_INVALID_FUNCTION_ID) {
		return E_INVALID_PARAMETER;
	}

	if (flag == DEVICE_RESPONSE_EXPECTED_ALWAYS_TRUE ||
		flag == DEVICE_RESPONSE_EXPECTED_TRUE) {
		*ret_response_expected = true;
	} else {
		*ret_response_expected = false;
	}

	return E_OK;
}

int device_set_response_expected(DevicePrivate *device_p, uint8_t function_id,
								 bool response_expected) {
	int current_flag = device_p->response_expected[function_id];

	if (current_flag != DEVICE_RESPONSE_EXPECTED_TRUE &&
		current_flag != DEVICE_RESPONSE_EXPECTED_FALSE) {
		return E_INVALID_PARAMETER;
	}

	device_p->response_expected[function_id] =
		response_expected ? DEVICE_RESPONSE_EXPECTED_TRUE
						  : DEVICE_RESPONSE_EXPECTED_FALSE;

	return E_OK;
}

int device_set_response_expected_all(DevicePrivate *device_p, bool response_expected) {
	int flag = response_expected ? DEVICE_RESPONSE_EXPECTED_TRUE
								 : DEVICE_RESPONSE_EXPECTED_FALSE;
	int i;

	for (i = 0; i < DEVICE_NUM_FUNCTION_IDS; ++i) {
		if (device_p->response_expected[i] == DEVICE_RESPONSE_EXPECTED_TRUE ||
			device_p->response_expected[i] == DEVICE_RESPONSE_EXPECTED_FALSE) {
			device_p->response_expected[i] = flag;
		}
	}

	return E_OK;
}

void device_register_callback(DevicePrivate *device_p, int16_t callback_id,
							  void (*function)(void), void *user_data) {
	if (callback_id <= -DEVICE_NUM_FUNCTION_IDS || callback_id >= DEVICE_NUM_FUNCTION_IDS) {
		return;
	}

	device_p->registered_callbacks[DEVICE_NUM_FUNCTION_IDS + callback_id] = function;
	device_p->registered_callback_user_data[DEVICE_NUM_FUNCTION_IDS + callback_id] = user_data;
}

int device_get_api_version(DevicePrivate *device_p, uint8_t ret_api_version[3]) {
	ret_api_version[0] = device_p->api_version[0];
	ret_api_version[1] = device_p->api_version[1];
	ret_api_version[2] = device_p->api_version[2];

	return E_OK;
}

// NOTE: assumes that device_check_validity was successful
int device_send_request(DevicePrivate *device_p, Packet *request, Packet *response,
						int expected_response_length) {
	int ret = E_OK;
	uint8_t sequence_number = packet_header_get_sequence_number(&request->header);
	uint8_t response_expected = packet_header_get_response_expected(&request->header);
	uint8_t error_code;

	if (response_expected) {
		mutex_lock(&device_p->request_mutex);

		event_reset(&device_p->response_event);

		device_p->expected_response_function_id = request->header.function_id;
		device_p->expected_response_sequence_number = sequence_number;
	}

	ret = ipcon_send_request(device_p->ipcon_p, request);

	if (ret != E_OK) {
		if (response_expected) {
			mutex_unlock(&device_p->request_mutex);
		}

		return ret;
	}

	if (response_expected) {
		if (event_wait(&device_p->response_event, device_p->ipcon_p->timeout) < 0) {
			ret = E_TIMEOUT;
		}

		device_p->expected_response_function_id = 0;
		device_p->expected_response_sequence_number = 0;

		event_reset(&device_p->response_event);

		if (ret == E_OK) {
			mutex_lock(&device_p->response_mutex);

			error_code = packet_header_get_error_code(&device_p->response_packet.header);

			if (device_p->response_packet.header.function_id != request->header.function_id ||
				packet_header_get_sequence_number(&device_p->response_packet.header) != sequence_number) {
				ret = E_TIMEOUT;
			} else if (error_code == 0) {
				if (expected_response_length == 0) {
					// setter with response-expected enabled
					expected_response_length = sizeof(PacketHeader);
				}

				if (device_p->response_packet.header.length != expected_response_length) {
					ret = E_WRONG_RESPONSE_LENGTH;
				} else if (response != NULL) {
					memcpy(response, &device_p->response_packet,
						   device_p->response_packet.header.length);
				}
			} else if (error_code == 1) {
				ret = E_INVALID_PARAMETER;
			} else if (error_code == 2) {
				ret = E_NOT_SUPPORTED;
			} else {
				ret = E_UNKNOWN_ERROR_CODE;
			}

			mutex_unlock(&device_p->response_mutex);
		}

		mutex_unlock(&device_p->request_mutex);
	}

	return ret;
}

int device_check_validity(DevicePrivate *device_p) {
	DeviceGetIdentity_Request request;
	DeviceGetIdentity_Response response;
	uint16_t device_identifier;
	int ret;

	if (device_p->replaced) {
		return E_DEVICE_REPLACED;
	}

	if (!device_p->uid_valid) {
		return E_INVALID_UID;
	}

	if (device_p->device_identifier_check == DEVICE_IDENTIFIER_CHECK_PENDING) {
		mutex_lock(&device_p->device_identifier_mutex);

		if (device_p->device_identifier_check == DEVICE_IDENTIFIER_CHECK_PENDING) {
			ret = packet_header_create(&request.header, sizeof(request), DEVICE_FUNCTION_GET_IDENTITY, device_p->ipcon_p, device_p);

			if (ret < 0) {
				mutex_unlock(&device_p->device_identifier_mutex);

				return ret;
			}

			// initialize to 0 to stop the clang static analyzer from warning about accessing
			// uninitialized memory when accessing the device_identifier member later on
			memset(&response, 0, sizeof(response));

			ret = device_send_request(device_p, (Packet *)&request, (Packet *)&response, sizeof(response));

			if (ret < 0) {
				mutex_unlock(&device_p->device_identifier_mutex);

				return ret;
			}

			device_identifier = leconvert_uint16_from(response.device_identifier);

			if (device_identifier == device_p->device_identifier) {
				device_p->device_identifier_check = DEVICE_IDENTIFIER_CHECK_MATCH;
			} else {
				device_p->device_identifier_check = DEVICE_IDENTIFIER_CHECK_MISMATCH;
			}
		}

		mutex_unlock(&device_p->device_identifier_mutex);
	}

	if (device_p->device_identifier_check == DEVICE_IDENTIFIER_CHECK_MISMATCH) {
		return E_WRONG_DEVICE_TYPE;
	}

	return E_OK; // DEVICE_IDENTIFIER_CHECK_MATCH
}

/*****************************************************************************
 *
 *                                 Brick Daemon
 *
 *****************************************************************************/

enum {
	BRICK_DAEMON_FUNCTION_GET_AUTHENTICATION_NONCE = 1,
	BRICK_DAEMON_FUNCTION_AUTHENTICATE = 2
};

static void brickd_create(BrickDaemon *brickd, const char *uid, IPConnection *ipcon) {
	IPConnectionPrivate *ipcon_p = ipcon->p;
	DevicePrivate *device_p;

	device_create(brickd, uid, ipcon_p, 2, 0, 0, 0);

	device_p = brickd->p;

	device_p->response_expected[BRICK_DAEMON_FUNCTION_GET_AUTHENTICATION_NONCE] = DEVICE_RESPONSE_EXPECTED_ALWAYS_TRUE;
	device_p->response_expected[BRICK_DAEMON_FUNCTION_AUTHENTICATE] = DEVICE_RESPONSE_EXPECTED_TRUE;

	ipcon_add_device(ipcon_p, device_p);
}

static void brickd_destroy(BrickDaemon *brickd) {
	device_release(brickd->p);
}

static int brickd_get_authentication_nonce(BrickDaemon *brickd, uint8_t ret_server_nonce[4]) {
	DevicePrivate *device_p = brickd->p;
	BrickDaemonGetAuthenticationNonce_Request request;
	BrickDaemonGetAuthenticationNonce_Response response;
	int ret;

	ret = packet_header_create(&request.header, sizeof(request), BRICK_DAEMON_FUNCTION_GET_AUTHENTICATION_NONCE, device_p->ipcon_p, device_p);

	if (ret < 0) {
		return ret;
	}

	ret = device_send_request(device_p, (Packet *)&request, (Packet *)&response, sizeof(response));

	if (ret < 0) {
		return ret;
	}

	memcpy(ret_server_nonce, response.server_nonce, 4 * sizeof(uint8_t));

	return ret;
}

static int brickd_authenticate(BrickDaemon *brickd, uint8_t client_nonce[4], uint8_t digest[20]) {
	DevicePrivate *device_p = brickd->p;
	BrickDaemonAuthenticate_Request request;
	int ret;

	ret = packet_header_create(&request.header, sizeof(request), BRICK_DAEMON_FUNCTION_AUTHENTICATE, device_p->ipcon_p, device_p);

	if (ret < 0) {
		return ret;
	}

	memcpy(request.client_nonce, client_nonce, 4 * sizeof(uint8_t));
	memcpy(request.digest, digest, 20 * sizeof(uint8_t));

	ret = device_send_request(device_p, (Packet *)&request, NULL, 0);

	return ret;
}

/*****************************************************************************
 *
 *                                 IPConnection
 *
 *****************************************************************************/

struct _CallbackContext {
	IPConnectionPrivate *ipcon_p;
	Queue queue;
	Mutex mutex;
	Thread thread;
	bool packet_dispatch_allowed;
};

static int ipcon_connect_unlocked(IPConnectionPrivate *ipcon_p, bool is_auto_reconnect);
static void ipcon_disconnect_unlocked(IPConnectionPrivate *ipcon_p);

static DevicePrivate *ipcon_acquire_device(IPConnectionPrivate *ipcon_p, uint32_t uid) {
	DevicePrivate *device_p;

	if (uid == 0) {
		return NULL;
	}

	mutex_lock(&ipcon_p->devices_ref_mutex);

	device_p = (DevicePrivate *)table_get(&ipcon_p->devices, uid);

	if (device_p != NULL) {
		++device_p->ref_count;
	}

	mutex_unlock(&ipcon_p->devices_ref_mutex);

	return device_p;
}

static void ipcon_dispatch_meta(IPConnectionPrivate *ipcon_p, Meta *meta) {
	ConnectedCallbackFunction connected_callback_function;
	DisconnectedCallbackFunction disconnected_callback_function;
	void *user_data;
	bool retry;

	if (meta->function_id == IPCON_CALLBACK_CONNECTED) {
		if (ipcon_p->registered_callbacks[IPCON_CALLBACK_CONNECTED] != NULL) {
			connected_callback_function = (ConnectedCallbackFunction)ipcon_p->registered_callbacks[IPCON_CALLBACK_CONNECTED];
			user_data = ipcon_p->registered_callback_user_data[IPCON_CALLBACK_CONNECTED];

			connected_callback_function(meta->parameter, user_data);
		}
	} else if (meta->function_id == IPCON_CALLBACK_DISCONNECTED) {
		// need to do this here, the receive loop is not allowed to
		// hold the socket mutex because this could cause a deadlock
		// with a concurrent call to the (dis-)connect function
		if (meta->parameter != IPCON_DISCONNECT_REASON_REQUEST) {
			mutex_lock(&ipcon_p->socket_mutex);

			// don't close the socket if it got disconnected or
			// reconnected in the meantime
			if (ipcon_p->socket != NULL && ipcon_p->socket_id == meta->socket_id) {
				// destroy disconnect probe thread
				event_set(&ipcon_p->disconnect_probe_event);
				thread_join(&ipcon_p->disconnect_probe_thread);
				thread_destroy(&ipcon_p->disconnect_probe_thread);

				// destroy socket
				socket_destroy(ipcon_p->socket);
				free(ipcon_p->socket);
				ipcon_p->socket = NULL;
			}

			mutex_unlock(&ipcon_p->socket_mutex);
		}

		// FIXME: wait a moment here, otherwise the next connect
		// attempt will succeed, even if there is no open server
		// socket. the first receive will then fail directly
		millisleep(100);

		if (ipcon_p->registered_callbacks[IPCON_CALLBACK_DISCONNECTED] != NULL) {
			disconnected_callback_function = (DisconnectedCallbackFunction)ipcon_p->registered_callbacks[IPCON_CALLBACK_DISCONNECTED];
			user_data = ipcon_p->registered_callback_user_data[IPCON_CALLBACK_DISCONNECTED];

			disconnected_callback_function(meta->parameter, user_data);
		}

		if (meta->parameter != IPCON_DISCONNECT_REASON_REQUEST &&
			ipcon_p->auto_reconnect && ipcon_p->auto_reconnect_allowed) {
			ipcon_p->auto_reconnect_pending = true;
			retry = true;

			// block here until reconnect. this is okay, there is no
			// callback to deliver when there is no connection
			while (retry) {
				retry = false;

				mutex_lock(&ipcon_p->socket_mutex);

				if (ipcon_p->auto_reconnect_allowed && ipcon_p->socket == NULL) {
					if (ipcon_connect_unlocked(ipcon_p, true) < 0) {
						retry = true;
					}
				} else {
					ipcon_p->auto_reconnect_pending = false;
				}

				mutex_unlock(&ipcon_p->socket_mutex);

				if (retry) {
					// wait a moment to give another thread a chance to
					// interrupt the auto-reconnect
					millisleep(100);
				}
			}
		}
	}
}

static void ipcon_dispatch_packet(IPConnectionPrivate *ipcon_p, Packet *packet) {
	EnumerateCallbackFunction enumerate_callback_function;
	void *user_data;
	DeviceEnumerate_Callback *enumerate_callback;
	DevicePrivate *device_p;
	CallbackWrapperFunction callback_wrapper_function;

	if (packet->header.function_id == IPCON_CALLBACK_ENUMERATE) {
		if (ipcon_p->registered_callbacks[IPCON_CALLBACK_ENUMERATE] != NULL) {
			if (packet->header.length != sizeof(DeviceEnumerate_Callback)) {
				return; // silently ignoring callback with wrong length
			}

			enumerate_callback_function = (EnumerateCallbackFunction)ipcon_p->registered_callbacks[IPCON_CALLBACK_ENUMERATE];
			user_data = ipcon_p->registered_callback_user_data[IPCON_CALLBACK_ENUMERATE];
			enumerate_callback = (DeviceEnumerate_Callback *)packet;

			enumerate_callback_function(enumerate_callback->uid,
										enumerate_callback->connected_uid,
										enumerate_callback->position,
										enumerate_callback->hardware_version,
										enumerate_callback->firmware_version,
										leconvert_uint16_from(enumerate_callback->device_identifier),
										enumerate_callback->enumeration_type,
										user_data);
		}
	} else {
		device_p = ipcon_acquire_device(ipcon_p, packet->header.uid);

		if (device_p == NULL) {
			return;
		}

		callback_wrapper_function = device_p->callback_wrappers[packet->header.function_id];

		if (callback_wrapper_function == NULL) {
			device_release(device_p);

			return;
		}

		if (device_check_validity(device_p) < 0) {
			device_release(device_p);

			return; // silently ignoring callback for invalid device
		}

		callback_wrapper_function(device_p, packet);

		device_release(device_p);
	}
}

static void ipcon_destroy_callback_context(CallbackContext *callback) {
	thread_destroy(&callback->thread);
	mutex_destroy(&callback->mutex);
	queue_destroy(&callback->queue);

	free(callback);
}

static void ipcon_exit_callback_thread(CallbackContext *callback) {
	if (!thread_is_current(&callback->thread)) {
		queue_put(&callback->queue, QUEUE_KIND_EXIT, NULL);

		thread_join(&callback->thread);

		ipcon_destroy_callback_context(callback);
	} else {
		queue_put(&callback->queue, QUEUE_KIND_DESTROY_AND_EXIT, NULL);
	}
}

static void ipcon_callback_loop(void *opaque) {
	CallbackContext *callback = (CallbackContext *)opaque;
	int kind;
	void *data;

	while (true) {
		if (queue_get(&callback->queue, &kind, &data) < 0) {
			// FIXME: what to do here? try again? exit? -> yes try again, see https://github.com/Tinkerforge/brickd/issues/21
			break;
		}

		if (kind == QUEUE_KIND_EXIT) {
			break;
		} else if (kind == QUEUE_KIND_DESTROY_AND_EXIT) {
			ipcon_destroy_callback_context(callback);
			break;
		}

		// FIXME: cannot lock callback mutex here because this can
		//        deadlock due to an ordering problem with the socket mutex
		//mutex_lock(&callback->mutex);

		if (kind == QUEUE_KIND_META) {
			ipcon_dispatch_meta(callback->ipcon_p, (Meta *)data);
		} else if (kind == QUEUE_KIND_PACKET) {
			// don't dispatch callbacks when the receive thread isn't running
			if (callback->packet_dispatch_allowed) {
				ipcon_dispatch_packet(callback->ipcon_p, (Packet *)data);
			}
		}

		//mutex_unlock(&callback->mutex);

		free(data);
	}
}

// NOTE: assumes that socket_mutex is locked if disconnect_immediately is true
static void ipcon_handle_disconnect_by_peer(IPConnectionPrivate *ipcon_p,
											uint8_t disconnect_reason,
											uint64_t socket_id,
											bool disconnect_immediately) {
	Meta *meta;

	ipcon_p->auto_reconnect_allowed = true;

	if (disconnect_immediately) {
		ipcon_disconnect_unlocked(ipcon_p);
	}

	meta = (Meta *)malloc(sizeof(Meta));
	meta->function_id = IPCON_CALLBACK_DISCONNECTED;
	meta->parameter = disconnect_reason;
	meta->socket_id = socket_id;

	queue_put(&ipcon_p->callback->queue, QUEUE_KIND_META, meta);
}

enum {
	IPCON_DISCONNECT_PROBE_INTERVAL = 5000
};

enum {
	IPCON_FUNCTION_DISCONNECT_PROBE = 128
};

// NOTE: the disconnect probe loop is not allowed to hold the socket_mutex at any
//       time because it is created and joined while the socket_mutex is locked
static void ipcon_disconnect_probe_loop(void *opaque) {
	IPConnectionPrivate *ipcon_p = (IPConnectionPrivate *)opaque;
	PacketHeader disconnect_probe;

	packet_header_create(&disconnect_probe, sizeof(PacketHeader),
						 IPCON_FUNCTION_DISCONNECT_PROBE, ipcon_p, NULL);

	while (event_wait(&ipcon_p->disconnect_probe_event,
					  IPCON_DISCONNECT_PROBE_INTERVAL) < 0) {
		if (ipcon_p->disconnect_probe_flag) {
			// FIXME: this might block
			if (socket_send(ipcon_p->socket, &disconnect_probe,
							disconnect_probe.length) < 0) {
				ipcon_handle_disconnect_by_peer(ipcon_p, IPCON_DISCONNECT_REASON_ERROR,
												ipcon_p->socket_id, false);
				break;
			}
		} else {
			ipcon_p->disconnect_probe_flag = true;
		}
	}
}

static void ipcon_handle_response(IPConnectionPrivate *ipcon_p, Packet *response) {
	DevicePrivate *device_p;
	uint8_t sequence_number = packet_header_get_sequence_number(&response->header);
	Packet *callback;

	ipcon_p->disconnect_probe_flag = false;

	response->header.uid = leconvert_uint32_from(response->header.uid);

	if (sequence_number == 0 &&
		response->header.function_id == IPCON_CALLBACK_ENUMERATE) {
		if (ipcon_p->registered_callbacks[IPCON_CALLBACK_ENUMERATE] != NULL) {
			callback = (Packet *)malloc(response->header.length);

			memcpy(callback, response, response->header.length);
			queue_put(&ipcon_p->callback->queue, QUEUE_KIND_PACKET, callback);
		}

		return;
	}

	device_p = ipcon_acquire_device(ipcon_p, response->header.uid);

	if (device_p == NULL) {
		// ignoring response for an unknown device
		return;
	}

	if (sequence_number == 0) {
		if (device_p->registered_callbacks[DEVICE_NUM_FUNCTION_IDS + response->header.function_id] != NULL ||
			device_p->high_level_callbacks[response->header.function_id].exists) {
			callback = (Packet *)malloc(response->header.length);

			memcpy(callback, response, response->header.length);
			queue_put(&ipcon_p->callback->queue, QUEUE_KIND_PACKET, callback);
		}

		device_release(device_p);

		return;
	}

	if (device_p->expected_response_function_id == response->header.function_id &&
		device_p->expected_response_sequence_number == sequence_number) {
		mutex_lock(&device_p->response_mutex);
		memcpy(&device_p->response_packet, response, response->header.length);
		mutex_unlock(&device_p->response_mutex);

		event_set(&device_p->response_event);

		device_release(device_p);

		return;
	}

	device_release(device_p);

	// response seems to be OK, but can't be handled
}

// NOTE: the receive loop is now allowed to hold the socket_mutex at any time
//       because it is created and joined while the socket_mutex is locked
static void ipcon_receive_loop(void *opaque) {
	IPConnectionPrivate *ipcon_p = (IPConnectionPrivate *)opaque;
	uint64_t socket_id = ipcon_p->socket_id;
	Packet pending_data[10];
	int pending_length = 0;
	int length;
	uint8_t disconnect_reason;

	while (ipcon_p->receive_flag) {
		length = socket_receive(ipcon_p->socket, (uint8_t *)pending_data + pending_length,
								sizeof(pending_data) - pending_length);

		if (!ipcon_p->receive_flag) {
			return;
		}

		if (length <= 0) {
			if (length < 0 && errno == EINTR) {
				continue;
			}

			if (length == 0) {
				disconnect_reason = IPCON_DISCONNECT_REASON_SHUTDOWN;
			} else {
				disconnect_reason = IPCON_DISCONNECT_REASON_ERROR;
			}

			ipcon_handle_disconnect_by_peer(ipcon_p, disconnect_reason, socket_id, false);
			return;
		}

		pending_length += length;

		while (ipcon_p->receive_flag) {
			if (pending_length < (int)sizeof(PacketHeader)) {
				// wait for complete header
				break;
			}

			length = pending_data[0].header.length;

			if (pending_length < length) {
				// wait for complete packet
				break;
			}

			ipcon_handle_response(ipcon_p, pending_data);

			memmove(pending_data, (uint8_t *)pending_data + length,
					pending_length - length);
			pending_length -= length;
		}
	}
}

// NOTE: assumes that socket is NULL and socket_mutex is locked
static int ipcon_connect_unlocked(IPConnectionPrivate *ipcon_p, bool is_auto_reconnect) {
	char service[32];
	struct addrinfo hints;
	struct addrinfo *resolved = NULL;
	Socket *tmp;
	uint8_t connect_reason;
	Meta *meta;

	// create callback queue and thread
	if (ipcon_p->callback == NULL) {
		ipcon_p->callback = (CallbackContext *)malloc(sizeof(CallbackContext));

		ipcon_p->callback->ipcon_p = ipcon_p;
		ipcon_p->callback->packet_dispatch_allowed = false;

		queue_create(&ipcon_p->callback->queue);
		mutex_create(&ipcon_p->callback->mutex);

		if (thread_create(&ipcon_p->callback->thread, ipcon_callback_loop,
						  ipcon_p->callback) < 0) {
			mutex_destroy(&ipcon_p->callback->mutex);
			queue_destroy(&ipcon_p->callback->queue);

			free(ipcon_p->callback);
			ipcon_p->callback = NULL;

			return E_NO_THREAD;
		}
	}

	// create and connect socket
	snprintf(service, sizeof(service), "%u", ipcon_p->port);

	memset(&hints, 0, sizeof(hints));

	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	if (getaddrinfo(ipcon_p->host, service, &hints, &resolved) != 0) {
		// destroy callback thread
		if (!is_auto_reconnect) {
			ipcon_exit_callback_thread(ipcon_p->callback);
			ipcon_p->callback = NULL;
		}

		return E_HOSTNAME_INVALID;
	}

	tmp = (Socket *)malloc(sizeof(Socket));

	if (socket_create(tmp, resolved->ai_family, resolved->ai_socktype,
					  resolved->ai_protocol) < 0) {
		// destroy callback thread
		if (!is_auto_reconnect) {
			ipcon_exit_callback_thread(ipcon_p->callback);
			ipcon_p->callback = NULL;
		}

		// destroy socket
		free(tmp);
		freeaddrinfo(resolved);

		return E_NO_STREAM_SOCKET;
	}

	if (socket_connect(tmp, resolved->ai_addr, resolved->ai_addrlen) < 0) {
		// destroy callback thread
		if (!is_auto_reconnect) {
			ipcon_exit_callback_thread(ipcon_p->callback);
			ipcon_p->callback = NULL;
		}

		// destroy socket
		socket_destroy(tmp);
		free(tmp);
		freeaddrinfo(resolved);

		return E_NO_CONNECT;
	}

	freeaddrinfo(resolved);

	ipcon_p->socket = tmp;
	++ipcon_p->socket_id;

	// create disconnect probe thread
	ipcon_p->disconnect_probe_flag = true;

	event_reset(&ipcon_p->disconnect_probe_event);

	if (thread_create(&ipcon_p->disconnect_probe_thread,
					  ipcon_disconnect_probe_loop, ipcon_p) < 0) {
		// destroy callback thread
		if (!is_auto_reconnect) {
			ipcon_exit_callback_thread(ipcon_p->callback);
			ipcon_p->callback = NULL;
		}

		// destroy socket
		socket_destroy(ipcon_p->socket);
		free(ipcon_p->socket);
		ipcon_p->socket = NULL;

		return E_NO_THREAD;
	}

	// create receive thread
	ipcon_p->receive_flag = true;
	ipcon_p->callback->packet_dispatch_allowed = true;

	if (thread_create(&ipcon_p->receive_thread, ipcon_receive_loop, ipcon_p) < 0) {
		ipcon_p->receive_flag = false;

		// destroy socket
		ipcon_disconnect_unlocked(ipcon_p);

		// destroy callback thread
		if (!is_auto_reconnect) {
			ipcon_exit_callback_thread(ipcon_p->callback);
			ipcon_p->callback = NULL;
		}

		return E_NO_THREAD;
	}

	ipcon_p->auto_reconnect_allowed = false;
	ipcon_p->auto_reconnect_pending = false;

	// trigger connected callback
	if (is_auto_reconnect) {
		connect_reason = IPCON_CONNECT_REASON_AUTO_RECONNECT;
	} else {
		connect_reason = IPCON_CONNECT_REASON_REQUEST;
	}

	meta = (Meta *)malloc(sizeof(Meta));
	meta->function_id = IPCON_CALLBACK_CONNECTED;
	meta->parameter = connect_reason;
	meta->socket_id = 0;

	queue_put(&ipcon_p->callback->queue, QUEUE_KIND_META, meta);

	return E_OK;
}

// NOTE: assumes that socket is not NULL and socket_mutex is locked
static void ipcon_disconnect_unlocked(IPConnectionPrivate *ipcon_p) {
	// destroy disconnect probe thread
	event_set(&ipcon_p->disconnect_probe_event);
	thread_join(&ipcon_p->disconnect_probe_thread);
	thread_destroy(&ipcon_p->disconnect_probe_thread);

	// stop dispatching packet callbacks before ending the receive
	// thread to avoid timeout exceptions due to callback functions
	// trying to call getters
	if (!thread_is_current(&ipcon_p->callback->thread)) {
		// FIXME: cannot lock callback mutex here because this can
		//        deadlock due to an ordering problem with the socket mutex
		//mutex_lock(&ipcon->callback->mutex);

		ipcon_p->callback->packet_dispatch_allowed = false;

		//mutex_unlock(&ipcon->callback->mutex);
	} else {
		ipcon_p->callback->packet_dispatch_allowed = false;
	}

	// destroy receive thread
	if (ipcon_p->receive_flag) {
		ipcon_p->receive_flag = false;

		socket_shutdown(ipcon_p->socket);

		thread_join(&ipcon_p->receive_thread);
		thread_destroy(&ipcon_p->receive_thread);
	}

	// destroy socket
	socket_destroy(ipcon_p->socket);
	free(ipcon_p->socket);
	ipcon_p->socket = NULL;
}

static int ipcon_send_request(IPConnectionPrivate *ipcon_p, Packet *request) {
	int ret = E_OK;

	mutex_lock(&ipcon_p->socket_mutex);

	if (ipcon_p->socket == NULL) {
		ret = E_NOT_CONNECTED;
	}

	if (ret == E_OK) {
		if (socket_send(ipcon_p->socket, request, request->header.length) < 0) {
			ipcon_handle_disconnect_by_peer(ipcon_p, IPCON_DISCONNECT_REASON_ERROR, 0, true);

			ret = E_NOT_CONNECTED;
		} else {
			ipcon_p->disconnect_probe_flag = false;
		}
	}

	mutex_unlock(&ipcon_p->socket_mutex);

	return ret;
}

void ipcon_create(IPConnection *ipcon) {
	IPConnectionPrivate *ipcon_p;
	int i;

	ipcon_p = (IPConnectionPrivate *)malloc(sizeof(IPConnectionPrivate));
	ipcon->p = ipcon_p;

#ifdef _WIN32
	ipcon_p->wsa_startup_done = false;
#endif

	ipcon_p->host = NULL;
	ipcon_p->port = 0;

	ipcon_p->timeout = 2500;

	ipcon_p->auto_reconnect = true;
	ipcon_p->auto_reconnect_allowed = false;
	ipcon_p->auto_reconnect_pending = false;

	mutex_create(&ipcon_p->sequence_number_mutex);
	ipcon_p->next_sequence_number = 0;

	mutex_create(&ipcon_p->authentication_mutex);
	ipcon_p->next_authentication_nonce = 0;

	mutex_create(&ipcon_p->devices_ref_mutex);
	table_create(&ipcon_p->devices);

	for (i = 0; i < IPCON_NUM_CALLBACK_IDS; ++i) {
		ipcon_p->registered_callbacks[i] = NULL;
		ipcon_p->registered_callback_user_data[i] = NULL;
	}

	mutex_create(&ipcon_p->socket_mutex);
	ipcon_p->socket = NULL;
	ipcon_p->socket_id = 0;

	ipcon_p->receive_flag = false;

	ipcon_p->callback = NULL;

	ipcon_p->disconnect_probe_flag = false;
	event_create(&ipcon_p->disconnect_probe_event);

	semaphore_create(&ipcon_p->wait);

	brickd_create(&ipcon_p->brickd, "2", ipcon);
}

void ipcon_destroy(IPConnection *ipcon) {
	IPConnectionPrivate *ipcon_p = ipcon->p;

	ipcon_disconnect(ipcon); // FIXME: disable disconnected callback before?

	brickd_destroy(&ipcon_p->brickd);

	mutex_destroy(&ipcon_p->authentication_mutex);

	mutex_destroy(&ipcon_p->sequence_number_mutex);

	table_destroy(&ipcon_p->devices); // FIXME: destroy all devices?
	mutex_destroy(&ipcon_p->devices_ref_mutex);

	mutex_destroy(&ipcon_p->socket_mutex);

	event_destroy(&ipcon_p->disconnect_probe_event);

	semaphore_destroy(&ipcon_p->wait);

	free(ipcon_p->host);

	free(ipcon_p);
}

int ipcon_connect(IPConnection *ipcon, const char *host, uint16_t port) {
	IPConnectionPrivate *ipcon_p = ipcon->p;
	int ret;
#ifdef _WIN32
	WSADATA wsa_data;
#endif

	mutex_lock(&ipcon_p->socket_mutex);

#ifdef _WIN32
	if (!ipcon_p->wsa_startup_done) {
		if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
			mutex_unlock(&ipcon_p->socket_mutex);

			return E_NO_STREAM_SOCKET;
		}

		ipcon_p->wsa_startup_done = true;
	}
#endif

	if (ipcon_p->socket != NULL) {
		mutex_unlock(&ipcon_p->socket_mutex);

		return E_ALREADY_CONNECTED;
	}

	free(ipcon_p->host);

	ipcon_p->host = strdup(host);
	ipcon_p->port = port;

	ret = ipcon_connect_unlocked(ipcon_p, false);

	mutex_unlock(&ipcon_p->socket_mutex);

	return ret;
}

int ipcon_disconnect(IPConnection *ipcon) {
	IPConnectionPrivate *ipcon_p = ipcon->p;
	CallbackContext *callback;
	Meta *meta;

	mutex_lock(&ipcon_p->socket_mutex);

	ipcon_p->auto_reconnect_allowed = false;

	if (ipcon_p->auto_reconnect_pending) {
		// abort pending auto-reconnect
		ipcon_p->auto_reconnect_pending = false;
	} else {
		if (ipcon_p->socket == NULL) {
			mutex_unlock(&ipcon_p->socket_mutex);

			return E_NOT_CONNECTED;
		}

		ipcon_disconnect_unlocked(ipcon_p);
	}

	// destroy callback thread
	callback = ipcon_p->callback;
	ipcon_p->callback = NULL;

	mutex_unlock(&ipcon_p->socket_mutex);

	// do this outside of socket_mutex to allow calling (dis-)connect from
	// the callbacks while blocking on the join call here
	meta = (Meta *)malloc(sizeof(Meta));
	meta->function_id = IPCON_CALLBACK_DISCONNECTED;
	meta->parameter = IPCON_DISCONNECT_REASON_REQUEST;
	meta->socket_id = 0;

	queue_put(&callback->queue, QUEUE_KIND_META, meta);

	ipcon_exit_callback_thread(callback);

	return E_OK;
}

int ipcon_authenticate(IPConnection *ipcon, const char *secret) {
	IPConnectionPrivate *ipcon_p = ipcon->p;
	int ret;
	uint32_t nonces[2]; // server, client
	uint8_t digest[SHA1_DIGEST_LENGTH];
	int i;
	int secret_length;

	secret_length = string_length(secret, IPCON_MAX_SECRET_LENGTH);

	for (i = 0; i < secret_length; ++i) {
		if ((secret[i] & 0x80) != 0) {
			return E_NON_ASCII_CHAR_IN_SECRET;
		}
	}

	mutex_lock(&ipcon_p->authentication_mutex);

	if (ipcon_p->next_authentication_nonce == 0) {
		ipcon_p->next_authentication_nonce = get_random_uint32();
	}

	ret = brickd_get_authentication_nonce(&ipcon_p->brickd, (uint8_t *)nonces);

	if (ret < 0) {
		mutex_unlock(&ipcon_p->authentication_mutex);

		return ret;
	}

	nonces[1] = ipcon_p->next_authentication_nonce++;

	hmac_sha1((uint8_t *)secret, secret_length,
			  (uint8_t *)nonces, sizeof(nonces), digest);

	ret = brickd_authenticate(&ipcon_p->brickd, (uint8_t *)&nonces[1], digest);

	if (ret < 0) {
		mutex_unlock(&ipcon_p->authentication_mutex);

		return ret;
	}

	mutex_unlock(&ipcon_p->authentication_mutex);

	return E_OK;
}

int ipcon_get_connection_state(IPConnection *ipcon) {
	IPConnectionPrivate *ipcon_p = ipcon->p;

	if (ipcon_p->socket != NULL) {
		return IPCON_CONNECTION_STATE_CONNECTED;
	} else if (ipcon_p->auto_reconnect_pending) {
		return IPCON_CONNECTION_STATE_PENDING;
	} else {
		return IPCON_CONNECTION_STATE_DISCONNECTED;
	}
}

void ipcon_set_auto_reconnect(IPConnection *ipcon, bool auto_reconnect) {
	IPConnectionPrivate *ipcon_p = ipcon->p;

	ipcon_p->auto_reconnect = auto_reconnect;

	if (!ipcon_p->auto_reconnect) {
		// abort potentially pending auto reconnect
		ipcon_p->auto_reconnect_allowed = false;
	}
}

bool ipcon_get_auto_reconnect(IPConnection *ipcon) {
	return ipcon->p->auto_reconnect;
}

void ipcon_set_timeout(IPConnection *ipcon, uint32_t timeout) { // in msec
	ipcon->p->timeout = timeout;
}

uint32_t ipcon_get_timeout(IPConnection *ipcon) { // in msec
	return ipcon->p->timeout;
}

int ipcon_enumerate(IPConnection *ipcon) {
	IPConnectionPrivate *ipcon_p = ipcon->p;
	DeviceEnumerate_Broadcast enumerate;
	int ret;

	ret = packet_header_create(&enumerate.header, sizeof(DeviceEnumerate_Broadcast),
							   DEVICE_FUNCTION_ENUMERATE, ipcon_p, NULL);

	if (ret < 0) {
		return ret;
	}

	return ipcon_send_request(ipcon_p, (Packet *)&enumerate);
}

void ipcon_wait(IPConnection *ipcon) {
	semaphore_acquire(&ipcon->p->wait);
}

void ipcon_unwait(IPConnection *ipcon) {
	semaphore_release(&ipcon->p->wait);
}

void ipcon_register_callback(IPConnection *ipcon, int16_t callback_id,
							 void (*function)(void), void *user_data) {
	IPConnectionPrivate *ipcon_p = ipcon->p;

	if (callback_id <= -1 || callback_id >= IPCON_NUM_CALLBACK_IDS) {
		return;
	}

	ipcon_p->registered_callbacks[callback_id] = function;
	ipcon_p->registered_callback_user_data[callback_id] = user_data;
}

void ipcon_add_device(IPConnectionPrivate *ipcon_p, DevicePrivate *device_p) {
	DevicePrivate *replaced_device_p;

	if (device_p->uid_valid) {
		replaced_device_p = (DevicePrivate *)table_insert(&ipcon_p->devices, device_p->uid, device_p);

		if (replaced_device_p != NULL) {
			replaced_device_p->replaced = true;
		}
	}
}

int packet_header_create(PacketHeader *header, uint8_t length,
						 uint8_t function_id, IPConnectionPrivate *ipcon_p,
						 DevicePrivate *device_p) {
	uint8_t sequence_number;
	bool response_expected = false;
	int ret = E_OK;

	mutex_lock(&ipcon_p->sequence_number_mutex);

	sequence_number = ipcon_p->next_sequence_number + 1;
	ipcon_p->next_sequence_number = sequence_number % 15;

	mutex_unlock(&ipcon_p->sequence_number_mutex);

	memset(header, 0, sizeof(PacketHeader));

	if (device_p != NULL) {
		header->uid = leconvert_uint32_to(device_p->uid);
	}

	header->length = length;
	header->function_id = function_id;
	packet_header_set_sequence_number(header, sequence_number);

	if (device_p != NULL) {
		ret = device_get_response_expected(device_p, function_id, &response_expected);
		packet_header_set_response_expected(header, response_expected);
	}

	return ret;
}

uint8_t packet_header_get_sequence_number(PacketHeader *header) {
	return (header->sequence_number_and_options >> 4) & 0x0F;
}

void packet_header_set_sequence_number(PacketHeader *header, uint8_t sequence_number) {
	header->sequence_number_and_options &= ~0xF0;
	header->sequence_number_and_options |= (sequence_number << 4) & 0xF0;
}

uint8_t packet_header_get_response_expected(PacketHeader *header) {
	return (header->sequence_number_and_options >> 3) & 0x01;
}

void packet_header_set_response_expected(PacketHeader *header, bool response_expected) {
	if (response_expected) {
		header->sequence_number_and_options |= 0x01 << 3;
	} else {
		header->sequence_number_and_options &= ~(0x01 << 3);
	}
}

uint8_t packet_header_get_error_code(PacketHeader *header) {
	return (header->error_code_and_future_use >> 6) & 0x03;
}

int16_t leconvert_int16_to(int16_t native) {
	return leconvert_uint16_to(native);
}

uint16_t leconvert_uint16_to(uint16_t native) {
	union {
		uint8_t bytes[2];
		uint16_t little;
	} c;

	c.bytes[0] = (native >> 0) & 0xFF;
	c.bytes[1] = (native >> 8) & 0xFF;

	return c.little;
}

int32_t leconvert_int32_to(int32_t native) {
	return leconvert_uint32_to(native);
}

uint32_t leconvert_uint32_to(uint32_t native) {
	union {
		uint8_t bytes[4];
		uint32_t little;
	} c;

	c.bytes[0] = (native >>  0) & 0xFF;
	c.bytes[1] = (native >>  8) & 0xFF;
	c.bytes[2] = (native >> 16) & 0xFF;
	c.bytes[3] = (native >> 24) & 0xFF;

	return c.little;
}

int64_t leconvert_int64_to(int64_t native) {
	return leconvert_uint64_to(native);
}

uint64_t leconvert_uint64_to(uint64_t native) {
	union {
		uint8_t bytes[8];
		uint64_t little;
	} c;

	c.bytes[0] = (native >>  0) & 0xFF;
	c.bytes[1] = (native >>  8) & 0xFF;
	c.bytes[2] = (native >> 16) & 0xFF;
	c.bytes[3] = (native >> 24) & 0xFF;
	c.bytes[4] = (native >> 32) & 0xFF;
	c.bytes[5] = (native >> 40) & 0xFF;
	c.bytes[6] = (native >> 48) & 0xFF;
	c.bytes[7] = (native >> 56) & 0xFF;

	return c.little;
}

float leconvert_float_to(float native) {
	union {
		uint32_t u;
		float f;
	} c;

	c.f = native;
	c.u = leconvert_uint32_to(c.u);

	return c.f;
}

int16_t leconvert_int16_from(int16_t little) {
	return leconvert_uint16_from(little);
}

uint16_t leconvert_uint16_from(uint16_t little) {
	uint8_t *bytes = (uint8_t *)&little;

	return ((uint16_t)bytes[1] << 8) |
			(uint16_t)bytes[0];
}

int32_t leconvert_int32_from(int32_t little) {
	return leconvert_uint32_from(little);
}

uint32_t leconvert_uint32_from(uint32_t little) {
	uint8_t *bytes = (uint8_t *)&little;

	return ((uint32_t)bytes[3] << 24) |
		   ((uint32_t)bytes[2] << 16) |
		   ((uint32_t)bytes[1] <<  8) |
			(uint32_t)bytes[0];
}

int64_t leconvert_int64_from(int64_t little) {
	return leconvert_uint64_from(little);
}

uint64_t leconvert_uint64_from(uint64_t little) {
	uint8_t *bytes = (uint8_t *)&little;

	return ((uint64_t)bytes[7] << 56) |
		   ((uint64_t)bytes[6] << 48) |
		   ((uint64_t)bytes[5] << 40) |
		   ((uint64_t)bytes[4] << 32) |
		   ((uint64_t)bytes[3] << 24) |
		   ((uint64_t)bytes[2] << 16) |
		   ((uint64_t)bytes[1] <<  8) |
			(uint64_t)bytes[0];
}

float leconvert_float_from(float little) {
	union {
		uint32_t u;
		float f;
	} c;

	c.f = little;
	c.u = leconvert_uint32_from(c.u);

	return c.f;
}

char *string_copy(char *dest, const char *src, size_t n) {
	size_t idx = 0;

	while(src[idx] != '\0' && idx < n) {
		dest[idx] = src[idx];
		++idx;
	}

	while (idx < n) {
		dest[idx] = '\0';
		++idx;
	}

	return dest;
}


#ifdef __cplusplus
}
#endif
