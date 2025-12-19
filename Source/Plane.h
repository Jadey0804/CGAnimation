#pragma once
#include"Mesh.h"
#include <PSO.h>
#include <Shaders.h>
#include<TextureManager.h>

class Plane
{
public:
	Mesh mesh;


	std::string albedoPath = "Models/ground_with_rocks_01_color_1k.png";
	std::string normalPath = "Models/ground_with_rocks_01_normal_dx_1k.png";

	void init(Core* core, PSOManager* psos, Shaders* shaders, TextureManager* textureManager)
	{
		std::vector<STATIC_VERTEX> vertices;
		//vertices.push_back(addVertex(Vec3(-1, 0, -1), Vec3(0, 1, 0), 0, 0));
		//vertices.push_back(addVertex(Vec3(1, 0, -1), Vec3(0, 1, 0), 1, 0));
		//vertices.push_back(addVertex(Vec3(-1, 0, 1), Vec3(0, 1, 0), 0, 1));
		//vertices.push_back(addVertex(Vec3(1, 0, 1), Vec3(0, 1, 0), 1, 1));
		//std::vector<unsigned int> indices;
		//indices.push_back(0);
		//indices.push_back(1);
		//indices.push_back(2);
		//indices.push_back(1);
		//indices.push_back(3);
		//indices.push_back(2);

		auto v0 = addVertex(Vec3(-1, 0, -1), Vec3(0, 1, 0), 0, 0); v0.tangent = Vec3(1, 0, 0); vertices.push_back(v0);
		auto v1 = addVertex(Vec3(1, 0, -1), Vec3(0, 1, 0), 1, 0); v1.tangent = Vec3(1, 0, 0); vertices.push_back(v1);
		auto v2 = addVertex(Vec3(-1, 0, 1), Vec3(0, 1, 0), 0, 1); v2.tangent = Vec3(1, 0, 0); vertices.push_back(v2);
		auto v3 = addVertex(Vec3(1, 0, 1), Vec3(0, 1, 0), 1, 1); v3.tangent = Vec3(1, 0, 0); vertices.push_back(v3);

		std::vector<unsigned int> indices = { 0,1,2, 1,3,2 };

		mesh.init(core, vertices, indices);


		shaders->load(core, "Plane_Textured", "Source/ShaderFile/PlaneTexturedVS.txt", "Source/ShaderFile/PlaneTexturedPS.txt");

		psos->createPSO(core, "Plane_TexturedPSO", shaders->find("Plane_Textured")->vs, shaders->find("Plane_Textured")->ps, VertexLayoutCache::getStaticLayout());

		textureManager->loadTexture(core, albedoPath);
		textureManager->loadTexture(core, normalPath);
	}


	void draw(Core* core, PSOManager* psos, Shaders* shaders, TextureManager* textureManager,Matrix& vp)
	{
		Matrix planeWorld;
		psos->bind(core, "Plane_TexturedPSO");
		shaders->updateConstantVS("Plane_Textured", "staticMeshBuffer", "VP", &vp);
		shaders->updateConstantVS("Plane_Textured", "staticMeshBuffer", "W", &planeWorld);
		shaders->apply(core, "Plane_Textured");
		
		int albedoIdx = textureManager->find(albedoPath);
		if (albedoIdx >= 0)
			shaders->updateTexturePS(core, "Plane_Textured", "albedoMap", albedoIdx);

		int normalIdx = textureManager->find(normalPath);
		if (normalIdx >= 0)
			shaders->updateTexturePS(core, "Plane_Textured", "normalMap", normalIdx);

		mesh.draw(core);
	}
};
