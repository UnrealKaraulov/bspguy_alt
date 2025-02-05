#pragma once
#include <vector>
#include "vectors.h"
#include "mat4x4.h"
#include "Line2D.h"

// points must be at least this far inside the polygon edges
// large enough to prevent PointContents returning empty on the edges of underground polys
// smaller than the starting offset for content checks
#define INPOLY_EPSILON (0.4f)

// convex 3D polygon
class Polygon3D {
public:
	bool isValid = false;
	vec3 plane_x;
	vec3 plane_y;
	vec3 plane_z; // plane normal
	float fdist = 0;
	
	std::vector<vec3> verts;
	std::vector<vec2> localVerts; // points relative to the plane orientation
	std::vector<vec2> topdownVerts; // points without a z coordinate

	mat4x4 worldToLocal;
	mat4x4 localToWorld;

	// extents of local coordinates
	vec2 localMins;
	vec2 localMaxs;

	// extents of world coordinates
	vec3 worldMins;
	vec3 worldMaxs;

	vec3 center; // average/centroid in world coordinates
	
	float area = 0; // area of the 2D polygon

	int idx = -1; // for octree lookup

	Polygon3D() = default;

	Polygon3D(const std::vector<vec3>& verts);
	
	Polygon3D(const std::vector<vec3>& verts, int idx);

	void init();

	size_t sizeBytes();

	float distance(const vec3& p) const;

	bool isConvex();

	void removeColinearVerts();
	void removeDuplicateVerts(float epsilon=0.125f);

	void extendAlongAxis(float amt);

	// returns split polys for first edge on cutPoly that contacts this polygon
	// multiple intersections (overlapping polys) are not handled
	// returns empty on no intersection
	// only applies to edges that lie flat on the plane, not ones that pierce it
	std::vector<std::vector<vec3>> split(const Polygon3D& cutPoly);

	// cut the polygon by an edge defined in this polygon's coordinate space
	// returns empty if cutting not possible
	// returns 2 new convex polygons otherwise
	std::vector<std::vector<vec3>> cut(Line2D cutLine);

	// returns merged polygon vertices if polys are coplaner and share an edge
	// otherwise returns an empty polygon
	Polygon3D merge(const Polygon3D& mergePoly);

	// returns the area of intersection if polys are coplaner and overlap
	// otherwise returns an empty polygon
	Polygon3D coplanerIntersectArea(Polygon3D otherPoly);

	// returns true if the polygons intersect
	bool intersects(const Polygon3D& otherPoly) const;

	// if true, ipos is set to the intersection point with the given line segment
	bool intersect(const vec3& p1, const vec3& p2, vec3& ipos) const;

	// if true, ipos is set to the projected intersection point of the given line segment
	// the line segment is first projected onto the plane for a 2D intersect test
	bool intersect2D(const vec3& p1, const  vec3& p2, vec3& ipos) const;

	// is point inside this polygon? Coordinates are in world space.
	// Points within EPSILON of an edge are not inside.
	bool isInside(const vec3& p) const;

	// is point inside this polygon? coordinates are in polygon's local space.
	// Points within EPSILON of an edge are not inside.
	bool isInside(const vec2& p, bool includeEdge=false) const;

	// project a 3d point onto this polygon's local coordinate system
	vec2 project(const vec3& p) const;

	// get the world position of a point in the polygon's local coordinate system
	vec3 unproject(const vec2& p) const;
};