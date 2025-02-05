#pragma once
#include "Polygon3D.h"
#include "LeafNavMesh.h"

class Bsp;
class LeafOctree;

// generates a navigation mesh for a BSP
class LeafNavMeshGenerator {
public:
	LeafNavMeshGenerator() = default;

	// generate a nav mesh from the bsp
	// returns polygons used to construct the mesh
	LeafNavMesh* generate(Bsp* map);

private:
	int octreeDepth = 6;

	// get leaves of the bsp tree with the given contents
	std::vector<LeafNode> getHullLeaves(Bsp* map, int modelIdx, int contents);

	// get smallest octree box that can contain the entire map
	void getOctreeBox(Bsp* map, vec3& min, vec3& max);

	// group polys that are close together for fewer collision checks later
	LeafOctree* createLeafOctree(Bsp* map, std::vector<LeafNode>& mesh, int treeDepth);

	// finds best origin for a leaf
	void setLeafOrigins(Bsp* map, LeafNavMesh* mesh);

	// find point on poly which is closest to a floor, using distance to the bias point as a tie breaker
	vec3 getBestPolyOrigin(Bsp* map, Polygon3D& poly, vec3 bias);

	// links nav leaves which have faces touching each other
	void linkNavLeaves(Bsp* map, LeafNavMesh* mesh);

	// use entities to create cheaper paths between leaves
	void linkEntityLeaves(Bsp* map, LeafNavMesh* mesh);

	void linkEntityLeaves(Bsp* map, LeafNavMesh* mesh, LeafNode& entNode, std::vector<bool>& regionLeaves);

	// returns a combined node for an entity, which is the bounding box of all its model leaves
	LeafNode& addSolidEntityNode(Bsp* map, LeafNavMesh* mesh, int entidx);

	// returns a node for an entity, which is its bounding box
	LeafNode& addPointEntityNode(Bsp* map, LeafNavMesh* mesh, int entidx, vec3 mins, vec3 maxs);

	int tryFaceLinkLeaves(Bsp* map, LeafNavMesh* mesh, int srcLeafIdx, int dstLeafIdx);

	void calcPathCosts(Bsp* bsp, LeafNavMesh* mesh);

	void addPathCost(LeafLink& link, Bsp* bsp, vec3 start, vec3 end, bool isDrop);
};