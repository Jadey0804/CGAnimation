#pragma once

#include <vector>
#include <string>
#include "Core.h"
#include "Mesh.h"
#include "Shaders.h"
#include "PSO.h"
#include "GEMLoader.h"
#include "TextureManager.h"
#include "Maths.h"

class Tree
{
public:
    std::vector<Mesh*> meshes;
    std::vector<std::string> textureFilenames;
    std::vector<int> textureIndices;
    TextureManager* textureManager;
    Matrix worldMatrix;
    Vec3 position;
    float scale;
    float rotation;
	//std::string modelPath = "Models/willow.gem";

    Tree()
    {
        position = Vec3(0, 0, 0);
        scale = 1.0f;
        rotation = 0.0f;
        // Matrix默认构造函数已经调用identity()
        textureManager = nullptr;
    }

    void init(Core* core, Shaders* shaders, PSOManager* psos, TextureManager* texManager, 
              std::string modelPath)
    {
        textureManager = texManager;

        // 加载着色器
        shaders->load(core, "TreeShader", "Source/ShaderFile/TreeVS.txt", "Source/ShaderFile/TreePS.txt");
        
        // 创建PSO（用于带Alpha测试的渲染）
        createTreePSO(core, psos, shaders);

        // 加载模型
        GEMLoader::GEMModelLoader loader;
        std::vector<GEMLoader::GEMMesh> gemmeshes;
        loader.load(modelPath, gemmeshes);

        for (size_t i = 0; i < gemmeshes.size(); i++)
        {
            Mesh* mesh = new Mesh();
            std::vector<STATIC_VERTEX> vertices;
            
            for (size_t j = 0; j < gemmeshes[i].verticesStatic.size(); j++)
            {
                STATIC_VERTEX v;
                memcpy(&v, &gemmeshes[i].verticesStatic[j], sizeof(STATIC_VERTEX));
                vertices.push_back(v);
            }
            
            mesh->init(core, vertices, gemmeshes[i].indices);
            meshes.push_back(mesh);

            // 获取纹理文件名
            std::string texName = gemmeshes[i].material.find("diffuse").getValue("");
            textureFilenames.push_back(texName);

            // 加载纹理
            if (!texName.empty())
            {
                int texIdx = textureManager->getTextureIndex(texName);
                textureIndices.push_back(texIdx);
            }
            else
            {
                textureIndices.push_back(-1);
            }
        }

        // 更新世界矩阵
        updateWorldMatrix();
    }

    void createTreePSO(Core* core, PSOManager* psos, Shaders* shaders)
    {
        if (psos->psos.find("TreePSO") != psos->psos.end())
        {
            return;
        }

        Shader* shader = shaders->find("TreeShader");
        
        D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
        desc.InputLayout = VertexLayoutCache::getStaticLayout();
        desc.pRootSignature = core->rootSignature;
        desc.VS = { shader->vs->GetBufferPointer(), shader->vs->GetBufferSize() };
        desc.PS = { shader->ps->GetBufferPointer(), shader->ps->GetBufferSize() };

        // 光栅化设置 - 关闭背面剔除（树叶双面可见）
        D3D12_RASTERIZER_DESC rasterDesc = {};
        rasterDesc.FillMode = D3D12_FILL_MODE_SOLID;
        rasterDesc.CullMode = D3D12_CULL_MODE_NONE;
        rasterDesc.FrontCounterClockwise = FALSE;
        rasterDesc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
        rasterDesc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
        rasterDesc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
        rasterDesc.DepthClipEnable = TRUE;
        rasterDesc.MultisampleEnable = FALSE;
        rasterDesc.AntialiasedLineEnable = FALSE;
        rasterDesc.ForcedSampleCount = 0;
        rasterDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
        desc.RasterizerState = rasterDesc;

        // 混合设置
        D3D12_BLEND_DESC blendDesc = {};
        blendDesc.AlphaToCoverageEnable = FALSE;
        blendDesc.IndependentBlendEnable = FALSE;
        const D3D12_RENDER_TARGET_BLEND_DESC defaultRenderTargetBlend = {
            FALSE, FALSE,
            D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
            D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
            D3D12_LOGIC_OP_NOOP,
            D3D12_COLOR_WRITE_ENABLE_ALL
        };
        for (int i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; i++)
        {
            blendDesc.RenderTarget[i] = defaultRenderTargetBlend;
        }
        desc.BlendState = blendDesc;

        // 深度模板设置
        D3D12_DEPTH_STENCIL_DESC depthStencilDesc = {};
        depthStencilDesc.DepthEnable = TRUE;
        depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
        depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
        depthStencilDesc.StencilEnable = FALSE;
        desc.DepthStencilState = depthStencilDesc;

        desc.SampleMask = UINT_MAX;
        desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        desc.NumRenderTargets = 1;
        desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
        desc.SampleDesc.Count = 1;

        ID3D12PipelineState* pso;
        HRESULT hr = core->device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pso));
        psos->psos.insert({ "TreePSO", pso });
    }

    void setPosition(float x, float y, float z)
    {
        position = Vec3(x, y, z);
        updateWorldMatrix();
    }

    void setPosition(Vec3 pos)
    {
        position = pos;
        updateWorldMatrix();
    }

    void setScale(float s)
    {
        scale = s;
        updateWorldMatrix();
    }

    void setRotation(float r)
    {
        rotation = r;
        updateWorldMatrix();
    }

    void updateWorldMatrix()
    {
        Matrix s = Matrix::scaling(Vec3(scale, scale, scale));
        Matrix r = Matrix::rotateY(rotation);
        Matrix t = Matrix::translation(position);
        worldMatrix = s * r * t;
    }

    void draw(Core* core, PSOManager* psos, Shaders* shaders, Matrix& vp)
    {
        // 设置VP矩阵
        shaders->updateConstantVS("TreeShader", "staticMeshBuffer", "VP", &vp);
        
        // 设置世界矩阵
        shaders->updateConstantVS("TreeShader", "staticMeshBuffer", "W", &worldMatrix);

        // 绑定PSO
        psos->bind(core, "TreePSO");

        // 应用着色器
        shaders->apply(core, "TreeShader");

        // 绘制每个网格
        for (size_t i = 0; i < meshes.size(); i++)
        {
            // 绑定纹理
            if (textureIndices[i] >= 0)
            {
                shaders->updateTexturePS(core, "TreeShader", "colorTex", textureIndices[i]);
            }

            meshes[i]->draw(core);
        }
    }

    ~Tree()
    {
        for (auto mesh : meshes)
        {
            delete mesh;
        }
        meshes.clear();
    }
};
