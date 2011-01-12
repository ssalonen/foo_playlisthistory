#include "stdafx.h"

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
};

unsigned history_playlist_callback::get_flags() {
	return flag_playlist_ops;
}

void history_playlist_callback::on_playlist_activate(t_size p_old, t_size p_new) {
	TRACK_CALL_TEXT_DEBUG("history_playlist_callback::on_playlist_activate");	
	playlist_history::on_playlist_activate(p_old, p_new);
}

void  history_playlist_callback::on_playlist_created (t_size p_index, const char *p_name, t_size p_name_len) {
	TRACK_CALL_TEXT_DEBUG("history_playlist_callback::on_playlist_created");
	playlist_history::on_playlist_created(p_index, p_name, p_name_len);
}

void  history_playlist_callback::on_playlists_reorder (const t_size *p_order, t_size p_count) {
	TRACK_CALL_TEXT_DEBUG("history_playlist_callback::on_playlists_reorder");
	playlist_history::on_playlists_reorder(p_order, p_count);
}

void  history_playlist_callback::on_playlists_removing (const bit_array &p_mask, t_size p_old_count, t_size p_new_count) {
	TRACK_CALL_TEXT_DEBUG("history_playlist_callback::on_playlists_removing");
	playlist_history::on_playlists_removing(p_mask, p_old_count, p_new_count);
}

void  history_playlist_callback::on_playlists_removed (const bit_array &p_mask, t_size p_old_count, t_size p_new_count) {
	TRACK_CALL_TEXT_DEBUG("history_playlist_callback::on_playlists_removed");
	playlist_history::on_playlists_removed(p_mask, p_old_count, p_new_count);
}

static service_factory_single_t< history_playlist_callback > history_playlist_callback_service;


