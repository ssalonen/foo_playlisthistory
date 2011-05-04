#pragma once

#include "stdafx.h"

// Thread safe single boolean value
class bool_mt : public syncd_storage_flagged<bool>{
public:
	bool_mt(bool value = false) : syncd_storage_flagged(value) {

	}
};