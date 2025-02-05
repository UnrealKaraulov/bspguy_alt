#pragma once
#include <cstring>
#include <string>
#include "bsptypes.h"

int calcMipsSize(int w, int h);

#pragma pack(push, 1)

COLOR4 operator*(COLOR4 v, float f);
bool operator==(COLOR4 c1, COLOR4 c2);

struct WADHEADER
{
	char szMagic[4];    // should be WAD2/WAD3
	int nDir;			// number of directory entries
	int nDirOffset;		// offset into directories
};

struct WADDIRENTRY
{
	int nFilePos;				 // offset in WAD
	int nDiskSize;				 // size in file
	int nSize;					 // uncompressed size
	char nType;					 // type of entry
	bool bCompression;           // 0 if none
	short nDummy;				 // not used
	char szName[MAXTEXTURENAME]; // must be null terminated
};

struct WADTEX
{
	char szName[MAXTEXTURENAME];
	int nWidth, nHeight;
	int nOffsets[MIPLEVELS];
	unsigned char* data; // all mip-maps and pallete
	int dataLen;
	bool needclean;
	WADTEX()
	{
		dataLen = 0;
		needclean = false;
		szName[0] = '\0';
		data = NULL;
		nWidth = nHeight = 0;
		nOffsets[0] = nOffsets[1] = nOffsets[2] = nOffsets[3] = 0;
	}

	WADTEX(BSPMIPTEX* tex, unsigned char* palette = NULL, unsigned short colors = 256)
	{
		if (!tex || tex->nWidth == 0 || tex->nHeight == 0)
		{
			dataLen = 0;
			needclean = false;
			szName[0] = '\0';
			data = NULL;
			nWidth = nHeight = 0;
			nOffsets[0] = nOffsets[1] = nOffsets[2] = nOffsets[3] = 0;
			return;
		}
		memcpy(szName, tex->szName, MAXTEXTURENAME);

		nWidth = tex->nWidth;
		nHeight = tex->nHeight;
		for (int i = 0; i < MIPLEVELS; i++)
			nOffsets[i] = tex->nOffsets[i]/* - sizeof(BSPMIPTEX)*/;

		if (nOffsets[0] <= 0)
		{
			dataLen = 0;
			needclean = false;
			data = NULL;
			return;
		}

		int sz = calcMipsSize(tex->nWidth,tex->nHeight);

		dataLen = sz + sizeof(short) + sizeof(COLOR3) * 256;
		data = new unsigned char[dataLen];
		memset(data, 0, dataLen);

		unsigned char* texdata = ((unsigned char*)tex) + tex->nOffsets[0];

		memcpy(data, texdata, palette ? sz : dataLen);
		if (palette)
		{
			*(unsigned short*)(data + sz) = colors;
			memcpy(data + sz + sizeof(short), palette, sizeof(COLOR3) * colors);
		}

		needclean = true;
	}
	~WADTEX()
	{
		if (needclean && data)
			delete[] data;
		needclean = false;
		szName[0] = '\0';
		data = NULL;
		nWidth = nHeight = 0;
		nOffsets[0] = nOffsets[1] = nOffsets[2] = nOffsets[3] = 0;
	}
};

#pragma pack(pop)


class Wad
{
public:
	std::string filename = std::string();
	std::string wadname = std::string();

	unsigned char* filedata = NULL;
	int fileLen = 0;
	bool usableTextures = false;

	WADHEADER header = WADHEADER();

	std::vector<WADDIRENTRY> dirEntries = std::vector<WADDIRENTRY>();

	Wad(std::string file);
	Wad(void);

	~Wad(void);

	bool readInfo();

	bool hasTexture(int dirIndex);
	bool hasTexture(const std::string& name);

	bool write(const std::string& filename, std::vector<WADTEX*> textures);
	bool write(WADTEX** textures, int numTex);
	bool write(std::vector<WADTEX*> textures);

	WADTEX* readTexture(int dirIndex, int* texturetype = NULL);
	WADTEX* readTexture(const std::string& texname, int* texturetype = NULL);
};

WADTEX* create_wadtex(const char* name, COLOR3* data, int width, int height);
COLOR3* ConvertWadTexToRGB(WADTEX* wadTex, COLOR3* palette = NULL);
COLOR3* ConvertMipTexToRGB(BSPMIPTEX* wadTex, COLOR3* palette = NULL);
COLOR4* ConvertWadTexToRGBA(WADTEX* wadTex, COLOR3* palette = NULL, int colors = 256);
COLOR4* ConvertMipTexToRGBA(BSPMIPTEX* tex, COLOR3* palette = NULL, int colors = 256);

COLOR3 GetMipTexAplhaColor(BSPMIPTEX* wadTex, COLOR3* palette = NULL, int colors = 256);
COLOR3 GetWadTexAplhaColor(WADTEX* wadTex, COLOR3* palette = NULL, int colors = 256);
