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
#include"Level.h"
#include"Tree.h"

// Properties -> Linker -> System -> Windows

#include <d3dcompiler.h>
#include <Plane.h>
#include <StaticModel.h>
#include <AnimatedModel.h>
#pragma comment(lib, "d3dcompiler.lib")


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

	TextureManager textureManager;
	textureManager.init(&core);

	Plane plane;
	plane.init(&core, &psos, &shaders, &textureManager);

	StaticModel staticModel;
	staticModel.load(&core, "Models/acacia_003.gem", &shaders, &psos);

	AnimatedModel animatedModel;
	animatedModel.load(&core, "Models/TRex.gem", &psos, &shaders);

	Skybox skybox;
	skybox.init(&core, &psos, &shaders, &textureManager,5000,500,10000);

	// 初始化柳树
	Tree willowTree;
	willowTree.init(&core, &shaders, &psos, &textureManager, "Models/willow.gem");
	willowTree.setPosition(20, 0, 0);  // 放置在原点附近
	willowTree.setScale(0.01f);         // 设置缩放
	willowTree.setRotation(0.0f);      // 设置旋转角度

	Level level;

	level.init(&core, &psos, &shaders, &textureManager, &plane, &skybox);
	level.loadFromFile("Levels/Level01.txt");

	//手动设置纹理路径
	//for (int i = 0; i < animatedModel.textureFilenames.size(); i++)
	//{
	//	// 清空或设置默认路径
	//	animatedModel.textureFilenames[i] = "Models/Textures/T-rex_Base_Color_alb.png";
	//	// 或者根据网格部分设置不同的纹理
	//	// if (i == 0) animatedModel.textureFilenames[i] = "Models/TRex_body.png";
	//	// else if (i == 1) animatedModel.textureFilenames[i] = "Models/TRex_eyes.png";
	//}

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
	float t = 0.0f;

	while (1)
	{
		core.beginFrame();

		float dt = timer.dt();
		t += dt;

		window.checkInput();
		if (window.keys[VK_ESCAPE] == 1) break;

		fpscamera.update(dt, &window);

		Matrix p = Matrix::perspective(0.01f, 10000.0f, 1920.0f / 1080.0f, 60.0f);
		Matrix v = fpscamera.getViewMatrix();
		Matrix vp = v * p;

		Matrix skyV = v.removeTranslation();
		Matrix skyVP = skyV * p;

		
		core.beginRenderPass();

		level.update(dt);

		level.draw(vp, skyVP, t, fpscamera.position);

		// 绘制柳树
		willowTree.draw(&core, &psos, &shaders, vp);

		core.finishFrame();
	}
	core.flushGraphicsQueue();

}