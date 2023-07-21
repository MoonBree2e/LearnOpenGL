#include "FluidParticle.h"

namespace fs = std::filesystem;

FluidParticle::FluidParticle(std::string vPath)
{

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    loadParticleData(vPath);
}

FluidParticle::~FluidParticle()
{
}

void FluidParticle::bindFrameData(int index) {
    glCall(glBindBuffer(GL_ARRAY_BUFFER, VBO));
    glCall(glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vertices[index].size(), vertices[index].data(), GL_DYNAMIC_DRAW));
    glCall(glBindBuffer(GL_ARRAY_BUFFER, 0));
}

void FluidParticle::loadParticleData(std::filesystem::path path) {
    std::vector<std::string> plyFiles;
    for (const auto& entry : fs::directory_iterator(path)) {
        if (entry.is_regular_file() && entry.path().extension() == ".ply") {
            std::cout << "reading: " << entry.path() << std::endl;
            plyFiles.push_back(entry.path().string());
            loadVertices(entry.path());
        }
    }
}

void FluidParticle::loadVertices(std::filesystem::path file) {
    std::ifstream inFile;
    inFile.open(file);
    if (!inFile.is_open()) {
        std::cout << "Could not open the file" << std::endl;
        exit(EXIT_FAILURE);
    }
    int index = vertices_num.size();
    int vertexNum = 0;
    std::string temp;
    std::string endName = "end_header";
    inFile >> temp;
    while (temp != endName) {
        if (temp == std::string("vertex")) {
            inFile >> vertexNum;
        }
        inFile >> temp;
    }

    vertices_num.push_back(vertexNum);
    vertices.push_back(std::vector<double>());
    
    glm::dvec3 origin;

    double scale = (1 / 20.0);
    for (int i = 0; i < vertexNum; i++) {
        double x, y, z;
        float r, g, b;
        inFile >> x >> y >> z >> r >> g >> b;
        if (i == 0) {
            origin.x = x;
            origin.y = y;
            origin.z = z;
        }
        auto pos = translate + (glm::dvec3(x, y, z) - origin) * scale;

        vertices[index].push_back(pos.x);
        vertices[index].push_back(pos.y);
        vertices[index].push_back(pos.z);
    }

    inFile.close();
}