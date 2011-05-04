#pragma once

#include "stdafx.h"

class playlist_activator : public main_thread_callback {
public:
	virtual void callback_run();
};