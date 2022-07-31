#pragma once
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "baseObject.h"

namespace glcs {
    typedef std::vector<BaseObjectRef> ObjectList;
    typedef std::set<BaseObjectRef> DisplayList;
    typedef std::map<std::string, BaseObjectRef> ObjectMap;

    typedef std::shared_ptr<class Scene> SceneRef;

    class Scene {
    public:
        Scene();
        ~Scene();

        int numObjects() { return int(m_ObjectList.size()); }
        bool exists(const std::string& name) const;
        bool addObject(BaseObjectRef object, bool visible = true);
        BaseObjectRef getObject(const std::string& name) const;
        BaseObjectRef getObjectFromIndex(unsigned int index) const;

        void update(double time);
        void draw();
        void reset();
        void clear();

        static SceneRef create();

    private:
        ObjectMap m_ObjectMap;
        ObjectList m_ObjectList;
        DisplayList m_DisplayObjectList;
    };
}