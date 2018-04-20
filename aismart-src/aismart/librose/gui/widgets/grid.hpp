/* $Id: grid.hpp 54906 2012-07-29 19:52:01Z mordante $ */
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

#ifndef GUI_WIDGETS_GRID_HPP_INCLUDED
#define GUI_WIDGETS_GRID_HPP_INCLUDED

#include "gui/widgets/widget.hpp"

namespace gui2 {

class tspacer;

/**
 * Base container class.
 *
 * This class holds a number of widgets and their wanted layout parameters. It
 * also layouts the items in the grid and hanldes their drawing.
 */
class tgrid : public twidget
{
	friend struct tgrid_implementation;
	friend class tvisual_layout;

public:

	explicit tgrid(const unsigned rows = 0, const unsigned cols = 0);

	virtual ~tgrid();

	/***** ***** ***** ***** LAYOUT FLAGS ***** ***** ***** *****/
	enum {
		VERTICAL_ALIGN_EDGE            = 1,
		VERTICAL_ALIGN_TOP             = 2,
		VERTICAL_ALIGN_CENTER          = 3,
		VERTICAL_ALIGN_BOTTOM          = 4,
		VERTICAL_MASK                  = 7,
    
	
		HORIZONTAL_ALIGN_EDGE          = 1 << 3,
		HORIZONTAL_ALIGN_LEFT          = 2 << 3,
		HORIZONTAL_ALIGN_CENTER        = 3 << 3,
		HORIZONTAL_ALIGN_RIGHT         = 4 << 3,
		HORIZONTAL_MASK                = 7 << 3,
    
		BORDER_TOP                     = 1 << 6,
		BORDER_BOTTOM                  = 1 << 7,
		BORDER_LEFT                    = 1 << 8,
		BORDER_RIGHT                   = 1 << 9,

		USER_PRIVATE                   = 1 << 10
	};

	static const unsigned VERTICAL_SHIFT                 = 0;
	static const unsigned HORIZONTAL_SHIFT               = 3;
	static const unsigned BORDER_ALL                     =
		BORDER_TOP | BORDER_BOTTOM | BORDER_LEFT | BORDER_RIGHT;

	/***** ***** ***** ***** ROW COLUMN MANIPULATION ***** ***** ***** *****/

	/**
	 * Addes a row to end of the grid.
	 *
	 * @param count               Number of rows to add, should be > 0.
	 *
	 * @returns                   The row number of the first row added.
	 */
	unsigned add_row(const unsigned count = 1);

	/**
	 * Sets the grow factor for a row.
	 *
	 * @todo refer to a page with the layout manipulation info.
	 *
	 * @param row                 The row to modify.
	 * @param factor              The grow factor.
	 */
	void set_row_grow_factor(const unsigned row, const unsigned factor);

	/**
	 * Sets the grow factor for a column.
	 *
	 * @todo refer to a page with the layout manipulation info.
	 *
	 * @param column              The column to modify.
	 * @param factor              The grow factor.
	 */
	void set_column_grow_factor(const unsigned column, const unsigned factor);

	/***** ***** ***** ***** CHILD MANIPULATION ***** ***** ***** *****/

	/**
	 * Sets a child in the grid.
	 *
	 * When the child is added to the grid the grid 'owns' the widget.
	 * The widget is put in a cell, and every cell can only contain 1 widget if
	 * the wanted cell already contains a widget, that widget is freed and
	 * removed.
	 *
	 * @param widget              The widget to put in the grid.
	 * @param row                 The row of the cell.
	 * @param col                 The columnof the cell.
	 * @param flags               The flags for the widget in the cell.
	 * @param border_size         The size of the border for the cell, how the
	 *                            border is applied depends on the flags.
	 */
	void set_child(twidget* widget, const unsigned row,
		const unsigned col, const unsigned flags, const unsigned border_size);

	/**
	 * Exchangs a child in the grid.
	 *
	 * It replaced the child with a certain id with the new widget but doesn't
	 * touch the other settings of the child.
	 *
	 * @param id                  The id of the widget to free.
	 * @param widget              The widget to put in the grid.
	 * @param recurse             Do we want to decent into the child grids.
	 * @param new_parent          The new parent for the swapped out widget.
	 *
	 * returns                    The widget which got removed (the parent of
	 *                            the widget is cleared). If no widget found
	 *                            and thus not replace NULL will returned.
	 */
	twidget* swap_child(
		const std::string& id, twidget* widget, const bool recurse,
		twidget* new_parent = NULL);

	/**
	 * Removes and frees a widget in a cell.
	 *
	 * @param row                 The row of the cell.
	 * @param col                 The columnof the cell.
	 */
	void remove_child(const unsigned row, const unsigned col);

	/**
	 * Removes and frees a widget in a cell by it's id.
	 *
	 * @param id                  The id of the widget to free.
	 * @param find_all            If true if removes all items with the id,
	 *                            otherwise it stops after removing the first
	 *                            item (or once all children have been tested).
	 */
	void remove_child(const std::string& id, const bool find_all = false);

	/**
	 * Activates all children.
	 *
	 * If a child inherits from tcontrol or is a tgrid it will call
	 * set_active() for the child otherwise it ignores the widget.
	 *
	 * @param active              Parameter for set_active.
	 */
	void set_active(const bool active);


	/** Returns the widget in the selected cell. */
	const twidget* widget(const unsigned row, const unsigned col) const
		{ return child(row, col).widget_; }

	/** Returns the widget in the selected cell. */
	twidget* widget(const unsigned row, const unsigned col)
		{ return child(row, col).widget_; }

	/***** ***** ***** ***** layout functions ***** ***** ***** *****/

	/** Inherited from twidget. */
	void layout_init(bool linked_group_only) override;

	/** Inherited from twidget. */
	tpoint calculate_best_size() const override;

	/** Inherited from twidget. */
	void impl_draw_children(texture& frame_buffer, int x_offset, int y_offset) override;
	void broadcast_frame_buffer(texture& frame_buffer);
	void clear_texture() override;

	bool canvas_texture_is_null() const;
	bool has_child_visible() const;

public:
	/** Inherited from twidget. */
	tpoint calculate_best_size_bh(const int width) override;
	void place(const tpoint& origin, const tpoint& size) override;

	/***** ***** ***** ***** Inherited ***** ***** ***** *****/

	/** Inherited from twidget. */
	void set_origin(const tpoint& origin);

	/** Inherited from twidget. */
	void set_visible_area(const SDL_Rect& area);

	/** Inherited from twidget. */
	void layout_children();

	/** Inherited from twidget. */
	void child_populate_dirty_list(twindow& caller, const std::vector<twidget*>& call_stack) override;

	bool popup_new_window() override;

	void dirty_under_rect(const SDL_Rect& clip) override;

	/** Inherited from twidget. */
	twidget* find_at(const tpoint& coordinate, const bool must_be_active) override;

	/** Inherited from twidget.*/
	twidget* find(const std::string& id, const bool must_be_active);

	/** Inherited from twidget.*/
	const twidget* find(const std::string& id,
			const bool must_be_active) const;

	/** Inherited from twidget.*/
	bool has_widget(const twidget* widget) const;

	/** Inherited from tcontrol. */
	bool disable_click_dismiss() const;

	/***** ***** ***** setters / getters for members ***** ****** *****/

	void set_rows(const unsigned rows);
	unsigned int get_rows() const { return rows_; }

	void set_cols(const unsigned cols);
	unsigned int get_cols() const { return cols_; }

	/**
	 * Wrapper to set_rows and set_cols.
	 *
	 * @param rows                Parameter to call set_rows with.
	 * @param cols                Parameter to call set_cols with.
	 */
	void set_rows_cols(const unsigned rows, const unsigned cols);

	void resize_children(int size);

	/** Child item of the grid. */
	struct tchild {
		/** The flags for the border and cell setup. */
		unsigned flags_;

		/**
		 * The size of the border, the actual configuration of the border
		 * is determined by the flags.
		 */
		unsigned border_size_;

		/**
		 * Pointer to the widget.
		 *
		 * Once the widget is assigned to the grid we own the widget and are
		 * responsible for it's destruction.
		 */
		twidget* widget_;

	}; // class tchild

	struct titerator {
		const tchild* children;
		int size;
	};

	tchild* children() { return children_; }
	const tchild* children() const { return children_; }
	int children_vsize() const;

	const tchild& child(const unsigned row, const unsigned col) const
		{ return children_[row * cols_ + col]; }
	tchild& child(const unsigned row, const unsigned col)
		{ return children_[row * cols_ + col]; }

	const tchild& child(const unsigned at) const
	{ 
		return children_[at]; 
	}

private:
	/** Layouts the children in the grid. */
	void layout(const tpoint& origin);

protected:
	/** The number of grid rows. */
	unsigned rows_;

	/** The number of grid columns. */
	unsigned cols_;

	/***** ***** ***** ***** size caching ***** ***** ***** *****/

	/** The row heights in the grid. */
	mutable std::vector<unsigned> row_height_;

	/** The column widths in the grid. */
	mutable std::vector<unsigned> col_width_;

	/** The grow factor for all rows. */
	std::vector<unsigned> row_grow_factor_;

	/** The grow factor for all columns. */
	std::vector<unsigned> col_grow_factor_;

	/**
	 * The child items.
	 *
	 * All children are stored in a 1D vector and the formula to access a cell
	 * is: row * cols_ + col. All other vectors use the same access formula.
	 */
	tchild* children_;
	int children_size_;
	int children_vsize_;

	std::vector<tspacer*> stuff_widget_;
	int valid_stuff_size_;

	const std::string& get_control_type() const;
};

/** Returns the best size for the cell. */
tpoint cell_get_best_size(const tgrid::tchild& cell);

/**
 * Places the widget in the cell.
 *
 * @param origin          The origin (x, y) for the widget.
 * @param size            The size for the widget.
 */
void cell_place(tgrid::tchild& cell, tpoint origin, tpoint size);

/** Returns the space needed for the border. */
tpoint cell_border_space(const tgrid::tchild& cell);

/** Returns the space needed for the border. */
tpoint cell_border_space(const tgrid::tchild& cell);

} // namespace gui2

#endif

