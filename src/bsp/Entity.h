#pragma once
#include "Keyvalue.h"
#include <map>

typedef std::map< std::string, std::string > hashmap;
class Bsp;

extern size_t totalEntityStructs;

class Entity
{
public:
	hashmap keyvalues;
	std::vector<std::string> keyOrder;

	std::string classname;

	int cachedModelIdx; // -2 = not cached
	std::vector<std::string> cachedTargets;
	bool targetsCached;
	bool hide;

	Entity()
	{
		classname = "";
		cachedModelIdx = -2;
		targetsCached = false;
		rendermode = kRenderNormal;
		renderamt = 0;
		renderfx = kRenderFxNone;
		rendercolor = vec3(1.0f, 1.0f, 1.0f);
		origin = vec3();
		targetsCached = false;
		hide = false;
		totalEntityStructs++;
		realIdx = totalEntityStructs;
	}

	Entity(const std::string& _classname)
	{
		setOrAddKeyvalue("classname", _classname);
		cachedModelIdx = -2;
		targetsCached = false;
		rendermode = kRenderNormal;
		renderamt = 0;
		renderfx = kRenderFxNone;
		rendercolor = vec3(1.0f, 1.0f, 1.0f);
		targetsCached = false;
		origin = vec3();
		hide = false;
		totalEntityStructs++;
		realIdx = totalEntityStructs;
	}
	void addKeyvalue(const std::string& key, const std::string& value, bool multisupport = false);
	void removeKeyvalue(const std::string& key);
	bool renameKey(int idx, const std::string& newName);
	bool renameKey(const std::string& oldName, const std::string& newName);
	void clearAllKeyvalues();
	void clearEmptyKeyvalues();

	void setOrAddKeyvalue(const std::string& key, const std::string& value);

	// returns -1 for invalid idx
	int getBspModelIdx();
	int getBspModelIdxForce();

	bool isBspModel();

	bool isWorldSpawn();

	bool hasKey(const std::string& key);

	std::vector<std::string> getTargets();

	bool hasTarget(const std::string& checkTarget);

	void renameTargetnameValues(const std::string& oldTargetname, const std::string& newTargetname);

	void updateRenderModes();

	size_t getMemoryUsage(); // aproximate

	vec3 origin;	
	
	vec3 getHullOrigin(Bsp* map);

	
	bool isEverVisible();

	std::string serialize();
	
	int rendermode;
	int renderamt;
	int renderfx;
	vec3 rendercolor;

	size_t realIdx;
};

