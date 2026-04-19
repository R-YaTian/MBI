/*
    This file is port from Goldleaf (https://github.com/XorTroll/Goldleaf)
    Goldleaf - Multipurpose homebrew tool for Nintendo Switch
    Copyright © 2018-2025 XorTroll

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "nx/acc.hpp"
#include "nx/misc.hpp"
#include "nx/fs.hpp"
#include <switch-ipcext.h>
#include <sstream>
#include <random>
#include <chrono>
#include <fstream>
#include <iostream>

namespace nx::acc
{
    constexpr std::string ACCOUNT_PATH = "account:/su";

    class Generator
    {
        unsigned long generateBytes()
        {
            std::uniform_int_distribution<unsigned long> byteGenerator(0x0UL, 0xFFFFFFFFFFFFFFFF);
            return byteGenerator(engine);
        }

        const std::string generateRandomAlphanumericString(size_t len)
        {
            static constexpr auto chars =
                "0123456789"
                "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                "abcdefghijklmnopqrstuvwxyz";

            std::uniform_int_distribution<size_t> dist(0, std::strlen(chars) - 1);
            auto result = std::string(len, '\0');
            std::generate_n(std::begin(result), len, [&]() { return chars[dist(engine)]; });
            return result;
        }

        std::string stringReplace(const std::string& str, const std::string& from, const std::string& to)
        {
            std::string strCopy = str;
            size_t startPos = strCopy.find(from);
            if (startPos == std::string::npos)
            {
                return str;
            }
            strCopy.replace(startPos, from.length(), to);
            return strCopy;
        }

        private:
            std::mt19937_64 engine;
            unsigned long _naccountId;
            unsigned long _baasUserId;
            std::string _naccountIdStr;
            const u64 BAAS_HEADER2 = 0x0000006E00000001;
            const u64 BAAS_HEADER3 = 0x0000000100000001;
            const std::string PROFILE = R"({"id":"#NAS_ID#","language":"#LOCALE#","timezone":"#TIMEZONE#","country":"#COUNTRY_CODE#","gender":"male","birthday":"2000-01-01","isChild":false,"email":"•","screenName":"•","region":"","loginId":"•","nickname":"•","isNnLinked":false,"isTwitterLinked":false,"isFacebookLinked":false,"isGoogleLinked":false})";

        public:
            Generator():
                engine(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count())
            {
                _baasUserId = generateBytes();
                _naccountId = generateBytes();
                std::stringstream ss;
                ss << std::hex << _naccountId;
                _naccountIdStr = ss.str();
            }

            const std::string& GetNaccountIdStr()
            {
                return _naccountIdStr;
            }

            void WriteBaas(const std::string& fullpath)
            {
                auto account_id = generateBytes();
                std::ofstream ofs(fullpath, std::ios::binary | std::ios::trunc);
                ofs.write(reinterpret_cast<char*>(&account_id), sizeof(account_id));
                ofs.write(reinterpret_cast<const char*>(&BAAS_HEADER2), sizeof(BAAS_HEADER2));
                ofs.write(reinterpret_cast<char*>(&_naccountId), sizeof(_naccountId));
                ofs.write(reinterpret_cast<const char*>(&BAAS_HEADER3), sizeof(BAAS_HEADER3));
                ofs.write(reinterpret_cast<char*>(&_baasUserId), sizeof(_baasUserId));
                ofs << generateRandomAlphanumericString(40);
            }

            void WriteProfileData(const std::string& fullpath)
            {
                std::ofstream ofs(fullpath, std::ios::binary | std::ios::trunc);
                ofs << generateRandomAlphanumericString(128);
            }

            void WriteProfileJson(const std::string& fullpath)
            {
                std::ofstream ofs(fullpath, std::ios::binary | std::ios::trunc);
                std::string locale = misc::GetLocale();
                std::string timezone = misc::GetTimeZone();
                std::string country_code = misc::GetCountryCode(locale);
                std::string generated_profile = stringReplace(
                    stringReplace(
                        stringReplace(
                            stringReplace(
                                PROFILE,
                                "#NAS_ID#",
                                _naccountIdStr
                            ),
                            "#LOCALE#",
                            locale
                        ),
                        "#TIMEZONE#",
                        timezone
                    ),
                    "#COUNTRY_CODE#",
                    country_code
                );
                std::cout << "generated profile: " << generated_profile << std::endl;
                ofs << generated_profile;
            }
    };

    static void TerminateAccountProgram()
    {
        pmshellInitialize();
        pmshellTerminateProgram(0x010000000000000C);  // BCAT
        pmshellTerminateProgram(0x010000000000001E);  // ACCOUNT
        pmshellTerminateProgram(0x010000000000003E);  // OLSC
        pmshellExit();
    }

    static FsFileSystem MountAccountData()
    {
        FsFileSystem acc;
        TerminateAccountProgram();
        fsOpen_SystemSaveData(&acc, FsSaveDataSpaceId_System, 0x8000000000000010, (AccountUid) {0});
        fsdevMountDevice("account", acc);
        return acc;
    }

    static void UnmountAccountData(FsFileSystem& acc, bool commit=false)
    {
        if (commit)
        {
            fsdevCommitDevice("account");
        }
        fsdevUnmountDevice("account");
        fsFsClose(&acc);
    }

    static std::string FormatAccountUidString(const AccountUid user_id)
    {
        std::stringstream uid_str;
        uid_str << std::setfill('0') << std::setw(8) << std::hex << (user_id.uid[0] & 0xffffffff) << "-";
        uid_str << std::setfill('0') << std::setw(4) << std::hex << ((user_id.uid[0] >> 32) & 0xffff) << "-";
        uid_str << std::setfill('0') << std::setw(4) << std::hex << ((user_id.uid[0] >> 48) & 0xffff) << "-";
        uid_str << std::setfill('0') << std::setw(2) << std::hex << (user_id.uid[1] & 0xff);
        uid_str << std::setfill('0') << std::setw(2) << std::hex << ((user_id.uid[1] >> 8) & 0xff) << "-";
        uid_str << std::setfill('0') << std::setw(8) << std::hex << ((user_id.uid[1] >> 32) & 0xffffffff);
        uid_str << std::setfill('0') << std::setw(4) << std::hex << ((user_id.uid[1] >> 16) & 0xffff);
        return uid_str.str();
    }

    namespace
    {
        AccountUid g_SelectedUser = {};

        inline Result ShowUserSelector(AccountUid *out_user_id)
        {
            const PselUserSelectionSettings selection_cfg = {};
            return pselShowUserSelector(out_user_id, &selection_cfg);
        }
    }

    AccountUid GetSelectedUser()
    {
        return g_SelectedUser;
    }

    bool HasSelectedUser()
    {
        return accountUidIsValid(&g_SelectedUser);
    }

    void SetSelectedUser(const AccountUid user_id)
    {
        g_SelectedUser = user_id;
    }

    bool SelectFromPreselectedUser()
    {
        AccountUid pre_user_id;
        const auto rc = accountGetPreselectedUser(&pre_user_id);
        if (R_SUCCEEDED(rc) && accountUidIsValid(&pre_user_id))
        {
            SetSelectedUser(pre_user_id);
            return true;
        }
        return false;
    }

    bool SelectUser()
    {
        AccountUid user_id;
        if (R_SUCCEEDED(ShowUserSelector(&user_id)))
        {
            if (accountUidIsValid(&user_id))
            {
                SetSelectedUser(user_id);
                return true;
            }
        }
        return false;
    }

    Result ReadSelectedUser(AccountProfileBase *out_prof_base, AccountUserData *out_user_data)
    {
        AccountProfile prof;
        auto rc = accountGetProfile(&prof, g_SelectedUser);
        if (R_SUCCEEDED(rc))
        {
            rc = accountProfileGet(&prof, out_user_data, out_prof_base);
            accountProfileClose(&prof);
        }
        return rc;
    }

    bool IsLinked()
    {
        bool linked = false;
        AccountExtAdministrator baas_admin;
        const auto rc = accountextGetBaasAccountAdministrator(g_SelectedUser, &baas_admin);
        if (R_SUCCEEDED(rc))
        {
            accountextAdministratorIsLinkedWithNintendoAccount(&baas_admin, &linked);
            accountextAdministratorClose(&baas_admin);
        }
        return linked;
    }

    Result UnlinkLocally()
    {
        AccountExtAdministrator baas_admin;
        auto rc = accountextGetBaasAccountAdministrator(g_SelectedUser, &baas_admin);
        if (R_SUCCEEDED(rc))
        {
            bool linked = false;
            rc = accountextAdministratorIsLinkedWithNintendoAccount(&baas_admin, &linked);
            if (R_SUCCEEDED(rc) && linked)
            {
                rc = accountextAdministratorDeleteRegistrationInfoLocally(&baas_admin);
            }
            accountextAdministratorClose(&baas_admin);
        }
        return rc;
    }

    void LinkLocally()
    {
        FsFileSystem acc = MountAccountData();
        const std::string baasDir = ACCOUNT_PATH + "/baas";
        const std::string nasDir = ACCOUNT_PATH + "/nas";
        nx::fs::MakeDirs(baasDir);
        nx::fs::MakeDirs(nasDir);

        Generator gen;
        auto linkerFile = baasDir + "/" + FormatAccountUidString(g_SelectedUser) + ".dat";
        gen.WriteBaas(linkerFile);

        auto profileDataFilename = nasDir + "/" + gen.GetNaccountIdStr() + ".dat";
        gen.WriteProfileData(profileDataFilename);

        auto profileJsonFilename = nasDir + "/" + gen.GetNaccountIdStr() + "_user.json";
        gen.WriteProfileJson(profileJsonFilename);

        UnmountAccountData(acc, true);
        misc::AttemptForceReboot();
    }

    std::vector<u8> GetSelectedUserIcon()
    {
        AccountProfile prof;
        if (R_SUCCEEDED(accountGetProfile(&prof, g_SelectedUser)))
        {
            u32 icon_size = 0;
            accountProfileGetImageSize(&prof, &icon_size);
            if (icon_size > 0)
            {
                std::vector<u8> icon(icon_size);
                u32 tmp_size = 0;
                if (R_SUCCEEDED(accountProfileLoadImage(&prof, icon.data(), icon_size, &tmp_size)) && tmp_size == icon_size)
                {
                    return icon;
                }
            }
            accountProfileClose(&prof);
        }
        return {};
    }
}
