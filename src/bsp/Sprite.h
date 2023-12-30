#pragma once
#include <cstring>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <array>
#include "Wad.h"
#include "Texture.h"
#include "PointEntRenderer.h"

#pragma pack(push, 1)

enum synctype_t {
	ST_SYNC = 0,
	ST_RAND
};

struct SPRITE_HEADER {
	int ident;
	int version;
	int type;
	int texFormat;
	float boundingradius;
	int width;
	int height;
	int numframes;
	float beamlength;
	synctype_t synctype;
};

#define SPR_VP_PARALLEL_UPRIGHT 0
#define SPR_FACING_UPRIGHT 1
#define SPR_VP_PARALLEL 2
#define SPR_ORIENTED 3
#define SPR_VP_PARALLEL_ORIENTED 4

#define SPR_NORMAL 0
#define SPR_ADDITIVE 1
#define SPR_INDEXALPHA 2
#define SPR_ALPHTEST 3

struct dspriteframe_t {
	int origin[2];
	int width;
	int height;
};
#pragma pack(pop)


struct SpriteImage
{
	dspriteframe_t frameinfo;
	std::vector<COLOR3> image;
	std::vector<unsigned char> raw_image;
	float interval;
	Texture* texture;
	EntCube* spriteCube;
};

struct SpriteGroup
{
	std::vector<SpriteImage> sprites;
	float totalinterval;
};

class Sprite {
public:
	Sprite(const std::string& filename);
	~Sprite();
	std::string name;
	SPRITE_HEADER header;
	short colors;
	std::vector<COLOR3> palette;
	std::vector<SpriteGroup> sprite_groups;
};

void TestSprite();

extern std::map<int, Sprite*> spr_models;
Sprite* AddNewSpriteToRender(const std::string & path, unsigned int sum = 0);