#pragma once

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
