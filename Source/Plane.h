#pragma once
#include"Mesh.h"
#include <PSO.h>
#include <Shaders.h>
#include <TextureManager.h>
#include <string>


class Plane
{
public:
	Mesh mesh;
	std::string colorTextureFilename;
	std::string normalTextureFilename;

	void init(Core* core, PSOManager* psos, Shaders* shaders, TextureManager* textureManager)
	{
		std::vector<STATIC_VERTEX> vertices;
		vertices.push_back(addVertex(Vec3(-1, 0, -1), Vec3(0, 1, 0), 0, 0));
		vertices.push_back(addVertex(Vec3(1, 0, -1), Vec3(0, 1, 0), 1, 0));
		vertices.push_back(addVertex(Vec3(-1, 0, 1), Vec3(0, 1, 0), 0, 1));
		vertices.push_back(addVertex(Vec3(1, 0, 1), Vec3(0, 1, 0), 1, 1));
		std::vector<unsigned int> indices;
		indices.push_back(0);
		indices.push_back(1);
		indices.push_back(2);
		indices.push_back(1);
		indices.push_back(3);
		indices.push_back(2);
		mesh.init(core, vertices, indices);

		shaders->load(core, "Plane_Textured", "Source/ShaderFile/PlaneTexturedVS.txt", "Source/ShaderFile/PlaneTexturedPS.txt");

		psos->createPSO(core, "Plane_TexturedPSO", shaders->find("Plane_Textured")->vs, shaders->find("Plane_Textured")->ps, VertexLayoutCache::getStaticLayout());

		// ???????????
		colorTextureFilename = "Models/Textures/ground_with_rocks_01_color_1k.png";
		normalTextureFilename = "Models/Textures/ground_with_rocks_01_normal_dx_1k.png";
		
		// ?????
		textureManager->getTextureIndex(colorTextureFilename);
		textureManager->getTextureIndex(normalTextureFilename);
	}

	void draw(Core* core, PSOManager* psos, Shaders* shaders, TextureManager* textureManager, Matrix& vp)
	{
		Matrix planeWorld;
		shaders->updateConstantVS("Plane_Textured", "staticMeshBuffer", "VP", &vp);
		shaders->updateConstantVS("Plane_Textured", "staticMeshBuffer", "W", &planeWorld);
		psos->bind(core, "Plane_TexturedPSO");
		shaders->apply(core, "Plane_Textured");
		
		// ?????? (t0)
		int colorTexIndex = textureManager->find(colorTextureFilename);
		if (colorTexIndex >= 0)
		{
			shaders->updateTexturePS(core, "Plane_Textured", "colorTex", colorTexIndex);
		}
		
		// ?????? (t1)
		int normalTexIndex = textureManager->find(normalTextureFilename);
		if (normalTexIndex >= 0)
		{
			shaders->updateTexturePS(core, "Plane_Textured", "normalTex", normalTexIndex);
		}
		
		mesh.draw(core);
	}
};
