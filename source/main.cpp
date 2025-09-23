#include <thread>
#include "switch.h"
#include "util/error.hpp"
#include "ui/MainApplication.hpp"
#include "util/util.hpp"
#include "util/config.hpp"

using namespace pu::ui::render;
int main(int argc, char* argv[])
{
    inst::util::initApp();
    try {
        auto renderer_opts = pu::ui::render::RendererInitOptions(SDL_INIT_EVERYTHING, pu::ui::render::RendererHardwareFlags);
        renderer_opts.UseImage(pu::ui::render::ImgAllFlags);
        renderer_opts.UseRomfs();
        renderer_opts.AddDefaultAllSharedFonts();
        renderer_opts.AddExtraDefaultFontSize(22);
        renderer_opts.AddExtraDefaultFontSize(30);
        renderer_opts.AddExtraDefaultFontSize(32);
        renderer_opts.AddExtraDefaultFontSize(42);
        Mix_Init(pu::audio::MixerAllFlags);

        auto renderer = Renderer::New(renderer_opts);
        auto main = inst::ui::MainApplication::New(renderer);
        std::thread updateThread;
        if (inst::config::autoUpdate && inst::util::getIPAddress() != "1.0.0.127") updateThread = std::thread(inst::util::checkForAppUpdate);
        main->Load();
        main->ShowWithFadeIn();
        updateThread.join();
    } catch (std::exception& e) {
        LOG_DEBUG("An error occurred:\n%s", e.what());
    }
    inst::util::deinitApp();
    return 0;
}
