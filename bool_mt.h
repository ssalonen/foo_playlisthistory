#pragma once

#include "stdafx.h"

// Thread safe single boolean value
class bool_mt {
public:
	bool_mt(bool value = false) : m_val(value), m_sync() {

	}

	void operator() (bool value){
		insync(m_sync);
		m_val = value;
	}

	bool operator() (){
		insync(m_sync);
		return m_val;
	}
private:
	bool m_val;
	critical_section m_sync;
};