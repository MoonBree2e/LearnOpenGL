#version 330 core

out vec2 Texcoord;

void main()
{
	const vec4 vertices[4] = vec4[4](
		vec4(1.f,  1.f, 0.0f, 1.0),  // 右上
		vec4(1.f, -1.f, 0.0f, 1.0),  // 右下
		vec4(-1.f, -1.f, 0.0f, 1.0), // 左下
		vec4(-1.f,  1.f, 0.0f, 1.0));// 左上
	const vec2 coord[4] = vec2[4](
		vec2(1.0f, 1.0f),
		vec2(1.0f, .0f),
		vec2(.0f, .0f),
		vec2(.0f, 1.0f));

	unsigned int indices[6] = unsigned int[6](0, 1, 3, 1, 2, 3);
	// Index into our array using gl_VertexID
	gl_Position = vertices[indices[gl_VertexID]];
	Texcoord = coord[indices[gl_VertexID]];
}