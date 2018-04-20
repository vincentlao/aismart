/* $Id: scroll_panel.hpp 52533 2012-01-07 02:35:17Z shadowmaster $ */
/*
   Copyright (C) 2008 - 2012 by Mark de Wever <koraq@xs4all.nl>
   Part of the Battle for Wesnoth Project http://www.wesnoth.org/

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/

#ifndef GUI_AUXILIARY_WINDOW_BUILDER_SCROLL_PANEL_HPP_INCLUDED
#define GUI_AUXILIARY_WINDOW_BUILDER_SCROLL_PANEL_HPP_INCLUDED

#include "gui/auxiliary/window_builder/control.hpp"

#include "gui/widgets/scroll_container.hpp"

namespace gui2 {

namespace implementation {

struct tbuilder_scroll_panel	: public tbuilder_control
{
	explicit tbuilder_scroll_panel(const config& cfg);

	twidget* build () const;

	tscroll_container::tscrollbar_mode vertical_scrollbar_mode;
	tscroll_container::tscrollbar_mode horizontal_scrollbar_mode;

	tbuilder_grid_ptr grid;

	int left_margin;
	int right_margin;
	int top_margin;
	int bottom_margin;
};

} // namespace implementation

} // namespace gui2

#endif

