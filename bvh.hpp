#ifndef BVH_H_
#define BVH_H_

// Standard headers
#include <vector>

// App headers
#include "core.hpp"

// Acceleration structure
struct BBox {
	glm::vec3 min;
	glm::vec3 max;

	float surface_area() const {
		glm::vec3 size = max - min;
		return 2.0f * (size.x * size.y + size.x * size.z + size.y * size.z);
	}
};

using BVHBuffer = std::vector <aligned_vec4>;

struct BVH {
	BBox bbox;
	int primitive = -1;
	BVH *left = nullptr;
	BVH *right = nullptr;

	BVH(const BBox &bbox_, int p) : bbox(bbox_), primitive(p) {}

	~BVH() {
		if (left) delete left;
		if (right) delete right;
	}

	int size() const {
		int size = 1;
		if (left) size += left->size();
		if (right) size += right->size();
		return size;
	}

	void serialize(BVHBuffer &buffer, int miss = -1) {
		// Compute indices and hit/miss indices
		int size = buffer.size();
		int after = size + 3;

		int left_index = left ? after : -1;
		int left_size = left ? 3 * left->size() : 0;
		int right_index = right ? after + left_size : -1;

		int hit = left_index;
		if (!left && !right) hit = miss;

		int miss_left = right_index;
		if (!right) miss_left = miss;

		// Serialize
		aligned_vec4 header = glm::vec3 {
			*reinterpret_cast <float *> (&primitive),
			*reinterpret_cast <float *> (&hit),
			*reinterpret_cast <float *> (&miss)
		};

		buffer.push_back(header);
		buffer.push_back(bbox.min);
		buffer.push_back(bbox.max);

		if (left) left->serialize(buffer, miss_left);
		if (right) right->serialize(buffer, miss);
	}

	void print(int indentation = 0) {
		std::string ident(indentation, '\t');
		std::cout << ident << "BVH [" << primitive << "]: "
			<< glm::to_string(bbox.min)
			<< " -> " << glm::to_string(bbox.max) << std::endl;
		if (left) left->print(indentation + 1);
		if (right) right->print(indentation + 1);
	}
};

// Union bounding boxes
inline BBox union_of(const std::vector <BVH *> &nodes) {
	glm::vec3 min = nodes[0]->bbox.min;
	glm::vec3 max = nodes[0]->bbox.max;

	for (int i = 1; i < nodes.size(); i++) {
		min = glm::min(min, nodes[i]->bbox.min);
		max = glm::max(max, nodes[i]->bbox.max);
	}

	return BBox{min, max};
}

inline float cost_split(const std::vector <BVH *> &nodes, float split, int axis) {
	float cost = 0.0f;

	glm::vec3 min_left = glm::vec3(std::numeric_limits <float> ::max());
	glm::vec3 max_left = glm::vec3(-std::numeric_limits <float> ::max());

	glm::vec3 min_right = glm::vec3(std::numeric_limits <float> ::max());
	glm::vec3 max_right = glm::vec3(-std::numeric_limits <float> ::max());

	glm::vec3 tmin = nodes[0]->bbox.min;
	glm::vec3 tmax = nodes[0]->bbox.max;

	int prims_left = 0;
	int prims_right = 0;

	for (BVH *node : nodes) {
		glm::vec3 min = node->bbox.min;
		glm::vec3 max = node->bbox.max;

		tmin = glm::min(tmin, min);
		tmax = glm::max(tmax, max);

		float value = (min[axis] + max[axis]) / 2.0f;

		if (value < split) {
			// Left
			prims_left++;

			min_left = glm::min(min_left, min);
			max_left = glm::max(max_left, max);
		} else {
			// Right
			prims_right++;

			min_right = glm::min(min_right, min);
			max_right = glm::max(max_right, max);
		}
	}

	// Max cost when all primitives are in one side
	if (prims_left == 0 || prims_right == 0)
		return std::numeric_limits <float> ::max();

	// Compute cost
	float sa_left = BBox {min_left, max_left}.surface_area();
	float sa_right = BBox {min_right, max_right}.surface_area();
	float sa_total = BBox {tmin, tmax}.surface_area();

	return 1 + (prims_left * sa_left + prims_right * sa_right) / sa_total;
}

inline BVH *partition(std::vector <BVH *> &nodes) {
	// Base cases
	if (nodes.size() == 0)
		return nullptr;

	if (nodes.size() == 1)
		return nodes[0];

	if (nodes.size() == 2) {
		BVH *node = new BVH(union_of(nodes), -1);
		node->left = nodes[0];
		node->right = nodes[1];
		return node;
	}

	// Get axis with largest extent
	int axis = 0;
	float max_extent = 0.0f;

	float min_value = std::numeric_limits <float> ::max();
	float max_value = -std::numeric_limits <float> ::max();

	for (size_t n = 0; n < nodes.size(); n++) {
		for (int i = 0; i < 3; i++) {
			glm::vec3 min = nodes[n]->bbox.min;
			glm::vec3 max = nodes[n]->bbox.max;

			float extent = std::abs(max[i] - min[i]);
			if (extent > max_extent) {
				max_extent = extent;
				min_value = min[i];
				max_value = max[i];
				axis = i;
			}
		}
	}

	// Binary search optimal partition (using SAH)
	float min_cost = std::numeric_limits <float> ::max();
	float min_split = 0.0f;
	int bins = 10;

	for (int i = 0; i < bins; i++) {
		float split = (max_value - min_value) / bins * i + min_value;
		float cost = cost_split(nodes, split, axis);

		if (cost < min_cost) {
			min_cost = cost;
			min_split = split;
		}
	}

	std::vector <BVH *> left;
	std::vector <BVH *> right;

	if (min_cost == std::numeric_limits <float> ::max()) {
		// Partition evenly
		for (int i = 0; i < nodes.size(); i++) {
			if (i % 2 == 0)
				left.push_back(nodes[i]);
			else
				right.push_back(nodes[i]);
		}
	} else {
		// Centroid partition with optimal split
		for (BVH *node : nodes) {
			glm::vec3 min = node->bbox.min;
			glm::vec3 max = node->bbox.max;

			float value = (min[axis] + max[axis]) / 2.0f;

			if (value < min_split)
				left.push_back(node);
			else
				right.push_back(node);
		}
	}

	// Create left and right nodes
	BBox bbox = union_of(nodes);
	BVH *left_node = partition(left);
	BVH *right_node = partition(right);

	BVH *node = new BVH(bbox, -1);
	node->left = left_node;
	node->right = right_node;

	return node;
}

#endif
