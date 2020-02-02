//---------------------------------------------------------------------------

#include "stdafx.h"
#include "desam.h"

DES::DES() : m_outbuffer(NULL)
{
	m_defaultkey[0] = 0x1a;
	m_defaultkey[1] = 0x2a;
	m_defaultkey[2] = 0x1e; //dummy key
	m_defaultkey[3] = 0x11; //dummy key
	m_defaultkey[4] = 0x1f; //dummy key
	m_defaultkey[5] = 0x21;
	m_defaultkey[6] = 0xea;
	m_defaultkey[7] = 0xd1;
}

DES::~DES()
{
	delete [] m_outbuffer;
	m_outbuffer = NULL;
}

int GetValidLength(int nOldLength)
{
	int nContentSize = nOldLength+1;
	if ((nContentSize % 8)) //����8�ı���
	{
		nContentSize += 8 - nContentSize % 8;
	}
	return nContentSize;
}

int DES::EncryptFile(char* key, char* infile, char* outfile)
{
	int nRet = -1;
	m_defaultkey[2] = 0x3e;
	m_defaultkey[3] = 0xf1;
	m_defaultkey[4] = 0xff;

	try
	{
		FILE* iFileHandle;
		int iFileLength;
		char *pszBuffer;

		if (!key)
			key = m_defaultkey;

		//	iFileHandle = FileOpen(infile, fmOpenRead | fmShareDenyWrite);bcb����
		//	iFileLength = FileSeek(iFileHandle,0,2);
		iFileHandle = fopen(infile,"rb");
		fseek(iFileHandle,0,SEEK_END);
		iFileLength = ftell(iFileHandle);

		int nContentSize = GetValidLength(iFileLength);
		const int nRevSize = sizeof(int);
		int nFileSize = nContentSize + nRevSize;
		pszBuffer = new char[nFileSize];
		char* pContentBuf = pszBuffer + nRevSize;

		//	FileSeek(iFileHandle,0,0);bcb����
		//iBytesRead =
		//	FileRead(iFileHandle, pContentBuf, iFileLength);
		//	FileClose(iFileHandle);
		fseek(iFileHandle,0,0);
		fread(pContentBuf,sizeof(char),1,iFileHandle);
		fclose(iFileHandle);

		*(int*)pszBuffer = iFileLength;

		//encrypt
		//	int hNewFile = FileCreate(outfile, fmOpenWrite | fmCreate);
		FILE* hNewFile = fopen(outfile,"w+b");
		encrypt(key, pContentBuf, nContentSize / 8);
		encrypt(key, pContentBuf, nContentSize / 8);
		encrypt(key, pContentBuf, nContentSize / 8);
		//	FileWrite(hNewFile, pszBuffer, nFileSize);
		//	FileClose(hNewFile);
		fwrite(pszBuffer,sizeof(char),nFileSize,hNewFile);
		fclose(hNewFile);
		delete [] pszBuffer;
		nRet = 1;
	}	
	catch(...)
	{
		//Application->MessageBox("Can't perform one of the following file operations: Open, Seek, Read, Close.", "File Error", IDOK);
		nRet = -1;
	}
	return nRet;
}

int DES::MyDecryptFile(char* key, const char* infile, char** outbuffer, int* size)
{
	int nRet = -1;
	m_defaultkey[2] = 0x3e;
	m_defaultkey[3] = 0xf1;
	m_defaultkey[4] = 0xff;

	try
	{
		FILE* iFileHandle;
		int iFileLength;
		char *pszBuffer;
		const int nRevSize = sizeof(int);
		try
		{
			if (!key)
				key = m_defaultkey;

			//    iFileHandle = FileOpen(infile, fmOpenRead | fmShareDenyNone);
			//    iFileLength = FileSeek(iFileHandle,0,2);
			iFileHandle = fopen(infile,"rb");
			fseek(iFileHandle,0,SEEK_END);
			iFileLength = ftell(iFileHandle);

			int nFileSize = iFileLength;//nContentSize + nRevSize;
			pszBuffer = new char[nFileSize];
			if (m_outbuffer) delete [] m_outbuffer;
			m_outbuffer = pszBuffer;

			//    FileSeek(iFileHandle,0,0);
			//iBytesRead =
			//    FileRead(iFileHandle, pszBuffer, iFileLength);
			//    FileClose(iFileHandle);
			fseek(iFileHandle,0,0);
			fread(pszBuffer,sizeof(char),iFileLength,iFileHandle);
			fclose(iFileHandle);

			//decrypt
			int nContentSize = *(int*)pszBuffer;
			*size = nContentSize;
			char* pContentBuf = pszBuffer + nRevSize;
			int nRealSize = GetValidLength(nContentSize);
			decrypt(key, pContentBuf, nRealSize / 8);
			decrypt(key, pContentBuf, nRealSize / 8);
			decrypt(key, pContentBuf, nRealSize / 8);
			*outbuffer = pContentBuf;
		}
		catch(...)
		{
			//delete [] pszBuffer;
		}
		nRet = 1;
	}
	catch(...)
	{
		//Application->MessageBox("Can't perform one of the following file operations: Open, Seek, Read, Close.", "File Error", IDOK);
		nRet = -1;
	}
	return nRet;
}


int DES::encrypt ( char key[8], char* data, int blocks )
{ 
	if ((!data)||(blocks<1))
		return 0;
	deskey ( (unsigned char*)key, ENCRYPT ); 
	des ( (unsigned char*)data, (unsigned char*)data, blocks); 
	return 1; 
}; 

int DES::decrypt ( char key[8], char* data, int blocks ) 
{ 
	if ((!data)||(blocks<1)) 
		return 0; 
	deskey ( (unsigned char*)key, DECRYPT ); 
	des ( (unsigned char*)data, (unsigned char*)data, blocks); 
	return 1; 
}; 

int DES::yencrypt ( char key[8], char* data, int size ) 
{ 
	if ((!data)||(size<1)) 
		return 0; 

	// The last char of data is bitwise complemented and filled the rest 
	// buffer.If size is 16, it will extend to 24,and 17 still 24. 
	char lastChar = *(data+size-1); 
	int blocks = size/8+1; 
	memset (data+size, ~lastChar, blocks*8-size); 
	deskey ( (unsigned char*)key, ENCRYPT ); 
	return encrypt ( data, data, blocks); 
}; 

int DES::ydecrypt ( char key[8], char* data, int blocks, int* size ) 
{ 
	if ( (!data) || (blocks<1) ) 
		return 0; 

	deskey ( (unsigned char*)key, DECRYPT ); 
	if ( !decrypt ( data, data, blocks) ) 
		return 0; 
	if ( size != 0 ) 
	{ 
		int pos = blocks*8-1; 
		char endChar = data[pos]; 
		while ((pos>0)&&(data[pos]==endChar)) 
			pos--; 
		if ( data[pos] != ~endChar ) 
			return 0; 
		*size = pos+1; 
	} 
	return 1; 
}; 

// ----------------------------------------------------------------------- 
// des 
// Encrpts/Decrypts(according to the key currently loaded int the 
// internal key register) SOME blocks of eight bytes at address 'in' 
// into the block at address 'out'. They can be the same. 
// 
// "in" 
// "out" 
// "block" Number of blocks. 
// ----------------------------------------------------------------------- 
void DES::des ( unsigned char* in, unsigned char* out, int blocks ) 
{ 
	for (int i = 0; i < blocks; i++,in+=8,out+=8) 
		des_block(in,out); 
}; 

// ----------------------------------------------------------------------- 
// des_block 
// Encrpts/Decrypts(according to the key currently loaded int the 
// internal key register) one block of eight bytes at address 'in' 
// into the block at address 'out'. They can be the same. 
// 
// "in" 
// "out" 
// ----------------------------------------------------------------------- 
void DES::des_block(unsigned char *in, unsigned char *out) 
{ 
	unsigned long work[2]; 

	scrunch(in, work); 
	desfunc(work, KnL);
	unscrun(work, out);
}

// ---------------------------------------------------------------------- 
// deskey 
// Sets the internal key register (KnR) according to the hexadecimal 
// key contained in the 8 bytes of hexkey, according to the DES, 
// for encryption or decrytion according to MODE 
// 
// "key" is the 64 bits key. 
// "md" means encryption or decryption. 
// ---------------------------------------------------------------------- 
void DES::deskey(unsigned char key[8], Mode md) /* Thanks to James Gillogly & Phil Karn! */ 
{ 
	register int i, j, l, m, n; 
	unsigned char pc1m[56], pcr[56]; 
	unsigned long kn[32]; 

	for (j = 0; j < 56; j++) { 
		l = pc1[j]; 
		m = l & 07; 
		pc1m[j] = (key[l >> 3] & bytebit[m]) ? 1:0; 
	} 
	for (i = 0; i < 16; i++) { 
		if (md == DECRYPT) m = (15 - i) << 1; 
		else m = i << 1; 
		n = m + 1; 
		kn[m] = kn[n] = 0L; 
		for (j = 0; j < 28; j++) { 
			l = j + totrot[i]; 
			if (l < 28) pcr[j] = pc1m[l]; 
			else pcr[j] = pc1m[l - 28]; 
		} 
		for (j = 28; j < 56; j++) { 
			l = j + totrot[i]; 
			if (l < 56) pcr[j] = pc1m[l]; 
			else pcr[j] = pc1m[l - 28]; 
		} 
		for (j = 0; j < 24; j++) { 
			if (pcr[ pc2[j] ]) kn[m] |= bigbyte[j]; 
			if (pcr[ pc2[j+24] ]) kn[n] |= bigbyte[j]; 
		} 
	} 
	cookey(kn); 
	return; 
} 

// ---------------------------------------------------------------------- 
// cookey 
// Only called by deskey. 
// ----------------------------------------------------------------------- 
void DES::cookey(register unsigned long *raw1) 
{ 
	register unsigned long *cook, *raw0; 
	unsigned long dough[32]; 
	register int i; 

	cook = dough; 
	for (i = 0; i < 16; i++, raw1++) { 
		raw0 = raw1++; 
		*cook = (*raw0 & 0x00fc0000L) << 6; 
		*cook |= (*raw0 & 0x00000fc0L) << 10; 
		*cook |= (*raw1 & 0x00fc0000L) >> 10; 
		*cook++ |= (*raw1 & 0x00000fc0L) >> 6; 
		*cook = (*raw0 & 0x0003f000L) << 12; 
		*cook |= (*raw0 & 0x0000003fL) << 16; 
		*cook |= (*raw1 & 0x0003f000L) >> 4; 
		*cook++ |= (*raw1 & 0x0000003fL); 
	} 
	usekey(dough); 
	return; 
} 

// ---------------------------------------------------------------------- 
// usekey 
// Only called by cookey. 
// Loads the interal key register with the data in cookedkey. 
// ----------------------------------------------------------------------- 
void DES::usekey(register unsigned long *from) 
{ 
	register unsigned long *to, *endp; 

	to = KnL, endp = &KnL[32]; 
	while (to < endp) *to++ = *from++; 
	return; 
} 

void DES::scrunch(register unsigned char *outof, register unsigned long *into ) 
{ 
	*into = (*outof++ & 0xffL) << 24; 
	*into |= (*outof++ & 0xffL) << 16; 
	*into |= (*outof++ & 0xffL) << 8; 
	*into++ |= (*outof++ & 0xffL); 
	*into = (*outof++ & 0xffL) << 24; 
	*into |= (*outof++ & 0xffL) << 16; 
	*into |= (*outof++ & 0xffL) << 8; 
	*into |= (*outof & 0xffL); 
	return; 
} 

void DES::unscrun(register unsigned long *outof, register unsigned char *into) 
{ 
	*into++ = (*outof >> 24) & 0xffL; 
	*into++ = (*outof >> 16) & 0xffL; 
	*into++ = (*outof >> 8) & 0xffL; 
	*into++ = *outof++ & 0xffL; 
	*into++ = (*outof >> 24) & 0xffL; 
	*into++ = (*outof >> 16) & 0xffL; 
	*into++ = (*outof >> 8) & 0xffL; 
	*into = *outof & 0xffL; 
	return; 
} 

void DES::desfunc(register unsigned long *block,register unsigned long *keys) 
{ 
	register unsigned long fval, work, right, leftt; 
	register int round; 

	leftt = block[0]; 
	right = block[1]; 
	work = ((leftt >> 4) ^ right) & 0x0f0f0f0fL; 
	right ^= work; 
	leftt ^= (work << 4); 
	work = ((leftt >> 16) ^ right) & 0x0000ffffL; 
	right ^= work; 
	leftt ^= (work << 16); 
	work = ((right >> 2) ^ leftt) & 0x33333333L; 
	leftt ^= work; 
	right ^= (work << 2); 
	work = ((right >> 8) ^ leftt) & 0x00ff00ffL; 
	leftt ^= work; 
	right ^= (work << 8); 
	right = ((right << 1) | ((right >> 31) & 1L)) & 0xffffffffL; 
	work = (leftt ^ right) & 0xaaaaaaaaL; 
	leftt ^= work; 
	right ^= work; 
	leftt = ((leftt << 1) | ((leftt >> 31) & 1L)) & 0xffffffffL; 

	for (round = 0; round < 8; round++) { 
		work = (right << 28) | (right >> 4); 
		work ^= *keys++; 
		fval = SP7[work & 0x3fL]; 
		fval |= SP5[(work >> 8) & 0x3fL]; 
		fval |= SP3[(work >> 16) & 0x3fL]; 
		fval |= SP1[(work >> 24) & 0x3fL]; 
		work = right ^ *keys++; 
		fval |= SP8[work & 0x3fL]; 
		fval |= SP6[(work >> 8) & 0x3fL]; 
		fval |= SP4[(work >> 16) & 0x3fL]; 
		fval |= SP2[(work >> 24) & 0x3fL]; 
		leftt ^= fval; 
		work = (leftt << 28) | (leftt >> 4); 
		work ^= *keys++; 
		fval = SP7[work & 0x3fL]; 
		fval |= SP5[(work >> 8) & 0x3fL]; 
		fval |= SP3[(work >> 16) & 0x3fL]; 
		fval |= SP1[(work >> 24) & 0x3fL]; 
		work = leftt ^ *keys++; 
		fval |= SP8[work & 0x3fL]; 
		fval |= SP6[(work >> 8) & 0x3fL]; 
		fval |= SP4[(work >> 16) & 0x3fL]; 
		fval |= SP2[(work >> 24) & 0x3fL]; 
		right ^= fval; 
	} 
	right = (right << 31) | (right >> 1); 
	work = (leftt ^ right) & 0xaaaaaaaaL; 
	leftt ^= work; 
	right ^= work; 
	leftt = (leftt << 31) | ( leftt >> 1); 
	work = ((leftt >> 8) ^ right) & 0x00ff00ffL; 
	right ^= work; 
	leftt ^= (work << 8); 
	work = ((leftt >> 2) ^ right) & 0x33333333L; 
	right ^= work; 
	leftt ^= (work << 2); 
	work = ((right >> 16) ^ leftt) & 0x0000ffffL; 
	leftt ^= work; 
	right ^= (work << 16); 
	work = ((right >> 4) ^ leftt) & 0x0f0f0f0fL; 
	leftt ^= work; 
	right ^= (work << 4); 
	*block++ = right; 
	*block = leftt; 
	return; 
} 

// ----------------------------------------------------------------------- 
// Initial of static data members. These data will be used by all the 
// instances of class,and can not be changed. 
// ----------------------------------------------------------------------- 
unsigned char DES::Df_Key[24] = 
{ 
	0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef, 
	0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10, 
	0x89, 0xab, 0xcd, 0xef, 0x01, 0x23, 0x45, 0x67 
}; 

unsigned short DES::bytebit[8] = 
{ 
	0200, 0100, 040, 020, 010, 04, 02, 01 
}; 

unsigned long DES::bigbyte[24] = 
{ 
	0x800000L, 0x400000L, 0x200000L, 0x100000L, 
	0x80000L, 0x40000L, 0x20000L, 0x10000L, 
	0x8000L, 0x4000L, 0x2000L, 0x1000L, 
	0x800L, 0x400L, 0x200L, 0x100L, 
	0x80L, 0x40L, 0x20L, 0x10L, 
	0x8L, 0x4L, 0x2L, 0x1L 
}; 

unsigned char DES::pc1[56] = 
{ 
	56, 48, 40, 32, 24, 16, 8, 0, 57, 49, 41, 33, 25, 17, 
	9, 1, 58, 50, 42, 34, 26, 18, 10, 2, 59, 51, 43, 35, 
	62, 54, 46, 38, 30, 22, 14, 6, 61, 53, 45, 37, 29, 21, 
	13, 5, 60, 52, 44, 36, 28, 20, 12, 4, 27, 19, 11, 3 
}; 

unsigned char DES::totrot[16] = 
{ 
	1, 2, 4, 6, 8, 10, 12, 14, 15, 17, 19, 21, 23, 25, 27, 28 
}; 

unsigned char DES::pc2[48] = 
{ 
	13, 16, 10, 23, 0, 4, 2, 27, 14, 5, 20, 9, 
	22, 18, 11, 3, 25, 7, 15, 6, 26, 19, 12, 1, 
	40, 51, 30, 36, 46, 54, 29, 39, 50, 44, 32, 47, 
	43, 48, 38, 55, 33, 52, 45, 41, 49, 35, 28, 31 
}; 

unsigned long DES::SP1[64] = 
{ 
	0x01010400L, 0x00000000L, 0x00010000L, 0x01010404L, 
	0x01010004L, 0x00010404L, 0x00000004L, 0x00010000L, 
	0x00000400L, 0x01010400L, 0x01010404L, 0x00000400L, 
	0x01000404L, 0x01010004L, 0x01000000L, 0x00000004L, 
	0x00000404L, 0x01000400L, 0x01000400L, 0x00010400L, 
	0x00010400L, 0x01010000L, 0x01010000L, 0x01000404L, 
	0x00010004L, 0x01000004L, 0x01000004L, 0x00010004L, 
	0x00000000L, 0x00000404L, 0x00010404L, 0x01000000L, 
	0x00010000L, 0x01010404L, 0x00000004L, 0x01010000L, 
	0x01010400L, 0x01000000L, 0x01000000L, 0x00000400L, 
	0x01010004L, 0x00010000L, 0x00010400L, 0x01000004L, 
	0x00000400L, 0x00000004L, 0x01000404L, 0x00010404L, 
	0x01010404L, 0x00010004L, 0x01010000L, 0x01000404L, 
	0x01000004L, 0x00000404L, 0x00010404L, 0x01010400L, 
	0x00000404L, 0x01000400L, 0x01000400L, 0x00000000L, 
	0x00010004L, 0x00010400L, 0x00000000L, 0x01010004L 
}; 

unsigned long DES::SP2[64] = 
{ 
	0x80108020L, 0x80008000L, 0x00008000L, 0x00108020L, 
	0x00100000L, 0x00000020L, 0x80100020L, 0x80008020L, 
	0x80000020L, 0x80108020L, 0x80108000L, 0x80000000L, 
	0x80008000L, 0x00100000L, 0x00000020L, 0x80100020L, 
	0x00108000L, 0x00100020L, 0x80008020L, 0x00000000L, 
	0x80000000L, 0x00008000L, 0x00108020L, 0x80100000L, 
	0x00100020L, 0x80000020L, 0x00000000L, 0x00108000L, 
	0x00008020L, 0x80108000L, 0x80100000L, 0x00008020L, 
	0x00000000L, 0x00108020L, 0x80100020L, 0x00100000L, 
	0x80008020L, 0x80100000L, 0x80108000L, 0x00008000L, 
	0x80100000L, 0x80008000L, 0x00000020L, 0x80108020L, 
	0x00108020L, 0x00000020L, 0x00008000L, 0x80000000L, 
	0x00008020L, 0x80108000L, 0x00100000L, 0x80000020L, 
	0x00100020L, 0x80008020L, 0x80000020L, 0x00100020L, 
	0x00108000L, 0x00000000L, 0x80008000L, 0x00008020L, 
	0x80000000L, 0x80100020L, 0x80108020L, 0x00108000L 
}; 

unsigned long DES::SP3[64] =
{ 
	0x00000208L, 0x08020200L, 0x00000000L, 0x08020008L, 
	0x08000200L, 0x00000000L, 0x00020208L, 0x08000200L, 
	0x00020008L, 0x08000008L, 0x08000008L, 0x00020000L, 
	0x08020208L, 0x00020008L, 0x08020000L, 0x00000208L, 
	0x08000000L, 0x00000008L, 0x08020200L, 0x00000200L, 
	0x00020200L, 0x08020000L, 0x08020008L, 0x00020208L, 
	0x08000208L, 0x00020200L, 0x00020000L, 0x08000208L, 
	0x00000008L, 0x08020208L, 0x00000200L, 0x08000000L, 
	0x08020200L, 0x08000000L, 0x00020008L, 0x00000208L, 
	0x00020000L, 0x08020200L, 0x08000200L, 0x00000000L, 
	0x00000200L, 0x00020008L, 0x08020208L, 0x08000200L, 
	0x08000008L, 0x00000200L, 0x00000000L, 0x08020008L, 
	0x08000208L, 0x00020000L, 0x08000000L, 0x08020208L, 
	0x00000008L, 0x00020208L, 0x00020200L, 0x08000008L, 
	0x08020000L, 0x08000208L, 0x00000208L, 0x08020000L, 
	0x00020208L, 0x00000008L, 0x08020008L, 0x00020200L 
}; 

unsigned long DES::SP4[64] =
{ 
	0x00802001L, 0x00002081L, 0x00002081L, 0x00000080L, 
	0x00802080L, 0x00800081L, 0x00800001L, 0x00002001L, 
	0x00000000L, 0x00802000L, 0x00802000L, 0x00802081L, 
	0x00000081L, 0x00000000L, 0x00800080L, 0x00800001L, 
	0x00000001L, 0x00002000L, 0x00800000L, 0x00802001L, 
	0x00000080L, 0x00800000L, 0x00002001L, 0x00002080L, 
	0x00800081L, 0x00000001L, 0x00002080L, 0x00800080L, 
	0x00002000L, 0x00802080L, 0x00802081L, 0x00000081L, 
	0x00800080L, 0x00800001L, 0x00802000L, 0x00802081L, 
	0x00000081L, 0x00000000L, 0x00000000L, 0x00802000L, 
	0x00002080L, 0x00800080L, 0x00800081L, 0x00000001L, 
	0x00802001L, 0x00002081L, 0x00002081L, 0x00000080L, 
	0x00802081L, 0x00000081L, 0x00000001L, 0x00002000L, 
	0x00800001L, 0x00002001L, 0x00802080L, 0x00800081L, 
	0x00002001L, 0x00002080L, 0x00800000L, 0x00802001L, 
	0x00000080L, 0x00800000L, 0x00002000L, 0x00802080L 
}; 

unsigned long DES::SP5[64] = 
{ 
	0x00000100L, 0x02080100L, 0x02080000L, 0x42000100L, 
	0x00080000L, 0x00000100L, 0x40000000L, 0x02080000L, 
	0x40080100L, 0x00080000L, 0x02000100L, 0x40080100L, 
	0x42000100L, 0x42080000L, 0x00080100L, 0x40000000L, 
	0x02000000L, 0x40080000L, 0x40080000L, 0x00000000L, 
	0x40000100L, 0x42080100L, 0x42080100L, 0x02000100L, 
	0x42080000L, 0x40000100L, 0x00000000L, 0x42000000L, 
	0x02080100L, 0x02000000L, 0x42000000L, 0x00080100L, 
	0x00080000L, 0x42000100L, 0x00000100L, 0x02000000L, 
	0x40000000L, 0x02080000L, 0x42000100L, 0x40080100L, 
	0x02000100L, 0x40000000L, 0x42080000L, 0x02080100L, 
	0x40080100L, 0x00000100L, 0x02000000L, 0x42080000L, 
	0x42080100L, 0x00080100L, 0x42000000L, 0x42080100L, 
	0x02080000L, 0x00000000L, 0x40080000L, 0x42000000L, 
	0x00080100L, 0x02000100L, 0x40000100L, 0x00080000L, 
	0x00000000L, 0x40080000L, 0x02080100L, 0x40000100L 
}; 

unsigned long DES::SP6[64] = 
{ 
	0x20000010L, 0x20400000L, 0x00004000L, 0x20404010L, 
	0x20400000L, 0x00000010L, 0x20404010L, 0x00400000L, 
	0x20004000L, 0x00404010L, 0x00400000L, 0x20000010L, 
	0x00400010L, 0x20004000L, 0x20000000L, 0x00004010L, 
	0x00000000L, 0x00400010L, 0x20004010L, 0x00004000L, 
	0x00404000L, 0x20004010L, 0x00000010L, 0x20400010L, 
	0x20400010L, 0x00000000L, 0x00404010L, 0x20404000L, 
	0x00004010L, 0x00404000L, 0x20404000L, 0x20000000L, 
	0x20004000L, 0x00000010L, 0x20400010L, 0x00404000L, 
	0x20404010L, 0x00400000L, 0x00004010L, 0x20000010L, 
	0x00400000L, 0x20004000L, 0x20000000L, 0x00004010L, 
	0x20000010L, 0x20404010L, 0x00404000L, 0x20400000L, 
	0x00404010L, 0x20404000L, 0x00000000L, 0x20400010L, 
	0x00000010L, 0x00004000L, 0x20400000L, 0x00404010L, 
	0x00004000L, 0x00400010L, 0x20004010L, 0x00000000L, 
	0x20404000L, 0x20000000L, 0x00400010L, 0x20004010L 
}; 

unsigned long DES::SP7[64] = 
{ 
	0x00200000L, 0x04200002L, 0x04000802L, 0x00000000L, 
	0x00000800L, 0x04000802L, 0x00200802L, 0x04200800L, 
	0x04200802L, 0x00200000L, 0x00000000L, 0x04000002L, 
	0x00000002L, 0x04000000L, 0x04200002L, 0x00000802L, 
	0x04000800L, 0x00200802L, 0x00200002L, 0x04000800L, 
	0x04000002L, 0x04200000L, 0x04200800L, 0x00200002L, 
	0x04200000L, 0x00000800L, 0x00000802L, 0x04200802L, 
	0x00200800L, 0x00000002L, 0x04000000L, 0x00200800L, 
	0x04000000L, 0x00200800L, 0x00200000L, 0x04000802L, 
	0x04000802L, 0x04200002L, 0x04200002L, 0x00000002L, 
	0x00200002L, 0x04000000L, 0x04000800L, 0x00200000L, 
	0x04200800L, 0x00000802L, 0x00200802L, 0x04200800L, 
	0x00000802L, 0x04000002L, 0x04200802L, 0x04200000L, 
	0x00200800L, 0x00000000L, 0x00000002L, 0x04200802L, 
	0x00000000L, 0x00200802L, 0x04200000L, 0x00000800L, 
	0x04000002L, 0x04000800L, 0x00000800L, 0x00200002L 

}; 

unsigned long DES::SP8[64] = 
{ 
	0x10001040L, 0x00001000L, 0x00040000L, 0x10041040L, 
	0x10000000L, 0x10001040L, 0x00000040L, 0x10000000L, 
	0x00040040L, 0x10040000L, 0x10041040L, 0x00041000L, 
	0x10041000L, 0x00041040L, 0x00001000L, 0x00000040L, 
	0x10040000L, 0x10000040L, 0x10001000L, 0x00001040L, 
	0x00041000L, 0x00040040L, 0x10040040L, 0x10041000L, 
	0x00001040L, 0x00000000L, 0x00000000L, 0x10040040L, 
	0x10000040L, 0x10001000L, 0x00041040L, 0x00040000L, 
	0x00041040L, 0x00040000L, 0x10041000L, 0x00001000L, 
	0x00000040L, 0x10040040L, 0x00001000L, 0x00041040L, 
	0x10001000L, 0x00000040L, 0x10000040L, 0x10040000L, 
	0x10040040L, 0x10000000L, 0x00040000L, 0x10001040L, 
	0x00000000L, 0x10041040L, 0x00040040L, 0x10000040L, 
	0x10040000L, 0x10001000L, 0x10001040L, 0x00000000L, 
	0x10041040L, 0x00041000L, 0x00041000L, 0x00001040L, 
	0x00001040L, 0x00040040L, 0x10000000L, 0x10041000L
}; 
