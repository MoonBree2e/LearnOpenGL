#include "FluidParticleManager.h"

namespace fs = std::filesystem;

void FluidParticleManager::loadParticleData(std::filesystem::path path, glm::dmat4& model, glm::dmat4& view, glm::dvec3& T) {
    if (loaded) return;
    std::vector<std::string> plyFiles;
    for (const auto& entry : fs::directory_iterator(path)) {
        if (entry.is_regular_file() && entry.path().extension() == ".ply") {
            std::cout << "reading: " << entry.path() << std::endl;
            plyFiles.push_back(entry.path().string());
            auto pFP = new FluidParticle(entry.path(), model, view, T);
            particleMap[entry.path().filename().string()] = pFP->loadVertices(entry.path());
                //std::async(std::launch::async, [](FluidParticle* p, fs::path path) { return p->loadVertices(path); }, pFP, entry.path());
        }
    }
    loaded = true;
    frameNum = particleMap.size();
    for (auto it = particleMap.begin(); it != particleMap.end(); it++) {
        particleVec.push_back(it->second);
        it->second->preMultiView(model, view);
    }
}

void FluidParticleManager::activeFrame(int index) {
    particleVec[index]->bind();
}