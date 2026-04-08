#include "nx/misc.hpp"
#include "nx/error.hpp"
#include "nx/ncm.hpp"
#include <switch.h>
#include <sstream>
#include <iomanip>

namespace nx::misc
{
    void SetBoostMode(bool enable)
    {
        static u32 previousCPU = 0;
        static u32 previousGPU = 0;
        static u32 previousEMC = 0;
        if (hosversionAtLeast(8,0,0))
        {
            appletSetCpuBoostMode(enable ? ApmCpuBoostMode_FastLoad : ApmCpuBoostMode_Normal);
        }
        else
        {
            pcvInitialize();
            if (enable)
            {
                pcvGetClockRate(PcvModule_CpuBus, &previousCPU);
                pcvGetClockRate(PcvModule_GPU, &previousGPU);
                pcvGetClockRate(PcvModule_EMC, &previousEMC);
                pcvSetClockRate(PcvModule_CpuBus, 1785000000);
                pcvSetClockRate(PcvModule_GPU, 76800000);
                pcvSetClockRate(PcvModule_EMC, 1600000000);
            }
            else
            {
                if (previousCPU != 0)
                {
                    pcvSetClockRate(PcvModule_CpuBus, previousCPU);
                    previousCPU = 0;
                }
                if (previousGPU != 0)
                {
                    pcvSetClockRate(PcvModule_GPU, previousGPU);
                    previousGPU = 0;
                }
                if (previousEMC != 0)
                {
                    pcvSetClockRate(PcvModule_EMC, previousEMC);
                    previousEMC = 0;
                }
            }
            pcvExit();
        }
    }

    std::string OpenSoftwareKeyboard(std::string guideText, std::string initialText, int LenMax)
    {
        Result rc = 0;
        SwkbdConfig kbd;
        char tmpoutstr[LenMax + 1] = {0};
        rc = swkbdCreate(&kbd, 0);
        if (R_SUCCEEDED(rc))
        {
            swkbdConfigMakePresetDefault(&kbd);
            swkbdConfigSetGuideText(&kbd, guideText.c_str());
            swkbdConfigSetInitialText(&kbd, initialText.c_str());
            swkbdConfigSetStringLenMax(&kbd, LenMax);
            rc = swkbdShow(&kbd, tmpoutstr, sizeof(tmpoutstr));
            swkbdClose(&kbd);
            if (R_SUCCEEDED(rc) && tmpoutstr[0] != 0)
            {
                return std::string(tmpoutstr);
            }
        }
        return "";
    }

    u32 GetBatteryValue()
    {
        u32 value = 255;
        Result rc = psmInitialize();

        if (R_SUCCEEDED(rc))
        {
            u32 tmp;
            rc = psmGetBatteryChargePercentage(&tmp);
            if (R_SUCCEEDED(rc))
            {
                value = tmp;
            }
            psmExit();
        }

        return value;
    }

    std::string UTF16toUTF8(const std::u16string& src)
    {
        ssize_t units = 0;
        units = utf16_to_utf8(nullptr, reinterpret_cast<const uint16_t*>(src.c_str()), 0);
        if (units <= 0)
        {
            return "";
        }

        std::string dst(units, '\0');
        units = utf16_to_utf8(reinterpret_cast<uint8_t*>(&dst[0]), reinterpret_cast<const uint16_t*>(src.c_str()), dst.size());
        if (units <= 0)
        {
            return "";
        }

        return dst;
    }

    std::u16string UTF8toUTF16(const std::string& src)
    {
        ssize_t units = 0;
        units = utf8_to_utf16(nullptr, reinterpret_cast<const uint8_t*>(src.c_str()), 0);
        if (units <= 0)
        {
            return u"";
        }

        std::u16string dst(units, '\0');
        units = utf8_to_utf16(reinterpret_cast<uint16_t*>(&dst[0]), reinterpret_cast<const uint8_t*>(src.c_str()), dst.size());
        if (units <= 0)
        {
            return u"";
        }

        return dst;
    }

    std::string ShortenString(const std::string& in, size_t maxLength, size_t preserve_tail_length, const std::string& marker)
    {
        std::u16string in_utf16 = UTF8toUTF16(in);
        size_t units = in_utf16.size();
        if (units - preserve_tail_length > maxLength)
        {
            std::u16string shortened = in_utf16.substr(0, maxLength - UTF8toUTF16(marker).size());
            if (preserve_tail_length > 0)
            {
                std::u16string tail = in_utf16.substr(units - preserve_tail_length);
                return UTF16toUTF8(shortened) + marker + UTF16toUTF8(tail);
            }
            return UTF16toUTF8(shortened) + marker;
        }
        else
        {
            return in;
        }
    }

    std::string Ticket::ToString() const
    {
        u64 app_id = esGetRightsIdApplicationId(&this->rights_id);
        std::stringstream strm;
        strm << std::uppercase << std::setfill('0') << std::setw(16) << std::hex << app_id;
        std::string str = strm.str();
        if (this->type == TicketType::Common)
        {
            str += " (Common)";
        }
        return str;
    }

    std::vector<Ticket> ScanTickets()
    {
        esInitialize();
        std::vector<Ticket> tickets;

        const auto common_count = esCountCommonTicket();
        if(common_count > 0)
        {
            const auto ids_size = common_count * sizeof(EsRightsId);
            auto ids = new EsRightsId[common_count]();
            u32 written = 0;
            if(R_SUCCEEDED(esListCommonTicket(&written, ids, ids_size)))
            {
                for(u32 i = 0; i < written; i++)
                {
                    const Ticket common_tik = {
                        .rights_id = ids[i],
                        .type = TicketType::Common
                    };
                    tickets.push_back(common_tik);
                }
            }
            delete[] ids;
        }

        const auto personalized_count = esCountPersonalizedTicket();
        if(personalized_count > 0)
        {
            const auto ids_size = personalized_count * sizeof(EsRightsId);
            auto ids = new EsRightsId[personalized_count]();
            u32 written = 0;
            if(R_SUCCEEDED(esListPersonalizedTicket(&written, ids, ids_size)))
            {
                for(u32 i = 0; i < written; i++)
                {
                    const Ticket personalized_tik = {
                        .rights_id = ids[i],
                        .type = TicketType::Personalized
                    };
                    tickets.push_back(personalized_tik);
                }
            }
            delete[] ids;
        }

        esExit();
        return tickets;
    }

    bool CleanPendingUpdate()
    {
        if (R_SUCCEEDED(nssuInitialize()))
        {
            nssuDestroySystemUpdateTask();
            nssuExit();
        }

        return ncm::CleanupPlaceHolder(NcmStorageId_BuiltInSystem);
    }
}
