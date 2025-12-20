#include "Level.h"
#include <fstream>
#include <sstream>

// 你项目里用到的：Shaders::updateConstantVS(...) 在 Game.cpp 里已存在 :contentReference[oaicite:3]{index=3}

//#include <new>// for std::align_val_t
#include <malloc.h>   // _aligned_malloc / _aligned_free


AnimatedModel* Level::allocAlignedAnim()
{
    void* mem = _aligned_malloc(sizeof(AnimatedModel), 64);
    if (!mem) return nullptr;
    return new (mem) AnimatedModel();
}

void Level::freeAlignedAnim(AnimatedModel* p)
{
    if (!p) return;
    p->~AnimatedModel();
    _aligned_free(p);
}




void Level::init(Core* core, PSOManager* psos, Shaders* shaders, TextureManager* textures,
    Plane* plane, Skybox* skybox)
{
    m_core = core;
    m_psos = psos;
    m_shaders = shaders;
    m_textures = textures;
    m_plane = plane;
    m_skybox = skybox;
}

void Level::clear()
{
    m_objects.clear();
    m_objectToAnimIndex.clear();
    m_animEntries.clear();

    for (auto& kv : m_staticCache) delete kv.second;
    for (auto& kv : m_animCache) freeAlignedAnim(kv.second);

    m_staticCache.clear();
    m_animCache.clear();
}

StaticModel* Level::getOrLoadStatic(const std::string& path)
{
    auto it = m_staticCache.find(path);
    if (it != m_staticCache.end()) {
        return it->second;
    }

    auto* m = new StaticModel();
    // StaticModel::load(Core*, std::string, Shaders*, PSOManager*) :contentReference[oaicite:4]{index=4}
    m->load(m_core, path, m_shaders, m_psos);
    m_staticCache[path] = m;
    return m;
}

AnimatedModel* Level::getOrLoadAnim(const std::string& path)
{
    auto it = m_animCache.find(path);
    if (it != m_animCache.end()) return it->second;

    auto* m = allocAlignedAnim();
    m->load(m_core, path, m_psos, m_shaders);

    // ——对齐你在 Game.cpp 里对 TRex 贴图的“手动指定 + 预加载”逻辑 :contentReference[oaicite:6]{index=6}
    // 如果该模型有 mesh 但贴图名没填，就全部指定为你当前使用的贴图：
    if (!m->textureFilenames.empty())
    {
        for (auto& s : m->textureFilenames)
        {
            if (s.empty())
                s = "Models/Textures/T-rex_Base_Color_alb.png";
            // 预加载纹理索引（你的 TextureManager 有 getTextureIndex 用法） :contentReference[oaicite:7]{index=7}
            m_textures->getTextureIndex(s);
        }
    }

    m_animCache[path] = m;
    return m;
}

bool Level::loadFromFile(const std::string& levelPath)
{
    std::ifstream f(levelPath);
    if (!f.is_open()) return false;

    m_objects.clear();
    m_objectToAnimIndex.clear();
    m_animEntries.clear();

    std::string line;
    while (std::getline(f, line)) {
        LevelObject obj;

		if (line.find("#") != std::string::npos || line.empty()) continue;

        if (!parseLine(line, obj)) continue;

        int animIndex = -1;

        if (obj.type == LevelObjType::Static)
        {
            (void)getOrLoadStatic(obj.modelPath);
        }
        else if (obj.type == LevelObjType::Anim)
        {
            AnimEntry e;
            e.model = getOrLoadAnim(obj.modelPath);

            // animName：优先用文件里的；没有就用模型里的第一个动画
            if (!obj.animName.empty())
            {
                e.animName = obj.animName;
            }
            else if (!e.model->animation.animations.empty())
            {
                e.animName = e.model->animation.animations.begin()->first;
            }
            else
            {
                e.animName = "";
            }

            // AnimationInstance::init(Animation*, int) :contentReference[oaicite:8]{index=8}
            e.instance.init(&e.model->animation, 0);
            e.inited = true;

            animIndex = (int)m_animEntries.size();
            m_animEntries.push_back(std::move(e));
        }

        m_objects.push_back(std::move(obj));
        m_objectToAnimIndex.push_back(animIndex);
    }

    return true;
}

void Level::update(float dt)
{
    for (auto& e : m_animEntries)
    {
        if (!e.inited) continue;
        if (e.animName.empty()) continue;

        // 保护：动画名不存在则不更新
        if (!e.model->animation.hasAnimation(e.animName)) continue;

        // AnimationInstance::update(name, dt) :contentReference[oaicite:9]{index=9}
        e.instance.update(e.animName, dt);

        if (e.instance.animationFinished())
        {
            e.instance.resetAnimationTime();
        }
    }
}

void Level::draw(Matrix& vp, Matrix& skyVP, float time, const Vec3& cameraPos)
{
    // 你 Game.cpp 里会更新 StaticModelUntextured / AnimatedTextured 的 VP 常量 :contentReference[oaicite:10]{index=10}
    m_shaders->updateConstantVS("StaticModelUntextured", "staticMeshBuffer", "VP", &vp);
    m_shaders->updateConstantVS("AnimatedTextured", "staticMeshBuffer", "VP", &vp);

    // 先画 plane（接口：Plane::draw(Core*,PSOManager*,Shaders*,TextureManager*,Matrix)）
    if (m_plane)
    {
        m_plane->draw(m_core, m_psos, m_shaders, m_textures, vp);
    }

    // 再画静态/动画对象
    for (size_t i = 0; i < m_objects.size(); i++)
    {
        const LevelObject& o = m_objects[i];

        if (o.type == LevelObjType::Static)
        {
            StaticModel* m = getOrLoadStatic(o.modelPath);
            Matrix W = buildWorld(o);
            // updateWorld(Shaders*, Matrix&) + draw(Core*,PSOManager*,Shaders*,Matrix&) :contentReference[oaicite:12]{index=12}
            m->updateWorld(m_shaders, W);
            m->draw(m_core, m_psos, m_shaders, vp);
        }
        else if (o.type == LevelObjType::Anim)
        {
            int idx = m_objectToAnimIndex[i];
            if (idx < 0 || idx >= (int)m_animEntries.size()) continue;

            AnimEntry& e = m_animEntries[idx];
            Matrix W = buildWorld(o);

            // AnimatedModel::draw(Core*,PSOManager*,Shaders*,TextureManager*,AnimationInstance*,Matrix&,Matrix&) :contentReference[oaicite:13]{index=13}
            e.model->draw(m_core, m_psos, m_shaders, m_textures, &e.instance, vp, W);
        }
    }

    // 最后画 skybox：W=translation(cameraPos), VP=无平移的 skyVP
    if (m_skybox)
    {
        Matrix W = Matrix::translation(cameraPos);
        m_skybox->draw(m_core, m_psos, m_shaders, m_textures, time, &W, &skyVP); //:contentReference[oaicite:14]{ index = 14 }
    }
}

bool Level::parseLine(const std::string& line, LevelObject& outObj)
{
    std::string s = line;
    if (s.empty()) return false;

    // 跳过空白行
    bool allSpace = true;
    for (char c : s) { if (!isspace((unsigned char)c)) { allSpace = false; break; } }
    if (allSpace) return false;

    // 跳过注释
    size_t first = s.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return false;
    if (s[first] == '#') return false;

    std::istringstream iss(s);

    std::string type;
    iss >> type;
    if (!iss) return false;

    auto toType = [&](const std::string& t) -> LevelObjType
        {
            if (t == "STATIC") return LevelObjType::Static;
            if (t == "ANIM")   return LevelObjType::Anim;
            if (t == "PLANE")  return LevelObjType::Plane;
            if (t == "SKYBOX") return LevelObjType::Skybox;
            return LevelObjType::Static;
        };

    outObj.type = toType(type);

    // 统一格式：
    // type model px py pz sx sy sz rx ry rz [animName]
    iss >> outObj.modelPath;
    iss >> outObj.pos.x >> outObj.pos.y >> outObj.pos.z;
    iss >> outObj.scale.x >> outObj.scale.y >> outObj.scale.z;
    iss >> outObj.rotDeg.x >> outObj.rotDeg.y >> outObj.rotDeg.z;

    if (outObj.type == LevelObjType::Anim)
    {
        iss >> outObj.animName; // 可选
    }

    return true;
}

Matrix Level::buildWorld(const LevelObject& o)
{
    Matrix S = Matrix::scaling(o.scale);
    Matrix Rx = Matrix::rotateX(degToRad(o.rotDeg.x));
    Matrix Ry = Matrix::rotateY(degToRad(o.rotDeg.y));
    Matrix Rz = Matrix::rotateZ(degToRad(o.rotDeg.z));
    Matrix T = Matrix::translation(o.pos);

    Matrix R = (Rx * Ry) * Rz;
    return (S * R) * T;
}
