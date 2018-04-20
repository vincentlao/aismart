/* $Id: settings.hpp 54604 2012-07-07 00:49:45Z loonycyborg $ */
/*
   Copyright (C) 2007 - 2012 by Mark de Wever <koraq@xs4all.nl>
   Part of the Battle for Wesnoth Project http://www.wesnoth.org/

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/

/**
 * @file
 * This file contains the settings handling of the widget library.
 */

#ifndef GUI_WIDGETS_SETTING_HPP_INCLUDED
#define GUI_WIDGETS_SETTING_HPP_INCLUDED

#include "gui/auxiliary/widget_definition/window.hpp"

#include <boost/function.hpp>
#include <boost/foreach.hpp>

#include <string>
#include <vector>

namespace gui2 {


struct tgui_definition;

/**
 * Registers a window.
 *
 * This function registers the available windows defined in WML. All windows
 * need to register themselves before @ref gui2::init) is called.
 *
 * @warning This function runs before @ref main() so needs to be careful
 * regarding the static initialization problem.
 *
 * @note Double registering a window can't hurt, but no way to probe for it,
 * this can be added if needed. The same for an unregister function.
 *
 * @param id                      The id of the window to register.
 */
void register_window(const std::string& app, const std::string& id);

/**
 * Special helper class to get the list of registered windows.
 *
 * This is used in the unit tests, but these implementation details shouldn't
 * be used in the normal code.
 */
class tunit_test_access_only
{
	friend std::vector<std::string>& unit_test_registered_window_list();

	/** Returns a copy of the list of registered windows. */
	static std::vector<std::string> get_registered_window_list();
};

/**
 * Registers a widgets.
 *
 * This function registers the available widgets defined in WML. All widgets
 * need to register themselves before @ref gui2::init) is called.
 *
 * @warning This function runs before @ref main() so needs to be careful
 * regarding the static initialization problem.
 *
 * @param id                      The id of the widget to register.
 * @param functor                 The function to load the definitions.
 */
void register_widget(const std::string& id
		, boost::function<void(
			  tgui_definition& gui_definition
			, const std::string& definition_type
			, const config& cfg
			, const char *key)> functor);

/**
 * Loads the definitions of a widget.
 *
 * @param gui_definition          The gui definition the widget definition
 *                                belongs to.
 * @param definition_type         The type of the widget whose definitions are
 *                                to be loaded.
 * @param definitions             The definitions serialized from a config
 *                                object.
 */
void load_widget_definitions(
	  tgui_definition& gui_definition
	, const std::string& definition_type
	, const std::vector<tcontrol_definition_ptr>& definitions);

/**
 * Loads the definitions of a widget.
 *
 * This function is templated and kept small so only loads the definitions from
 * the config and then lets the real job be done by the @ref
 * load_widget_definitions above.
 *
 * @param gui_definition          The gui definition the widget definition
 *                                belongs to.
 * @param definition_type         The type of the widget whose definitions are
 *                                to be loaded.
 * @param config                  The config to serialiaze the definitions
 *                                from.
 * @param key                     Optional id of the definition to load.
 */
template<class T>
void load_widget_definitions(
	  tgui_definition& gui_definition
	, const std::string& definition_type
	, const config& cfg
	, const char *key)
{
	std::vector<tcontrol_definition_ptr> definitions;

	BOOST_FOREACH(const config& definition, cfg.child_range(key ? key : definition_type + "_definition")) {
		definitions.push_back(new T(definition));
	}

	load_widget_definitions(gui_definition, definition_type, definitions);
}


tresolution_definition_ptr get_control(const std::string& control_type, const std::string& definition);

/**
 * Returns an interator to the requested builder.
 *
 * The builder is determined by the @p type and the current screen
 * resolution.
 *
 * @pre                       There is a valid builder for @p type at the
 *                            current resolution.
 *
 *
 * @param type                The type of builder window to get.
 *
 * @returns                   An iterator to the requested builder.
 */
std::vector<twindow_builder::tresolution>::const_iterator get_window_builder(const std::string& type);

bool is_control_definition(const std::string& type, const std::string& definition);

/** Loads the setting for the theme. */
void load_settings();

/** This namespace contains the 'global' settings. */
namespace settings {

	/**
	 * The screen resolution should be available for all widgets since
	 * their drawing method will depend on it.
	 */
	extern unsigned screen_width;
	extern unsigned screen_height;

	extern unsigned keyboard_height;

	/** These are copied from the active gui. */
	extern unsigned double_click_time;
	extern int longpress_time;
	extern int clear_click_threshold;

	extern std::string sound_button_click;
	extern std::string sound_toggle_button_click;
	extern std::string sound_toggle_panel_click;
	extern std::string sound_slider_adjust;

	extern std::string horizontal_scrollbar_id;
	extern std::string vertical_scrollbar_id;
	extern std::string tooltip_id;
	extern std::string drag_id;
	extern std::string magnifier_id;
	extern std::string edit_select_all_id;
	extern std::string edit_select_id;
	extern std::string edit_copy_id;
	extern std::string edit_paste_id;

	extern bool actived;
}

const ttpl_widget& get_tpl_widget(const std::string& tpl_id);

bool is_tpl_widget(const std::string& tpl_id);

struct tgui_definition
{
	tgui_definition()
		: id()
		, control_definition()
		, windows()
		, window_types()
	{
	}

	std::string id;

	const std::string& read(const config& cfg);

	typedef std::map <std::string /*control type*/,
		std::map<std::string /*id*/, tcontrol_definition_ptr> >
		tcontrol_definition_map;

	tcontrol_definition_map control_definition;

	std::map<std::string, twindow_definition> windows;

	std::map<std::string, twindow_builder> window_types;

	std::map<std::string, ttpl_widget> tpl_widgets;

	void load_widget_definitions(
			  const std::string& definition_type
			, const std::vector<tcontrol_definition_ptr>& definitions);
};

extern tgui_definition gui;

} // namespace gui2

#endif

