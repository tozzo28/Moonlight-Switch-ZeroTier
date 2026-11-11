/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * (c) ZeroTier, Inc.
 * https://www.zerotier.com/
 */

#include "Identity.hpp"

#include "Constants.hpp"
#include "ECC.hpp"
#include "SHA512.hpp"
#include "Salsa20.hpp"
#include "Utils.hpp"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// These can't be changed without a new identity type. They define the
// parameters of the hashcash hashing/searching algorithm.

#define ZT_IDENTITY_GEN_HASHCASH_FIRST_BYTE_LESS_THAN 17
#define ZT_IDENTITY_GEN_MEMORY						  2097152

namespace ZeroTier {

// A memory-hard composition of SHA-512 and Salsa20 for hashcash hashing
static inline void _computeMemoryHardHash(const void* publicKey, unsigned int publicKeyBytes, void* digest, void* genmem)
{
	// Digest publicKey[] to obtain initial digest
	SHA512(digest, publicKey, publicKeyBytes);

	// Initialize genmem[] using Salsa20 in a CBC-like configuration since
	// ordinary Salsa20 is randomly seek-able. This is good for a cipher
	// but is not what we want for sequential memory-hardness.
	memset(genmem, 0, ZT_IDENTITY_GEN_MEMORY);
	Salsa20 s20(digest, (char*)digest + 32);
	s20.crypt20((char*)genmem, (char*)genmem, 64);
	for (unsigned long i = 64; i < ZT_IDENTITY_GEN_MEMORY; i += 64) {
		unsigned long k = i - 64;
		*((uint64_t*)((char*)genmem + i)) = *((uint64_t*)((char*)genmem + k));
		*((uint64_t*)((char*)genmem + i + 8)) = *((uint64_t*)((char*)genmem + k + 8));
		*((uint64_t*)((char*)genmem + i + 16)) = *((uint64_t*)((char*)genmem + k + 16));
		*((uint64_t*)((char*)genmem + i + 24)) = *((uint64_t*)((char*)genmem + k + 24));
		*((uint64_t*)((char*)genmem + i + 32)) = *((uint64_t*)((char*)genmem + k + 32));
		*((uint64_t*)((char*)genmem + i + 40)) = *((uint64_t*)((char*)genmem + k + 40));
		*((uint64_t*)((char*)genmem + i + 48)) = *((uint64_t*)((char*)genmem + k + 48));
		*((uint64_t*)((char*)genmem + i + 56)) = *((uint64_t*)((char*)genmem + k + 56));
		s20.crypt20((char*)genmem + i, (char*)genmem + i, 64);
	}

	// Render final digest using genmem as a lookup table
	for (unsigned long i = 0; i < (ZT_IDENTITY_GEN_MEMORY / sizeof(uint64_t));) {
		unsigned long idx1 = (unsigned long)(Utils::ntoh(((uint64_t*)genmem)[i++]) % (64 / sizeof(uint64_t)));
		unsigned long idx2 = (unsigned long)(Utils::ntoh(((uint64_t*)genmem)[i++]) % (ZT_IDENTITY_GEN_MEMORY / sizeof(uint64_t)));
		uint64_t tmp = ((uint64_t*)genmem)[idx2];
		((uint64_t*)genmem)[idx2] = ((uint64_t*)digest)[idx1];
		((uint64_t*)digest)[idx1] = tmp;
		s20.crypt20(digest, digest, 64);
	}
}

// Hashcash generation halting condition -- halt when first byte is less than
// threshold value.
struct _Identity_generate_cond {
	_Identity_generate_cond()
	{
	}
	_Identity_generate_cond(unsigned char* sb, char* gm) : digest(sb), genmem(gm)
	{
	}
	inline bool operator()(const ECC::Pair& kp) const
	{
		_computeMemoryHardHash(kp.pub.data, ZT_ECC_PUBLIC_KEY_SET_LEN, digest, genmem);
		return (digest[0] < ZT_IDENTITY_GEN_HASHCASH_FIRST_BYTE_LESS_THAN);
	}
	unsigned char* digest;
	char* genmem;
};

#ifdef __SWITCH__
/* Horizon: this is a 2 MB allocation, and a sysmodule's heap is a fixed arena
 * of a few megabytes. libstratosphere's operator new[] is a plain std::malloc
 * -- it returns null on failure rather than throwing, and neither caller below
 * checks, so an exhausted arena turns into a memset of 2 MB starting at address
 * zero. That is a Data Abort on boot, which is how this was found.
 *
 * A `static char s_genmem[2 MB]` fixed that and caused a worse bug: 2 MB of
 * .bss, resident for the life of the process, for a buffer used twice per boot
 * for a few hundred milliseconds. On HOS 20.0.0+ Atmosphere can only move 14 MB
 * from the applet pool into the SYSTEM resource limit group, shared by every
 * custom sysmodule -- and `am` started aborting at boot with 2001-0132,
 * svc::ResultLimitReached, because this buffer was a quarter of what we cost.
 *
 * So it comes from the process heap now, which svcSetHeapSize charges only
 * while it is mapped, and goes straight back afterwards. The port layer owns
 * that; see ztnx_genmem_alloc in sys-zerotier/source/zt_port.cpp. Both callers
 * run on the node thread, so no lock is needed.
 */
extern "C" char *ztnx_genmem_alloc(void);
extern "C" void ztnx_genmem_free(char *p);
#define ZT_GENMEM_ALLOC() (ztnx_genmem_alloc())
#define ZT_GENMEM_FREE(p) (ztnx_genmem_free(p))
#else
#define ZT_GENMEM_ALLOC() (new char[ZT_IDENTITY_GEN_MEMORY])
#define ZT_GENMEM_FREE(p) (delete[] (p))
#endif

void Identity::generate()
{
	unsigned char digest[64];
	char* genmem = ZT_GENMEM_ALLOC();

	ECC::Pair kp;
	do {
		kp = ECC::generateSatisfying(_Identity_generate_cond(digest, genmem));
		_address.setTo(digest + 59, ZT_ADDRESS_LENGTH);	  // last 5 bytes are address
	} while (_address.isReserved());

	_publicKey = kp.pub;
	if (! _privateKey) {
		_privateKey = new ECC::Private();
	}
	*_privateKey = kp.priv;

	ZT_GENMEM_FREE(genmem);
}

bool Identity::locallyValidate() const
{
	if (_address.isReserved()) {
		return false;
	}

	unsigned char digest[64];
	char* genmem = ZT_GENMEM_ALLOC();
	_computeMemoryHardHash(_publicKey.data, ZT_ECC_PUBLIC_KEY_SET_LEN, digest, genmem);
	ZT_GENMEM_FREE(genmem);

	unsigned char addrb[5];
	_address.copyTo(addrb, 5);

	return ((digest[0] < ZT_IDENTITY_GEN_HASHCASH_FIRST_BYTE_LESS_THAN) && (digest[59] == addrb[0]) && (digest[60] == addrb[1]) && (digest[61] == addrb[2]) && (digest[62] == addrb[3]) && (digest[63] == addrb[4]));
}

char* Identity::toString(bool includePrivate, char buf[ZT_IDENTITY_STRING_BUFFER_LENGTH]) const
{
	char* p = buf;
	Utils::hex10(_address.toInt(), p);
	p += 10;
	*(p++) = ':';
	*(p++) = '0';
	*(p++) = ':';
	Utils::hex(_publicKey.data, ZT_ECC_PUBLIC_KEY_SET_LEN, p);
	p += ZT_ECC_PUBLIC_KEY_SET_LEN * 2;
	if ((_privateKey) && (includePrivate)) {
		*(p++) = ':';
		Utils::hex(_privateKey->data, ZT_ECC_PRIVATE_KEY_SET_LEN, p);
		p += ZT_ECC_PRIVATE_KEY_SET_LEN * 2;
	}
	*p = (char)0;
	return buf;
}

bool Identity::fromString(const char* str)
{
	if (! str) {
		_address.zero();
		return false;
	}
	char tmp[ZT_IDENTITY_STRING_BUFFER_LENGTH];
	if (! Utils::scopy(tmp, sizeof(tmp), str)) {
		_address.zero();
		return false;
	}

	delete _privateKey;
	_privateKey = (ECC::Private*)0;

	int fno = 0;
	char* saveptr = (char*)0;
	for (char* f = Utils::stok(tmp, ":", &saveptr); (f); f = Utils::stok((char*)0, ":", &saveptr)) {
		switch (fno++) {
			case 0:
				_address = Address(Utils::hexStrToU64(f));
				if (_address.isReserved()) {
					_address.zero();
					return false;
				}
				break;
			case 1:
				if ((f[0] != '0') || (f[1])) {
					_address.zero();
					return false;
				}
				break;
			case 2:
				if (Utils::unhex(f, _publicKey.data, ZT_ECC_PUBLIC_KEY_SET_LEN) != ZT_ECC_PUBLIC_KEY_SET_LEN) {
					_address.zero();
					return false;
				}
				break;
			case 3:
				_privateKey = new ECC::Private();
				if (Utils::unhex(f, _privateKey->data, ZT_ECC_PRIVATE_KEY_SET_LEN) != ZT_ECC_PRIVATE_KEY_SET_LEN) {
					_address.zero();
					return false;
				}
				break;
			default:
				_address.zero();
				return false;
		}
	}
	if (fno < 3) {
		_address.zero();
		return false;
	}

	return true;
}

}	// namespace ZeroTier
