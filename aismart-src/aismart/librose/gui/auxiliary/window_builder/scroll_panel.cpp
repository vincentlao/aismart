/* $Id: scroll_panel.cpp 52533 2012-01-07 02:35:17Z shadowmaster $ */
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

#define GETTEXT_DOMAIN "rose-lib"

#include "gui/auxiliary/window_builder/scroll_panel.hpp"

#include "config.hpp"
#include "gettext.hpp"

#include "gui/auxiliary/widget_definition/scroll_panel.hpp"
#include "gui/auxiliary/window_builder/helper.hpp"
#include "gui/widgets/scroll_panel.hpp"
#include "wml_exception.hpp"

namespace gui2 {

namespace implementation {

tbuilder_scroll_panel::tbuilder_scroll_panel(const config& cfg)
	: tbuilder_control(cfg)
	, left_margin(cfg["left_margin"].to_int(nposm))
	, right_margin(cfg["right_margin"].to_int(nposm))
	, top_margin(cfg["top_margin"].to_int(nposm))
	, bottom_margin(cfg["bottom_margin"].to_int(nposm))
	, vertical_scrollbar_mode(
			get_scrollbar_mode(cfg["vertical_scrollbar_mode"]))
	, horizontal_scrollbar_mode(
			get_scrollbar_mode(cfg["horizontal_scrollbar_mode"]))
	, grid(NULL)
{
	if (left_margin != nposm) {
		left_margin = cfg_2_os_size(left_margin);
	}
	if (right_margin != nposm) {
		right_margin = cfg_2_os_size(right_margin);
	}
	if (top_margin != nposm) {
		top_margin = cfg_2_os_size(top_margin);
	}
	if (bottom_margin != nposm) {
		bottom_margin = cfg_2_os_size(bottom_margin);
	}

	const config &definition = cfg.child("grid");

	VALIDATE(definition, _("No list defined."));
	grid = new tbuilder_grid(definition);
	VALIDATE(grid, null_str);
}

twidget* tbuilder_scroll_panel::build() const
{
	tscroll_panel *widget = new tscroll_panel();

	init_control(widget);
	widget->set_margin(left_margin, right_margin, top_margin, bottom_margin);

	widget->set_vertical_scrollbar_mode(vertical_scrollbar_mode);
	widget->set_horizontal_scrollbar_mode(horizontal_scrollbar_mode);

	// Window builder: placed scroll_panel 'id' with definition 'definition'

	boost::intrusive_ptr<
			const tscroll_panel_definition::tresolution> conf =
				boost::dynamic_pointer_cast
					<const tscroll_panel_definition::tresolution>
						(widget->config());
	assert(conf);

	// widget->init_grid(conf->grid);
	widget->finalize_setup(conf->grid);

	/*** Fill the content grid. ***/
	tgrid* content_grid = widget->content_grid();
	assert(content_grid);

	const unsigned rows = grid->rows;
	const unsigned cols = grid->cols;

	content_grid->set_rows_cols(rows, cols);

	for(unsigned x = 0; x < rows; ++x) {
		content_grid->set_row_grow_factor(x, grid->row_grow_factor[x]);
		for(unsigned y = 0; y < cols; ++y) {

			if(x == 0) {
				content_grid->set_column_grow_factor(y
						, grid->col_grow_factor[y]);
			}

			twidget* widget = grid->widgets[x * cols + y]->build();
			content_grid->set_child(widget
					, x
					, y
					, grid->flags[x * cols + y]
					, grid->border_size[x * cols + y]);
		}
	}

	return widget;
}

} // namespace implementation

} // namespace gui2

/*WIKI
 * @page = GUIWidgetInstanceWML
 * @order = 2_scrollbar_panel
 *
 * == Scrollbar panel ==
 * @begin{parent}{name="gui/window/resolution/grid/row/column/"}
 * @begin{tag}{name="scroll_panel"}{min="0"}{max="-1"}{super="generic/widget_instance"}
 * Instance of a scroll_panel.
 *
 * List with the scroll_panel specific variables:
 * @begin{table}{config}
 *     vertical_scrollbar_mode & scrollbar_mode & initial_auto &
 *                                     Determines whether or not to show the
 *                                     scrollbar. $
 *     horizontal_scrollbar_mode & scrollbar_mode & initial_auto &
 *                                     Determines whether or not to show the
 *                                     scrollbar. $
 *
 *     definition & section & &        This defines how a scroll_panel item
 *                                     looks. It must contain the grid
 *                                     definition for 1 row of the list. $
 *
 * @end{table}
 * @begin{tag}{name="definition"}{min=0}{max=1}{super="gui/window/resolution/grid"}
 * @end{tag}{name="definition"}
 * @end{tag}{name="scroll_panel"}
 * @end{parent}{name="gui/window/resolution/grid/row/column/"}
 */

