#pragma once

#include <glad/glad.h>
#include <string>
/**
 * DebugGroup
 */
class DebugGroupOverrride {
public:
	DebugGroupOverrride(const std::string& message) {
		glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, message.c_str());
	}
	~DebugGroupOverrride() {
		Pop();
	}
	void Pop() {
		glPopDebugGroup();
	}
};
