#include "nx/ext.hpp"
#include "nx/ncm.hpp"
#include "nx/error.hpp"
#include <switch.h>
#include <sstream>
#include <iomanip>

namespace nx::ext
{
    constexpr size_t ApplicationRecordBufferCount = 30;
    NsApplicationRecord g_ApplicationRecordBuffer[ApplicationRecordBufferCount];

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

    AppMetaMap ScanApplicationsContentMetaStatus()
    {
        nsInitialize();
        AppMetaMap applicationsMetaMap;
        s32 cur_offset = 0;
        while (true)
        {
            s32 record_count = 0;
            if(R_FAILED(nsListApplicationRecord(g_ApplicationRecordBuffer, ApplicationRecordBufferCount, cur_offset, &record_count)) || record_count == 0)
            {
                break;
            }

            cur_offset += record_count;
            for(s32 i = 0; i < record_count; i++)
            {
                u64 app_id = g_ApplicationRecordBuffer[i].application_id;
                s32 status_count = 0;
                nsCountApplicationContentMeta(app_id, &status_count);
                std::vector<NsApplicationContentMetaStatus> entries(status_count);
                nsListApplicationContentMetaStatus(app_id, 0, entries.data(), entries.size(), &status_count);
                entries.resize(status_count);
                if (status_count > 0)
                {
                    applicationsMetaMap[app_id] = entries;
                }
            }
        }
        nsExit();
        return applicationsMetaMap;
    }

    static bool ExistsApplicationContent(const AppMetaMap& app_meta_map, const u64 program_id, const NcmContentMetaType content_type)
    {
        const auto app_id = ncm::GetBaseTitleId(program_id, content_type);
        auto it = app_meta_map.find(app_id);
        if (it != app_meta_map.end())
        {
            const auto& entries = it->second;
            const auto cnt_it = std::find_if(entries.begin(), entries.end(), [&](const NsApplicationContentMetaStatus &cnt_status) -> bool {
                return (cnt_status.application_id == program_id) && (cnt_status.storageID != NcmStorageId_GameCard);
            });

            if (cnt_it != entries.end())
            {
                return true;
            }
            else
            {
                return false;
            }
        }
        return false;
    }

    static bool ExistsApplicationAnyContents(const AppMetaMap& app_meta_map, const u64 program_id)
    {
        if (ExistsApplicationContent(app_meta_map, program_id, NcmContentMetaType_Application))
        {
            return true;
        }
        if (ExistsApplicationContent(app_meta_map, program_id, NcmContentMetaType_Patch))
        {
            return true;
        }
        if (ExistsApplicationContent(app_meta_map, program_id, NcmContentMetaType_AddOnContent))
        {
            return true;
        }
        return false;
    }

    std::vector<Ticket> ScanTickets(const AppMetaMap* app_meta_map)
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
                    if (app_meta_map != nullptr)
                    {
                        const auto app_id = esGetRightsIdApplicationId(&common_tik.rights_id);
                        if (ExistsApplicationAnyContents(*app_meta_map, app_id))
                        {
                            continue; // Skip this ticket if any contents exist for the application
                        }
                    }
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
                    if (app_meta_map != nullptr)
                    {
                        const auto app_id = esGetRightsIdApplicationId(&personalized_tik.rights_id);
                        if (ExistsApplicationAnyContents(*app_meta_map, app_id))
                        {
                            continue; // Skip this ticket if any contents exist for the application
                        }
                    }
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
