#include "FluidParticle.h"

namespace fs = std::filesystem;

FluidParticle::FluidParticle(std::filesystem::path path, glm::dmat4& model, glm::dmat4& view, glm::dvec3& T)
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

    setTranslate(T);
}

FluidParticle::~FluidParticle() {
    glDeleteBuffers(1, &VBO);
    glDeleteVertexArrays(1, &VAO);
}


FluidParticle* FluidParticle::loadVertices(std::filesystem::path file) {
    std::ifstream inFile;
    inFile.open(file);
    if (!inFile.is_open()) {
        std::cout << "Could not open the file" << std::endl;
        exit(EXIT_FAILURE);
    }
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

        vertices.push_back(pos.x);
        vertices.push_back(pos.y);
        vertices.push_back(pos.z);
    }

    inFile.close();
    return this;
}

void FluidParticle::preMultiView(glm::dmat4& model, glm::dmat4& view) {
    std::vector<float> Result(vertices.size());
    int stride = 3;
    int offset = 0;
    for (size_t i = 0; i < vertices.size(); i += stride)
    {
        for (int k = 0; k < stride; ++k) { Result[i + k] = vertices[i + k]; }
        glm::dvec4 Pos(vertices[i + offset], vertices[i + offset + 1], vertices[i + offset + 2], 1.0);
        auto PosCS = view * model * Pos;
        Result[i + offset] = PosCS.x / PosCS.w;
        Result[i + offset + 1] = PosCS.y / PosCS.w;
        Result[i + offset + 2] = PosCS.z / PosCS.w;
    }

    glCall(glBindBuffer(GL_ARRAY_BUFFER, VBO));
    glCall(glBufferData(GL_ARRAY_BUFFER, sizeof(float) * Result.size(), Result.data(), GL_DYNAMIC_DRAW));
    glCall(glBindBuffer(GL_ARRAY_BUFFER, 0));
}