/*
 * Engine - Implementation wrapping The Forge
 *
 * This file encapsulates all The Forge complexity.
 * The Game project never sees any of this.
 */

#include "Engine/IEngine.h"

// The Forge headers (internal only)
#include "Application/Interfaces/IApp.h"
#include "Application/Interfaces/IFont.h"
#include "Application/Interfaces/IUI.h"
#include "Graphics/Interfaces/IGraphics.h"
#include "Utilities/Interfaces/ILog.h"
#include "Utilities/Interfaces/ITime.h"

// Simple app implementation
class EngineApp : public IApp
{
public:
    bool Init() override
    {
        LOGF(LogLevel::eINFO, "Engine Init");
        return true;
    }

    void Exit() override
    {
        LOGF(LogLevel::eINFO, "Engine Exit");
    }

    bool Load(ReloadDesc* pReloadDesc) override
    {
        LOGF(LogLevel::eINFO, "Engine Load");
        return true;
    }

    void Unload(ReloadDesc* pReloadDesc) override
    {
        LOGF(LogLevel::eINFO, "Engine Unload");
    }

    void Update(float deltaTime) override
    {
        // Game logic updates go here
    }

    void Draw() override
    {
        // Rendering goes here
    }

    const char* GetName() override
    {
        return "Engine";
    }
};

// Global engine state
static EngineApp* gpEngineApp = NULL;

// C API implementation
bool engineInit(EngineDesc* pDesc)
{
    if (!pDesc)
        return false;

    // Create the engine app
    gpEngineApp = new EngineApp();
    if (!gpEngineApp)
        return false;

    // Configure app settings
    gpEngineApp->mSettings.mWidth = pDesc->mWidth > 0 ? pDesc->mWidth : 1920;
    gpEngineApp->mSettings.mHeight = pDesc->mHeight > 0 ? pDesc->mHeight : 1080;
    gpEngineApp->mSettings.mFullScreen = pDesc->mFullScreen;

    // Initialize using The Forge's Windows main
    extern int WindowsMain(int argc, char** argv, IApp* app);

    // Note: This is a simplified version. In production, you'd need to handle
    // the main loop differently to give control back to the game

    LOGF(LogLevel::eINFO, "Engine initialized: %s", pDesc->pApplicationName);
    return true;
}

void engineShutdown()
{
    if (gpEngineApp)
    {
        delete gpEngineApp;
        gpEngineApp = NULL;
    }
}

void engineUpdate(float deltaTime)
{
    if (gpEngineApp)
    {
        gpEngineApp->Update(deltaTime);
    }
}

void engineDraw()
{
    if (gpEngineApp)
    {
        gpEngineApp->Draw();
    }
}

bool engineShouldQuit()
{
    if (gpEngineApp)
    {
        return gpEngineApp->mSettings.mQuit;
    }
    return true;
}
