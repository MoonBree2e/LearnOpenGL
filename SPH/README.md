Uniform Offset: 

```c++
const GLchar* names[] = {"a", "b"};
GLuint indices[2];
glGetUniformIndices(m_TestCS.getProgram(), 2, names, indices);
GLint offset[2];
glGetActiveUniformsiv(m_TestCS.getProgram(), 2, indices, GL_UNIFORM_OFFSET, offset);
```



GL_SHADER_STORAGE_BARRIER_BIT

保证往shader storage block写数据不会冲突



`GL_BUFFER_UPDATE_BARRIER_BIT`

保证在`barrier`之后的 [glBufferSubData](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glBufferSubData.xhtml), [glCopyBufferSubData](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glCopyBufferSubData.xhtml), or [glGetBufferSubData](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glGetBufferSubData.xhtml) 读写，或者 [glMapBuffer](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glMapBuffer.xhtml) 或 [glMapBufferRange](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glMapBufferRange.xhtml) 缓冲对象内存映射将会反映到`barrier`之前的shader写的内容。另外，通过这些命令写的内容也会等到`barrier`之前的shader写完之后再写入。



SPH