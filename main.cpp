#include "common.hpp"

// Aligned types
struct alignas(16) aligned_vec4 {
	glm::vec4 v;

	aligned_vec4() : v(glm::vec4(0.0f)) {}
	aligned_vec4(const glm::vec3 &v) : v(glm::vec4(v, 0.0f)) {}
	aligned_vec4(const glm::vec4 &v) : v(v) {}
};

std::ostream &operator<<(std::ostream &os, const aligned_vec4 &v)
{
	return os << glm::to_string(v.v);
}

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
BBox union_of(const std::vector <BVH *> &nodes) {
	glm::vec3 min = nodes[0]->bbox.min;
	glm::vec3 max = nodes[0]->bbox.max;

	for (int i = 1; i < nodes.size(); i++) {
		min = glm::min(min, nodes[i]->bbox.min);
		max = glm::max(max, nodes[i]->bbox.max);
	}

	return BBox{min, max};
}

float cost_split(const std::vector <BVH *> &nodes, float split, int axis) {
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

BVH *partition(std::vector <BVH *> &nodes) {
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

// TODO: add info about normals
// TODO: use indices and another vertex structure
struct Vertex {
	glm::vec3 position;
};

using VBuffer = std::vector <aligned_vec4>;
using IBuffer = std::vector <unsigned int>;

struct Mesh {
	std::vector <Vertex> vertices;
	std::vector <uint32_t> indices;

	// Serialize mesh vertices and indices to buffers
	void serialize_vertices(VBuffer &vbuffer) const {
		vbuffer.reserve(vertices.size());
		for (const auto &v : vertices)
			vbuffer.push_back(aligned_vec4(v.position));
	}

	// TODO: format should be uvec4: v1, v2, v3, type (water, leaves, grass,
	// etc.)
	void serialize_indices(IBuffer &ibuffer) const {
		ibuffer.reserve(indices.size());
		for (const auto &i : indices)
			ibuffer.push_back(i);
	}

	// Make BVH
	BVH *make_bvh() {
		std::vector <BVH *> nodes;
		nodes.reserve(indices.size());

		for (int i = 0; i < indices.size(); i += 3) {
			glm::vec3 min = vertices[indices[i]].position;
			glm::vec3 max = vertices[indices[i]].position;

			for (int j = 1; j < 3; j++) {
				min = glm::min(min, vertices[indices[i + j]].position);
				max = glm::max(max, vertices[indices[i + j]].position);
			}

			nodes.push_back(new BVH(BBox {min, max}, i/3));
		}

		return partition(nodes);
	}

	size_t triangles() const {
		return indices.size() / 3;
	}
};

float randf()
{
	return (float) rand() / (float) RAND_MAX;
}

// Generate terrain tile mesh
Mesh generate_tile()
{
	float width = 10.0f;
	float height = 10.0f;

	size_t resolution = 10;

	// Height map
	size_t mapres = resolution + 1;
	std::vector <float> height_map(mapres * mapres);

	srand(clock());
	for (size_t i = 0; i < mapres * mapres; i++)
		height_map[i] = randf() * 1.0f;

	float slice = width/resolution;

	float x = -width/2.0f;
	float z = -height/2.0f;

	Mesh tile;
	for (int i = 0; i <= resolution; i++) {
		for (int j = 0; j <= resolution; j++) {
			Vertex vertex;
			vertex.position = glm::vec3(x, height_map[i * resolution + j], z);
			tile.vertices.push_back(vertex);
			x += slice;
		}

		x = -width/2.0f;
		z += slice;
	}

	// Generate indices
	for (int i = 0; i < resolution; i++) {
		for (int j = 0; j < resolution; j++) {
			// Indices of square
			uint32_t a = i * (resolution + 1) + j;
			uint32_t b = (i + 1) * (resolution + 1) + j;
			uint32_t c = (i + 1) * (resolution + 1) + j + 1;
			uint32_t d = i * (resolution + 1) + j + 1;

			// Push
			tile.indices.push_back(a);
			tile.indices.push_back(b);
			tile.indices.push_back(c);

			tile.indices.push_back(a);
			tile.indices.push_back(c);
			tile.indices.push_back(d);
		}
	}

	/* std::cout << "Vertices: " << tile.vertices.size() << std::endl;
	for (const auto &v : tile.vertices)
		std::cout << glm::to_string(v.position) << std::endl; */

	return tile;
}

int main()
{
	// Basic window
	GLFWwindow *window = NULL;
	if (!glfwInit())
		return -1;

	window = glfwCreateWindow(WIDTH, HEIGHT, "Weaxor", NULL, NULL);

	// Check if window was created
	if (!window) {
		glfwTerminate();
		return -1;
	}

	// Make the window's context current
	glfwMakeContextCurrent(window);

	// Load OpenGL functions using GLAD
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		printf("Failed to initialize OpenGL context\n");
		return -1;
	}

	const GLubyte* renderer = glGetString(GL_RENDERER);
	printf("Renderer: %s\n", renderer);

	// Create texture for output
	unsigned int texture;

	// Create and bind texture
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);

	// Set texture parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// Texture data
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, WIDTH, HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);

	// Bind texture as image
	glBindImageTexture(0, texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

	// Read shader source
	unsigned int shader = compile_shader("shader.glsl", GL_COMPUTE_SHADER);

	// Create program
	unsigned int program = glCreateProgram();
	glAttachShader(program, shader);

	// Link program
	if (!link_program(program))
		return -1;

	// Delete shaders
	glDeleteShader(shader);

	glm::vec3 origin {0, 0, -5};
	glm::vec3 lookat {0, 0, 0};
	glm::vec3 up {0, 1, 0};

	auto setup_camera = [program](const glm::vec3 &origin, const glm::vec3 &lookat, const glm::vec3 &up_) {
		glm::vec3 a = lookat - origin;
		glm::vec3 b = up_;

		glm::vec3 front = glm::normalize(a);
		glm::vec3 right = glm::normalize(glm::cross(b, front));
		glm::vec3 up = glm::cross(front, right);

		set_int(program, "width", WIDTH);
		set_int(program, "height", HEIGHT);
		set_int(program, "pixel", PIXEL_SIZE);

		set_vec3(program, "camera.origin", origin);
		set_vec3(program, "camera.front", front);
		set_vec3(program, "camera.up", up);
		set_vec3(program, "camera.right", right);
	};

	setup_camera(origin, lookat, up);

	// Set up vertex data
	float vs[] = {
		// positions		// texture coords
		-1.0f,	-1.0f,	0.0f,	0.0f,	0.0f,
		1.0f,	-1.0f,	0.0f,	1.0f,	0.0f,
		-1.0f,	1.0f,	0.0f,	0.0f,	1.0f,
		1.0f,	1.0f,	0.0f,	1.0f,	1.0f
	};

	// Set up vertex array object
	unsigned int vao;

	// Create vertex array object
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	// Create vertex buffer object
	unsigned int vbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);

	// Upload vertex data
	glBufferData(GL_ARRAY_BUFFER, sizeof(vs), vs, GL_STATIC_DRAW);

	// Set up vertex attributes
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 5, (void *) 0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 5, (void *) (sizeof(float) * 3));
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);

	// Compile respective shaders
	unsigned int vertex_shader = compile_shader("texture.vert", GL_VERTEX_SHADER);
	unsigned int fragment_shader = compile_shader("texture.frag", GL_FRAGMENT_SHADER);

	// Create program
	unsigned int program_texture = glCreateProgram();
	glAttachShader(program_texture, vertex_shader);
	glAttachShader(program_texture, fragment_shader);

	// Link program
	if (!link_program(program_texture))
		return -1;

	// Vertices of tile
	Mesh tile = generate_tile();

	BVH *bvh = tile.make_bvh();

	// bvh->print();

	BVHBuffer bvh_buffer;
	bvh->serialize(bvh_buffer);

	/* for (int i = 0; i < bvh_buffer.size(); i += 3) {
		glm::uvec4 header = *reinterpret_cast <glm::uvec4 *> (&bvh_buffer[i]);
		std::cout << "Header: " << glm::to_string(header) << std::endl;
		std::cout << "\t" << bvh_buffer[i + 1] << std::endl;
		std::cout << "\t" << bvh_buffer[i + 2] << std::endl;
		std::cout << "\n";
	} */

	delete bvh;

	VBuffer vertices;
	IBuffer indices;

	tile.serialize_vertices(vertices);
	tile.serialize_indices(indices);

	// Create storage buffer for tile vertices
	unsigned int ssbo;

	// Create storage buffer object
	glGenBuffers(1, &ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(aligned_vec4) * vertices.size(), vertices.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 1);

	// Create storage buffer for tile indices
	unsigned int issbo;

	// Create storage buffer object
	glGenBuffers(1, &issbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, issbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(unsigned int) * indices.size(), indices.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 2);

	// Create storage buffer for BVH
	unsigned int bsbo;

	// Create storage buffer object
	glGenBuffers(1, &bsbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, bsbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(aligned_vec4) * bvh_buffer.size(), bvh_buffer.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 3);

	set_int(program, "primitives", tile.triangles());

	std::cout << "Buffer size = " << bvh_buffer.size() << std::endl;
	std::cout << "Triangles = " << tile.triangles() << std::endl;

	// Loop until the user closes the window
	float last_time = glfwGetTime();

	while (!glfwWindowShouldClose(window)) {
		// Close if escape or q
		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS ||
				glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
			glfwSetWindowShouldClose(window, GL_TRUE);
			continue;
		}

		// Update
		float t = glfwGetTime();
		// origin = glm::vec3 {cos(t) * 5, sin(t) * 5, sin(t) * 5};
		t /= 2;

		float radius = 10;
		origin = glm::vec3 {cos(t) * radius, 5, sin(t) * radius};
		setup_camera(origin, lookat, up);

		// Ray tracing
		{
			// Bind program and dispatch
			glUseProgram(program);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, issbo);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, bsbo);
			glDispatchCompute(WIDTH/PIXEL_SIZE, HEIGHT/PIXEL_SIZE, 1);
		}

		// Wait
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		// Render texture
		{
			// Bind program and draw
			glUseProgram(program_texture);
			glBindVertexArray(vao);
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		}

		// Swap front and back buffers
		glfwSwapBuffers(window);

		// Poll for and process events
		glfwPollEvents();

		// Print frame-time
		// TODO: set frame rate to 24 fps
		float current_time = glfwGetTime();
		float fps = 1.0f / (current_time - last_time);
		// printf("%f, fps: %f\n", current_time - last_time, fps);
		last_time = current_time;
	}
}
