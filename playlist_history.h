#pragma once

#include "stdafx.h"


class playlist_history {
public:
	// Updates from foobar
	static void on_playlist_activate (t_size p_old, t_size p_new);
	static void on_playlist_created (t_size p_index, const char *p_name, t_size p_name_len);
	static void on_playlists_reorder (const t_size *p_order, t_size p_count);
	static void on_playlists_removing (const bit_array &p_mask, t_size p_old_count, t_size p_new_count);
	static void on_playlists_removed (const bit_array &p_mask, t_size p_old_count, t_size p_new_count);	

	// Active playlists from history
	static void activate_previous();
	static void activate_next();
	static void activate_last();

	// Is activate_previous() possible?
	static bool is_previous_valid();

	// Is activate_next() possible?
	static bool is_next_valid();

	// Get playlist matching the current index. Return pfc::infinite_size on fail.
	static t_size get_playlist_matching_current_position();
private:
	// Helper to block updates to history for a while
	class updates_disabled_scope {
	public:
		updates_disabled_scope() {
			playlist_history::update_enabled(false);
		}

		~updates_disabled_scope() {
			playlist_history::update_enabled(true);
		}
	};

	// Not synchronized!
	static bool is_previous_valid(t_size & new_position_in_history);

	// Not synchronized!
	static bool is_next_valid(t_size & new_position_in_history);	

	// Remove history entries which no longer reference a valid playlist (i.e. index = pfc::infinite_size)
	// Also updates the position_in_history
	// Not synchronized!
	static void clean_history();

	
	// For debugging
#ifndef _DEBUG
	static void inline print_history() {}
#else
	static void inline print_history() {
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


	// History of recently activated playlists (their indices)
	static pfc::list_t<position_tracker> history;
	// Determines the position in the history. pfc::infinite_size indicates "most recent" entry.
	static position_tracker position_in_history;
	// Whether to process playlist activations. Can be set to false when moving in history.
	static bool_mt update_enabled;

	// Mutex to control access to history
	static critical_section history_sync;

	// Whether active playlist is being removed. Access should be synchronized with history_sync.
	static bool active_playlist_is_being_removed;

	const static t_size MAX_HISTORY_SIZE = 50;
};
