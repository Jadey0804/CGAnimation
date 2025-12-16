#pragma once
#include <string>
#include "Mesh.h"
#include "PSO.h"
#include "Shaders.h"
#include "Maths.h"
#include"TextureManager.h"


class Skybox {
    Mesh mesh;
    std::string shaderName = "SkyShader";
	std::string PSOName = "SkyPSO";
	std::string textureName = "Models/Textures/skyboxTexture.png";
    std::string vsPath = "Source/ShaderFile/SkyboxVS.txt";
    std::string psPath = "Source/ShaderFile/SkyboxPS.txt";
	std::string cbName = "skyboxbuffer";
	std::string texSlot = "tex";


public:
    void init(Core* core, PSOManager* psoManager, Shaders* shaders, TextureManager* textureManager, int rings, int segments, float radius) {
        std::vector<STATIC_VERTEX> vertices;
        std::vector<unsigned int> indices;

        vertices = getVertices(rings, segments, radius);
        indices = getIndices(rings, segments, radius);
        mesh.init(core, vertices, indices);

        shaders->load(core, shaderName, vsPath, psPath);

		textureManager->loadTexture(core, textureName);
        
        psoManager->createSkyPSO(
            core,
            PSOName,
            shaders->find(shaderName)->vs,
            shaders->find(shaderName)->ps,
            VertexLayoutCache::getStaticLayout());
    }

    std::vector<STATIC_VERTEX> getVertices(int rings, int segments, float radius) {
        std::vector<STATIC_VERTEX> vertices;
        for (int lat = 0; lat <= rings; lat++) {
            float theta = lat * M_PI / rings;
            float sinTheta = sinf(theta);
            float cosTheta = cosf(theta);
            for (int lon = 0; lon <= segments; lon++) {
                float phi = lon * 2.0f * M_PI / segments;
                float sinPhi = sinf(phi);
                float cosPhi = cosf(phi);
                Vec3 position(radius * sinTheta * cosPhi, radius * cosTheta, radius * sinTheta * sinPhi);
                Vec3 normal = position.normalize();
                float tu = (float)lon / segments;
                float tv = (float)lat / rings;
                vertices.push_back(addVertex(position, normal, tu, tv));
            }
        }
        return vertices;
    }

    std::vector<unsigned int> getIndices(int rings, int segments, float radius) {
        std::vector<unsigned int> indices;
        for (int lat = 0; lat < rings; lat++) {
            for (int lon = 0; lon < segments; lon++) {
                int current = lat * (segments + 1) + lon;
                int next = current + segments + 1;
                indices.push_back(current);
                indices.push_back(next);
                indices.push_back(current + 1);
                indices.push_back(current + 1);
                indices.push_back(next);
                indices.push_back(next + 1);
            }
        }
        return indices;
    }

    void draw(Core* core, PSOManager* psos, Shaders* shaders, TextureManager* textureManager, float time, Matrix* W, Matrix* vp) {
        shaders->updateConstantVS(shaderName, cbName, "VP", vp);
        shaders->updateConstantVS(shaderName, cbName, "W", W);
        shaders->apply(core, shaderName);
        psos->bind(core, PSOName);
		int offset = textureManager->find(textureName);
		shaders->updateTexturePS(core, shaderName, texSlot, offset);

        mesh.draw(core);
    }
};
