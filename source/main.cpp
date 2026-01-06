#include <thread>
#include "switch.h"
#include "util/error.hpp"
#include "ui/MainApplication.hpp"
#include "util/util.hpp"
#include "util/config.hpp"

using namespace pu::ui::render;
int main(int argc, char* argv[])
{
    app::util::initApp();
    try {
        auto renderer_opts = pu::ui::render::RendererInitOptions(SDL_INIT_EVERYTHING, pu::ui::render::RendererHardwareFlags);
        renderer_opts.UseImage(pu::ui::render::ImgAllFlags);
        renderer_opts.UseRomfs();
        renderer_opts.SetPlServiceType(PlServiceType_User);
        renderer_opts.AddDefaultAllSharedFonts();
        renderer_opts.AddExtraDefaultFontSize(30);
        renderer_opts.AddExtraDefaultFontSize(32);
        renderer_opts.AddExtraDefaultFontSize(42);
        renderer_opts.SetInputPlayerCount(1);
        renderer_opts.AddInputNpadStyleTag(HidNpadStyleSet_NpadStandard);
        renderer_opts.AddInputNpadIdType(HidNpadIdType_Handheld);
        renderer_opts.AddInputNpadIdType(HidNpadIdType_No1);
        Mix_Init(pu::audio::MixerAllFlags);

        auto renderer = Renderer::New(renderer_opts);
        auto main = app::ui::MainApplication::New(renderer);
        main->Load();
        main->ShowWithFadeIn();
    } catch (std::exception& e) {
        LOG_DEBUG("An error occurred:\n%s", e.what());
    }
    app::util::deinitApp();
    return 0;
}
