#pragma once

#include "stdafx.h"

// Related to playlist activation
class history_mainmenu_commands : public mainmenu_commands_v2 {
public:
	virtual t_uint32 get_command_count();
	virtual GUID get_command(t_uint32 p_index);
	virtual void get_name(t_uint32 p_index, pfc::string_base & p_out);
	virtual bool get_description(t_uint32 p_index, pfc::string_base & p_out);
	virtual GUID get_parent();
	virtual void execute(t_uint32 p_index, service_ptr_t<service_base> p_callback);

	virtual bool is_command_dynamic(t_uint32 index);
	virtual mainmenu_node::ptr dynamic_instantiate(t_uint32 index);
	virtual bool dynamic_execute(t_uint32 index, const GUID & subID, service_ptr_t<service_base> callback);

	// overridden
	virtual bool get_display(t_uint32 p_index, pfc::string_base & p_text, t_uint32 & p_flags);

private:

	enum {
		MENU_PREVIOUS_PLAYLIST = 0,
		MENU_NEXT_PLAYLIST = 1,
		MENU_COMMAND_SEPARATOR = 2,
		MENU_AFTER_DELETE_GO_TO_LAST_ACTIVE = 3
	};
};

// 'Restore last deleted playlist'
class history_restore_mainmenu_commands : public mainmenu_commands {
public:
	virtual t_uint32 get_command_count();
	virtual GUID get_command(t_uint32 p_index);
	virtual void get_name(t_uint32 p_index, pfc::string_base & p_out);
	virtual bool get_description(t_uint32 p_index, pfc::string_base & p_out);
	virtual GUID get_parent();
	virtual void execute(t_uint32 p_index, service_ptr_t<service_base> p_callback);

	// overridden
	virtual bool get_display(t_uint32 p_index, pfc::string_base & p_text, t_uint32 & p_flags);

private:

	enum {
		MENU_RESTORE_PLAYLIST = 0
	};
};