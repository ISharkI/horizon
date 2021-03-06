#include "tool_copy_placement.hpp"
#include "core_board.hpp"
#include "imp/imp_interface.hpp"
#include <iostream>

namespace horizon {

ToolCopyPlacement::ToolCopyPlacement(Core *c, ToolID tid) : ToolBase(c, tid)
{
}

bool ToolCopyPlacement::can_begin()
{
    if (!core.b)
        return false;

    return std::count_if(core.r->selection.begin(), core.r->selection.end(),
                         [](const auto &x) { return x.type == ObjectType::BOARD_PACKAGE; })
           > 0;
}

ToolResponse ToolCopyPlacement::begin(const ToolArgs &args)
{
    imp->tool_bar_set_tip("LMB: pick reference RMB: cancel");
    return ToolResponse();
}
ToolResponse ToolCopyPlacement::update(const ToolArgs &args)
{
    if (args.type == ToolEventType::CLICK) {
        if (args.button == 1) {
            if (args.target.type == ObjectType::BOARD_PACKAGE || args.target.type == ObjectType::PAD) {
                auto pkg_uuid = args.target.path.at(0);
                auto brd = core.b->get_board();
                const auto &ref_pkg = brd->packages.at(pkg_uuid);
                const auto &ref_group = ref_pkg.component->group;
                const auto &ref_tag = ref_pkg.component->tag;

                std::set<BoardPackage *> target_pkgs;
                for (const auto &it : core.r->selection) {
                    if (it.type == ObjectType::BOARD_PACKAGE) {
                        target_pkgs.insert(&brd->packages.at(it.uuid));
                    }
                }

                UUID target_group;
                for (const auto it : target_pkgs) {
                    if (it->component->group) {
                        target_group = it->component->group;
                        break;
                    }
                }

                if (!target_group) {
                    imp->tool_bar_flash("no target group found");
                    core.r->revert();
                    return ToolResponse::end();
                }

                BoardPackage *target_pkg = nullptr;
                for (auto it : target_pkgs) {
                    if (it->component->tag == ref_tag) {
                        target_pkg = it;
                        break;
                    }
                }

                if (!target_pkg) {
                    imp->tool_bar_flash("no target package found");
                    core.r->revert();
                    return ToolResponse::end();
                }

                for (auto it : target_pkgs) {
                    if (it != target_pkg) {
                        BoardPackage *this_ref_pkg = nullptr;
                        for (auto &it_ref : brd->packages) {
                            if (it_ref.second.component->tag == it->component->tag
                                && it_ref.second.component->group == ref_group) {
                                this_ref_pkg = &it_ref.second;
                            }
                        }
                        if (this_ref_pkg) {
                            auto offset = this_ref_pkg->placement.shift - ref_pkg.placement.shift;
                            auto delta_angle = target_pkg->placement.get_angle() - ref_pkg.placement.get_angle();
                            Placement tr;
                            tr.set_angle(delta_angle);

                            it->placement.shift = target_pkg->placement.shift + tr.transform(offset);
                            it->placement.set_angle(this_ref_pkg->placement.get_angle() + delta_angle);
                        }
                    }
                }

                core.r->commit();
                return ToolResponse::end();
            }
            else {
                imp->tool_bar_flash("please click on a package");
            }
        }
        else if (args.button == 3) {
            core.r->revert();
            return ToolResponse::end();
        }
    }
    return ToolResponse();
}
} // namespace horizon
