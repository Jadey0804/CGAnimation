#pragma once
#include <vector>
#include <Animation.h>
#include <Mesh.h>
#include <PSO.h>
#include <Shaders.h>
#include <GEMLoader.h>
#include <TextureManager.h>
#include <algorithm>


class AnimatedModel
{
public:
	std::vector<Mesh*> meshes;
	Animation animation;
	std::vector<std::string> textureFilenames;

	//// 标准化纹理路径
	//static std::string normalizePath(const std::string& texPath)
	//{
	//	if (texPath.empty()) return "";
	//	
	//	std::string normalized = texPath;
	//	// 替换反斜杠为正斜杠
	//	std::replace(normalized.begin(), normalized.end(), '\\', '/');
	//	
	//	// 如果路径不包含Models/Textures，添加它
	//	if (normalized.find("Models/Textures/") == std::string::npos)
	//	{
	//		// 只取文件名部分
	//		size_t lastSlash = normalized.find_last_of("/");
	//		if (lastSlash != std::string::npos)
	//		{
	//			normalized = "Models/Textures/" + normalized.substr(lastSlash + 1);
	//		}
	//		else
	//		{
	//			normalized = "Models/Textures/" + normalized;
	//		}
	//	}
	//	return normalized;
	//}

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

			// 从材质中加载纹理路径并标准化
			std::string texFilename = gemmeshes[i].material.find("albedo").getValue();
			//texFilename = normalizePath(texFilename);
			textureFilenames.push_back(texFilename);
		}
		
		shaders->load(core, "AnimatedTextured", "Source/ShaderFile/VSAnimTextured.txt", "Source/ShaderFile/PSAnimTextured.txt");
		psos->createPSO(core, "AnimatedTexturedPSO", shaders->find("AnimatedTextured")->vs, shaders->find("AnimatedTextured")->ps, VertexLayoutCache::getAnimatedLayout());
		
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

	// 预加载所有纹理
	void preloadTextures(TextureManager* textureManager)
	{
		for (const auto& texPath : textureFilenames)
		{
			if (!texPath.empty())
			{
				textureManager->getTextureIndex(texPath);
			}
		}
	}

	void draw(Core* core, PSOManager* psos, Shaders* shaders, TextureManager* textureManager, AnimationInstance* instance, Matrix& vp, Matrix& w)
	{
		psos->bind(core, "AnimatedTexturedPSO");
		shaders->updateConstantVS("AnimatedTextured", "staticMeshBuffer", "W", &w);
		shaders->updateConstantVS("AnimatedTextured", "staticMeshBuffer", "VP", &vp);
		shaders->updateConstantVS("AnimatedTextured", "staticMeshBuffer", "bones", instance->matrices);
		shaders->apply(core, "AnimatedTextured");

		for (int i = 0; i < meshes.size(); i++)
		{
			int texIndex = textureManager->find(textureFilenames[i]);
			if (texIndex >= 0)
			{
				shaders->updateTexturePS(core, "AnimatedTextured", "tex", texIndex);
			}
			meshes[i]->draw(core);
		}
	}
};
