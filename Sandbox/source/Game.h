#pragma once
#include <Nova.h>

using namespace Nova;

namespace Assets
{
    enum : u8
    {
        AnimationIdle = 0,
        AnimationRun,
        AnimationJump,
        ModelRobot,
        TextureGrid,
        _Length
    };
}

namespace Game
{
    using AssetIndex = u8;

    void OnCreate();
    void OnEvent();
    void OnUpdate();
    void OnRender();
    void OnRenderUI();
    void OnShutdown();

    AssetHandle GetAsset(AssetIndex index);
}
