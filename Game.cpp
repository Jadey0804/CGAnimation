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

	// willow init
	Tree willowTree;
	willowTree.init(&core, &shaders, &psos, &textureManager, "Models/willow.gem");
	willowTree.setPosition(-20, -0.3, 0);  
	willowTree.setScale(0.01f);         
	willowTree.setRotation(0.0f);      

	Level level;

	level.init(&core, &psos, &shaders, &textureManager, &plane, &skybox);
	level.loadFromFile("Levels/Level01.txt");

	//maually set texture paths
	//for (int i = 0; i < animatedModel.textureFilenames.size(); i++)
	//
	//	animatedModel.textureFilenames[i] = "Models/Textures/T-rex_Base_Color_alb.png";
	//	// if (i == 0) animatedModel.textureFilenames[i] = "Models/TRex_body.png";
	//	// else if (i == 1) animatedModel.textureFilenames[i] = "Models/TRex_eyes.png";
	//}
	// manually set texture paths
	//for (int i = 0; i < animatedModel.textureFilenames.size(); i++)
	//{
	//	if (!animatedModel.textureFilenames[i].empty())
	//	{
	//		textureManager.getTextureIndex(animatedModel.textureFilenames[i]);
	//	}
	//}

	AnimationInstance animatedInstance;
	animatedInstance.init(&animatedModel.animation, 0);

	Timer timer;
	float t = 0.0f;
	bool key1Pressed = false; 

	while (1)
	{
		core.beginFrame();

		float dt = timer.dt();
		t += dt;

		window.checkInput();
		if (window.keys[VK_ESCAPE] == 1) break;

		// press 1 to switch tree instantiation.
		if (window.keys['1'] == 1)
		{
			if (!key1Pressed)
			{
				willowTree.changeInstancing();
				key1Pressed = true;
			}
		}
		else
		{
			key1Pressed = false;
		}

		fpscamera.update(dt, &window);

		Matrix p = Matrix::perspective(0.01f, 10000.0f, 1920.0f / 1080.0f, 60.0f);
		Matrix v = fpscamera.getViewMatrix();
		Matrix vp = v * p;

		Matrix skyV = v.removeTranslation();
		Matrix skyVP = skyV * p;

		
		core.beginRenderPass();

		level.update(dt, fpscamera.position);  // Input camera position for collision detection

		level.draw(vp, skyVP, t, fpscamera.position);

		// draw a willow tree ,time parameters for wind animation
		willowTree.draw(&core, &psos, &shaders, vp, t);

		core.finishFrame();
	}
	core.flushGraphicsQueue();

}