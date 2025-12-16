

#include "Core.h"
#include "Window.h"
#include "Timer.h"
#include "Maths.h"
#include "Shaders.h"
#include "Mesh.h"
#include "PSO.h"
#include "GEMLoader.h"
#include "Animation.h"
#include "TextureManager.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include"FPSCamera.h"
#include"skybox.h"

// Properties -> Linker -> System -> Windows

#include <d3dcompiler.h>
#pragma comment(lib, "d3dcompiler.lib")


class Plane
{
public:
	Mesh mesh;

	void init(Core* core, PSOManager *psos, Shaders* shaders)
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
		
		shaders->load(core, "StaticModelUntextured", "Source/ShaderFile/VS.txt", "Source/ShaderFile/PSUntextured.txt");
	
		psos->createPSO(core, "StaticModelUntexturedPSO", shaders->find("StaticModelUntextured")->vs, shaders->find("StaticModelUntextured")->ps, VertexLayoutCache::getStaticLayout());
	}
	void draw(Core* core, PSOManager* psos, Shaders* shaders, Matrix &vp)
	{
		Matrix planeWorld;
		shaders->updateConstantVS("StaticModelUntextured", "staticMeshBuffer", "VP", &vp);
		shaders->updateConstantVS("StaticModelUntextured", "staticMeshBuffer", "W", &planeWorld);
		shaders->apply(core, "StaticModelUntextured");
		psos->bind(core, "StaticModelUntexturedPSO");
		mesh.draw(core);
	}
};

class StaticModel
{
public:
	std::vector<Mesh *> meshes;
	std::vector<std::string> textureFilenames;
	void load(Core* core, std::string filename, Shaders* shaders, PSOManager* psos)
	{
		GEMLoader::GEMModelLoader loader;
		std::vector<GEMLoader::GEMMesh> gemmeshes;
		loader.load(filename, gemmeshes);
		for (int i = 0; i < gemmeshes.size(); i++)
		{
			Mesh* mesh = new Mesh();
			std::vector<STATIC_VERTEX> vertices;
			for (int j = 0; j < gemmeshes[i].verticesStatic.size(); j++)
			{
				STATIC_VERTEX v;
				memcpy(&v, &gemmeshes[i].verticesStatic[j], sizeof(STATIC_VERTEX));
				vertices.push_back(v);
			}
			mesh->init(core, vertices, gemmeshes[i].indices);
			meshes.push_back(mesh);
		}
		shaders->load(core, "StaticModelUntextured", "Source/ShaderFile/VS.txt", "Source/ShaderFile/PSUntextured.txt");
		psos->createPSO(core, "StaticModelPSO", shaders->find("StaticModelUntextured")->vs, shaders->find("StaticModelUntextured")->ps, VertexLayoutCache::getStaticLayout());
	}
	void updateWorld(Shaders* shaders, Matrix& w)
	{
		shaders->updateConstantVS("StaticModelUntextured", "staticMeshBuffer", "W", &w);
	}
	void draw(Core* core, PSOManager* psos, Shaders* shaders, Matrix &vp)
	{
		shaders->updateConstantVS("StaticModelUntextured", "staticMeshBuffer", "VP", &vp);
		shaders->apply(core, "StaticModelUntextured");
		psos->bind(core, "StaticModelPSO");
		for (int i = 0; i < meshes.size(); i++)
		{
			meshes[i]->draw(core);
		}
	}
};

class AnimatedModel
{
public:
	std::vector<Mesh *> meshes;
	Animation animation;
	std::vector<std::string> textureFilenames;
	void load(Core* core, std::string filename, PSOManager* psos, Shaders* shaders)
	{
		GEMLoader::GEMModelLoader loader;
		std::vector<GEMLoader::GEMMesh> gemmeshes;
		GEMLoader::GEMAnimation gemanimation;
		loader.load(filename, gemmeshes, gemanimation);
		for (int i = 0; i < gemmeshes.size(); i++)
		{
			Mesh* mesh = new Mesh();
			std::vector<ANIMATED_VERTEX> vertices;
			for (int j = 0; j < gemmeshes[i].verticesAnimated.size(); j++)
			{
				ANIMATED_VERTEX v;
				memcpy(&v, &gemmeshes[i].verticesAnimated[j], sizeof(ANIMATED_VERTEX));
				vertices.push_back(v);
			}
			mesh->init(core, vertices, gemmeshes[i].indices);
			meshes.push_back(mesh);

			// Load texture filename from gemmesh material
			std::string texFilename = gemmeshes[i].material.find("albedo").getValue();
			textureFilenames.push_back(texFilename);
		}
		shaders->load(core, "AnimatedTextured", "Source/ShaderFile/VSAnimTextured.txt", "Source/ShaderFile/PSTextured.txt");
		psos->createPSO(core, "AnimatedTexturedPSO",shaders->find("AnimatedTextured")->vs,shaders->find("AnimatedTextured")->ps,VertexLayoutCache::getAnimatedLayout());
		memcpy(&animation.skeleton.globalInverse, &gemanimation.globalInverse, 16 * sizeof(float));
		for (int i = 0; i < gemanimation.bones.size(); i++)
		{
			Bone bone;
			bone.name = gemanimation.bones[i].name;
			memcpy(&bone.offset, &gemanimation.bones[i].offset, 16 * sizeof(float));
			bone.parentIndex = gemanimation.bones[i].parentIndex;
			animation.skeleton.bones.push_back(bone);
		}
		for (int i = 0; i < gemanimation.animations.size(); i++)
		{
			std::string name = gemanimation.animations[i].name;
			AnimationSequence aseq;
			aseq.ticksPerSecond = gemanimation.animations[i].ticksPerSecond;
			for (int j = 0; j < gemanimation.animations[i].frames.size(); j++)
			{
				AnimationFrame frame;
				for (int index = 0; index < gemanimation.animations[i].frames[j].positions.size(); index++)
				{
					Vec3 p;
					Quaternion q;
					Vec3 s;
					memcpy(&p, &gemanimation.animations[i].frames[j].positions[index], sizeof(Vec3));
					frame.positions.push_back(p);
					memcpy(&q, &gemanimation.animations[i].frames[j].rotations[index], sizeof(Quaternion));
					frame.rotations.push_back(q);
					memcpy(&s, &gemanimation.animations[i].frames[j].scales[index], sizeof(Vec3));
					frame.scales.push_back(s);
				}
				aseq.frames.push_back(frame);
			}
			animation.animations.insert({ name, aseq });
		}
	}
	void draw(Core* core, PSOManager* psos, Shaders* shaders,TextureManager* textureManager,AnimationInstance* instance, Matrix& vp, Matrix& w)
	{
		psos->bind(core, "AnimatedTexturedPSO");
		shaders->updateConstantVS("AnimatedTextured", "staticMeshBuffer", "W", &w);
		shaders->updateConstantVS("AnimatedTextured", "staticMeshBuffer", "VP", &vp);
		shaders->updateConstantVS("AnimatedTextured", "staticMeshBuffer", "bones", instance->matrices);
		shaders->apply(core, "AnimatedTextured");

		for (int i = 0; i < meshes.size(); i++)
		{
			// 绑定纹理（对应PPT第109页）
			int texIndex = textureManager->find(textureFilenames[i]);
			if (texIndex >= 0)
			{
				shaders->updateTexturePS(core, "AnimatedTextured", "tex", texIndex);
			}
			meshes[i]->draw(core);
		}
	}
};


#define WIDTH 1920
#define HEIGHT 1080


int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, int nCmdShow)
{
	Window window;
	window.create(WIDTH, HEIGHT, "My Window");

	FPSCamera fpscamera;
	fpscamera.position = Vec3(0, 1.7f, 5);
	fpscamera.pitch = 0;
	fpscamera.yaw = 0;
	window.setMouseLock(true);


	Core core;
	core.init(window.hwnd, WIDTH, HEIGHT);
	Shaders shaders;
	PSOManager psos;

	Plane plane;
	plane.init(&core, &psos, &shaders);

	StaticModel staticModel;
	staticModel.load(&core, "Models/acacia_003.gem", &shaders, &psos);

	TextureManager textureManager;
	textureManager.init(&core); // Re-enable textureManager initialization
	AnimatedModel animatedModel;
	animatedModel.load(&core, "Models/TRex.gem", &psos, &shaders);

	Skybox skybox;
	skybox.init(&core, &psos, &shaders, &textureManager,5000,500,10000);

	//手动设置纹理路径
	for (int i = 0; i < animatedModel.textureFilenames.size(); i++)
	{
		// 清空或设置默认路径
		animatedModel.textureFilenames[i] = "Models/Textures/T-rex_Base_Color_alb.png";
		// 或者根据网格部分设置不同的纹理
		// if (i == 0) animatedModel.textureFilenames[i] = "Models/TRex_body.png";
		// else if (i == 1) animatedModel.textureFilenames[i] = "Models/TRex_eyes.png";
	}

	// 然后确保纹理被加载
	for (int i = 0; i < animatedModel.textureFilenames.size(); i++)
	{
		if (!animatedModel.textureFilenames[i].empty())
		{
			textureManager.getTextureIndex(animatedModel.textureFilenames[i]);
		}
	}

	AnimationInstance animatedInstance;
	animatedInstance.init(&animatedModel.animation, 0);

	Timer timer;
	float t = 0;
	while (1)
	{
		core.beginFrame();
		float dt = timer.dt();
		window.checkInput();
		if (window.keys[VK_ESCAPE] == 1)
		{
			break;
		}

		fpscamera.update(dt, &window);

		t += dt;
		Matrix vp;
		Matrix p = Matrix::perspective(0.01f, 10000.0f, 1920.0f / 1080.0f, 60.0f);
		//Vec3 from = Vec3(11 * cos(t), 5, 11 * sinf(t));
		//Matrix v = Matrix::lookAt(from, Vec3(0, 0, 0), Vec3(0, 1, 0));
		Matrix v = fpscamera.getViewMatrix();
		vp = v * p;

		//Matrix vvp;
		//Vec3 from = Vec3(0, 5, 10);
		//Matrix vv = Matrix::lookAt(from, Vec3(0, 0, 0), Vec3(0, 1, 0));
		//vvp = vv * p;


		shaders.updateConstantVS("StaticModelUntextured", "staticMeshBuffer", "VP", &vp);
		core.beginRenderPass();
		
		plane.draw(&core, &psos, &shaders, vp);

		Matrix W;
		W = Matrix::scaling(Vec3(0.01f, 0.01f, 0.01f)) * Matrix::translation(Vec3(5, 0, 0));
		staticModel.updateWorld(&shaders, W);
		//staticModel.draw(&core, &psos, &shaders, vp);

		W = Matrix::scaling(Vec3(0.01f, 0.01f, 0.01f)) * Matrix::translation(Vec3(10, 0, 0));
		staticModel.updateWorld(&shaders, W);
		staticModel.draw(&core, &psos, &shaders, vp);


		animatedInstance.update("run", dt);
		if (animatedInstance.animationFinished() == true)
		{
			animatedInstance.resetAnimationTime();
		}
		//textureManager.loadTexture(&core, "Models/Textures/T-rex_Base_Color_alb.png");
		shaders.updateConstantVS("AnimatedTextured", "staticMeshBuffer", "VP", &vp);
		W = Matrix::scaling(Vec3(0.01f, 0.01f, 0.01f));
		animatedModel.draw(&core, &psos, &shaders, &textureManager, &animatedInstance, vp, W);

		W = Matrix::translation(fpscamera.position);
		v = v.removeTranslation();
		vp = v * p;
		skybox.draw(&core, &psos, &shaders, &textureManager, dt, &W, &vp);

		core.finishFrame();
	}
	core.flushGraphicsQueue();
}