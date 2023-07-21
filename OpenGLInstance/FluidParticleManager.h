#pragma once

#include "FluidParticle.h"
#include <map>
#include <future>

template <class T>
class Singleton {
protected:
    Singleton() {}
    virtual ~Singleton() {}

public:
    static T& getInstance() {
        static T instance;
        return instance;
    }

    // 禁止复制构造函数和赋值操作符
    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;
};

class FluidParticleManager : public Singleton<FluidParticleManager> {

public:
    void loadParticleData(std::filesystem::path path, glm::dmat4& model, glm::dmat4& view, glm::dvec3& T);
    void activeFrame(int index);
    int getFrameNum() { return frameNum; }
    FluidParticle* getFrameData(int index) { return particleVec[index]; }
    ~FluidParticleManager() {
        for (auto it = particleMap.begin(); it != particleMap.end(); it++) {
            delete it->second;
        }
    }

private:
    int frameNum = 0;
    bool loaded = false;
    std::map<std::string, FluidParticle*> particleMap;
    std::vector<FluidParticle*> particleVec;
};