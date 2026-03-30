#include "ui/MainApplication.hpp"
#include "ui/LocalInstallPage.hpp"
#include "installer.hpp"
#include "facade.hpp"
#include "util/util.hpp"
#include "util/config.hpp"
#include "util/i18n.hpp"

namespace app::ui
{
    struct LocalInstallPage::InternalData
    {
        int subPathCounter = 0;
        bool isRootDirectory = true;
        installer::Local::StorageSource storageSrc = installer::Local::StorageSource::SD;
        nx::fs::Path currentDir;
        std::vector<nx::fs::Path> menuDirectories;
        std::vector<nx::fs::Path> menuFiles;
        std::vector<nx::fs::Path> selectedTitles;
        std::vector<size_t> menuIndices;
        std::vector<int> lastIndex;
    };

    LocalInstallPage::~LocalInstallPage() = default;

    LocalInstallPage::LocalInstallPage() : BaseMenuPage()
    {
        this->SetOnInput(std::bind(&LocalInstallPage::onInput, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
        this->menu = pu::ui::elm::Menu::New(0, 154, 1920, COLOR("#FFFFFF00"), COLOR("#00000033"), app::config::subMenuItemSize, (836 / app::config::subMenuItemSize));
        this->menu->SetScrollbarColor(COLOR("#17090980"));
        this->menu->SetShadowBaseAlpha(0);
        this->Add(this->menu);
        pageData = std::make_unique<InternalData>();
    }

    void LocalInstallPage::drawMenuItems(bool clearItems, nx::fs::Path ourPath)
    {
        s32 menuIndex = this->menu->GetSelectedIndex();
        if (clearItems)
        {
            pageData->selectedTitles = {};
        }

        pageData->currentDir = ourPath;

        auto pathStr = pageData->currentDir.string();
        if(pathStr.length())
        {
            if(pathStr[pathStr.length() - 1] == ':' || pathStr.substr(pathStr.length() - 2, pathStr.length() - 1) == ":/")
            {
                std::string rootDir = pathStr.substr(0, pathStr.find_last_of(':') + 1);
                pageData->currentDir = nx::fs::Path(rootDir + "/");
                pageData->isRootDirectory = true;
            }
            else
            {
                pageData->isRootDirectory = false;
            }
        }

        this->menu->ClearItems();
        pageData->menuIndices = {};

        try
        {
            pageData->menuDirectories = nx::fs::GetDirsAtPath(pageData->currentDir);
            std::sort(pageData->menuDirectories.begin(), pageData->menuDirectories.end(), app::util::IgnoreCaseCompare);
            pageData->menuFiles = nx::fs::GetDirectoryFiles(pageData->currentDir, {".nsp", ".nsz", ".xci", ".xcz"});
            std::sort(pageData->menuFiles.begin(), pageData->menuFiles.end(), app::util::IgnoreCaseCompare);
        }
        catch (std::exception& e)
        {
            this->drawMenuItems(false, pageData->currentDir.parent_path());
            return;
        }

        if (!pageData->isRootDirectory)
        {
            std::string itm = "..";
            auto ourEntry = pu::ui::elm::MenuItem::New(itm);
            ourEntry->SetColor(COLOR(app::config::DirTextColor));
            ourEntry->SetIcon(GetResource(Resources::BackToParentImage));
            this->menu->AddItem(ourEntry);
        }

        for (auto& file : pageData->menuDirectories)
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

        for (size_t i = 0; i < pageData->menuFiles.size(); i++)
        {
            auto& file = pageData->menuFiles[i];

            std::string itm = file.filename().string();
            auto ourEntry = pu::ui::elm::MenuItem::New(itm);
            ourEntry->SetColor(COLOR(app::config::FileTextColor));
            ourEntry->SetIcon(GetResource(Resources::UncheckedImage));
            ourEntry->SetPreserveTailLength(4);
            ourEntry->SetTruncationMarker("(...)");
            for (size_t j = 0; j < pageData->selectedTitles.size(); j++)
            {
                if (pageData->selectedTitles[j] == file)
                {
                    ourEntry->SetIcon(GetResource(Resources::CheckedImage));
                }
            }
            this->menu->AddItem(ourEntry);
            this->menu->SetSelectedIndex(menuIndex);
            pageData->menuIndices.push_back(i);
        }
    }

    void LocalInstallPage::followDirectory()
    {
        int selectedIndex = this->menu->GetSelectedIndex();
        int dirListSize = pageData->menuDirectories.size();
        int selectNewIndex = 0;
        if (!pageData->isRootDirectory)
        {
            dirListSize++;
            selectedIndex--;
        }

        if (selectedIndex < dirListSize)
        {
            if (this->menu->GetItems()[this->menu->GetSelectedIndex()]->GetName() == ".." && this->menu->GetSelectedIndex() == 0)
            {
                this->drawMenuItems(true, pageData->currentDir.parent_path());
                if (pageData->subPathCounter > 0)
                {
                    pageData->subPathCounter--;
                    selectNewIndex = pageData->lastIndex[pageData->subPathCounter];
                    pageData->lastIndex.pop_back();
                }
            }
            else
            {
                this->drawMenuItems(true, pageData->menuDirectories[selectedIndex]);
                if (pageData->subPathCounter > 0)
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

    void LocalInstallPage::selectFile(int selectedIndex, bool redraw)
    {
        int dirListSize = pageData->menuDirectories.size();
        if (!pageData->isRootDirectory)
        {
            dirListSize++;
        }

        size_t fileIdx = 0;
        if (pageData->menuIndices.size() > 0)
        {
            fileIdx = pageData->menuIndices[selectedIndex - dirListSize];
        }

        if (this->menu->GetItems()[selectedIndex]->GetIconTexture() == GetResource(Resources::CheckedImage))
        {
            for (size_t i = 0; i < pageData->selectedTitles.size(); i++)
            {
                if (pageData->selectedTitles[i] == pageData->menuFiles[fileIdx])
                {
                    pageData->selectedTitles.erase(pageData->selectedTitles.begin() + i);
                    break;
                }
            }
        }
        else if (this->menu->GetItems()[selectedIndex]->GetIconTexture() == GetResource(Resources::UncheckedImage))
        {
            pageData->selectedTitles.push_back(pageData->menuFiles[fileIdx]);
        }
        else
        {
            this->followDirectory();
            return;
        }
        if (redraw)
        {
            this->drawMenuItems(false, pageData->currentDir);
        }
    }

    void LocalInstallPage::startInstall()
    {
        int dialogResult = -1;
        dialogResult = app::facade::ShowDialog("inst.target.desc00"_lang +
                                               std::to_string(pageData->selectedTitles.size()) +
                                               "inst.target.desc01"_lang,
                                               "common.cancel_desc"_lang,
                                              {"inst.target.opt0"_lang, "inst.target.opt1"_lang, "common.cancel"_lang}, true,
                                               static_cast<int>(Resources::InstallDiskImage));
        if (dialogResult < 0)
        {
            return;
        }
        app::installer::Local::InstallFromFile(pageData->selectedTitles, dialogResult ? NcmStorageId_BuiltInUser : NcmStorageId_SdCard, pageData->storageSrc);
        pageData->subPathCounter = 0;
        pageData->lastIndex.clear();
    }

    void LocalInstallPage::onCancel()
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

    void LocalInstallPage::onConfirm()
    {
        if (pageData->selectedTitles.size() == 0 && this->menu->GetItems()[this->menu->GetSelectedIndex()]->GetIconTexture() == GetResource(Resources::UncheckedImage))
        {
            this->selectFile(this->menu->GetSelectedIndex());
        }
        if (pageData->selectedTitles.size() > 0)
        {
            this->startInstall();
        }
    }

    void LocalInstallPage::onSelectAll()
    {
        if (pageData->selectedTitles.size() == pageData->menuFiles.size())
        {
            this->drawMenuItems(true, pageData->currentDir);
        }
        else
        {
            int topDir = 0;
            if (!pageData->isRootDirectory)
            {
                topDir++;
            }
            for (size_t i = pageData->menuDirectories.size() + topDir; i < this->menu->GetItems().size(); i++)
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
            this->drawMenuItems(false, pageData->currentDir);
        }
    }

    void LocalInstallPage::onInput(const u64 Down, const u64 Up, const u64 Held, const pu::ui::TouchPoint Pos)
    {
        if (Down & HidNpadButton_B)
        {
            onCancel();
        }

        if ((Down & HidNpadButton_A) || IsTouchUp())
        {
            this->selectFile(this->menu->GetSelectedIndex());
            if (pageData->menuFiles.size() == 1 && pageData->selectedTitles.size() == 1)
            {
                this->startInstall();
            }
        }

        if ((Down & HidNpadButton_Y))
        {
            onSelectAll();
        }

        if ((Down & HidNpadButton_Minus))
        {
            app::facade::ShowDialog(pageData->storageSrc == installer::Local::StorageSource::SD ? "inst.sd.help.title"_lang : "inst.hdd.help.title"_lang,
                                    pageData->storageSrc == installer::Local::StorageSource::SD ? "inst.sd.help.desc"_lang : "inst.hdd.help.desc"_lang,
                                    {"common.ok"_lang}, true, static_cast<int>(Resources::InformationImage));
        }

        if (Down & HidNpadButton_ZL)
        {
            onPageUp();
        }

        if (Down & HidNpadButton_ZR)
        {
            onPageDown();
        }

        if (Down & HidNpadButton_Plus)
        {
            onConfirm();
        }

        UpdateTouchState(Pos, 0, 154, 1920, std::min(this->menu->GetItems().size() * app::config::subMenuItemSize, (size_t)836));
    }

    void LocalInstallPage::setStorageSourceToSdmc()
    {
        this->menu->SetSelectedIndex(0);
        pageData->storageSrc = installer::Local::StorageSource::SD;
    }

    void LocalInstallPage::setStorageSourceToUdisk()
    {
        this->menu->SetSelectedIndex(0);
        pageData->storageSrc = installer::Local::StorageSource::UDISK;
    }
}
