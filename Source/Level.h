#pragma once
#include <string>
#include <vector>
#include <unordered_map>

#include "Maths.h"
#include "StaticModel.h"
#include "AnimatedModel.h"
#include "Animation.h"
#include "Plane.h"
#include "skybox.h"

#include <malloc.h>  // for _aligned_malloc / _aligned_free

class Core;
class PSOManager;
class Shaders;
class TextureManager;

enum class LevelObjType { Static, Anim, Plane, Skybox };

struct LevelObject
{
    LevelObjType type = LevelObjType::Static;
    std::string modelPath;   // STATIC / ANIM
    std::string animName;    // ANIM 可选：比如 run

    Vec3 pos{ 0,0,0 };
    Vec3 scale{ 1,1,1 };
    Vec3 rotDeg{ 0,0,0 };
};

class Level
{
public:
    void init(Core* core, PSOManager* psos, Shaders* shaders, TextureManager* textures,
        Plane* plane, Skybox* skybox);

    bool loadFromFile(const std::string& levelPath);

    void update(float dt);

    // vp：正常场景 VP（含平移）
    // skyVP：天空盒 VP（view 去掉平移）
    void draw(Matrix& vp, Matrix& skyVP, float time, const Vec3& cameraPos);

    void clear();

    ~Level() { clear(); }

private:
    Core* m_core = nullptr;
    PSOManager* m_psos = nullptr;
    Shaders* m_shaders = nullptr;
    TextureManager* m_textures = nullptr;
    Plane* m_plane = nullptr;
    Skybox* m_skybox = nullptr;

    AnimatedModel* allocAlignedAnim();
    void freeAlignedAnim(AnimatedModel* p);

    // 为 AnimationInstance 分配对齐内存
    AnimationInstance* allocAlignedAnimInstance();
    void freeAlignedAnimInstance(AnimationInstance* p);

    std::unordered_map<std::string, StaticModel*>   m_staticCache;
    std::unordered_map<std::string, AnimatedModel*> m_animCache;

    // 改用指针存储 AnimationInstance，确保内存对齐
    struct AnimEntry
    {
        AnimatedModel* model = nullptr;
        AnimationInstance* instance = nullptr;  // 改为指针
        std::string animName;
        bool inited = false;
    };

    std::vector<LevelObject> m_objects;
    std::vector<int> m_objectToAnimIndex; // 与 m_objects 同长度，非Anim为-1
    std::vector<AnimEntry> m_animEntries;

private:
    StaticModel* getOrLoadStatic(const std::string& path);
    AnimatedModel* getOrLoadAnim(const std::string& path);

    static bool parseLine(const std::string& line, LevelObject& outObj);
    static Matrix buildWorld(const LevelObject& o);
    static float degToRad(float deg) { return deg * 3.141592654f / 180.0f; }
};
