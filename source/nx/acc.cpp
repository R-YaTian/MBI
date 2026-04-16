
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
#include <switch-ipcext.h>

namespace nx::acc
{
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
