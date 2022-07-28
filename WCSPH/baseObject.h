#pragma once
#include <memory>
#include <string>

namespace glcs {
    typedef std::shared_ptr<class BaseObject> BaseObjectRef;

    class BaseObject {
    public:
        BaseObject(const std::string& name) : m_name(name) {}
        virtual ~BaseObject() {}

        std::string name() { return m_name; }
        BaseObject& name(std::string& name) {
            m_name = name;
            return *this;
        }

        virtual void update(double time) { }
        virtual void draw() { }
        virtual void reset() { }

        static BaseObjectRef create(const std::string& name) {
            return std::make_shared<BaseObject>(name);
        }

        std::string m_name;
    };
}