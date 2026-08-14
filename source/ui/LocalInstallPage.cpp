#include "ui/MainApplication.hpp"
#include "ui/LocalInstallPage.hpp"
#include "installer.hpp"
#include "facade.hpp"
#include "util/util.hpp"
#include "util/config.hpp"
#include "util/i18n.hpp"
#include "nx/udisk.hpp"

namespace app::ui
{
    struct LocalInstallPage::InternalData
    {
        size_t subPathCounter = 0;
        size_t selectedSize = 0;
        bool isRootDirectory = true;
        installer::Local::StorageSource storageSrc = installer::Local::StorageSource::SD;
        std::string driveMountPointName{};
        int driveIndex = -1;
        nx::fs::Path currentDir;
        std::vector<nx::fs::Path> menuDirectories;
        std::vector<nx::fs::Path> menuFiles;
        std::unordered_map<std::string, nx::fs::Path> selectedTitles;
        std::vector<u32> lastIndex;
    };

    LocalInstallPage::~LocalInstallPage() = default;

    LocalInstallPage::LocalInstallPage() : BaseMenuPage()
    {
        this->SetOnInput(std::bind(&LocalInstallPage::onInput, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
        this->menu = pu::ui::elm::Menu::New(0, 154, 1920, COLOR("#FFFFFF00"), COLOR("#5F5F5FFF"), app::config::subMenuItemSize, (836 / app::config::subMenuItemSize));
        this->menu->SetShadowBaseAlpha(0);
        this->menu->SetIconMargin(20);
        this->menu->SetTextMargin(20);
        this->Add(this->menu);
        pageData = std::make_unique<InternalData>();
    }

    void LocalInstallPage::drawMenuItems(nx::fs::Path ourPath)
    {
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

        try
        {
            pageData->menuDirectories.clear();
            pageData->menuDirectories = nx::fs::GetDirsAtPath(pageData->currentDir);
            std::sort(pageData->menuDirectories.begin(), pageData->menuDirectories.end(), app::util::IgnoreCaseCompare);
            pageData->menuFiles.clear();
            pageData->menuFiles = nx::fs::GetDirectoryFiles(pageData->currentDir, {".nsp", ".nsz", ".xci", ".xcz"});
            std::sort(pageData->menuFiles.begin(), pageData->menuFiles.end(), app::util::IgnoreCaseCompare);
        }
        catch (std::exception& e)
        {
            this->drawMenuItems(pageData->currentDir.parent_path());
            return;
        }

        this->menu->ClearItems();
        pageData->selectedSize = 0;

        if (!pageData->isRootDirectory)
        {
            std::string itm = "..";
            auto ourEntry = pu::ui::elm::MenuItem::New(itm);
            ourEntry->SetColor(COLOR(app::config::DirTextColor));
            ourEntry->SetIcon(GetResource(Resources::BackToParentImage));
            this->menu->AddItem(ourEntry);
        }

        for (const auto& file : pageData->menuDirectories)
        {
            std::string itm = file.filename().string();
            auto ourEntry = pu::ui::elm::MenuItem::New(itm);
            ourEntry->SetColor(COLOR(app::config::DirTextColor));
            ourEntry->SetIcon(GetResource(Resources::DirectoryImage));
            this->menu->AddItem(ourEntry);
        }

        for (const auto& file : pageData->menuFiles)
        {
            std::string itm = file.filename().string();
            auto ourEntry = pu::ui::elm::MenuItem::New(itm);
            ourEntry->SetColor(COLOR(app::config::FileTextColor));
            ourEntry->SetPreserveTailLength(4);
            ourEntry->SetTruncationMarker("(...)");
            if (pageData->selectedTitles.contains(file.string()))
            {
                ourEntry->SetIcon(GetResource(Resources::CheckedImage));
                ++pageData->selectedSize;
            }
            else
            {
                ourEntry->SetIcon(GetResource(Resources::UncheckedImage));
            }
            this->menu->AddItem(ourEntry);
        }
    }

    void LocalInstallPage::followDirectory()
    {
        int selectedIndex = this->menu->GetSelectedIndex();
        int dirListSize = pageData->menuDirectories.size() + (pageData->isRootDirectory ? 0 : 1);
        int newIndex = 0;

        if (selectedIndex < dirListSize)
        {
            if (this->menu->GetItems()[this->menu->GetSelectedIndex()]->GetName() == ".." && this->menu->GetSelectedIndex() == 0)
            {
                this->drawMenuItems(pageData->currentDir.parent_path());
                if (pageData->subPathCounter > 0)
                {
                    pageData->subPathCounter--;
                    newIndex = pageData->lastIndex[pageData->subPathCounter];
                    pageData->lastIndex.pop_back();
                }
            }
            else
            {
                this->drawMenuItems(pageData->menuDirectories[selectedIndex + (pageData->isRootDirectory ? 0 : -1)]);
                pageData->lastIndex.push_back(selectedIndex);
                pageData->subPathCounter++;
            }
            this->menu->SetSelectedIndex(newIndex);
        }
    }

    void LocalInstallPage::selectFile(int selectedIndex)
    {
        if (pageData->driveIndex != -1 && nx::udisk::getMountPointName(pageData->driveIndex) != pageData->driveMountPointName)
        {
            pageData->subPathCounter = 0;
            onCancel();
            return;
        }

        int dirListSize = pageData->menuDirectories.size() + (pageData->isRootDirectory ? 0 : 1);
        size_t fileIdx = selectedIndex - dirListSize;

        if (this->menu->GetItems()[selectedIndex]->GetIconTexture() == GetResource(Resources::CheckedImage))
        {
            this->menu->GetItems()[selectedIndex]->SetIcon(GetResource(Resources::UncheckedImage));
            this->pageData->selectedTitles.erase(pageData->menuFiles[fileIdx].string());
            --pageData->selectedSize;
        }
        else if (this->menu->GetItems()[selectedIndex]->GetIconTexture() == GetResource(Resources::UncheckedImage))
        {
            this->menu->GetItems()[selectedIndex]->SetIcon(GetResource(Resources::CheckedImage));
            this->pageData->selectedTitles[pageData->menuFiles[fileIdx].string()] = this->pageData->menuFiles[fileIdx];
            ++pageData->selectedSize;
        }
        else
        {
            this->followDirectory();
        }
    }

    void LocalInstallPage::startInstall()
    {
        if (pageData->driveIndex != -1 && nx::udisk::getMountPointName(pageData->driveIndex) != pageData->driveMountPointName)
        {
            pageData->subPathCounter = 0;
            onCancel();
            return;
        }

        int dialogResult = -1;
        dialogResult = app::facade::ShowDialog("inst.target.desc00"_lang +
                                               std::to_string(pageData->selectedTitles.size()) +
                                               "inst.target.desc01"_lang,
                                               "common.cancel_desc"_lang,
                                              {"inst.target.opt0"_lang, "inst.target.opt1"_lang, "common.cancel"_lang}, true,
                                               "install-disk");
        if (dialogResult < 0)
        {
            return;
        }
        std::vector<nx::fs::Path> fileList;
        for (const auto& pair : this->pageData->selectedTitles)
        {
            fileList.push_back(pair.second);
        }
        ClearPageData();
        app::installer::Local::InstallFromFile(fileList, dialogResult ? NcmStorageId_BuiltInUser : NcmStorageId_SdCard, pageData->storageSrc);
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
            ClearPageData();
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
        if (pageData->selectedSize == pageData->menuFiles.size())
        {
            for (size_t i = pageData->menuDirectories.size() + (pageData->isRootDirectory ? 0 : 1); i < this->menu->GetItems().size(); i++)
            {
                this->selectFile(i);
            }
        }
        else
        {
            for (size_t i = pageData->menuDirectories.size() + (pageData->isRootDirectory ? 0 : 1); i < this->menu->GetItems().size(); i++)
            {
                if (this->menu->GetItems()[i]->GetIconTexture() == GetResource(Resources::CheckedImage))
                {
                    continue;
                }
                else
                {
                    this->selectFile(i);
                }
            }
        }
    }

    void LocalInstallPage::onInput(const u64 Down, const u64 Up, const u64 Held, const pu::ui::TouchPoint Pos)
    {
        if (Down & HidNpadButton_B)
        {
            if (pageData->driveIndex != -1 && nx::udisk::getMountPointName(pageData->driveIndex) != pageData->driveMountPointName)
            {
                pageData->subPathCounter = 0;
            }
            onCancel();
        }

        if ((Down & HidNpadButton_A) || IsTouchUp())
        {
            this->selectFile(this->menu->GetSelectedIndex());
        }

        if ((Down & HidNpadButton_Y))
        {
            onSelectAll();
        }

        if ((Down & HidNpadButton_Minus))
        {
            app::facade::ShowDialog("common.help"_lang,
                                    "inst.sd.help_desc"_lang,
                                    {"common.ok"_lang}, true, "information");
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
        pageData->storageSrc = installer::Local::StorageSource::SD;
        this->drawMenuItems("sdmc:");
    }

    bool LocalInstallPage::setStorageSourceToUdisk()
    {
        int ret = -1;
        u32 deviceCount = nx::udisk::getDeviceCount();
        if(deviceCount > 1)
        {
            std::vector<std::string> mountPointList;
            for (u32 i = 0; i < deviceCount; i++)
            {
                mountPointList.push_back(nx::udisk::getMountPointName(i));
            }
            mountPointList.push_back("common.cancel"_lang);
            ret = app::facade::ShowDialog("main.hdd.title"_lang, "inst.hdd.multi_device_desc"_lang, mountPointList, true, "install-disk");
            if (ret < 0)
            {
                return false;
            }
        }
        else if (deviceCount == 1)
        {
            ret = 0;
        }
        else
        {
            app::facade::ShowDialog("main.hdd.title"_lang, "main.hdd.notfound"_lang, {"common.ok"_lang}, true, "information");
            return false;
        }
        pageData->storageSrc = installer::Local::StorageSource::UDISK;
        pageData->driveMountPointName = nx::udisk::getMountPointName(ret);
        pageData->driveIndex = ret;
        this->drawMenuItems(pageData->driveMountPointName);
        return true;
    }

    void LocalInstallPage::ClearPageData()
    {
        pageData->subPathCounter = 0;
        pageData->selectedSize = 0;
        pageData->driveIndex = -1;
        pageData->driveMountPointName.clear();
        pageData->currentDir.clear();
        pageData->menuDirectories.clear();
        pageData->menuFiles.clear();
        pageData->selectedTitles.clear();
        pageData->lastIndex.clear();
        this->menu->ClearItems();
    }
}
