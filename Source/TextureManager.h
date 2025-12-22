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

    // Modify the getTextureIndex function to support automatic loading
    int getTextureIndex(const std::string& filename)
    {
        // if filename is empty, return -1 (no texture)
        if (filename.empty())
        {
            return -1;
        }

        // if  already been loaded, return the index
        auto it = textures.find(filename);
        if (it != textures.end())
        {
            return it->second->heapOffset;
        }

        // Automatically load texture

        Texture* texture = new Texture();
        if (!texture->init(core, filename))  // Change init function to return bool
        {
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
        if (texture->init(core, filename)) {
            textures[filename] = texture;

            return texture->heapOffset;
        }
        else {
            delete texture;
            return -1;
        }
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
