
#pragma once
#include "stb_image.h"
#include "Core.h"
#include <string>
#include <vector>

class Texture
{
public:
    ID3D12Resource* tex;
    int heapOffset;
    int width, height, channels;

    bool init(Core* core, const std::string& filename)
    {
        // load textures using stb_image
        unsigned char* texels = stbi_load(filename.c_str(), &width, &height, &channels, 0);
        if (!texels)
        {
            printf("Failed to load texture: %s\n", filename.c_str());
            return false;
        }

        // if there are only 3 channels, convert to 4 channels.
        unsigned char* texelsWithAlpha = nullptr;
        if (channels == 3)
        {
            channels = 4;
            texelsWithAlpha = new unsigned char[width * height * channels];
            for (int i = 0; i < (width * height); i++)
            {
                texelsWithAlpha[i * 4] = texels[i * 3];
                texelsWithAlpha[(i * 4) + 1] = texels[(i * 3) + 1];
                texelsWithAlpha[(i * 4) + 2] = texels[(i * 3) + 2];
                texelsWithAlpha[(i * 4) + 3] = 255;
            }
        }

        // creating texture resources
        DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;

        D3D12_HEAP_PROPERTIES heapDesc;
        memset(&heapDesc, 0, sizeof(D3D12_HEAP_PROPERTIES));
        heapDesc.Type = D3D12_HEAP_TYPE_DEFAULT;

        D3D12_RESOURCE_DESC textureDesc;
        memset(&textureDesc, 0, sizeof(D3D12_RESOURCE_DESC));
        textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        textureDesc.Width = width;
        textureDesc.Height = height;
        textureDesc.DepthOrArraySize = 1;
        textureDesc.MipLevels = 1;
        textureDesc.Format = format;
        textureDesc.SampleDesc.Count = 1;
        textureDesc.SampleDesc.Quality = 0;
        textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

        core->device->CreateCommittedResource(&heapDesc, D3D12_HEAP_FLAG_NONE,
            &textureDesc, D3D12_RESOURCE_STATE_COPY_DEST, NULL, IID_PPV_ARGS(&tex));

        // Calculate the memory layout required for uploading
        D3D12_RESOURCE_DESC desc = tex->GetDesc();
        unsigned long long size;
        D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint;
        UINT numRows;
        UINT64 rowSizeInBytes;
        UINT64 totalBytes;

        core->device->GetCopyableFootprints(&desc, 0, 1, 0, &footprint, &numRows, &rowSizeInBytes, &totalBytes);

        // Upload texture data
        unsigned int alignedWidth = ((width * channels) + 255) & ~255;
        core->uploadResource(tex,
            (channels == 4 && texelsWithAlpha) ? texelsWithAlpha : texels,
            alignedWidth * height,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            &footprint);

        // Create Shader Resource View
        D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = core->srvHeap.getNextCPUHandle();
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Format = format;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;

        core->device->CreateShaderResourceView(tex, &srvDesc, srvHandle);

        // save heaoffset
        heapOffset = core->srvHeap.used - 1;

       
        stbi_image_free(texels);
        if (texelsWithAlpha) delete[] texelsWithAlpha;

		return true;
    }

    void cleanUp()
    {
        if (tex) tex->Release();
    }

    ~Texture()
    {
        cleanUp();
    }
};