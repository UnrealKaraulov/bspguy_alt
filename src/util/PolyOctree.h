#pragma once
#include "Polygon3D.h"
#include <set>
#include <vector>

struct PolyOctant {
    vec3 min;
    vec3 max;
    vector<Polygon3D*> polygons;
    PolyOctant* children[8]; // Eight children octants

    PolyOctant(vec3 min, vec3 max);

    ~PolyOctant();

    void removePolygon(Polygon3D* polygon);
};

class PolygonOctree {
public:
    PolyOctant* root;
    int maxDepth;

    PolygonOctree(const vec3& min, const vec3& max, int depth);

    ~PolygonOctree();

    void buildOctree(PolyOctant* node, int currentDepth);

    void insertPolygon(Polygon3D* polygon);

    void insertPolygon(PolyOctant* node, Polygon3D* polygon, int currentDepth);

    void removePolygon(Polygon3D* polygon);

    bool isPolygonInOctant(Polygon3D* polygon, PolyOctant* node);

    void getPolysInRegion(Polygon3D* poly, vector<bool>& regionPolys);

    void getPolysInRegion(PolyOctant* node, Polygon3D* poly, int currentDepth, vector<bool>& regionPolys);
};