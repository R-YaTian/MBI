#include "ui/MainApplication.hpp"
#include "ui/MainPage.hpp"
#include "ui/sdInstPage.hpp"
#include "installer.hpp"
#include "facade.hpp"
#include "util/util.hpp"
#include "util/config.hpp"
#include "util/i18n.hpp"

namespace app::ui
{
    static s32 prev_touchcount = 0;

    struct sdInstPage::InternalData
    {
        std::vector<int> lastIndex;
        int subPathCounter = 0;
        installer::Local::StorageSource storageSrc = installer::Local::StorageSource::SD;
    };

    sdInstPage::~sdInstPage() = default;

    sdInstPage::sdInstPage() : Layout::Layout()
    {
        this->menu = pu::ui::elm::Menu::New(0, 154, 1920, COLOR("#FFFFFF00"), COLOR("#00000033"), app::config::subMenuItemSize, (836 / app::config::subMenuItemSize));
        this->menu->SetScrollbarColor(COLOR("#17090980"));
        this->menu->SetShadowBaseAlpha(0);
        this->Add(this->menu);
        pageData = std::make_unique<InternalData>();
    }

    void sdInstPage::drawMenuItems(bool clearItems, std::filesystem::path ourPath)
    {
        s32 menuIndex = this->menu->GetSelectedIndex();
        if (clearItems)
        {
            this->selectedTitles = {};
        }

        this->currentDir = ourPath;

		auto pathStr = this->currentDir.string();
		if(pathStr.length())
		{
			if(pathStr[pathStr.length() - 1] == ':')
			{
				this->currentDir = std::filesystem::path(pathStr + "/");
			}
		}

        this->menu->ClearItems();
        this->menuIndices = {};

        try
        {
            this->ourDirectories = util::getDirsAtPath(this->currentDir);
            this->ourFiles = util::getDirectoryFiles(this->currentDir, {".nsp", ".nsz", ".xci", ".xcz"});
        }
        catch (std::exception& e)
        {
            this->drawMenuItems(false, this->currentDir.parent_path());
            return;
        }

        if (this->currentDir != "sdmc:/")
        {
            std::string itm = "..";
            auto ourEntry = pu::ui::elm::MenuItem::New(itm);
            ourEntry->SetColor(COLOR(app::config::DirTextColor));
            ourEntry->SetIcon(GetResource(Resources::BackToParentImage));
            this->menu->AddItem(ourEntry);
        }

        for (auto& file: this->ourDirectories)
        {
            if (file == "..")
            {
                break;
            }
            std::string itm = file.filename().string();
            auto ourEntry = pu::ui::elm::MenuItem::New(itm);
            ourEntry->SetColor(COLOR(app::config::DirTextColor));
            ourEntry->SetIcon(GetResource(Resources::DirectoryImage));
            this->menu->AddItem(ourEntry);
        }

        for (long unsigned int i = 0; i < this->ourFiles.size(); i++)
        {
            auto& file = this->ourFiles[i];

            std::string itm = file.filename().string();
            auto ourEntry = pu::ui::elm::MenuItem::New(itm);
            ourEntry->SetColor(COLOR(app::config::FileTextColor));
            ourEntry->SetIcon(GetResource(Resources::UncheckedImage));
            for (long unsigned int j = 0; j < this->selectedTitles.size(); j++)
            {
                if (this->selectedTitles[j] == file)
                {
                    ourEntry->SetIcon(GetResource(Resources::CheckedImage));
                }
            }
            this->menu->AddItem(ourEntry);
            this->menu->SetSelectedIndex(menuIndex);
            this->menuIndices.push_back(i);
        }
    }

    void sdInstPage::followDirectory()
    {
        int selectedIndex = this->menu->GetSelectedIndex();
        int dirListSize = this->ourDirectories.size();
        int selectNewIndex = 0;
        if (this->currentDir != "sdmc:/")
        {
            dirListSize++;
            selectedIndex--;
        }

        if (selectedIndex < dirListSize)
        {
            if (this->menu->GetItems()[this->menu->GetSelectedIndex()]->GetName() == ".." && this->menu->GetSelectedIndex() == 0)
            {
                this->drawMenuItems(true, this->currentDir.parent_path());
                if (pageData->subPathCounter > 0)
                {
                    pageData->subPathCounter--;
                    selectNewIndex = pageData->lastIndex[pageData->subPathCounter];
                    pageData->lastIndex.pop_back();
                }
            }
            else
            {
                this->drawMenuItems(true, this->ourDirectories[selectedIndex]);
                if (pageData->subPathCounter > 0 || pageData->storageSrc == installer::Local::StorageSource::UDISK)
                {
                    pageData->lastIndex.push_back(selectedIndex + 1);
                }
                else
                {
                    pageData->lastIndex.push_back(selectedIndex);
                }
                pageData->subPathCounter++;
            }
            this->menu->SetSelectedIndex(selectNewIndex);
        }
    }

    void sdInstPage::selectFile(int selectedIndex, bool redraw)
    {
        int dirListSize = this->ourDirectories.size();
        if (this->currentDir != "sdmc:/")
        {
            dirListSize++;
        }

        long unsigned int fileIdx = 0;
        if (this->menuIndices.size() > 0)
        {
            fileIdx = this->menuIndices[selectedIndex - dirListSize];
        }

        if (this->menu->GetItems()[selectedIndex]->GetIconTexture() == GetResource(Resources::CheckedImage))
        {
            for (long unsigned int i = 0; i < this->selectedTitles.size(); i++)
            {
                if (this->selectedTitles[i] == this->ourFiles[fileIdx])
                {
                    this->selectedTitles.erase(this->selectedTitles.begin() + i);
                    break;
                }
            }
        }
        else if (this->menu->GetItems()[selectedIndex]->GetIconTexture() == GetResource(Resources::UncheckedImage))
        {
            this->selectedTitles.push_back(this->ourFiles[fileIdx]);
        }
        else
        {
            this->followDirectory();
            return;
        }
        if (redraw)
        {
            this->drawMenuItems(false, currentDir);
        }
    }

    void sdInstPage::startInstall()
    {
        int dialogResult = -1;
        if (this->selectedTitles.size() == 1)
        {
            dialogResult = app::facade::ShowDialog("inst.target.desc0"_lang + app::util::shortenString(std::filesystem::path(this->selectedTitles[0]).filename().string(), 32, true) + "inst.target.desc1"_lang, "common.cancel_desc"_lang, {"inst.target.opt0"_lang, "inst.target.opt1"_lang}, false);
        }
        else
        {
            dialogResult = app::facade::ShowDialog("inst.target.desc00"_lang + std::to_string(this->selectedTitles.size()) + "inst.target.desc01"_lang, "common.cancel_desc"_lang, {"inst.target.opt0"_lang, "inst.target.opt1"_lang}, false);
        }
        if (dialogResult == -1)
        {
            return;
        }
        app::installer::Local::installFromFile(this->selectedTitles, dialogResult, pageData->storageSrc);
        pageData->subPathCounter = 0;
        pageData->lastIndex.clear();
    }

    void sdInstPage::onInput(u64 Down, u64 Up, u64 Held, pu::ui::TouchPoint Pos)
    {
        if (Down & HidNpadButton_B)
        {
            if (pageData->subPathCounter > 0)
            {
                this->menu->SetSelectedIndex(0);
                this->followDirectory();
            }
            else
            {
                SceneJump(Scene::Main);
            }
        }

        if ((Down & HidNpadButton_A) || (!IsTouched() && prev_touchcount == 1))
        {
            prev_touchcount = 0;
            this->selectFile(this->menu->GetSelectedIndex());
            if (this->ourFiles.size() == 1 && this->selectedTitles.size() == 1)
            {
                this->startInstall();
            }
        }

        if ((Down & HidNpadButton_Y))
        {
            if (this->selectedTitles.size() == this->ourFiles.size())
            {
                this->drawMenuItems(true, currentDir);
            }
            else
            {
                int topDir = 0;
                if (this->currentDir != "sdmc:/")
                {
                    topDir++;
                }
                for (long unsigned int i = this->ourDirectories.size() + topDir; i < this->menu->GetItems().size(); i++)
                {
                    if (this->menu->GetItems()[i]->GetIconTexture() == GetResource(Resources::CheckedImage))
                    {
                        continue;
                    }
                    else
                    {
                        this->selectFile(i, false);
                    }
                }
                this->drawMenuItems(false, currentDir);
            }
        }

        if ((Down & HidNpadButton_Minus))
        {
            app::facade::ShowDialog(pageData->storageSrc == installer::Local::StorageSource::SD ? "inst.sd.help.title"_lang : "inst.hdd.help.title"_lang,
                                    pageData->storageSrc == installer::Local::StorageSource::SD ? "inst.sd.help.desc"_lang : "inst.hdd.help.desc"_lang,
                                    {"common.ok"_lang}, true);
        }

        if (Down & HidNpadButton_ZL)
        {
            this->menu->SetSelectedIndex(std::max(0, this->menu->GetSelectedIndex() - 6));
        }
        if (Down & HidNpadButton_ZR)
        {
            this->menu->SetSelectedIndex(std::min((s32)this->menu->GetItems().size() - 1, this->menu->GetSelectedIndex() + 6));
        }

        if (Down & HidNpadButton_Plus)
        {
            if (this->selectedTitles.size() == 0 && this->menu->GetItems()[this->menu->GetSelectedIndex()]->GetIconTexture() == GetResource(Resources::UncheckedImage))
            {
                this->selectFile(this->menu->GetSelectedIndex());
            }
            if (this->selectedTitles.size() > 0)
            {
                this->startInstall();
            }
        }

        if (IsTouched())
        {
            prev_touchcount = 1;
        }
    }

    void sdInstPage::setMenuIndex(int index)
    {
        this->menu->SetSelectedIndex(index);
    }

    void sdInstPage::setStorageSourceToSdmc()
    {
        pageData->storageSrc = installer::Local::StorageSource::SD;
    }

    void sdInstPage::setStorageSourceToUdisk()
    {
        pageData->storageSrc = installer::Local::StorageSource::UDISK;
    }
}
