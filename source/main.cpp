#include <switch.h>

#include "manager.hpp"
#include "nx/error.hpp"
#include "ui/MainApplication.hpp"

int main(int argc, char* argv[])
{
    app::manager::initApp();
    try
    {
        auto renderer_opts = pu::ui::render::RendererInitOptions(SDL_INIT_EVERYTHING, pu::ui::render::RendererHardwareFlags);
        renderer_opts.UseImage(pu::ui::render::ImgAllFlags);
        renderer_opts.UseRomfs();
        renderer_opts.SetPlServiceType(PlServiceType_User);
        renderer_opts.AddDefaultFontPath("romfs:/DroidSansCJK-Regular.ttf");
        renderer_opts.AddDefaultSharedFont(PlSharedFontType_NintendoExt);
        renderer_opts.AddExtraDefaultFontSize(32);
        renderer_opts.AddExtraDefaultFontSize(42);
        renderer_opts.SetInputPlayerCount(1);
        renderer_opts.AddInputNpadStyleTag(HidNpadStyleSet_NpadStandard);
        renderer_opts.AddInputNpadIdType(HidNpadIdType_Handheld);
        renderer_opts.AddInputNpadIdType(HidNpadIdType_No1);

        auto renderer = pu::ui::render::Renderer::New(renderer_opts);
        auto main = app::ui::MainApplication::New(renderer);
        main->Load();
        main->ShowWithFadeIn();
    }
    catch (std::exception& e)
    {
        LOG_DEBUG("An error occurred:\n%s", e.what());
    }
    app::manager::deinitApp();
    return 0;
}
