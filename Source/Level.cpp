#include "Level.h"
#include <fstream>
#include <sstream>
#include <malloc.h>


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
    m->preloadTextures(m_textures);

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

            // AnimationInstance::init(Animation*, int)
            e.instance.init(&e.model->animation, 0);
            e.inited = true;
            
            // Initialization State
            e.isColliding = false;
            
            // Automatically detect idle and run animation names
            e.idleAnimName = "";
            e.runAnimName = "";
            for (auto& animPair : e.model->animation.animations)
            {
                std::string name = animPair.first;
                // Convert to lowercase for comparison
                std::string lower = name;
                for (auto& c : lower) c = tolower(c);
                
                if (lower.find("idle") != std::string::npos)
                {
                    e.idleAnimName = name;
                }
                else if (lower.find("run") != std::string::npos || lower.find("walk") != std::string::npos)
                {
                    e.runAnimName = name;
                }
            }

            animIndex = (int)m_animEntries.size();
            m_animEntries.push_back(std::move(e));
        }

        m_objects.push_back(std::move(obj));
        m_objectToAnimIndex.push_back(animIndex);
    }

    return true;
}

void Level::update(float dt, const Vec3& cameraPos)
{
    // payer AABB,centered on the camera position
    Vec3 playerHalfSize(10.0f, 10.0f, 10.0f);
    AABB playerAABB = MakePlayerAABB(cameraPos, playerHalfSize);
    
    for (size_t i = 0; i < m_objects.size(); i++)
    {
        const LevelObject& o = m_objects[i];
        int animIdx = m_objectToAnimIndex[i];
        
        if (o.type != LevelObjType::Anim || animIdx < 0)
            continue;
            
        AnimEntry& e = m_animEntries[animIdx];
        if (!e.inited) continue;
        
        //computing the world space of animals AABB
        Matrix W = buildWorld(o);
        AABB animalWorldAABB = TransformAABB(e.model->localAABB, W);
        
        // Collision detection
        bool nowColliding = Intersects(playerAABB, animalWorldAABB);
        
        // Switching animations upon state change
        if (nowColliding && !e.isColliding)
        {
            // switch to run animation
            e.animName = e.runAnimName;
            e.isColliding = true;
        }
        else if (!nowColliding && e.isColliding)
        {
            // Switch back to idle animation
            e.animName = e.idleAnimName;
            e.isColliding = false;
        }
    }
    
    // Updating all animations
    for (auto& e : m_animEntries)
    {
        if (!e.inited) continue;
        if (e.animName.empty()) continue;

        // if the animation name does not exist, do not update
   /*     if (!e.model->animation.hasAnimation(e.animName)) continue;*/

        // AnimationInstance::update(name, dt)
        e.instance.update(e.animName, dt);

        if (e.instance.animationFinished())
        {
            e.instance.resetAnimationTime();
        }
    }
}

void Level::draw(Matrix& vp, Matrix& skyVP, float time, const Vec3& cameraPos)
{
  
    m_shaders->updateConstantVS("StaticModelUntextured", "staticMeshBuffer", "VP", &vp);
    m_shaders->updateConstantVS("AnimatedTextured", "staticMeshBuffer", "VP", &vp);

    if (m_plane)
    {
        m_plane->draw(m_core, m_psos, m_shaders, m_textures, vp);
    }

    // Draw static/animated objects
    for (size_t i = 0; i < m_objects.size(); i++)
    {
        const LevelObject& o = m_objects[i];

        if (o.type == LevelObjType::Static)
        {
            StaticModel* m = getOrLoadStatic(o.modelPath);
            Matrix W = buildWorld(o);
            // updateWorld(Shaders*, Matrix&) + draw(Core*,PSOManager*,Shaders*,Matrix&)
            m->updateWorld(m_shaders, W);
            m->draw(m_core, m_psos, m_shaders, vp);
        }
        else if (o.type == LevelObjType::Anim)
        {
            int idx = m_objectToAnimIndex[i];
            if (idx < 0 || idx >= (int)m_animEntries.size()) continue;

            AnimEntry& e = m_animEntries[idx];
            Matrix W = buildWorld(o);

            // AnimatedModel::draw(Core*,PSOManager*,Shaders*,TextureManager*,AnimationInstance*,Matrix&,Matrix&)
            e.model->draw(m_core, m_psos, m_shaders, m_textures, &e.instance, vp, W);
        }
    }

    //
    if (m_skybox)
    {
        Matrix W = Matrix::translation(cameraPos);
        m_skybox->draw(m_core, m_psos, m_shaders, m_textures, time, &W, &skyVP);
    }
}

bool Level::parseLine(const std::string& line, LevelObject& outObj)
{
    std::string s = line;
    if (s.empty()) return false;

    // skip blank lines
    bool allSpace = true;
    for (char c : s) { if (!isspace((unsigned char)c)) { allSpace = false; break; } }
    if (allSpace) return false;

    // Skip comments
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

    // format: type model px py pz sx sy sz rx ry rz [animName]
    iss >> outObj.modelPath;
    iss >> outObj.pos.x >> outObj.pos.y >> outObj.pos.z;
    iss >> outObj.scale.x >> outObj.scale.y >> outObj.scale.z;
    iss >> outObj.rotDeg.x >> outObj.rotDeg.y >> outObj.rotDeg.z;

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
