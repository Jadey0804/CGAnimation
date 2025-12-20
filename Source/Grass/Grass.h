#pragma once

#include <d3d12.h>
#include <vector>
#include <string>
#include "../Core.h"
#include "../Maths.h"
#include "../Shaders.h"
#include "../PSO.h"
#include "../TextureManager.h"
#include "../Mesh.h"

// 草片顶点结构（简单版本：位置 + UV）
struct GRASS_VERTEX
{
    Vec3 pos;
    Vec3 normal;
    Vec3 tangent;
    float tu;
    float tv;
};

// 单个草实例的数据
struct GrassInstance
{
    Vec3 position;      // 草的世界位置
    float scale;        // 草的缩放
    float rotation;     // Y轴旋转（可选，用于非billboard模式）
};

class Grass
{
public:
    // 草片的顶点和索引缓冲
    ID3D12Resource* vertexBuffer = nullptr;
    ID3D12Resource* indexBuffer = nullptr;
    D3D12_VERTEX_BUFFER_VIEW vbView;
    D3D12_INDEX_BUFFER_VIEW ibView;
    unsigned int numIndices = 0;

    // 草实例列表
    std::vector<GrassInstance> instances;

    // 纹理文件名
    std::string textureFilename;

    // 草片尺寸
    float grassWidth = 0.5f;
    float grassHeight = 1.0f;

    // 是否初始化成功
    bool initialized = false;

    void init(Core* core, PSOManager* psos, Shaders* shaders, TextureManager* textureManager)
    {
        // 创建草片的quad顶点/索引缓冲
        createQuadBuffers(core);

        // 加载shader
        shaders->load(core, "Grass", "Source/ShaderFile/GrassVS.txt", "Source/ShaderFile/GrassPS.txt");

        // 创建专门的草地PSO：CullMode=NONE（双面），Depth开启，Blend关闭
        createGrassPSO(core, psos, shaders);

        // 检查PSO是否创建成功
        if (psos->psos.find("GrassPSO") != psos->psos.end() && psos->psos["GrassPSO"] != nullptr)
        {
            initialized = true;
            printf("Grass PSO created successfully!\n");
        }
        else
        {
            printf("Warning: Grass PSO creation failed!\n");
        }

        // 设置纹理路径并预加载
        textureFilename = "Models/Textures/vegetation_grass_card_03.png";
        textureManager->getTextureIndex(textureFilename);
    }

    // 添加草实例
    void addInstance(const Vec3& pos, float scale = 1.0f, float rotation = 0.0f)
    {
        GrassInstance inst;
        inst.position = pos;
        inst.scale = scale;
        inst.rotation = rotation;
        instances.push_back(inst);
    }

    // 在指定区域随机散布草
    void scatterGrass(const Vec3& center, float radius, int count, float minScale = 0.8f, float maxScale = 1.2f)
    {
        for (int i = 0; i < count; i++)
        {
            // 随机位置（圆形区域）
            float angle = ((float)rand() / RAND_MAX) * 2.0f * 3.141592654f;
            float dist = ((float)rand() / RAND_MAX) * radius;
            float x = center.x + cosf(angle) * dist;
            float z = center.z + sinf(angle) * dist;
            float y = center.y; // 假设在同一高度

            // 随机缩放
            float scale = minScale + ((float)rand() / RAND_MAX) * (maxScale - minScale);

            // 随机旋转
            float rot = ((float)rand() / RAND_MAX) * 2.0f * 3.141592654f;

            addInstance(Vec3(x, y, z), scale, rot);
        }
    }

    void draw(Core* core, PSOManager* psos, Shaders* shaders, TextureManager* textureManager,
              Matrix& vp, const Vec3& cameraPos, const Vec3& cameraRight, const Vec3& cameraUp)
    {
        if (instances.empty()) return;
        if (!initialized) return;  // 如果初始化失败，不绘制

        // 绑定PSO
        psos->bind(core, "GrassPSO");

        

        // 绑定纹理
        int texIndex = textureManager->find(textureFilename);
        if (texIndex >= 0)
        {
            shaders->updateTexturePS(core, "Grass", "grassTex", texIndex);
        }

        // 设置顶点/索引缓冲
        core->getCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        core->getCommandList()->IASetVertexBuffers(0, 1, &vbView);
        core->getCommandList()->IASetIndexBuffer(&ibView);

        // 逐实例绘制（简单方式，可以后续优化为实例化渲染）
        for (size_t i = 0; i < instances.size(); i++)
        {
            const GrassInstance& inst = instances[i];

            // 更新VP矩阵
            shaders->updateConstantVS("Grass", "GrassBuffer", "VP", &vp);

            // 更新相机方向向量（用于billboard）
            shaders->updateConstantVS("Grass", "GrassBuffer", "cameraRight", (void*)&cameraRight);
            shaders->updateConstantVS("Grass", "GrassBuffer", "cameraUp", (void*)&cameraUp);

            // 更新草片尺寸
            shaders->updateConstantVS("Grass", "GrassBuffer", "grassWidth", &grassWidth);
            shaders->updateConstantVS("Grass", "GrassBuffer", "grassHeight", &grassHeight);

            // 构建世界矩阵
            Matrix W = Matrix::scaling(Vec3(inst.scale, inst.scale, inst.scale)) *
                       Matrix::rotateY(inst.rotation) *
                       Matrix::translation(inst.position);

            shaders->updateConstantVS("Grass", "GrassBuffer", "W", &W);
            shaders->apply(core, "Grass");

            core->getCommandList()->DrawIndexedInstanced(numIndices, 1, 0, 0, 0);
        }
    }

    void clear()
    {
        instances.clear();
    }

    ~Grass()
    {
        if (vertexBuffer) vertexBuffer->Release();
        if (indexBuffer) indexBuffer->Release();
    }

private:
    void createQuadBuffers(Core* core)
    {
        // 创建一个简单的quad（4个顶点，6个索引）
        // 草片中心在底部，向上延伸
        std::vector<GRASS_VERTEX> vertices(4);

        // 左下
        vertices[0].pos = Vec3(-0.5f, 0.0f, 0.0f);
        vertices[0].normal = Vec3(0, 0, 1);
        vertices[0].tangent = Vec3(1, 0, 0);
        vertices[0].tu = 0.0f;
        vertices[0].tv = 1.0f;

        // 右下
        vertices[1].pos = Vec3(0.5f, 0.0f, 0.0f);
        vertices[1].normal = Vec3(0, 0, 1);
        vertices[1].tangent = Vec3(1, 0, 0);
        vertices[1].tu = 1.0f;
        vertices[1].tv = 1.0f;

        // 左上
        vertices[2].pos = Vec3(-0.5f, 1.0f, 0.0f);
        vertices[2].normal = Vec3(0, 0, 1);
        vertices[2].tangent = Vec3(1, 0, 0);
        vertices[2].tu = 0.0f;
        vertices[2].tv = 0.0f;

        // 右上
        vertices[3].pos = Vec3(0.5f, 1.0f, 0.0f);
        vertices[3].normal = Vec3(0, 0, 1);
        vertices[3].tangent = Vec3(1, 0, 0);
        vertices[3].tu = 1.0f;
        vertices[3].tv = 0.0f;

        std::vector<unsigned int> indices = { 0, 2, 1, 1, 2, 3 };

        // 创建顶点缓冲
        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
        heapProps.CreationNodeMask = 1;
        heapProps.VisibleNodeMask = 1;

        D3D12_RESOURCE_DESC vbDesc = {};
        vbDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        vbDesc.Width = vertices.size() * sizeof(GRASS_VERTEX);
        vbDesc.Height = 1;
        vbDesc.DepthOrArraySize = 1;
        vbDesc.MipLevels = 1;
        vbDesc.SampleDesc.Count = 1;
        vbDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

        core->device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &vbDesc,
            D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&vertexBuffer));
        core->uploadResource(vertexBuffer, vertices.data(), (int)(vertices.size() * sizeof(GRASS_VERTEX)),
            D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

        // 创建索引缓冲
        D3D12_RESOURCE_DESC ibDesc = {};
        ibDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        ibDesc.Width = indices.size() * sizeof(unsigned int);
        ibDesc.Height = 1;
        ibDesc.DepthOrArraySize = 1;
        ibDesc.MipLevels = 1;
        ibDesc.SampleDesc.Count = 1;
        ibDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

        core->device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &ibDesc,
            D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&indexBuffer));
        core->uploadResource(indexBuffer, indices.data(), (int)(indices.size() * sizeof(unsigned int)),
            D3D12_RESOURCE_STATE_INDEX_BUFFER);

        // 设置视图
        vbView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
        vbView.StrideInBytes = sizeof(GRASS_VERTEX);
        vbView.SizeInBytes = (UINT)(vertices.size() * sizeof(GRASS_VERTEX));

        ibView.BufferLocation = indexBuffer->GetGPUVirtualAddress();
        ibView.Format = DXGI_FORMAT_R32_UINT;
        ibView.SizeInBytes = (UINT)(indices.size() * sizeof(unsigned int));

        numIndices = (unsigned int)indices.size();
    }

    void createGrassPSO(Core* core, PSOManager* psos, Shaders* shaders)
    {
        if (psos->psos.find("GrassPSO") != psos->psos.end())
        {
            return;
        }

        Shader* grassShader = shaders->find("Grass");
        if (!grassShader || !grassShader->vs || !grassShader->ps)
        {
            printf("Error: Grass shader not loaded properly!\n");
            return;
        }

        D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
        desc.pRootSignature = core->rootSignature;
        desc.VS = { grassShader->vs->GetBufferPointer(), grassShader->vs->GetBufferSize() };
        desc.PS = { grassShader->ps->GetBufferPointer(), grassShader->ps->GetBufferSize() };

        // 输入布局（使用静态顶点布局）
        desc.InputLayout = VertexLayoutCache::getStaticLayout();

        // 光栅化设置：CullMode = NONE（双面渲染）
        D3D12_RASTERIZER_DESC rasterDesc = {};
        rasterDesc.FillMode = D3D12_FILL_MODE_SOLID;
        rasterDesc.CullMode = D3D12_CULL_MODE_NONE;  // 双面渲染！
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

        // 混合设置：关闭（alpha test不靠blend）
        D3D12_BLEND_DESC blendDesc = {};
        blendDesc.AlphaToCoverageEnable = FALSE;
        blendDesc.IndependentBlendEnable = FALSE;
        D3D12_RENDER_TARGET_BLEND_DESC rtBlend = {};
        rtBlend.BlendEnable = FALSE;
        rtBlend.LogicOpEnable = FALSE;
        rtBlend.SrcBlend = D3D12_BLEND_ONE;
        rtBlend.DestBlend = D3D12_BLEND_ZERO;
        rtBlend.BlendOp = D3D12_BLEND_OP_ADD;
        rtBlend.SrcBlendAlpha = D3D12_BLEND_ONE;
        rtBlend.DestBlendAlpha = D3D12_BLEND_ZERO;
        rtBlend.BlendOpAlpha = D3D12_BLEND_OP_ADD;
        rtBlend.LogicOp = D3D12_LOGIC_OP_NOOP;
        rtBlend.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        for (int i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; i++)
        {
            blendDesc.RenderTarget[i] = rtBlend;
        }
        desc.BlendState = blendDesc;

        // 深度设置：开启深度测试和写入
        D3D12_DEPTH_STENCIL_DESC depthDesc = {};
        depthDesc.DepthEnable = TRUE;
        depthDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;  // 草参与遮挡
        depthDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
        depthDesc.StencilEnable = FALSE;
        desc.DepthStencilState = depthDesc;

        desc.SampleMask = UINT_MAX;
        desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        desc.NumRenderTargets = 1;
        desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
        desc.SampleDesc.Count = 1;

        ID3D12PipelineState* pso = nullptr;
        HRESULT hr = core->device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pso));
        if (SUCCEEDED(hr) && pso)
        {
            psos->psos.insert({ "GrassPSO", pso });
        }
        else
        {
            printf("Error: Failed to create GrassPSO! HRESULT: 0x%08X\n", hr);
        }
    }
};
