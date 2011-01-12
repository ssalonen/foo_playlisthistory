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

// Whether active playlist is being removed. Access should be synchronized with history_sync.
static bool active_playlist_is_being_removed = false;

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

class playlist_activator : public main_thread_callback {
public:
	// p_index: playlist to activate. Use infinite to denote latest in history.
	playlist_activator(t_size p_index = pfc::infinite_size);
	virtual void callback_run();

private:
	t_size playlist_index;
};

playlist_activator::playlist_activator(t_size p_index) : playlist_index(p_index) {
}

void playlist_activator::callback_run() {	
	t_size index = pfc::infinite_size;
	static_api_ptr_t<playlist_manager> playlist_api;

	if(playlist_index == pfc::infinite_size) {
		insync(history_sync);
		t_size history_count = history.get_count();
		if(history_count) {
			index = history[history_count-1].m_index;
		} else {
			// No history entries.
			DEBUG_PRINT << "Could not activate pfc::infinite_size playlist since there is no items in history";
		}
	} else {
		// Ensure that playlist really exists
		if(playlist_index >= playlist_api->get_playlist_count()) {
			DEBUG_PRINT << "Playlist (" << index << ") no longer exists. Cannot activate it.";
		} else {
			index = playlist_index;
		}
	}

	if(index != pfc::infinite_size) {
		block_update_enabled_scope block_updates;
		static_api_ptr_t<playlist_manager>()->set_active_playlist(index);
	}
}


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
	virtual void 	on_playlists_removing (const bit_array &p_mask, t_size p_old_count, t_size p_new_count);
	virtual void 	on_playlists_removed (const bit_array &p_mask, t_size p_old_count, t_size p_new_count);
	virtual void 	on_playlist_renamed (t_size p_index, const char *p_new_name, t_size p_new_name_len) {}
	virtual void 	on_default_format_changed () {}
	virtual void 	on_playback_order_changed (t_size p_new_index) {}
	virtual void 	on_playlist_locked (t_size p_playlist, bool p_locked) {}

private:
	void clean_history();
};

// Remove history entries which no longer reference a valid playlist (i.e. index = pfc::infinite_size)
// Also updates the position_in_history
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

	// Remove older history entries to keep history compact
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

void  history_playlist_callback::on_playlists_removing (const bit_array &p_mask, t_size p_old_count, t_size p_new_count) {
	TRACK_CALL_TEXT_DEBUG("history_playlist_callback::on_playlists_removing");
	insync(history_sync);
	// playlist_manager::get_active_playlist returns infinite at this point for reason if the 
	// current playlist is removed (-> unambiguity), therefore use playlist history to find out if
	// the currently active playlist is being removed
	t_size history_count = history.get_count();
	t_size active_playlist_index = history_count ? history[history_count-1].m_index : pfc::infinite_size;
	if(active_playlist_index != pfc::infinite_size) {
		active_playlist_is_being_removed = p_mask[active_playlist_index];
	}
	DEBUG_PRINT << "Removing active (" << active_playlist_index 
		<< ")playlist? " << active_playlist_is_being_removed;
}

void  history_playlist_callback::on_playlists_removed (const bit_array &p_mask, t_size p_old_count, t_size p_new_count) {
	TRACK_CALL_TEXT_DEBUG("history_playlist_callback::on_playlists_removed");
	insync(history_sync);
	for(t_size i = 0; i < history.get_count(); i++) {
		history[i].on_items_removed(p_mask, p_old_count, p_new_count);
	}
	// Remove obsolete playlists from history and 'invalidate' (->infinite) history
	// position counter, if necessary
	clean_history();

	if(active_playlist_is_being_removed && cfg_after_delete_go_to_last_active_playlist) {
		DEBUG_PRINT << "Active playlist is being removed, and 'after delete go to last active playlist' is active";				
		
		// Changing playlist here does not seem to work here. Let's use callback to activate last activated playlist (if any).
		static_api_ptr_t<main_thread_callback_manager>()->add_callback(new service_impl_t<playlist_activator>());		
	}
	print_history();
	active_playlist_is_being_removed = false;
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

private:
	enum {
		MENU_PREVIOUS_PLAYLIST = 0,
		MENU_NEXT_PLAYLIST = 1,
		MENU_AFTER_DELETE_GO_TO_LAST_ACTIVE = 2
	};
};


t_uint32 history_mainmenu_commands::get_command_count() {
	return 3;
}

GUID history_mainmenu_commands::get_command(t_uint32 p_index) {
	GUID guid;
	switch(p_index) {
	case MENU_PREVIOUS_PLAYLIST:
		guid = previous_playlist_guid;
		break;
	case MENU_NEXT_PLAYLIST:
		guid = next_playlist_guid;
		break;
	case MENU_AFTER_DELETE_GO_TO_LAST_ACTIVE:
		guid = after_delete_go_to_last_active_playlist_guid;
		break;
	default:
		guid = pfc::guid_null;
	}

	return guid;
}

void history_mainmenu_commands::get_name(t_uint32 p_index, pfc::string_base & p_out) {
	switch(p_index) {
	case MENU_PREVIOUS_PLAYLIST:
		p_out = "Previous playlist";
		break;
	case MENU_NEXT_PLAYLIST:
		p_out = "Next playlist";
		break;
	case MENU_AFTER_DELETE_GO_TO_LAST_ACTIVE:
		p_out = "After delete go to last active playlist";
		break;
	}
}

bool history_mainmenu_commands::get_description(t_uint32 p_index, pfc::string_base & p_out) {
	bool return_value = false;
	switch(p_index) {
	case MENU_PREVIOUS_PLAYLIST:
		p_out = "Activate previously activated playlist";
		return_value = true;
		break;
	case MENU_NEXT_PLAYLIST:
		p_out = "Activate next activated playlist";
		return_value = true;
		break;
	case MENU_AFTER_DELETE_GO_TO_LAST_ACTIVE:
		p_out = "If currently active playlist is deleted, go to last active playlist";
		return_value = true;
		break;
	default:
		return_value = false;
	}

	return return_value;
}

GUID history_mainmenu_commands::get_parent() {
	return playlisthistory_menu_guid;
}

void history_mainmenu_commands::execute(t_uint32 p_index, service_ptr_t<service_base> p_callback) {
	TRACK_CALL_TEXT_DEBUG("history_mainmenu_commands::execute");
	
	switch(p_index) {
	case MENU_PREVIOUS_PLAYLIST:
	case MENU_NEXT_PLAYLIST:
		{
			insync(history_sync);
			t_size newpos;
			if(history.get_count() && is_selection_enabled(p_index, newpos)) {
				block_update_enabled_scope lock;
				position_in_history.m_index = newpos;
				static_api_ptr_t<playlist_manager>()->set_active_playlist(history[position_in_history.m_index].m_index);
			}
			print_history();
		}
		break;
	case MENU_AFTER_DELETE_GO_TO_LAST_ACTIVE:
		cfg_after_delete_go_to_last_active_playlist = !cfg_after_delete_go_to_last_active_playlist;
		break;
	}
	
}	

// The standard version of this command does not support checked or disabled
// commands, so we use our own version.
bool history_mainmenu_commands::get_display(t_uint32 p_index, pfc::string_base & p_text, t_uint32 & p_flags) {
	TRACK_CALL_TEXT_DEBUG("history_mainmenu_commands::get_display");	
	DEBUG_PRINT << "p_index = " << p_index << ". Position in history: " << position_in_history.m_index;
	print_history();

	p_flags = 0;

	switch(p_index) {
	case MENU_PREVIOUS_PLAYLIST:
	case MENU_NEXT_PLAYLIST:
		{
			insync(history_sync);
			if(!is_selection_enabled(p_index)) {
				p_flags = flag_disabled;
			}
			print_history();
		}
		break;
	case MENU_AFTER_DELETE_GO_TO_LAST_ACTIVE: 
		if(cfg_after_delete_go_to_last_active_playlist) {
			p_flags = flag_checked;
		}
		break;
	}

	get_name(p_index, p_text);
	return true;
}

bool history_mainmenu_commands::is_selection_enabled(t_uint32 p_index) {
	t_size ignored;	
	return is_selection_enabled(p_index, ignored);
}

bool history_mainmenu_commands::is_selection_enabled(t_uint32 p_index, t_size & new_pos) {
	bool valid_command = false;
	PFC_ASSERT(p_index == MENU_PREVIOUS_PLAYLIST || p_index == MENU_NEXT_PLAYLIST);

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