#pragma once
#include "symbol.hpp"
#include "pool.hpp"
#include "canvas/selectables.hpp"
#include "canvas/target.hpp"
#include "object_descr.hpp"
#include <memory>
#include <iostream>
#include <sigc++/sigc++.h>
#include "cores.hpp"
#include "dialogs/dialogs.hpp"
#include "layer.hpp"
#include "json_fwd.hpp"
#include "constraints.hpp"
#include <gdk/gdkkeysyms.h>

namespace horizon {
	enum class ToolEventType {MOVE, CLICK, KEY, DATA};

	/**
	 * Add new tools here.
	 */
	enum class ToolID {NONE, MOVE, PLACE_JUNCTION, DRAW_LINE,
		DELETE, DRAW_ARC, ROTATE, MIRROR, MAP_PIN, MAP_SYMBOL,
		DRAW_NET, ADD_COMPONENT, PLACE_TEXT, PLACE_NET_LABEL,
		DISCONNECT, BEND_LINE_NET, SELECT_NET_SEGMENT, SELECT_NET,
		PLACE_POWER_SYMBOL, MOVE_NET_SEGMENT, MOVE_NET_SEGMENT_NEW,
		EDIT_COMPONENT_PIN_NAMES, PLACE_BUS_LABEL, PLACE_BUS_RIPPER,
		MANAGE_BUSES, DRAW_POLYGON, ENTER_DATUM, MOVE_EXACTLY, PLACE_HOLE,
		PLACE_PAD, PASTE, ASSIGN_PART, MAP_PACKAGE, DRAW_TRACK, PLACE_VIA,
		ROUTE_TRACK, DRAG_KEEP_SLOPE, ADD_PART, ANNOTATE, SMASH, UNSMASH
	};

	/**
	 * This is what a Tool receives when the user did something.
	 * i.e. moved the cursor or pressed key
	 */
	class ToolArgs {
		public :
			ToolEventType type;
			Coordi coords;
			std::set<SelectableRef> selection;
			bool keep_selection = false;
			unsigned int button;
			unsigned int key;
			Target target;
			int work_layer;
			ToolArgs() {}
	};

	/**
	 * To signal back to the core what the Tool did, a Tool returns a ToolResponse.
	 */
	class ToolResponse {
		public :
			ToolID next_tool = ToolID::NONE;
			bool end_tool = false;
			int layer = 10000;
			ToolResponse() {}
			/**
			 * Use this if you're done. The Core will then delete the active tool and initiate a rebuild.
			 */
			static ToolResponse end() {ToolResponse r; r.end_tool=true; return r;}

			/**
			 * Use this for changing the work layer from a Tool.
			 */
			static ToolResponse change_layer(int l) {ToolResponse r; r.layer=l; return r;}

			/**
			 * If you want another Tool to be launched you've finished, use this one.
			 */
			static ToolResponse next(ToolID t) {ToolResponse r; r.end_tool=true; r.next_tool = t; return r;};
	};

	/**
	 * Common interface for all Tools
	 */
	class ToolBase {
		public :
			ToolBase(class Core *c, ToolID tid);

			/**
			 * Gets called right after the constructor has finished.
			 * Used to get the initial placement right and set things up.
			 * For non-interactive Tools (e.g. DELETE), this one may return ToolResponse::end()
			 */
			virtual ToolResponse begin(const ToolArgs &args) = 0;

			/**
			 * Gets called whenever the user generated some sort of input.
			 */
			virtual ToolResponse update(const ToolArgs &args) = 0;

			/**
			 * @returns true if this Tool can begin in sensible way
			 */
			virtual bool can_begin() {return false;}
			virtual ~ToolBase() {}
			std::string name;

		protected :
			Cores core;
			ToolID tool_id = ToolID::NONE;
	};

	/**
	 * Where Tools and and documents meet.
	 * The Core provides a unified interface for Tools to access
	 * the objects common to all documents (whatever is being edited).
	 * It also provides the property interface for the property editor.
	 *
	 * A Core always stores to copies of the document, one of which is
	 * the working copy. Tools always operate on this one. Tools use Core::commit() to
	 * commit their changes by replacing the non-working document with the working document.
	 * Core::revert() does the opposite thing by replacing the working document with
	 * the non-working document, thereby discarding the changes made to the working copy.
	 * Usually, calling Core::commit() or Core::revert() is the last thing a Tool does before
	 * finishing.
	 *
	 * After a Tool has finished its work by returning ToolResponse::end(), the Core will
	 * initiate a rebuild. For CoreSchematic, a rebuild will update the Schematic according to its Block.
	 *
	 * The Core also handles undo/redo by storing a full copy for each step.
	 */
	class Core :public sigc::trackable {
		public :
			const std::string get_tool_name();
			virtual bool has_object_type(ObjectType ty) {return false;}

			virtual Junction *insert_junction(const UUID &uu, bool work = true);
			virtual Junction *get_junction(const UUID &uu, bool work=true);
			virtual void delete_junction(const UUID &uu, bool work = true);
			
			virtual Line *insert_line(const UUID &uu, bool work = true);
			virtual Line *get_line(const UUID &uu, bool work=true);
			virtual void delete_line(const UUID &uu, bool work = true);

			virtual Arc *insert_arc(const UUID &uu, bool work = true);
			virtual Arc *get_arc(const UUID &uu, bool work=true);
			virtual void delete_arc(const UUID &uu, bool work = true);
			
			virtual Text *insert_text(const UUID &uu, bool work = true);
			virtual Text *get_text(const UUID &uu, bool work=true);
			virtual void delete_text(const UUID &uu, bool work = true);

			virtual Polygon *insert_polygon(const UUID &uu, bool work = true);
			virtual Polygon *get_polygon(const UUID &uu, bool work=true);
			virtual void delete_polygon(const UUID &uu, bool work = true);
			virtual Hole *insert_hole(const UUID &uu, bool work = true);
			virtual Hole *get_hole(const UUID &uu, bool work=true);
			virtual void delete_hole(const UUID &uu, bool work = true);

			virtual std::vector<Line*> get_lines(bool work=true);
			virtual std::vector<Arc*> get_arcs(bool work=true);
			
			/**
			 * Expands the non-working document.
			 * And copies the non-working document to the working document.
			 */
			virtual void rebuild(bool from_undo = false);
			ToolResponse tool_begin(ToolID tool_id, const ToolArgs &args);
			ToolResponse tool_update(const ToolArgs &args);
			bool tool_can_begin(ToolID tool_id, const std::set<SelectableRef> &selection);
			virtual void commit() = 0;
			virtual void revert() = 0;
			virtual void save() = 0;
			void undo();
			void redo();
			
			
			inline bool tool_is_active() {return tool!=nullptr;}
			
			virtual bool property_is_settable(const UUID &uu, ObjectType type, ObjectProperty::ID property, bool *handled=nullptr) {if(handled){*handled=false;}return true;}

			virtual std::string get_property_string(const UUID &uu, ObjectType type, ObjectProperty::ID property, bool *handled=nullptr);
			virtual void set_property_string(const UUID &uu, ObjectType type, ObjectProperty::ID property, const std::string &value, bool *handled=nullptr);

			virtual bool get_property_bool(const UUID &uu, ObjectType type, ObjectProperty::ID property, bool *handled=nullptr);
			virtual void set_property_bool(const UUID &uu, ObjectType type, ObjectProperty::ID property, bool value, bool *handled=nullptr);

			virtual int64_t get_property_int(const UUID &uu, ObjectType type, ObjectProperty::ID property, bool *handled=nullptr);
			virtual void set_property_int(const UUID &uu, ObjectType type, ObjectProperty::ID property, int64_t value, bool *handled=nullptr);

			//std::string get_property_string(const UUID &uu, ObjectType type, ObjectProperty::ID property);
			//void set_property_string(const UUID &uu, ObjectType type, ObjectProperty::ID property, bool value);
			//virtual bool *get_set_property_bool(const UUID &uu, ObjectType type, ObjectProperty::ID property) {return nullptr;}

			virtual const std::map<int, Layer> &get_layers();
			const std::vector<int> &get_layers_sorted();

			/**
			 * @returns the current document's meta information.
			 * Meta information contains grid spacing and layer setup.
			 */
			virtual json get_meta();

			virtual Constraints *get_constraints() {return nullptr;}
			virtual std::pair<Coordi,Coordi> get_bbox() = 0;

			virtual ~Core() {}
			std::set<SelectableRef> selection;
			Pool *m_pool;

			/**
			 * These are used by Tools to query information from the user in a modal
			 * way.
			 */
			Dialogs dialogs;
			
			typedef sigc::signal<void, ToolID> type_signal_tool_changed;
			type_signal_tool_changed signal_tool_changed() {return s_signal_tool_changed;}
			typedef sigc::signal<void> type_signal_rebuilt;
			type_signal_rebuilt signal_rebuilt() {return s_signal_rebuilt;}
			/**
			 * Gets emitted right before saving. Gives the Imp an opportunity to write additional
			 * information to the document.
			 */
			type_signal_rebuilt signal_save() {return s_signal_save;}

			typedef sigc::signal<json> type_signal_request_save_meta;
			/**
			 * connect to this signal for providing meta information when the document is saved
			 */
			type_signal_request_save_meta signal_request_save_meta() {return s_signal_request_save_meta;}

		protected :
			virtual std::map<UUID, Junction> *get_junction_map(bool work=true) {return nullptr;}
			virtual std::map<UUID, Line> *get_line_map(bool work=true) {return nullptr;}
			virtual std::map<UUID, Arc> *get_arc_map(bool work=true) {return nullptr;}
			virtual std::map<UUID, Text> *get_text_map(bool work=true) {return nullptr;}
			virtual std::map<UUID, Polygon> *get_polygon_map(bool work=true) {return nullptr;}
			virtual std::map<UUID, Hole> *get_hole_map(bool work=true) {return nullptr;}


			bool reverted = false;
			std::unique_ptr<ToolBase> tool=nullptr;
			type_signal_tool_changed s_signal_tool_changed;
			type_signal_rebuilt s_signal_rebuilt;
			type_signal_rebuilt s_signal_save;
			type_signal_request_save_meta s_signal_request_save_meta;
			
			class HistoryItem {
				public:
				//Symbol sym;
				//HistoryItem(const Symbol &s): sym(s) {}
				std::string comment;
				virtual ~HistoryItem() {}
			};
			std::deque<std::unique_ptr<HistoryItem>> history;
			int history_current = -1;
			virtual void history_push() = 0;
			virtual void history_load(unsigned int i) = 0;

		private:
			std::unique_ptr<ToolBase> create_tool(ToolID tool_id);
	};
}