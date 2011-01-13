#include "stdafx.h"
#include "playlist_activator.h"

void playlist_activator::callback_run() {
	TRACK_CALL_TEXT_DEBUG("playlist_activator::callback_run");	
	static_api_ptr_t<playlist_history>()->activate_last();
}
