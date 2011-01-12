#include "stdafx.h"
#include "playlist_activator.h"

void playlist_activator::callback_run() {	
	playlist_history::activate_last();
}
