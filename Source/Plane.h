#pragma once
#include"Mesh.h"
#include <PSO.h>
#include <Shaders.h>


class Plane
{
public:
	Mesh mesh;

	void init(Core* core, PSOManager* psos, Shaders* shaders)
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
	}
	void draw(Core* core, PSOManager* psos, Shaders* shaders, Matrix& vp)
	{
		Matrix planeWorld;
		shaders->updateConstantVS("Plane_Textured", "staticMeshBuffer", "VP", &vp);
		shaders->updateConstantVS("Plane_Textured", "staticMeshBuffer", "W", &planeWorld);
		psos->bind(core, "Plane_TexturedPSO");
		shaders->apply(core, "Plane_Textured");
		
		mesh.draw(core);
	}
};
