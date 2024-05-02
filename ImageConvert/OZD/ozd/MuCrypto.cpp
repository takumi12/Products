#include "stdafx.h"
#include "MuCrypto.h"
#include <vector>
#include "cryptopp/tea.h"
#include "cryptopp/3way.h"
#include "cryptopp/cast.h"
#include "cryptopp/rc5.h"
#include "cryptopp/rc6.h"
#include "cryptopp/mars.h"
#include "cryptopp/idea.h"
#include "cryptopp/gost.h"

MuCrypto gMuCrypto;

bool MuCrypto::InitModulusCrypto(DWORD algorithm, BYTE* key, size_t keyLength)
{
	typedef ConcreteCipher < CryptoPP::TEA, 1024 * 8 >			Cipher0;
	typedef ConcreteCipher < CryptoPP::ThreeWay, 1024 * 8 >		Cipher1;
	typedef ConcreteCipher < CryptoPP::CAST128, 1024 * 8 >		Cipher2;
	typedef ConcreteCipher < CryptoPP::RC5, 1024 * 8 >			Cipher3;
	typedef ConcreteCipher < CryptoPP::RC6, 1024 * 8 >			Cipher4;
	typedef ConcreteCipher < CryptoPP::MARS, 1024 * 8 >			Cipher5;
	typedef ConcreteCipher < CryptoPP::IDEA, 1024 * 8 >			Cipher6;
	typedef ConcreteCipher < CryptoPP::GOST, 1024 * 8 >			Cipher7;

	if (this->m_cipher) delete this->m_cipher;

	switch (algorithm & 7)
	{
	case 0:
		this->m_cipher = new Cipher0();
		break;
	case 1:
		this->m_cipher = new Cipher1();
		break;
	case 2:
		this->m_cipher = new Cipher2();
		break;
	case 3:
		this->m_cipher = new Cipher3();
		break;
	case 4:
		this->m_cipher = new Cipher4();
		break;
	case 5:
		this->m_cipher = new Cipher5();
		break;
	case 6:
		this->m_cipher = new Cipher6();
		break;
	case 7:
		this->m_cipher = new Cipher7();
		break;
	default:
		this->m_cipher = new Cipher7();
		break;
	}

	return this->m_cipher->Init(key, keyLength);
}

int MuCrypto::BlockEncrypt(BYTE* inBuf, size_t len, BYTE* outBuf)
{
	if (this->m_cipher)
		return this->m_cipher->Encrypt(inBuf, len, outBuf);

	return -1;
}

int MuCrypto::BlockDecrypt(BYTE* inBuf, size_t len, BYTE* outBuf)
{
	if (this->m_cipher)
		return this->m_cipher->Decrypt(inBuf, len, outBuf);

	return -1;
}

BOOL MuCrypto::ModulusEncrypt(std::vector<BYTE>& buf)
{
	if (!buf.size()) return FALSE;

	BYTE key_1[33] = _MU_MODULUS_KEY_;
	BYTE key_2[33] = _MU_MODULUS_KEY_;	//able to pick any random key_2
	DWORD algorithm_1 = std::rand() % 8;
	DWORD algorithm_2 = std::rand() % 8;

	size_t data_size = buf.size();
	size_t size = data_size + 34;

	buf.insert(buf.begin(), std::begin(key_2), std::begin(key_2) + 32);
	buf.insert(buf.begin(), algorithm_1);
	buf.insert(buf.begin(), algorithm_2);

	InitModulusCrypto(algorithm_2, key_2, 32);
	size_t block_size = data_size - (data_size % GetBlockSize());

	BYTE* block = &buf[34];
	BlockEncrypt(block, block_size, block);

	InitModulusCrypto(algorithm_1, key_1, 32);
	block_size = 1024 - (1024 % GetBlockSize());

	if (data_size > block_size)
	{
		block = &buf[2];
		BlockEncrypt(block, block_size, block);
		block = &buf[size - block_size];
		BlockEncrypt(block, block_size, block);
	}

	if (data_size > (4 * block_size))
	{
		block = &buf[2 + (data_size >> 1)];
		BlockEncrypt(block, block_size, block);
	}

	return TRUE;
}

BOOL MuCrypto::ModulusDecryptv2(std::vector<BYTE>& buf)
{
	if (buf.size() < 34) return FALSE;

	BYTE key_1[33] = _MU_MODULUS_KEY_;
	BYTE key_2[33] = { 0 };
	DWORD algorithm_1 = buf[1];
	DWORD algorithm_2 = buf[0];

	size_t size = buf.size();
	size_t data_size = size - 34;

	InitModulusCrypto(algorithm_1, key_1, 32);
	size_t block_size = 1024 - (1024 % GetBlockSize());

	BYTE* block;

	if (data_size > (4 * block_size))
	{
		block = &buf[2 + (data_size >> 1)];
		BlockDecrypt(block, block_size, block);
	}

	if (data_size > block_size)
	{
		block = &buf[size - block_size];
		BlockDecrypt(block, block_size, block);
		block = &buf[2];
		BlockDecrypt(block, block_size, block);
	}

	memcpy(key_2, &buf[2], 32);

	InitModulusCrypto(algorithm_2, key_2, 32);
	block_size = data_size - (data_size % GetBlockSize());

	block = &buf[34];
	BlockDecrypt(block, block_size, block);

	buf.erase(buf.begin(), buf.begin() + 34);

	return TRUE;
}


