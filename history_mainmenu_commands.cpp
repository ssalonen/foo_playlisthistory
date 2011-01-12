#include "stdafx.h"
#include "history_mainmenu_commands.h"

t_uint32 history_mainmenu_commands::get_command_count() {
	return 4;
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
		playlist_history::activate_previous();
		break;
	case MENU_NEXT_PLAYLIST:
		playlist_history::activate_next();
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

	p_flags = 0;

	switch(p_index) {
	case MENU_PREVIOUS_PLAYLIST:
		if(playlist_history::is_previous_valid()) {
			p_flags = flag_disabled;
		}
		break;
	case MENU_NEXT_PLAYLIST:		
		if(playlist_history::is_next_valid()) {
			p_flags = flag_disabled;
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

bool history_mainmenu_commands::is_command_dynamic(t_uint32 index) {
	return index == MENU_COMMAND_SEPARATOR;
}

mainmenu_node::ptr history_mainmenu_commands::dynamic_instantiate(t_uint32 index) {
	TRACK_CALL_TEXT_DEBUG("history_mainmenu_commands::dynamic_instantiate");
	PFC_ASSERT(index == MENU_COMMAND_SEPARATOR);
	if(index == MENU_COMMAND_SEPARATOR) {
		DEBUG_PRINT << "Constructing separator";
		return new service_impl_t<mainmenu_node_separator>();
	} else {
		return NULL;
	}
}

bool history_mainmenu_commands::dynamic_execute(t_uint32 index, const GUID & subID, service_ptr_t<service_base> callback) {
	TRACK_CALL_TEXT_DEBUG("history_mainmenu_commands::dynamic_execute");
	PFC_ASSERT(index == MENU_COMMAND_SEPARATOR);
	return true;
}




static mainmenu_commands_factory_t< history_mainmenu_commands > history_menu;

static mainmenu_group_popup_factory mainmenu_group(playlisthistory_menu_guid, 
	mainmenu_groups::view, mainmenu_commands::sort_priority_dontcare, 
	"Recently activated playlists");