#include "ui/MainApplication.hpp"
#include "ui/LocalInstallPage.hpp"
#include "install/InstallTask.hpp"
#include "install/LocalWorker.hpp"
#include "util/config.hpp"
#include "util/i18n.hpp"
#include "util/util.hpp"
#include "nx/udisk.hpp"
#include "nx/misc.hpp"
#include "nx/nsp.hpp"
#include "nx/xci.hpp"
#include "facade.hpp"

namespace app
{
    void InstallFromLocalFile(std::vector<nx::fs::Path> ourTitleList, NcmStorageId destStorageId, install::LocalStorageSource storageSrc)
    {
        facade::ShowInstaller(storageSrc == install::LocalStorageSource::SDMC ? "inst.sd.source_string"_lang : "inst.hdd.source_string"_lang);

        bool fileInstalled = true;
        unsigned int titleItr;
        try
        {
            unsigned int titleCount = ourTitleList.size();
            for (titleItr = 0; titleItr < titleCount; titleItr++)
            {
                if (titleCount > 1)
                {
                    facade::SendPageInfoTextAndRender("inst.info_page.installing"_lang +
                                                      "(" + std::to_string(titleItr + 1) + "/"  + std::to_string(titleCount) +
                                                      ") " + nx::misc::ShortenString(ourTitleList[titleItr].filename().string(), 42, 4));
                }
                else
                {
                    facade::SendPageInfoTextAndRender("inst.info_page.installing"_lang + nx::misc::ShortenString(ourTitleList[titleItr].filename().string(), 42, 4));
                }

                std::string ext = ourTitleList[titleItr].extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                std::unique_ptr<nx::Content> content;
                if (ext == ".xci" || ext == ".xcz")
                {
                    content = std::make_unique<nx::XCI>();
                }
                else
                {
                    content = std::make_unique<nx::NSP>();
                }
                std::unique_ptr<install::Worker> worker = std::make_unique<install::LocalWorker>(std::move(content), ourTitleList[titleItr], storageSrc);
                std::unique_ptr<InstallTask> installTask = std::make_unique<InstallTask>(destStorageId, config::overClock, config::ignoreReqVers, config::fixTicket, config::skipBase, worker.get());

                facade::SendInstallProgress(0);
                installTask->Prepare();
                installTask->InstallTicketCert();
                installTask->Begin();
            }
        }
        catch (std::exception& e)
        {
            facade::NotifyInstallFailed(e, nx::misc::ShortenString(ourTitleList[titleItr].filename().string(), 42, 4));
            fileInstalled = false;
        }

        if (fileInstalled)
        {
            facade::NotifyInstallSuccess(ourTitleList.size(), nx::misc::ShortenString(ourTitleList[0].filename().string(), 42, 4));
            if (config::deletePrompt && storageSrc == install::LocalStorageSource::SDMC)
            {
                if(facade::ShowDialog(std::to_string(ourTitleList.size()) + "inst.sd.delete_info_multi"_lang,
                                    "inst.sd.delete_desc"_lang, {"common.no"_lang, "common.yes"_lang}, false, "delete") == 1)
                {
                    for (size_t i = 0; i < ourTitleList.size(); i++)
                    {
                        if (nx::fs::Exists(ourTitleList[i]))
                        {
                            try { nx::fs::Remove(ourTitleList[i]); } catch (...) {};
                        }
                    }
                }
            }
        }

        facade::SendInstallFinished();
    }

namespace ui
{
    struct LocalInstallPage::InternalData
    {
        size_t subPathCounter = 0;
        size_t selectedSize = 0;
        bool isRootDirectory = true;
        std::string driveMountPointName{};
        int driveIndex = -1; // -1 means SDMC
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
        this->menu = pu::ui::elm::Menu::New(0, 154_dp, 1920_dp, COLOR("#FFFFFF00"), COLOR("#5F5F5FFF"), config::GetSubMenuItemSize(), config::GetSubMenuHeight() / config::GetSubMenuItemSize());
        this->menu->SetScrollbarWidth(this->menu->GetScrollbarWidth() / config::GetScreenScaleFactor());
        this->menu->SetScrollbarMargin(this->menu->GetScrollbarMargin() / config::GetScreenScaleFactor());
        this->menu->SetIconMargin(20_dp);
        this->menu->SetTextMargin(20_dp);
        this->menu->SetItemsFocusBorderMargin(10_dp);
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
        if (pageData->driveIndex != -1 && !nx::udisk::DriveExists(pageData->driveIndex, pageData->driveMountPointName)) 
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
        if (pageData->driveIndex != -1 && !nx::udisk::DriveExists(pageData->driveIndex, pageData->driveMountPointName))
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
        clearPageData();
        InstallFromLocalFile(fileList,
                             dialogResult ? NcmStorageId_BuiltInUser : NcmStorageId_SdCard,
                             pageData->driveIndex == -1 ? install::LocalStorageSource::SDMC : install::LocalStorageSource::UDISK);
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
            clearPageData();
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
            if (pageData->driveIndex != -1 && !nx::udisk::DriveExists(pageData->driveIndex, pageData->driveMountPointName))
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

        UpdateTouchState(Pos, 0, 154_dp, 1920_dp, std::min(this->menu->GetItems().size() * config::GetSubMenuItemSize(), (size_t)config::GetSubMenuHeight()));
    }

    void LocalInstallPage::openSdmc()
    {
        pageData->driveMountPointName = "sdmc:";
        this->drawMenuItems(pageData->driveMountPointName);
    }

    bool LocalInstallPage::tryOpenUdisk()
    {
        int ret = -1;
        u32 deviceCount = nx::udisk::GetDriveCount();
        if(deviceCount > 1)
        {
            std::vector<std::string> mountPointList;
            for (u32 i = 0; i < deviceCount; i++)
            {
                mountPointList.push_back(nx::udisk::GetMountPointName(i));
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
        pageData->driveMountPointName = nx::udisk::GetMountPointName(ret);
        pageData->driveIndex = ret;
        this->drawMenuItems(pageData->driveMountPointName);
        return true;
    }

    void LocalInstallPage::clearPageData()
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
}
