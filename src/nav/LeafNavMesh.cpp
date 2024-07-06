#include "Renderer.h"
#include "LeafNavMesh.h"
#include "GLFW/glfw3.h"
#include "PolyOctree.h"
#include "Clipper.h"
#include "util.h"
#include <string.h>
#include "Bsp.h"
#include <unordered_map>
#include "Entity.h"
#include "Fgd.h"
#include "globals.h"
#include <queue>
#include <algorithm>
#include <limits.h>

LeafNode::LeafNode() {
	links.clear();
	leafFaces.clear();
	id = -1;
	center = bottom = top = mins = maxs = vec3();
}

bool LeafNode::isInside(vec3 p) {
	for (int i = 0; i < leafFaces.size(); i++) {
		if (leafFaces[i].distance(p) > 0) {
			return false;
		}
	}

	return true;
}

bool LeafNode::addLink(int node, Polygon3D linkArea) {	
	for (int i = 0; i < links.size(); i++) {
		if (links[i].node == node) {
			return true;
		}
	}

	LeafLink link;
	link.linkArea = linkArea;
	link.node = node;

	link.bottom = linkArea.center;
	if (fabs(linkArea.plane_z.z) < 0.7f) {
		linkArea.intersect2D(linkArea.center, linkArea.center - vec3(0, 0, 4096), link.bottom);
		link.bottom.z += NAV_BOTTOM_EPSILON;
	}

	links.push_back(link);

	return true;
}

int LeafNode::numLinks() {
	int numLinks = 0;

	for (int i = 0; i < links.size(); i++) {
		numLinks++;
	}

	return numLinks;
}

LeafNavMesh::LeafNavMesh() {
	clear();
}

void LeafNavMesh::clear() {
	memset(leafMap, 65535, sizeof(uint16_t) * MAX_MAP_CLIPNODE_LEAVES);
	nodes.clear();
}

LeafNavMesh::LeafNavMesh(vector<LeafNode> inleaves) {
	clear();

	nodes = inleaves;

	int totalSz = 0;
	for (int i = 0; i < nodes.size(); i++) {
		totalSz += sizeof(LeafNode) + (sizeof(LeafLink) * nodes[i].links.size());
	}

	logf("Created leaf nav mesh with %d leaves (%d KB)\n", 
		nodes.size(), totalSz / 1024);

	logf("LeafNode = %d bytes, LeafLink = %d bytes\n",
		sizeof(LeafNode), sizeof(LeafLink));
}

bool LeafNavMesh::addLink(int from, int to, Polygon3D linkArea) {
	if (from < 0 || to < 0 || from >= nodes.size() || to >= nodes.size()) {
		logf("Error: add link from/to invalid node %d %d\n", from, to);
		return false;
	}

	if (!nodes[from].addLink(to, linkArea)) {
		vec3& pos = nodes[from].center;
		logf("Failed to add link at %d %d %d\n", (int)pos.x, (int)pos.y, (int)pos.z);
		return false;
	}

	return true;
}

int LeafNavMesh::getNodeIdx(Bsp* map, Entity* ent) {
	vec3 ori = ent->getOrigin();
	vec3 mins, maxs;
	int modelIdx = ent->getBspModelIdx();

	if (modelIdx != -1) {
		BSPMODEL& model = map->models[modelIdx];

		map->get_model_vertex_bounds(modelIdx, mins, maxs);
		ori += (maxs + mins) * 0.5f;
	}
	else {
		FgdClass* fclass = g_app->fgd->getFgdClass(ent->keyvalues["classname"]);
		if (fclass->sizeSet) {
			mins = fclass->mins;
			maxs = fclass->maxs;
		}
	}

	// first try testing a few points on the entity box for an early exit

	mins += ori;
	maxs += ori;

	vec3 testPoints[10] = {
		ori,
		(mins + maxs) * 0.5f,
		vec3(mins.x, mins.y, mins.z),
		vec3(mins.x, mins.y, maxs.z),
		vec3(mins.x, maxs.y, mins.z),
		vec3(mins.x, maxs.y, maxs.z),
		vec3(maxs.x, mins.y, mins.z),
		vec3(maxs.x, mins.y, maxs.z),
		vec3(maxs.x, maxs.y, mins.z),
		vec3(maxs.x, maxs.y, maxs.z),
	};

	for (int i = 0; i < 10; i++) {
		int targetLeaf = map->get_leaf(testPoints[i], 3);

		if (targetLeaf >= 0 && targetLeaf < MAX_MAP_CLIPNODE_LEAVES) {
			int navIdx = leafMap[targetLeaf];
			
			if (navIdx < 65535) {
				return navIdx;
			}
		}
	}

	if ((maxs - mins).length() < 1) {
		return -1; // point sized, so can't intersect any leaf
	}

	// no points are inside, so test for plane intersections

	cCube entCube(mins, maxs, COLOR4(0, 0, 0, 0));
	cQuad* faces[6] = {
		&entCube.top,
		&entCube.bottom,
		&entCube.left,
		&entCube.right,
		&entCube.front,
		&entCube.back,
	};

	Polygon3D boxPolys[6];
	for (int i = 0; i < 6; i++) {
		cQuad& face = *faces[i];
		boxPolys[i] = vector<vec3>{ face.v1.pos(), face.v2.pos(), face.v3.pos(), face.v6.pos() };
	}

	for (int i = 0; i < nodes.size(); i++) {
		LeafNode& mesh = nodes[i];
		
		for (int k = 0; k < mesh.leafFaces.size(); k++) {
			Polygon3D& leafFace = mesh.leafFaces[k];

			for (int k = 0; k < 6; k++) {
				if (leafFace.intersects(boxPolys[k])) {
					return i;
				}
			}
		}
	}

	return -1;
}

float LeafNavMesh::path_cost(int a, int b) {

	LeafNode& nodea = nodes[a];
	LeafNode& nodeb = nodes[b];
	vec3 delta = nodea.bottom - nodeb.bottom;

	for (int i = 0; i < nodea.links.size(); i++) {
		LeafLink& link = nodea.links[i];
		if (link.node == b) {
			return link.baseCost + delta.length() * link.costMultiplier;
		}
	}

	return delta.length();
}

vector<int> LeafNavMesh::AStarRoute(int startNodeIdx, int endNodeIdx)
{
	set<int> closedSet;
	set<int> openSet;

	unordered_map<int, float> gScore;
	unordered_map<int, float> fScore;
	unordered_map<int, int> cameFrom;
	
	vector<int> emptyRoute;

	if (startNodeIdx < 0 || endNodeIdx < 0 || startNodeIdx > nodes.size() || endNodeIdx > nodes.size()) {
		logf("AStarRoute: invalid start/end nodes\n");
		return emptyRoute;
	}

	if (startNodeIdx == endNodeIdx) {
		emptyRoute.push_back(startNodeIdx);
		return emptyRoute;
	}

	LeafNode& start = nodes[startNodeIdx];
	LeafNode& goal = nodes[endNodeIdx];

	openSet.insert(startNodeIdx);
	gScore[startNodeIdx] = 0;
	fScore[startNodeIdx] = path_cost(start.id, goal.id);

	const int maxIter = 8192;
	int curIter = 0;
	while (!openSet.empty()) {
		if (++curIter > maxIter) {
			logf("AStarRoute exceeded max iterations searching path (%d)", maxIter);
			break;
		}

		// get node in openset with lowest cost
		int current = -1;
		float bestScore = 9e99;
		for (int nodeId : openSet)
		{
			float score = fScore[nodeId];
			if (score < bestScore) {
				bestScore = score;
				current = nodeId;
			}
		}

		//println("Current is " + current);

		if (current == goal.id) {
			//println("MAde it to the goal");
			// goal reached, build the route
			vector<int> path;
			path.push_back(current);

			int maxPathLen = 1000;
			int i = 0;
			while (cameFrom.count(current)) {
				current = cameFrom[current];
				path.push_back(current);
				if (++i > maxPathLen) {
					logf("AStarRoute exceeded max path length (%d)", maxPathLen);
					break;
				}
			}
			reverse(path.begin(), path.end());

			return path;
		}

		openSet.erase(current);
		closedSet.insert(current);

		LeafNode& currentNode = nodes[current];

		for (int i = 0; i < currentNode.links.size(); i++) {
			LeafLink& link = currentNode.links[i];
			if (link.node == -1) {
				break;
			}

			int neighbor = link.node;
			if (neighbor < 0 || neighbor >= nodes.size()) {
				continue;
			}
			if (closedSet.count(neighbor))
				continue;
			//if (currentNode.blockers.size() > i and currentNode.blockers[i] & blockers != 0)
			//	continue; // blocked by something (monsterclip, normal clip, etc.). Don't route through this path.

			// discover a new node
			openSet.insert(neighbor);

			// The distance from start to a neighbor
			LeafNode& neighborNode = nodes[neighbor];

			float tentative_gScore = gScore[current];
			tentative_gScore += path_cost(currentNode.id, neighborNode.id);

			float neighbor_gScore = 9e99;
			if (gScore.count(neighbor))
				neighbor_gScore = gScore[neighbor];

			if (tentative_gScore >= neighbor_gScore)
				continue; // not a better path

			// This path is the best until now. Record it!
			cameFrom[neighbor] = current;
			gScore[neighbor] = tentative_gScore;
			fScore[neighbor] = tentative_gScore + path_cost(neighborNode.id, goal.id);
		}
	}	
	
	return emptyRoute;
}

// Dijkstra's algorithm to find shortest path from start to end vertex (chat-gpt code)
vector<int> LeafNavMesh::dijkstraRoute(int start, int end) {
	vector<int> emptyRoute;

	if (start < 0 || end < 0 || start > nodes.size() || end > nodes.size()) {
		logf("dijkstraRoute: invalid start/end nodes\n");
		return emptyRoute;
	}

	if (start == end) {
		emptyRoute.push_back(start);
		return emptyRoute;
	}

	int n = nodes.size();
	vector<float> dist(n, INT_MAX); // Initialize distances with infinity
	vector<int> previous(n, -1); // Array to store previous node in the shortest path
	dist[start] = 0.0f; // Distance from start node to itself is 0

	priority_queue<pair<float, int>, vector<pair<float, int>>, greater<pair<float, int>>> pq;
	pq.push({ 0.0f, start }); // Push start node with distance 0

	while (!pq.empty()) {
		int u = pq.top().second; // Get node with smallest distance
		float d = pq.top().first; // Get the distance
		pq.pop();

		// If the extracted node is already processed, skip
		if (d > dist[u])
			continue;

		// Stop early if we reached the end node
		if (u == end)
			break;

		// Traverse all links of node u
		for (int i = 0; i < nodes[u].links.size(); i++) {
			LeafLink& link = nodes[u].links[i];

			if (link.node == -1) {
				break;
			}

			int v = link.node;
			float weight = path_cost(u, link.node);

			// Relaxation step
			if (dist[u] + weight < dist[v]) {
				dist[v] = dist[u] + weight;
				previous[v] = u; // Set previous node for path reconstruction
				pq.push({ dist[v], v });
			}
		}
	}

	// Reconstruct the shortest path from start to end node
	vector<int> path;
	for (int at = end; at != -1; at = previous[at]) {
		path.push_back(at);
		if (at == start)
			break;
	}
	reverse(path.begin(), path.end());

	// If end node is unreachable, return an empty path
	if (path.empty() || path[0] != start)
		return {};

	float len = 0;
	float cost = 0;
	for (int i = 1; i < path.size(); i++) {
		LeafNode& mesha = nodes[path[i-1]];
		LeafNode& meshb = nodes[path[i]];
		len += (mesha.bottom - meshb.bottom).length();
		cost += path_cost(path[i - 1], path[i]);
	}
	logf("Path length: %d, cost: %d\n", (int)len, (int)cost);

	return path;
}