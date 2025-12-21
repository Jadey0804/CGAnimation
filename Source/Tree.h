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
    std::vector<bool> isBark;
    TextureManager* textureManager;
    Matrix worldMatrix;
    Vec3 position;
    float scale;
    float rotation;
    
    // Instancing 相关
    static const int INSTANCE_COUNT = 5;
    float instanceOffsets[INSTANCE_COUNT * 4];
    bool useInstancing;  // 是否启用实例化

    // 风动画参数
    float windStrength;    // 风力强度
    float windSpeed;       // 风速
    float leafThreshold;   // 树叶高度阈值（低于此高度的顶点不受风影响）

    Tree()
    {
        position = Vec3(0, 0, 0);
        scale = 1.0f;
        rotation = 0.0f;
        textureManager = nullptr;
        useInstancing = false;  // 默认关闭实例化
        
        // 风动画默认参数
        windStrength = 2.0f;     // 风力强度
        windSpeed = 2.0f;        // 风速
        leafThreshold = 30.0f;   // 树叶高度阈值
        
        // 初始化实例偏移量，X轴相隔10
        for (int i = 0; i < INSTANCE_COUNT; i++)
        {
            instanceOffsets[i * 4 + 0] = i * 100.0f;  // X偏移
            instanceOffsets[i * 4 + 1] = 0.0f;       // Y偏移
            instanceOffsets[i * 4 + 2] = 0.0f;       // Z偏移
            instanceOffsets[i * 4 + 3] = 0.0f;       // padding
        }
    }

    void init(Core* core, Shaders* shaders, PSOManager* psos, TextureManager* texManager, 
              std::string modelPath)
    {
        textureManager = texManager;

        // 加载着色器
        shaders->load(core, "TreeShader", "Source/ShaderFile/TreeVS.txt", "Source/ShaderFile/TreePS.txt");
        
        // 创建PSO
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
            
            if (gemmeshes.size() == 2 && materialName.empty() && diffuseTex.empty())
            {
                isBarkMesh = (i == 0);
            }

            isBark.push_back(isBarkMesh);

            if (isBarkMesh)
            {
                textureFilenames.push_back("Models/Textures/bark02_ALB.png");
                textureIndices.push_back(barkTexIdx);
                normalFilenames.push_back("Models/Textures/bark02_NH.png");
                normalIndices.push_back(barkNormalIdx);
            }
            else
            {
                textureFilenames.push_back("Models/Textures/willow branch_ALB.png");
                textureIndices.push_back(leafTexIdx);
                normalFilenames.push_back("Models/Textures/willow branch_NH.png");
                normalIndices.push_back(leafNormalIdx);
            }
        }

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

    void setInstanceOffset(int index, float x, float y, float z)
    {
        if (index >= 0 && index < INSTANCE_COUNT)
        {
            instanceOffsets[index * 4 + 0] = x;
            instanceOffsets[index * 4 + 1] = y;
            instanceOffsets[index * 4 + 2] = z;
            instanceOffsets[index * 4 + 3] = 0.0f;
        }
    }

    // 启用/禁用实例化
    void setInstancing(bool enabled)
    {
        useInstancing = enabled;
    }

    // 切换实例化状态
    void toggleInstancing()
    {
        useInstancing = !useInstancing;
    }

    // 设置风动画参数
    void setWindParameters(float strength, float speed, float threshold)
    {
        windStrength = strength;
        windSpeed = speed;
        leafThreshold = threshold;
    }

    void setWindStrength(float strength)
    {
        windStrength = strength;
    }

    void setWindSpeed(float speed)
    {
        windSpeed = speed;
    }

    void setLeafThreshold(float threshold)
    {
        leafThreshold = threshold;
    }

    void draw(Core* core, PSOManager* psos, Shaders* shaders, Matrix& vp, float time)
    {
        // 设置VP矩阵
        shaders->updateConstantVS("TreeShader", "staticMeshBuffer", "VP", &vp);
        
        // 设置世界矩阵
        shaders->updateConstantVS("TreeShader", "staticMeshBuffer", "W", &worldMatrix);
        
        // 设置实例偏移量
        shaders->updateConstantVS("TreeShader", "staticMeshBuffer", "instanceOffsets", instanceOffsets);

        // 设置风动画参数
        shaders->updateConstantVS("TreeShader", "staticMeshBuffer", "time", &time);
        shaders->updateConstantVS("TreeShader", "staticMeshBuffer", "windStrength", &windStrength);
        shaders->updateConstantVS("TreeShader", "staticMeshBuffer", "windSpeed", &windSpeed);
        shaders->updateConstantVS("TreeShader", "staticMeshBuffer", "leafThreshold", &leafThreshold);

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

            // 根据实例化开关决定绘制方式
            if (useInstancing)
            {
                // 使用 instancing 绘制5棵树
                drawInstanced(core, meshes[i]);
            }
            else
            {
                // 普通绘制1棵树
                meshes[i]->draw(core);
            }
        }
    }

    // 使用instancing绘制
    void drawInstanced(Core* core, Mesh* mesh)
    {
        core->getCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        core->getCommandList()->IASetVertexBuffers(0, 1, &mesh->vbView);
        core->getCommandList()->IASetIndexBuffer(&mesh->ibView);
        core->getCommandList()->DrawIndexedInstanced(mesh->numMeshIndices, INSTANCE_COUNT, 0, 0, 0);
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
