// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#define COMPONENTNAME "Playlist History"
#define COMPONENTVERSION "0.1.1"
#define COMPONENTCONFIGVERSION 1

#include "targetver.h"

#include "../foobar2000/SDK/foobar2000.h"
#include "../foobar2000/helpers/helpers.h"

#include "guids.h"

#ifndef _DEBUG 

#include <ostream>
#include <cstdio>
#include <ios>

struct debug_nullstream : std::ostream {
	struct nullbuf: std::streambuf {
	int overflow(int c) { return traits_type::not_eof(c); }
	} m_sbuf;
	debug_nullstream() : std::ios(&m_sbuf), std::ostream(&m_sbuf) {}
};

#define DEBUG_PRINT debug_nullstream()
#endif

#ifdef _DEBUG
#define DEBUG_PRINT console::formatter()
#endif

#ifndef _DEBUG 
#define DEBUG_PRINT debug_nullstream()
#define TRACK_CALL_TEXT_DEBUG(text) TRACK_CALL_TEXT(text)
#endif

#ifdef _DEBUG
#define DEBUG_PRINT console::formatter()
#define TRACK_CALL_TEXT_DEBUG(text) DEBUG_PRINT << (text); TRACK_CALL_TEXT( (text) ); 
#endif