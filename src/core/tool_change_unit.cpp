#include "tool_change_unit.hpp"
#include "core_symbol.hpp"
#include "imp/imp_interface.hpp"
#include "pool/unit.hpp"
#include <iostream>

namespace horizon {

ToolChangeUnit::ToolChangeUnit(Core *c, ToolID tid) : ToolBase(c, tid)
{
}

bool ToolChangeUnit::can_begin()
{
    return core.y;
}

ToolResponse ToolChangeUnit::begin(const ToolArgs &args)
{
    bool r;
    UUID unit_uuid;
    std::tie(r, unit_uuid) = imp->dialogs.select_unit(core.r->m_pool);
    if (!r) {
        return ToolResponse::end();
    }
    auto new_unit = core.r->m_pool->get_unit(unit_uuid);
    auto sym = core.y->get_symbol();
    const auto old_unit = sym->unit;
    std::map<UUID, UUID> pinmap; // old pin->new pin
    for (const auto &it : new_unit->pins) {
        auto pin_name = it.second.primary_name;
        auto it_old_pin = std::find_if(old_unit->pins.begin(), old_unit->pins.end(),
                                       [pin_name](const auto &x) { return x.second.primary_name == pin_name; });
        if (it_old_pin != old_unit->pins.end()) {
            pinmap[it_old_pin->first] = it.first;
        }
    }
    sym->unit = new_unit;
    for (const auto &it : pinmap) {
        sym->pins.emplace(it.second, sym->pins.at(it.first));
    }
    core.r->commit();
    return ToolResponse::end();
}


ToolResponse ToolChangeUnit::update(const ToolArgs &args)
{
    return ToolResponse();
}
} // namespace horizon
