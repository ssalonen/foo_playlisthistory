#include "stdafx.h"
#include "playlist_history.h"


void playlist_history_impl::on_playlist_activate(t_size p_old, t_size p_new) {
	TRACK_CALL_TEXT_DEBUG("playlist_history_impl::on_playlist_activate");	
	if(!update_enabled.get()) return;
	insync(history_sync);	

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

	clean_history_until_no_changes(); // Just in case
	print_history();
}

void playlist_history_impl::on_playlist_created(t_size p_index, const char *p_name, t_size p_name_len) {
	TRACK_CALL_TEXT_DEBUG("playlist_history_impl::on_playlist_created");
	// Accept creations all the time // if(!update_enabled.get()) return;
	insync(history_sync);	
	for(t_size i = 0; i < history.get_count(); i++) {
		history[i].on_item_created(p_index, p_name, p_name_len);
	}
	clean_history_until_no_changes();
	print_history();
}

void playlist_history_impl::on_playlists_reorder(const t_size *p_order, t_size p_count) {
	TRACK_CALL_TEXT_DEBUG("playlist_history_impl::on_playlists_reorder");
	// Accept reorders all the time // if(!update_enabled.get()) return;
	insync(history_sync);	
	for(t_size i = 0; i < history.get_count(); i++) {
		history[i].on_items_reorder(p_order, p_count);
	}

	print_history();
}

void playlist_history_impl::on_playlists_removing(const bit_array &p_mask, t_size p_old_count, t_size p_new_count) {
	TRACK_CALL_TEXT_DEBUG("playlist_history_impl::on_playlists_removing");
	// Accept removals all the time // if(!update_enabled.get()) return;
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

void playlist_history_impl::on_playlists_removed(const bit_array &p_mask, t_size p_old_count, t_size p_new_count) {
	TRACK_CALL_TEXT_DEBUG("playlist_history_impl::on_playlists_removed");
	if(!update_enabled.get()) return;
	insync(history_sync);	
	for(t_size i = 0; i < history.get_count(); i++) {
		history[i].on_items_removed(p_mask, p_old_count, p_new_count);
	}
	// Remove obsolete playlists from history and 'invalidate' (->infinite) history
	// position counter, if necessary
	clean_history_until_no_changes();

	if(active_playlist_is_being_removed && cfg_after_delete_go_to_last_active_playlist) {
		DEBUG_PRINT << "Active playlist is being removed, and 'after delete go to last active playlist' is active";				
		
		// Changing playlist here does not seem to work here. Let's use callback to activate last activated playlist (if any).
		static_api_ptr_t<main_thread_callback_manager>()->add_callback(new service_impl_t<playlist_activator>());		
	}
	print_history();
	active_playlist_is_being_removed = false;
}

void playlist_history_impl::activate_previous() {
	TRACK_CALL_TEXT_DEBUG("playlist_history_impl::activate_previous()");
	t_size index;
	if(is_previous_valid(index)) {
		updates_disabled_scope lock(this);
		insync(history_sync);
		position_in_history.m_index = index;
		static_api_ptr_t<playlist_manager>()->set_active_playlist(history[index].m_index);
	}
}

void playlist_history_impl::activate_next() {
	TRACK_CALL_TEXT_DEBUG("playlist_history_impl::activate_next()");
	t_size index;
	if(is_next_valid(index)) {
		updates_disabled_scope lock(this);
		insync(history_sync);
		position_in_history.m_index = index;
		static_api_ptr_t<playlist_manager>()->set_active_playlist(history[index].m_index);
	}
}

void playlist_history_impl::activate_last() {
	TRACK_CALL_TEXT_DEBUG("playlist_history_impl::activate_last()");
	insync(history_sync);
	position_tracker original_position = position_in_history;

	// Try to find out playlist matching last history entry.
	position_in_history.m_index = pfc::infinite_size;
	t_size playlist_index = get_playlist_matching_current_position();

	// Fail (something is wrong if this happens).
	if(playlist_index == pfc::infinite_size) {
		DEBUG_PRINT << "Failed to activate last playlist";
		// Revert position
		position_in_history = original_position;
		return;
	}

	updates_disabled_scope lock(this);
	static_api_ptr_t<playlist_manager>()->set_active_playlist(playlist_index);
}

bool playlist_history_impl::is_previous_valid() {
	TRACK_CALL_TEXT_DEBUG("playlist_history_impl::is_previous_valid()");
	insync(history_sync);
	t_size ignored;
	return is_previous_valid(ignored);
}

bool playlist_history_impl::is_next_valid() {
	TRACK_CALL_TEXT_DEBUG("playlist_history_impl::is_next_valid()");
	insync(history_sync);
	t_size ignored;
	return is_next_valid(ignored);
}

bool playlist_history_impl::is_previous_valid(t_size & new_position_in_history) {
	TRACK_CALL_TEXT_DEBUG("playlist_history_impl::is_previous_valid(t_size)");
	bool valid_command = false;
	new_position_in_history = pfc::infinite_size;

	if(!history.get_count()) return false;

	// Activate previously activated playlist				
	if(position_in_history.m_index == pfc::infinite_size 
		&& history.get_count() >= 2) {									
		valid_command = true;
		// Last entry should be currently active playlist, thus -2
		new_position_in_history = history.get_count() - 2;	
	} else if(position_in_history.m_index != pfc::infinite_size 
		&& position_in_history.m_index != 0) {
			new_position_in_history = position_in_history.m_index - 1;
			valid_command = true;
	}

	PFC_ASSERT(new_position_in_history == pfc::infinite_size 
		|| new_position_in_history < history.get_count());

	if(valid_command) PFC_ASSERT(new_position_in_history != pfc::infinite_size);
	
	return valid_command;
}

bool playlist_history_impl::is_next_valid(t_size & new_position_in_history) {
	TRACK_CALL_TEXT_DEBUG("playlist_history_impl::is_next_valid(t_size)");
	bool valid_command = false;
	new_position_in_history = pfc::infinite_size;

	if(!history.get_count()) return false;

	new_position_in_history = pfc::infinite_size;
	if(position_in_history.m_index != pfc::infinite_size 
		&& position_in_history.m_index != history.get_count()-1) {
			new_position_in_history = position_in_history.m_index + 1;
			valid_command = true;					
	}

	PFC_ASSERT(new_position_in_history == pfc::infinite_size 
		|| new_position_in_history < history.get_count());

	if(valid_command) PFC_ASSERT(new_position_in_history != pfc::infinite_size);

	return valid_command;
}

t_size playlist_history_impl::get_playlist_matching_current_position() {
	TRACK_CALL_TEXT_DEBUG("playlist_history_impl::get_playlist_matching_current_position()");
	insync(history_sync);

	t_size playlist_index = pfc::infinite_size;
	static_api_ptr_t<playlist_manager> playlist_api;	

	if(!history.get_count()) return pfc::infinite_size;

	PFC_ASSERT(pfc::infinite_size == position_in_history.m_index 
		|| position_in_history.m_index < history.get_count());

	if(position_in_history.m_index == pfc::infinite_size) {		
		// Get last entry
		playlist_index = history[history.get_count()-1].m_index;		
	} else {
		t_size index = history[position_in_history.m_index].m_index;
		
		// Ensure that playlist really exists
		if(index >= playlist_api->get_playlist_count()) {
			DEBUG_PRINT << "Playlist (" << index << ") no longer exists!";
		} else {
			playlist_index = index;
		}
	}

	return playlist_index;
}


bool playlist_history_impl::clean_history() {
	TRACK_CALL_TEXT_DEBUG("history_playlist_callback::clean_history");
	t_size history_size = history.get_count();
	t_size new_history_size = history_size;
	bit_array_bittable helper(history_size);
	for(t_size i = 0; i < history.get_count(); i++) {
		if(history[i].m_index == pfc::infinite_size) {
			DEBUG_PRINT << "Removing (inf) " << i;
			if(!helper.get(i)) new_history_size--;
			helper.set(i, true);
		} else if(i > 0 && history[i-1].m_index == history[i].m_index) {
			DEBUG_PRINT << "Removing (same) " << i;
			// Two same playlists in the history (in the row)
			// -> remove the latter
			if(!helper.get(i)) new_history_size--;
			helper.set(i, true);
		}
	}
	history.remove_mask(helper);
	position_in_history.on_items_removed(helper, history_size, 
		new_history_size);

	// Reset invalid history position...
	PFC_ASSERT(position_in_history.m_index == pfc::infinite_size 
		|| position_in_history.m_index < history.get_count());
	if(position_in_history.m_index != pfc::infinite_size 
		&& position_in_history.m_index >= history.get_count()) {
		position_in_history.m_index = pfc::infinite_size;
	}
	print_history();

	return new_history_size != history_size;
}

void playlist_history_impl::clean_history_until_no_changes() {
	TRACK_CALL_TEXT_DEBUG("history_playlist_callback::clean_history_until_no_changes");
	while(clean_history()) {}
}

// {511FD2B4-CA27-4F4F-94E0-F519CBD412B2}
const GUID playlist_history::class_guid = { 0x511fd2b4, 0xca27, 0x4f4f, { 0x94, 0xe0, 0xf5, 0x19, 0xcb, 0xd4, 0x12, 0xb2 } }; 

// Register as entrypoint service
static playlist_history_factory_t<playlist_history_impl> playlist_history_registered;