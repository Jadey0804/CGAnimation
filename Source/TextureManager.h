#pragma once

#include "Texture.h"
#include <map>
#include <string>

class TextureManager
{
public:
    std::map<std::string, Texture*> textures;
	Core* core;
    void init(Core* _core)
    {
        core = _core;
    }

    // 修改getTextureIndex函数，支持自动加载
    int getTextureIndex(const std::string& filename)
    {
        // 如果文件名为空，返回-1（无纹理）
        if (filename.empty())
        {
            printf("Warning: Empty texture filename\n");
            return -1;
        }

        // 如果已经加载过，直接返回索引
        auto it = textures.find(filename);
        if (it != textures.end())
        {
            return it->second->heapOffset;
        }

        // 自动加载纹理
        printf("Loading texture: %s\n", filename.c_str());

        Texture* texture = new Texture();
        if (!texture->init(core, filename))  // 修改init函数返回bool
        {
            printf("Failed to load texture: %s\n", filename.c_str());
            delete texture;
            return -1;
        }

        textures[filename] = texture;
        return texture->heapOffset;
    }

    int loadTexture(Core* core, const std::string& filename)
    {
        auto it = textures.find(filename);
        if (it != textures.end())
        {
            return it->second->heapOffset;
        }

        Texture* texture = new Texture();
        texture->init(core, filename);
        textures[filename] = texture;

        return texture->heapOffset;
    }

    int find(const std::string& filename)
    {
        auto it = textures.find(filename);
        if (it != textures.end())
        {
            return it->second->heapOffset;
        }
        return -1;
    }

    ~TextureManager()
    {
        for (auto& pair : textures)
        {
            delete pair.second;
        }
        textures.clear();
    }
};
