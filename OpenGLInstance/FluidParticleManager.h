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

    // ��ֹ���ƹ��캯���͸�ֵ������
    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;
};

class FluidParticleManager : public Singleton<FluidParticleManager> {

public:
    void loadParticleData(std::filesystem::path path, glm::dmat4& model, glm::dmat4& view, glm::dvec3& T);
    void activeFrame(int index);
    int getFrameNum() { return frameNum; }
    ~FluidParticleManager() {
        for (auto it = particleMap.begin(); it != particleMap.end(); it++) {
            delete it->second.get();
        }
    }

private:
    int frameNum = 0;
    bool loaded = false;
    std::map<std::string, std::future<FluidParticle*>> particleMap;
    std::vector<std::future<FluidParticle*>*> particleVec;
};