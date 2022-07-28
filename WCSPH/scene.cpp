#include "scene.h"

using namespace glcs;

Scene::Scene() {}

Scene::~Scene() { clear(); }

bool Scene::exists(const std::string& name) const {
    auto found = m_ObjectMap.find(name);
    return found != m_ObjectMap.end();
}

bool Scene::addObject(BaseObjectRef object, bool visible) {
    if (!object) {
        return false;
    }

    auto name = object->name();
    if (exists(name)) {
        return false;
    }

    m_ObjectMap[name] = object;
    m_ObjectList.push_back(object);
    if (visible) {
        m_DisplayObjectList.insert(object);
    }

    return true;
}

BaseObjectRef Scene::getObject(const std::string& name) const {
    auto found = m_ObjectMap.find(name);
    if (found == m_ObjectMap.end()) {
        return BaseObjectRef(NULL);
    }
    return (*found).second;
}

BaseObjectRef Scene::getObjectFromIndex(unsigned int index) const {
    if (index >= m_ObjectList.size()) {
        return BaseObjectRef(NULL);
    }
    return m_ObjectList.at(index);
}

void Scene::update(double time) {
    for (const auto& o : m_ObjectList) {
        o->update(time);
    }
}

void Scene::draw() {
    for (const auto& o : m_DisplayObjectList) {
        o->draw();
    }
}

void Scene::reset() {
    for (const auto& o : m_ObjectList) {
        o->reset();
    }
}

void Scene::clear() {
    m_DisplayObjectList.clear();
    m_ObjectList.clear();
    m_ObjectMap.clear();
}

SceneRef Scene::create() { return std::make_shared<Scene>(); }
