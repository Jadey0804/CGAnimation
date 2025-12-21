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
    std::vector<std::string> normalFilenames;
    std::vector<int> textureIndices;
    std::vector<int> normalIndices;
    std::vector<bool> isBark;  // 标记是否是树干部分
    TextureManager* textureManager;
    Matrix worldMatrix;
    Vec3 position;
    float scale;
    float rotation;

    Tree()
    {
        position = Vec3(0, 0, 0);
        scale = 1.0f;
        rotation = 0.0f;
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

        // 预加载所有纹理
        int barkTexIdx = textureManager->getTextureIndex("Models/Textures/bark02_ALB.png");
        int barkNormalIdx = textureManager->getTextureIndex("Models/Textures/bark02_NH.png");
        int leafTexIdx = textureManager->getTextureIndex("Models/Textures/willow branch_ALB.png");
        int leafNormalIdx = textureManager->getTextureIndex("Models/Textures/willow branch_NH.png");

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

            // 检查材质名称来判断是树干还是树叶
            std::string materialName = gemmeshes[i].material.find("name").getValue("");
            std::string diffuseTex = gemmeshes[i].material.find("diffuse").getValue("");
            
            // 调试输出材质信息
            printf("Mesh %zu: material name = '%s', diffuse = '%s'\n", i, materialName.c_str(), diffuseTex.c_str());

            // 判断是否是树干（通过材质名称或纹理名称包含 "bark" 来判断）
            bool isBarkMesh = false;
            if (materialName.find("bark") != std::string::npos || 
                materialName.find("Bark") != std::string::npos ||
                materialName.find("trunk") != std::string::npos ||
                materialName.find("Trunk") != std::string::npos ||
                diffuseTex.find("bark") != std::string::npos ||
                diffuseTex.find("Bark") != std::string::npos)
            {
                isBarkMesh = true;
            }
            
            // 如果模型只有一个网格或者无法通过材质名判断，使用网格索引
            // 通常树模型中，索引0是树干，索引1是树叶
            if (gemmeshes.size() == 2 && materialName.empty() && diffuseTex.empty())
            {
                isBarkMesh = (i == 0);  // 假设第一个网格是树干
            }

            isBark.push_back(isBarkMesh);

            if (isBarkMesh)
            {
                // 树干使用树皮贴图
                textureFilenames.push_back("Models/Textures/bark02_ALB.png");
                textureIndices.push_back(barkTexIdx);
                normalFilenames.push_back("Models/Textures/bark02_NH.png");
                normalIndices.push_back(barkNormalIdx);
                printf("  -> Using bark texture\n");
            }
            else
            {
                // 树叶使用柳叶贴图
                textureFilenames.push_back("Models/Textures/willow branch_ALB.png");
                textureIndices.push_back(leafTexIdx);
                normalFilenames.push_back("Models/Textures/willow branch_NH.png");
                normalIndices.push_back(leafNormalIdx);
                printf("  -> Using leaf texture\n");
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
            // 绑定颜色纹理 (t0)
            if (textureIndices[i] >= 0)
            {
                shaders->updateTexturePS(core, "TreeShader", "colorTex", textureIndices[i]);
            }

            // 绑定法线纹理 (t1)
            if (normalIndices[i] >= 0)
            {
                shaders->updateTexturePS(core, "TreeShader", "normalTex", normalIndices[i]);
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
