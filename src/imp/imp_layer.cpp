#include "imp_layer.hpp"
#include "pool/part.hpp"
#include "widgets/layer_box.hpp"
#include "util/util.hpp"

namespace horizon {
void ImpLayer::construct_layer_box(bool pack)
{
    layer_box = Gtk::manage(new LayerBox(core.r->get_layer_provider(), pack));
    layer_box->show_all();
    if (pack)
        main_window->left_panel->pack_start(*layer_box, false, false, 0);

    work_layer_binding = Glib::Binding::bind_property(layer_box->property_work_layer(), canvas->property_work_layer(),
                                                      Glib::BINDING_BIDIRECTIONAL);
    layer_opacity_binding = Glib::Binding::bind_property(layer_box->property_layer_opacity(),
                                                         canvas->property_layer_opacity(), Glib::BINDING_BIDIRECTIONAL);
    layer_box->property_highlight_mode().signal_changed().connect(
            [this] { canvas->set_highlight_mode(layer_box->property_highlight_mode()); });
    canvas->set_highlight_mode(CanvasGL::HighlightMode::DIM);

    layer_box->signal_set_layer_display().connect([this](int index, const LayerDisplay &lda) {
        LayerDisplay ld = canvas->get_layer_display(index);
        ld.visible = lda.visible;
        ld.mode = lda.mode;
        canvas->set_layer_display(index, ld);
        canvas->queue_draw();
    });
    layer_box->property_select_work_layer_only().signal_changed().connect(
            [this] { canvas->selection_filter.work_layer_only = layer_box->property_select_work_layer_only(); });
    core.r->signal_request_save_meta().connect([this] {
        json j;
        j["layer_display"] = layer_box->serialize();
        j["grid_spacing"] = canvas->property_grid_spacing().get_value();
        return j;
    });
    core.r->signal_rebuilt().connect([this] { layer_box->update(); });

    connect_action(ActionID::LAYER_DOWN, [this](const auto &a) { this->layer_up_down(false); });
    connect_action(ActionID::LAYER_UP, [this](const auto &a) { this->layer_up_down(true); });

    connect_action(ActionID::LAYER_TOP, [this](const auto &a) { this->goto_layer(0); });
    connect_action(ActionID::LAYER_BOTTOM, [this](const auto &a) { this->goto_layer(-100); });
    connect_action(ActionID::LAYER_INNER1, [this](const auto &a) { this->goto_layer(-1); });
    connect_action(ActionID::LAYER_INNER2, [this](const auto &a) { this->goto_layer(-2); });
    connect_action(ActionID::LAYER_INNER3, [this](const auto &a) { this->goto_layer(-3); });
    connect_action(ActionID::LAYER_INNER4, [this](const auto &a) { this->goto_layer(-4); });
    connect_action(ActionID::LAYER_INNER5, [this](const auto &a) { this->goto_layer(-5); });
    connect_action(ActionID::LAYER_INNER6, [this](const auto &a) { this->goto_layer(-6); });
    connect_action(ActionID::LAYER_INNER7, [this](const auto &a) { this->goto_layer(-7); });
    connect_action(ActionID::LAYER_INNER8, [this](const auto &a) { this->goto_layer(-8); });

    json j = core.r->get_meta();
    bool layers_loaded = false;
    if (!j.is_null()) {
        canvas->property_grid_spacing() = j.value("grid_spacing", 1.25_mm);
        if (j.count("layer_display")) {
            layer_box->load_from_json(j.at("layer_display"));
            layers_loaded = true;
        }
    }
    if (!layers_loaded) {
        layer_box->load_from_json(
                json_from_resource("/net/carrotIndustries/horizon/imp/"
                                   "layer_display_default.json"));
    }
}

void ImpLayer::apply_preferences()
{
    auto canvas_prefs = get_canvas_preferences();
    for (const auto &it : canvas_prefs->appearance.layer_colors) {
        layer_box->set_layer_color(it.first, it.second);
    }
    ImpBase::apply_preferences();
}
} // namespace horizon
