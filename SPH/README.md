Uniform Offset: 

```c++
const GLchar* names[] = {"a", "b"};
GLuint indices[2];
glGetUniformIndices(m_TestCS.getProgram(), 2, names, indices);
GLint offset[2];
glGetActiveUniformsiv(m_TestCS.getProgram(), 2, indices, GL_UNIFORM_OFFSET, offset);
```