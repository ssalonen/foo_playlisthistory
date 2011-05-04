#pragma once

#include "stdafx.h"


class playlist_history : public service_base {	
public:	
	// Updates from foobar
	virtual void on_playlist_activate(t_size p_old, t_size p_new) = 0;
	virtual void on_playlist_created(t_size p_index, const char *p_name, t_size p_name_len) = 0;
	virtual void on_playlists_reorder(const t_size *p_order, t_size p_count) = 0;
	virtual void on_playlists_removing(const bit_array &p_mask, t_size p_old_count, t_size p_new_count) = 0;
	virtual void on_playlists_removed(const bit_array &p_mask, t_size p_old_count, t_size p_new_count) = 0;	

	// Active playlists from history
	virtual void activate_previous() = 0;
	virtual void activate_next() = 0;
	virtual void activate_last() = 0;

	// Is activate_previous() possible?
	virtual bool is_previous_valid() = 0;

	// Is activate_next() possible?
	virtual bool is_next_valid() = 0;

	// Get playlist matching the current index. Return pfc::infinite_size on fail.
	virtual t_size get_playlist_matching_current_position() = 0;

	FB2K_MAKE_SERVICE_INTERFACE_ENTRYPOINT(playlist_history);
};

class playlist_history_impl : public playlist_history {	
public:	
	// Updates from foobar
	void on_playlist_activate(t_size p_old, t_size p_new);
	void on_playlist_created(t_size p_index, const char *p_name, t_size p_name_len);
	void on_playlists_reorder(const t_size *p_order, t_size p_count);
	void on_playlists_removing(const bit_array &p_mask, t_size p_old_count, t_size p_new_count);
	void on_playlists_removed(const bit_array &p_mask, t_size p_old_count, t_size p_new_count);	

	// Active playlists from history
	void activate_previous();
	void activate_next();
	void activate_last();

	// Is activate_previous() possible?
	bool is_previous_valid();

	// Is activate_next() possible?
	bool is_next_valid();

	// Get playlist matching the current index. Return pfc::infinite_size on fail.
	t_size get_playlist_matching_current_position();
protected:
	playlist_history_impl() : playlist_history(), history(), position_in_history(), 
		update_enabled(true), history_sync(), active_playlist_is_being_removed(false) {
	}
	

private:
	// Helper to block updates to history for a while
	class updates_disabled_scope {
	public:
		updates_disabled_scope(playlist_history_impl* history) : parent(history) {
			PFC_ASSERT(parent != NULL);
			if(parent) parent->update_enabled.set(false);
		}

		~updates_disabled_scope() {
			if(parent) parent->update_enabled.set(true);
		}
	private:
		playlist_history_impl* parent;
	};
	// Allow updates_disabled_scope to access private members
	friend class updates_disabled_scope;

	// Not synchronized!
	bool is_previous_valid(t_size & new_position_in_history);

	// Not synchronized!
	bool is_next_valid(t_size & new_position_in_history);	

	// Remove history entries which no longer reference a valid playlist (i.e. index = pfc::infinite_size)
	// Also updates the position_in_history
	// Not synchronized!
	// Returns true if something is actually done
	bool clean_history();

	void clean_history_until_no_changes();

	
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


	// History of recently activated playlists (their indices)
	pfc::list_t<position_tracker> history;
	// Determines the position in the history. pfc::infinite_size indicates "most recent" entry.
	position_tracker position_in_history;
	// Whether to process playlist activations. Can be set to false when moving in history.
	bool_mt update_enabled;

	// Mutex to control access to history
	critical_section history_sync;

	// Whether active playlist is being removed. Access should be synchronized with history_sync.
	bool active_playlist_is_being_removed;

	const static t_size MAX_HISTORY_SIZE = 50;
};

template<typename T>
class playlist_history_factory_t : public service_factory_single_t<T> {};