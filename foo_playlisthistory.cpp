#include "stdafx.h"

// Copied from playlist_position_reference_tracker (SDK), kept only the playlist functions (no API registration)
// See sdk-license.txt for the license of playlist_position_reference_tracker (which also apply to the derived position_tracker)
class position_tracker  {
public:
	position_tracker() : m_index(pfc::infinite_size) {

	}

	position_tracker & operator= (const position_tracker & other) {
		m_index = other.m_index;
		return *this;
	}		

	void on_item_created(t_size p_index,const char * p_name,t_size p_name_len) {
		if (m_index != pfc_infinite && p_index <= m_index) m_index++;
	}
	void on_items_reorder(const t_size * p_order,t_size p_count) {
		if (m_index < p_count) m_index = order_helper::g_find_reverse(p_order,m_index);
		else m_index = pfc_infinite;
	}
	void on_items_removed(const bit_array & p_mask,t_size p_old_count,t_size p_new_count) {
		if (m_index < p_old_count) {
			const t_size playlist_before = m_index;
			for(t_size walk = p_mask.find_first(true,0,p_old_count); walk < p_old_count; walk = p_mask.find_next(true,walk,p_old_count)) {
				if (walk < playlist_before) {
					m_index--;
				} else if (walk == playlist_before) {
					m_index = pfc_infinite;
					break;
				} else {
					break;
				}
			}
		} else {
			m_index = pfc_infinite;
		}
	}

	t_size m_index;
};

// Thread safe single boolean value
class bool_mt {
public:
	bool_mt(bool value = false) : m_val(value), m_sync() {

	}

	void operator() (bool value){
		insync(m_sync);
		m_val = value;
	}

	bool operator() (){
		insync(m_sync);
		return m_val;
	}
private:
	bool m_val;
	critical_section m_sync;
};


// History of recently activated playlists (their indices)
static pfc::list_t<position_tracker> history;
// Determines the position in the history. pfc::infinite_size indicates "most recent" entry.
static position_tracker position_in_history;

// Whether to process playlist activations. Can be set to false when moving in history.
static bool_mt update_enabled = true;

// Mutex to control access to history
static critical_section history_sync;

const static t_size MAX_HISTORY_SIZE = 50;

// Helper to block updates for a while
class block_update_enabled_scope {
public:
	block_update_enabled_scope() {
		::update_enabled(false);
	}

	~block_update_enabled_scope() {
		::update_enabled(true);
	}
};

// For debugging
#ifndef _DEBUG
void inline print_history() {}
#else
void inline print_history() {
	console::formatter() << "---";
	console::formatter() << "Playlist history (from oldest to newest):";
	for(t_size i = 0; i < history.get_count(); i++) {
		console::formatter() << ((position_in_history.m_index == i || 
			(position_in_history.m_index == pfc::infinite_size && i == history.get_count()-1)) ?
			"->" : "  ") << history[i].m_index;
	}
	console::formatter() << "---";
}
#endif


class history_playlist_callback : public playlist_callback_static {
public:
	// playlist_callback_static
	virtual unsigned get_flags();

	// playlist_callback
	virtual void 	on_items_added (t_size p_playlist, t_size p_start, const pfc::list_base_const_t< metadb_handle_ptr > &p_data, const bit_array &p_selection) {}
	virtual void 	on_items_reordered (t_size p_playlist, const t_size *p_order, t_size p_count) {}
	virtual void 	on_items_removing (t_size p_playlist, const bit_array &p_mask, t_size p_old_count, t_size p_new_count) {}
	virtual void 	on_items_removed (t_size p_playlist, const bit_array &p_mask, t_size p_old_count, t_size p_new_count) {}
	virtual void 	on_items_selection_change (t_size p_playlist, const bit_array &p_affected, const bit_array &p_state) {}
	virtual void 	on_item_focus_change (t_size p_playlist, t_size p_from, t_size p_to) {}
	virtual void 	on_items_modified (t_size p_playlist, const bit_array &p_mask) {}
	virtual void 	on_items_modified_fromplayback (t_size p_playlist, const bit_array &p_mask, play_control::t_display_level p_level) {}
	virtual void 	on_items_replaced (t_size p_playlist, const bit_array &p_mask, const pfc::list_base_const_t< t_on_items_replaced_entry > &p_data) {}
	virtual void 	on_item_ensure_visible (t_size p_playlist, t_size p_idx) {}

	virtual void 	on_playlist_activate (t_size p_old, t_size p_new);
	virtual void 	on_playlist_created (t_size p_index, const char *p_name, t_size p_name_len);
	virtual void 	on_playlists_reorder (const t_size *p_order, t_size p_count);
	virtual void 	on_playlists_removing (const bit_array &p_mask, t_size p_old_count, t_size p_new_count){}
	virtual void 	on_playlists_removed (const bit_array &p_mask, t_size p_old_count, t_size p_new_count);
	virtual void 	on_playlist_renamed (t_size p_index, const char *p_new_name, t_size p_new_name_len) {}
	virtual void 	on_default_format_changed () {}
	virtual void 	on_playback_order_changed (t_size p_new_index) {}
	virtual void 	on_playlist_locked (t_size p_playlist, bool p_locked) {}

private:
	void clean_history();
};

// Remove history entries which no longer reference a valid playlist (i.e. index = pfc::infinite_size)
void history_playlist_callback::clean_history() {
	TRACK_CALL_TEXT_DEBUG("history_playlist_callback::clean_history");
	t_size history_size = history.get_count();
	t_size new_history_size = history_size;
	bit_array_bittable helper(history_size);
	for(t_size i = 0; i < history.get_count(); i++) {
		if(history[i].m_index == pfc::infinite_size) {
			if(!helper.get(i)) new_history_size--;
			helper.set(i, true);
		}
	}
	history.remove_mask(helper);
	position_in_history.on_items_removed(helper, history_size, 
		new_history_size);
}

unsigned history_playlist_callback::get_flags() {
	return flag_playlist_ops;
}

void history_playlist_callback::on_playlist_activate(t_size p_old, t_size p_new) {
	TRACK_CALL_TEXT_DEBUG("history_playlist_callback::on_playlist_activate");	
	insync(history_sync);
	if(!update_enabled()) return;

	// Remove history entries after the current position
	if(position_in_history.m_index != pfc::infinite_size) {
		t_size remove_pos = position_in_history.m_index + 1;
		t_size remove_count = history.get_count()-remove_pos;
		history.remove_from_idx(remove_pos, remove_count);
		// Set history position marker to the end
		position_in_history.m_index = pfc::infinite_size;
	}

	// The following additions does not affect position_in_history
	if(!history.get_count()) {
		t_size new_index = history.add_item(position_tracker());
		history[new_index].m_index = p_old;
	}
	t_size new_index = history.add_item(position_tracker()); 
	history[new_index].m_index = p_new;


	while(history.get_count() > MAX_HISTORY_SIZE) {
		history.remove_by_idx(0);
		position_in_history.on_items_removed(bit_array_range(0, 1), 
			history.get_count(), history.get_count()-1);
	}

	clean_history(); // Just in case
	print_history();
}

void  history_playlist_callback::on_playlist_created (t_size p_index, const char *p_name, t_size p_name_len) {
	TRACK_CALL_TEXT_DEBUG("history_playlist_callback::on_playlist_created");
	insync(history_sync);
	for(t_size i = 0; i < history.get_count(); i++) {
		history[i].on_item_created(p_index, p_name, p_name_len);
	}
	clean_history();
	print_history();
}

void  history_playlist_callback::on_playlists_reorder (const t_size *p_order, t_size p_count) {
	TRACK_CALL_TEXT_DEBUG("history_playlist_callback::on_playlists_reorder");
	insync(history_sync);
	for(t_size i = 0; i < history.get_count(); i++) {
		history[i].on_items_reorder(p_order, p_count);
	}

	print_history();
}
void  history_playlist_callback::on_playlists_removed (const bit_array &p_mask, t_size p_old_count, t_size p_new_count) {
	TRACK_CALL_TEXT_DEBUG("history_playlist_callback::on_playlists_removed");
	insync(history_sync);
	for(t_size i = 0; i < history.get_count(); i++) {
		history[i].on_items_removed(p_mask, p_old_count, p_new_count);
	}
	clean_history();
	print_history();
}

static service_factory_single_t< history_playlist_callback > history_playlist_callback_service;



class history_mainmenu_commands : public mainmenu_commands {
public:
	virtual t_uint32 get_command_count();
	virtual GUID get_command(t_uint32 p_index);
	virtual void get_name(t_uint32 p_index, pfc::string_base & p_out);
	virtual bool get_description(t_uint32 p_index, pfc::string_base & p_out);
	virtual GUID get_parent();
	virtual void execute(t_uint32 p_index, service_ptr_t<service_base> p_callback);

	// overridden
	virtual bool get_display(t_uint32 p_index, pfc::string_base & p_text, t_uint32 & p_flags);

	bool is_selection_enabled(t_uint32 p_index);
	bool is_selection_enabled(t_uint32 p_index, t_size & new_pos);	
};


t_uint32 history_mainmenu_commands::get_command_count() {
	return 2;
}

GUID history_mainmenu_commands::get_command(t_uint32 p_index) {
	switch(p_index) {
	case 0:
		return previous_playlist_guid;
		break;
	case 1:
		return next_playlist_guid;
		break;
	default:
		return pfc::guid_null;
	}
}

void history_mainmenu_commands::get_name(t_uint32 p_index, pfc::string_base & p_out) {
	switch(p_index) {
	case 0:
		p_out = "Previous playlist";
		break;
	case 1:
		p_out = "Next playlist";
		break;
	}
}

bool history_mainmenu_commands::get_description(t_uint32 p_index, pfc::string_base & p_out) {
	switch(p_index) {
	case 0:
		p_out = "Activate previously activated playlist";
		return true;
		break;
	case 1:
		p_out = "Activate next activated playlist";
		return true;
		break;
	default:
		return false;
	}
}

GUID history_mainmenu_commands::get_parent() {
	return playlisthistory_menu_guid;
}

void history_mainmenu_commands::execute(t_uint32 p_index, service_ptr_t<service_base> p_callback) {
	TRACK_CALL_TEXT_DEBUG("history_mainmenu_commands::execute");
	insync(history_sync);
	if(!history.get_count()) return;

	t_size newpos;
	if(is_selection_enabled(p_index, newpos)) {
		block_update_enabled_scope lock;
		position_in_history.m_index = newpos;
		static_api_ptr_t<playlist_manager>()->set_active_playlist(history[position_in_history.m_index].m_index);
	}
	print_history();
}	

// The standard version of this command does not support checked or disabled
// commands, so we use our own version.
bool history_mainmenu_commands::get_display(t_uint32 p_index, pfc::string_base & p_text, t_uint32 & p_flags) {
	TRACK_CALL_TEXT_DEBUG("history_mainmenu_commands::get_display");
	insync(history_sync);
	DEBUG_PRINT << "p_index = " << p_index << ". Position in history: " << position_in_history.m_index;
	print_history();

	p_flags = 0;

	if(!is_selection_enabled(p_index))
		p_flags = flag_disabled;			

	get_name(p_index, p_text);
	return true;
}

bool history_mainmenu_commands::is_selection_enabled(t_uint32 p_index) {
	t_size ignored;
	return is_selection_enabled(p_index, ignored);
}

bool history_mainmenu_commands::is_selection_enabled(t_uint32 p_index, t_size & new_pos) {
	bool valid_command = false;

	switch(p_index) {
	case 0:
		// Activate previously activated playlist				
		if(position_in_history.m_index == pfc::infinite_size 
			&& history.get_count() >= 2) {									
			valid_command = true;
			// Last entry should be currently active playlist, thus -2
			new_pos = history.get_count() - 2;	
		} else if(position_in_history.m_index != pfc::infinite_size 
			&& position_in_history.m_index != 0) {
				new_pos = position_in_history.m_index - 1;
				valid_command = true;
		}
		break;
	case 1:
		// Activate next activated playlist
		if(position_in_history.m_index != pfc::infinite_size 
			&& position_in_history.m_index != history.get_count()-1) {
				new_pos = position_in_history.m_index + 1;
				valid_command = true;					
		}
		break;
	}
	return valid_command;
}

static mainmenu_commands_factory_t< history_mainmenu_commands > history_menu;

static mainmenu_group_popup_factory mainmenu_group(playlisthistory_menu_guid, 
	mainmenu_groups::view, mainmenu_commands::sort_priority_dontcare, 
	"Recently activated playlists");