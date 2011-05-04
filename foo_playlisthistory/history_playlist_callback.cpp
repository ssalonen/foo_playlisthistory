#include "stdafx.h"
#include "history_playlist_callback.h"


unsigned history_playlist_callback::get_flags() {
	return flag_playlist_ops;
}

void history_playlist_callback::on_playlist_activate(t_size p_old, t_size p_new) {
	TRACK_CALL_TEXT_DEBUG("history_playlist_callback::on_playlist_activate");	
	static_api_ptr_t<playlist_history>()->on_playlist_activate(p_old, p_new);
}

void history_playlist_callback::on_playlist_created (t_size p_index, const char *p_name, t_size p_name_len) {
	TRACK_CALL_TEXT_DEBUG("history_playlist_callback::on_playlist_created");
	static_api_ptr_t<playlist_history>()->on_playlist_created(p_index, p_name, p_name_len);
}

void history_playlist_callback::on_playlists_reorder (const t_size *p_order, t_size p_count) {
	TRACK_CALL_TEXT_DEBUG("history_playlist_callback::on_playlists_reorder");
	static_api_ptr_t<playlist_history>()->on_playlists_reorder(p_order, p_count);
}

void history_playlist_callback::on_playlists_removing (const bit_array &p_mask, t_size p_old_count, t_size p_new_count) {
	TRACK_CALL_TEXT_DEBUG("history_playlist_callback::on_playlists_removing");
	static_api_ptr_t<playlist_history>()->on_playlists_removing(p_mask, p_old_count, p_new_count);
}

void history_playlist_callback::on_playlists_removed (const bit_array &p_mask, t_size p_old_count, t_size p_new_count) {
	TRACK_CALL_TEXT_DEBUG("history_playlist_callback::on_playlists_removed");
	static_api_ptr_t<playlist_history>()->on_playlists_removed(p_mask, p_old_count, p_new_count);
}

static service_factory_single_t< history_playlist_callback > history_playlist_callback_service;


