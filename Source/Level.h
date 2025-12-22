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

class Core;
class PSOManager;
class Shaders;
class TextureManager;

enum class LevelObjType { Static, Anim, Plane, Skybox };

struct LevelObject
{
    LevelObjType type = LevelObjType::Static;
    std::string modelPath;   // STATIC / ANIM
    std::string animName;    // 

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

    void update(float dt, const Vec3& cameraPos);

    void draw(Matrix& vp, Matrix& skyVP, float time, const Vec3& cameraPos);

    void clear();

private:
    Core* m_core = nullptr;
    PSOManager* m_psos = nullptr;
    Shaders* m_shaders = nullptr;
    TextureManager* m_textures = nullptr;
    Plane* m_plane = nullptr;
    Skybox* m_skybox = nullptr;

    AnimatedModel* allocAlignedAnim();
    void freeAlignedAnim(AnimatedModel* p);

    std::unordered_map<std::string, StaticModel*>   m_staticCache;
    std::unordered_map<std::string, AnimatedModel*> m_animCache;

    struct AnimEntry
    {
        AnimatedModel* model = nullptr;
        AnimationInstance instance;
        std::string animName;
        bool inited = false;
        
        //  avoid resetting animation every frame
        bool isColliding = false;
        std::string idleAnimName;  
        std::string runAnimName;   

        //float AnimRotY;
		//float AnimMoveSpeed;
		//Vec3 AnimCurrentPos;

    };

    std::vector<LevelObject> m_objects;
    std::vector<int> m_objectToAnimIndex;
    std::vector<AnimEntry> m_animEntries;

private:
    StaticModel* getOrLoadStatic(const std::string& path);
    AnimatedModel* getOrLoadAnim(const std::string& path);

    static bool parseLine(const std::string& line, LevelObject& outObj);
    static Matrix buildWorld(const LevelObject& o);
    static float degToRad(float deg) { return deg * 3.141592654f / 180.0f; }
};
