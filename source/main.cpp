#include <switch.h>

#include "manager.hpp"
#include "nx/error.hpp"
#include "ui/MainApplication.hpp"
#include "util/config.hpp"
#include "util/i18n.hpp"

int main(int argc, char* argv[])
{
    app::manager::initApp();
    int langCode = app::i18n::Load(app::config::languageSetting);
    PlSharedFontType defaultFont = PlSharedFontType_Standard;
    std::vector<PlSharedFontType> lang_fonts = {
        PlSharedFontType_NintendoExt,
        PlSharedFontType_ChineseSimplified,
        PlSharedFontType_ExtChineseSimplified,
        PlSharedFontType_ChineseTraditional,
        PlSharedFontType_Standard,
    };
    if (langCode == 6 || langCode == 15)
    {
        defaultFont = PlSharedFontType_ChineseSimplified;
        std::erase(lang_fonts, PlSharedFontType_ChineseSimplified);
    }
    else if (langCode == 11 || langCode == 16)
    {
        defaultFont = PlSharedFontType_ChineseTraditional;
        std::erase(lang_fonts, PlSharedFontType_ChineseTraditional);
    }
    else
    {
        std::erase(lang_fonts, PlSharedFontType_Standard);
    }
    lang_fonts.push_back(PlSharedFontType_KO);

    try
    {
        auto renderer_opts = pu::ui::render::RendererInitOptions(SDL_INIT_EVERYTHING, pu::ui::render::RendererHardwareFlags);
        renderer_opts.UseImage(pu::ui::render::ImgAllFlags);
        renderer_opts.SetPlServiceType();
        renderer_opts.AddDefaultSharedFont(defaultFont);
        for (const auto& font : lang_fonts)
        {
            renderer_opts.AddDefaultSharedFont(font);
        }
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
