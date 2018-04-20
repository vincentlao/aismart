/* $Id: campaign_difficulty.cpp 49602 2011-05-22 17:56:13Z mordante $ */
/*
   Copyright (C) 2010 - 2011 by Ignacio Riquelme Morelle <shadowm2006@gmail.com>
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

#include "gui/dialogs/chat.hpp"

#include "gettext.hpp"
#include "gui/dialogs/helper.hpp"
#include "gui/widgets/settings.hpp"
#include "gui/widgets/window.hpp"
#include "gui/widgets/label.hpp"
#include "gui/widgets/button.hpp"
#include "gui/widgets/tree.hpp"
#include "gui/widgets/toggle_panel.hpp"
#include "gui/widgets/text_box.hpp"
#include "gui/widgets/toggle_button.hpp"
#include "gui/widgets/scroll_text_box.hpp"
#include "gui/widgets/report.hpp"
#include "gui/widgets/listbox.hpp"
#include "gui/widgets/spacer.hpp"
#include "gui/widgets/stack.hpp"
#include "gui/widgets/track.hpp"
#include "gui/dialogs/transient_message.hpp"
#include "gui/dialogs/netdiag.hpp"
#include "gui/dialogs/message.hpp"
#include "formula_string_utils.hpp"
#include <hero.hpp>
#include "filesystem.hpp"
#include "proto_irc.hpp"
#include "rose_config.hpp"
#include "base_instance.hpp"

#include <iomanip>
#include <boost/foreach.hpp>
#include <boost/bind.hpp>

#include "rtc_base/json.h"

namespace gui2 {

REGISTER_DIALOG(rose, chat2)

#define ctrlid_contact_layer	"_chat_layer_contact"
#define ctrlid_find_layer	"_chat_layer_find"
#define ctrlid_channel_layer	"_chat_layer_channel"
#define ctrlid_channel_vrenderer	"_chat_layer_vrenderer"

#define ctrlid_msg_layer	"_chat_layer_msg"
#define ctrlid_contact_bar	"_chat_contact_bar"
#define ctrlid_contact_panel	"_chat_contact_panel"
#define ctrlid_notifies		"_chat_notifies"
#define ctrlid_persons		"_chat_persons"
#define ctrlid_notifies_layer	"_layer_chat_notifies"
#define ctrlid_persons_layer	"_layer_chat_persons"
#define ctrlid_previous		"_chat_previous"
#define ctrlid_next			"_chat_next"
#define ctrlid_pagenum		"_chat_pagenum"
#define ctrlid_history		"_chat_history"
#define ctrlid_qq_icon		"_chat_qq_icon"
#define ctrlid_qq_name		"_chat_qq_name"
#define ctrlid_tool_bar		"_chat_tool_bar"
#define ctrlid_input		"_chat_input"
#define ctrlid_input_scale	"_chat_input_scale"
#define ctrlid_send			"_chat_send"
#define ctrlid_send_tip		"_chat_send_tip"
#define ctrlid_switch_to_chat_find	"_chat_switch_to_chat_find"
#define ctrlid_switch_to_chat_channel	"_chat_switch_to_chat_channel"
#define ctrlid_switch_to_find	"_chat_switch_to_find"
#define ctrlid_return		"_chat_return"
#define ctrlid_find			"_chat_find"
#define ctrlid_join			"_chat_join"
#define ctrlid_find_filter	"_chat_find_filter"
#define ctrlid_find_min_users	"_chat_find_min_users"
#define ctrlid_chanlist		"_chat_chanlist"
#define ctrlid_vrenderer_status	"_chat_status"
#define ctrlid_relayonly	"_chat_relay"

#define ctrlid_channel_label	"_chat_channel_label"
#define ctrlid_channel2		"_chat_channel2"
#define ctrlid_chat_to		"_chat_chat_to"
#define ctrlid_join_friend	"_chat_join_friend"

#define person_png			"misc/rose-36.png"
#define channel_png			"misc/channel.png"

#define at_person			0
#define at_channel			1

// Names used for a IceCandidate JSON object.
static const char kCandidateSdpMidName[] = "sdpMid";
static const char kCandidateSdpMlineIndexName[] = "sdpMLineIndex";
static const char kCandidateSdpName[] = "candidate";

// Names used for a SessionDescription JSON object.
static const char kSessionDescriptionSdpName[] = "sdpOffer";

tchat_::tfunc::tfunc(int id)
	: id(id)
	, type(ft_none)
	, tooltip()
{
	if (id == f_netdiag) {
		type = ft_none | ft_person | ft_channel | ft_chan_person;
		image = "misc/network.png";
		tooltip = _("Network");

	} else if (id == f_copy) {
		type = ft_person | ft_channel | ft_chan_person;
		image = "misc/copy.png";
		tooltip = _("Copy");

	} else if (id == f_reply) {
		type = ft_person | ft_channel | ft_chan_person;
		image = "misc/reply.png";
		tooltip = _("Reply");

	} else if (id == f_face) {
		type = 0;
		image = "misc/face.png";
		tooltip = _("Face");

	} else if (id == f_part) {
		type = ft_channel;
		image = "misc/exit.png";
		tooltip = _("Part channel");

	} else if (id == f_part_friend) {
		type = ft_person | ft_chan_person;
		image = "misc/exit.png";
		tooltip = _("Part friend");

	} else if (id == f_explore) {
		type = ft_channel;
		image = "misc/explore.png";
		tooltip = _("Explore");
	}
}

int tchat_::tsession::logs_per_page = 50;

tchat_::tsession::tsession(chat_logs::treceiver& receiver)
	: receiver(&receiver)
	, current_page(0)
{
	const chat_logs::thistory_log* choice = NULL;
	for (std::set<chat_logs::thistory_log>::const_iterator it = chat_logs::history_logs.begin(); it != chat_logs::history_logs.end(); ++ it) {
		const chat_logs::thistory_log& logs = *it;
		if (logs.nick == receiver.nick) {
			choice = &logs;
			break;
		}
	}
	if (!choice) {
		return;
	}
	chat_logs::user_from_logfile(*choice, history);
}

int tchat_::tsession::current_logs(std::vector<chat_logs::tlog>& logs) const
{
	logs.clear();
	int history_size = history.size();
	int size = history_size + receiver->logs.size();
	if (!size) {
		return nposm;
	}

	int page = pages() - current_page - 1;
	// both start and end are index. end is last valid index.
	int start = page * logs_per_page;
	int end = start + logs_per_page - 1;
	int remainder = size % logs_per_page;
	if (remainder) {
		if (page) {
			start -= logs_per_page - remainder;
			end -= logs_per_page - remainder;
		} else {
			end -= logs_per_page - remainder;
		}
	}

	int history_start = -1, history_end = -1, now_start = -1, now_end = -1;
	if (history_size > start) {
		history_start = start;
		if (history_size > end) {
			history_end = end;
		} else {
			history_end = history_size - 1;
			now_start = 0;
			now_end = (end - start) - (history_end - history_start + 1);
		}
	} else {
		now_start = start - history_size;
		now_end = end - history_size;
	}

	std::vector<chat_logs::tlog>::const_iterator begin_it;
	std::vector<chat_logs::tlog>::const_iterator end_it;
	if (history_start != -1) {
		begin_it = history.begin();
		std::advance(begin_it, history_start);
		end_it = history.begin();
		std::advance(end_it, history_end + 1);
		std::copy(begin_it, end_it, std::back_inserter(logs));
	}
	if (now_start != -1) {
		begin_it = receiver->logs.begin();
		std::advance(begin_it, now_start);
		end_it = receiver->logs.begin();
		std::advance(end_it, now_end + 1);
		std::copy(begin_it, end_it, std::back_inserter(logs));
	}

	return start;
}

int tchat_::tsession::pages() const
{
	return ceil(1.0 * (history.size() + receiver->logs.size()) / logs_per_page);
}

bool tchat_::tsession::can_previous() const 
{
	int pgs = pages();
	return pgs && current_page < pgs - 1; 
}

bool tchat_::tsession::can_next() const 
{ 
	int pgs = pages();
	return pgs && current_page > 0; 
}

const chat_logs::tlog& tchat_::tsession::log(int at) const
{
	const chat_logs::tlog* log = NULL;
	int history_size = (int)history.size();
	if (at < history_size) {
		log = &history[at];
	} else {
		log = &receiver->logs[at - history_size];
	}
	return *log;
}

std::string tchat_::err_encode_str;

tchat_::tchat_(const std::string& widget_id, int chat_page)
	: trtc_client(*this, tpoint(nposm, nposm))
	, widget_id_(widget_id)
	, signature_(0)
	, person_cookies_()
	, channel_cookies_()
	, gamelist_()
	, contact_layer_(NULL)
	, find_layer_(NULL)
	, channel_layer_(NULL)
	, vrenderer_layer_(NULL)
	, msg_layer_(NULL)
	, history_(NULL)
	, input_(NULL)
	, input_tb_(NULL)
	, send_(NULL)
	, previous_page_(NULL)
	, next_page_(NULL)
	, src_pos_(0)
	, current_session_(NULL)
	, inputing_(false)
	, current_page_(nposm)
	, chat_page_(chat_page)
	, page_panel_(NULL)
	, current_ft_(ft_none)
	, in_find_chan_(false)
	, catalog_(NULL)
	, toolbar_(NULL)
	, swap_resultion_(false)
	, portrait_(false)
	, vrenderer_track_(NULL) // webrtc
	, local_render_size_(tpoint(nposm, nposm))
	, original_local_offset_(0, 0)
	, current_local_offset_(0, 0)
{
	// application maybe enter chat directly.
	// chat_::pre_show some action require lobby.chat to right state. for example channels/persons.
	lobby->pump();

	if (err_encode_str.empty()) {
		err_encode_str = _("Character encoding must be UTF-8!");
	}

}

tchat_::~tchat_()
{
}

void tchat_::user_to_title(const tcookie& cookie) const
{
	tcontrol& icon = find_widget<tcontrol>(window_, ctrlid_qq_icon, false);
	if (cookie.channel) {
		icon.set_label(channel_png);
	} else {
		icon.set_label(person_png);
	}

	tlabel& label = find_widget<tlabel>(window_, ctrlid_qq_name, false);
	label.set_label(ht::generate_format(cookie.nick, color_to_uint32(font::GRAY_COLOR)));
}

void tchat_::contact_person_toggled(twidget& widget)
{
	ttoggle_panel* toggle = dynamic_cast<ttoggle_panel*>(&widget);
	std::pair<std::vector<tcookie>*, tcookie* > pair = contact_find(true, toggle->get_data());

	tcookie& cookie = *pair.second;

	if (cookie.channel) {
		return;
	}

	switch_session(true, *pair.first, cookie);
}

void tchat_::contact_channel_toggled(ttree& view, ttree_node& node)
{
	ttoggle_panel& toggle = node;
	std::pair<std::vector<tcookie>*, tcookie* > pair = contact_find(false, toggle.get_data());

	if (!pair.second || !pair.second) {
		return;
	}

	tcookie& cookie = *pair.second;
	if (!cookie.channel) {
		return;
	}

	switch_session(false, *pair.first, cookie);
}

void tchat_::refresh_channel2_toolbox(const tlobby_user& user)
{
	bool favor = lobby->chat->is_favor_user(user.uid);

	// now only can chato favor's user
	chat_to_->set_active(!user.me && favor);
	join_friend_->set_active(!user.me && !favor);
}

void tchat_::channel2_toggled(twidget& widget)
{
	ttoggle_panel* toggle = dynamic_cast<ttoggle_panel*>(&widget);
	tcookie& cookie = channel2_branch_[toggle->cookie()];

	tlobby_user& user = lobby->chat->get_user(cookie.id);
	refresh_channel2_toolbox(user);
}

void tchat_::simulate_cookie_toggled(bool person, int cid, int id, bool channel)
{
	std::pair<std::vector<tcookie>*, tcookie* > pair = contact_find(person, cid, id, channel);
	ttree* tree = person_tree_;

	tcookie& cookie = *pair.second;
	tree->select_node(cookie.node);

	ttoggle_panel& toggle = *tree->cursel();
	if (person) {
		contact_person_toggled(toggle);
	} else {
		contact_channel_toggled(*tree, *(tree->cursel()));
	}
}

void tchat_::netdiag(twindow& window)
{
	gui2::tnetdiag dlg;
	dlg.show();
}

bool is_blank_str(const std::string& str)
{
	const char* blank_char = " \n";
	size_t pos = str.find_first_not_of(blank_char);
	return pos == std::string::npos;
}

void tchat_::handle_multiline(const char* nick, const char* chan, const char* text)
{
	irc::server* serv = lobby->chat->serv();
	char* tmp = (char*)text;

	int i = 0, pos = 0, len = (int)strlen(text);
	while (i < len) {
		if (text[i] == '\n') {
			tmp[pos] = 0;
			if (pos) {
				irc::send_msg(serv, nick, chan, tmp);
				// serv->p_message(serv, chan, tmp);
			}
			// recover to orignal charactor
			tmp[pos] = '\n';

			tmp = (char*)(text + i + 1);
			pos = 0;
		} else {
			pos ++;
		}
		i ++;
	}
	if (pos) {
		irc::send_msg(serv, nick, chan, tmp);
		// serv->p_message(serv, chan, tmp);
	}
}

void tchat_::chat_setup(twindow& window, bool caller)
{
	if (!peer_connection_.get()) {
		ttext_box* text_box = &find_widget<ttext_box>(window_, "_chat_my_nick", false);
		my_nick_ = text_box->label();

		if (my_nick_.size() < 2) {
			gui2::show_message("", _("Invalid my nick"));
			return;
		}

		if (caller) {
			peer_nick_ = input_->label();
			if (peer_nick_.size() < 2) {
				gui2::show_message("", _("Invalid peer nick"));
				return;
			}
		}

		caller_ = caller;
		StartLogin("133.130.113.73", 8080);
		// StartLogin("z.zhanggz.com", 8080);

	} else {
		if (caller) {
			

		} else {
			// close session
			if (state_ != CONNECTED) {
				return;
			}
			VALIDATE(peer_connection_.get(), null_str);

			DeletePeerConnection();
			{
				// {"id":"stop"}
				Json::StyledWriter writer;
				Json::Value jmessage;

				jmessage["id"] = "stop";
				msg_2_signaling_server(control_socket_.get(), writer.write(jmessage));
			}
			Close();
		}
	}
}

void tchat_::send(bool& handled, bool& halt)
{
	twindow& window = *window_;

	if (!lobby->chat->ready()) {
		return;
	}

	if (!current_session_) {
		return;
	}
	std::string input_str = input_->label();
	if (is_blank_str(input_str)) {
		return;
	}

	std::string orignal_input_str = input_str;

	handle_multiline(lobby->chat->nick().c_str(), current_session_->receiver->nick.c_str(), input_str.c_str());

	input_->set_label(null_str);

	chat_logs::add(current_session_->receiver->id, current_session_->receiver->channel, *lobby->chat->me, input_str);
	chat_2_scroll_label(*history_, *current_session_);
}

void tchat_::find(twindow& window)
{
	if (!in_find_chan_) {
		irc::server* serv = lobby->chat->serv();

		ttext_box* text_box = &find_widget<ttext_box>(window_, ctrlid_find_filter, false);
		std::string filter = text_box->label();

		text_box = &find_widget<ttext_box>(window_, ctrlid_find_min_users, false);
		cond_min_users_ = utils::to_int(text_box->label());
		if (cond_min_users_ < 1) {
			cond_min_users_ = 1;
		}

		std::string nick;
		if (!filter.empty()) {
			nick = std::string("#") + filter;
		}
		lobby->chat->find_channel(nick, cond_min_users_);

	} else {
		process_chanlist_end();
	}
}

void tchat_::join_channel(twindow& window)
{
	const std::string& chan = list_chans_[chanlist_->cursel()->at()];
	int cid = tlobby_channel::get_cid(chan);
	tlobby_channel& channel = lobby->chat->insert_channel(cid, chan);
	ready_branch(false, channel);

	lobby->chat->join_channel(channel, true);
	switch_to_home(window);
}

void tchat_::chat_to(twindow& window)
{
	ttoggle_panel& toggle = *channel2_tree_->cursel();
	const tcookie& cookie = channel2_branch_[toggle.cookie()];
	simulate_cookie_toggled(true, tlobby_channel::t_friend, cookie.id, false);

	if (portrait_) {
		switch_to_msg(window);
	} else {
		switch_to_home(window);
	}
}

void tchat_::join_friend(twindow& window)
{
	tlobby_channel& channel = lobby->chat->get_channel(tlobby_channel::t_friend);

	ttoggle_panel& toggle = *channel2_tree_->cursel();
	const tcookie& cookie = channel2_branch_[toggle.cookie()];
	tlobby_user& user = channel.insert_user(cookie.id, cookie.nick);

	std::vector<tcookie>& branch = person_cookies_.find(tlobby_channel::t_friend)->second;
	insert_user(true, branch, user);

	irc::server* serv = lobby->chat->serv();
	serv->p_monitor(serv, cookie.nick.c_str(), true);

	refresh_channel2_toolbox(user);

	std::stringstream ss;
	utils::string_map symbols;

	symbols["nick"] = ht::generate_format(user.nick, color_to_uint32(font::YELLOW_COLOR));
	symbols["channel"] = ht::generate_format(channel.name(), color_to_uint32(font::GOOD_COLOR));
	ss << vgettext2("You set $nick into $channel successfully!", symbols);
	gui2::show_message("", ss.str());
}

void tchat_::insert_face(twindow& window, int index)
{
	std::stringstream ss;
	ss << "face/default/" << index << ".png";
	ss.str();
	tscroll_text_box& input = find_widget<tscroll_text_box>(window_, ctrlid_input, false);
	input.insert_img(ss.str());
}

void tchat_::enter_inputing(bool enter)
{
	if ((enter && inputing_) || (!enter && !inputing_)) {
		return;
	}

	int height;
	if (enter) {
		if ((int)settings::screen_height <= twidget::hdpi_scale * twidget::max_effectable_point) {
			height = settings::screen_height * 20 / 100;
		} else {
			height = 108 * twidget::hdpi_scale;
		}
	} else {
		if ((int)settings::screen_height <= twidget::hdpi_scale * twidget::max_effectable_point) {
			height = settings::screen_height * 10 / 100;
		} else {
			height = 54 * twidget::hdpi_scale;
		}
	}

	inputing_ = enter;

	input_scale_->set_best_size_1th(nposm, height);
}

void tchat_::signal_handler_sdl_key_down(bool& handled
		, bool& halt
		, const SDL_Keycode key
		, SDL_Keymod modifier
		, const Uint16 unicode)
{
#if (defined(__APPLE__) && TARGET_OS_IPHONE) || defined(ANDROID)
	if (key == SDLK_PRINTSCREEN) {
		SDL_Rect rc;
		SDL_SetTextInputRect(&rc);
		settings::keyboard_height = rc.h * twidget::hdpi_scale;
        window_->invalidate_layout(nullptr);
	}
#endif

#ifdef _WIN32
	if ((key == SDLK_RETURN || key == SDLK_KP_ENTER) && !(modifier & KMOD_SHIFT)) {
		send(handled, halt);

		handled = true;
		halt = true;
	}
#else
	if (key == SDLK_RETURN) {
		settings::keyboard_height = 0;
		window_->invalidate_layout(nullptr);

		handled = true;
		halt = true;
	}
#endif

	if (key == SDLK_ESCAPE) {
		enter_inputing(false);

	} else if (key >= SDLK_SPACE && key < SDL_SCANCODE_TO_KEYCODE(1)) {
		enter_inputing(true);
	}
}

void tchat_::generate_channel_tree(tlobby_channel& channel)
{
	if (channel.users.empty()) {
		return;
	}

	ttree_node& root_node = channel2_tree_->get_root_node();
	std::stringstream ss;

	ss.str("");
	ss << ht::generate_img(channel_png);
	ss << channel.nick;
	ss << " (" << ht::generate_format(channel.users.size(), color_to_uint32(font::YELLOW_COLOR)) << ")";
	tlabel& label = find_widget<tlabel>(window_, ctrlid_channel_label, false);
	label.set_label(ss.str());

	for (std::vector<tlobby_user*>::const_iterator it = channel.users.begin(); it != channel.users.end(); ++ it) {
		tlobby_user& user = **it;
		
		std::map<std::string, std::string> tree_group_item;

		tree_group_item["name"] = user.nick;
		channel2_branch_.push_back(tcookie(signature_, &root_node.insert_node("item", tree_group_item), user.nick, user.uid, false));
		tcookie& cookie = channel2_branch_.back();
		cookie.online = true;

		ttoggle_panel& toggle = *cookie.node;
		toggle.set_cookie(channel2_branch_.size() - 1);
		toggle.set_did_state_changed(boost::bind(&tchat_::channel2_toggled, this, _1));

		ss.str("");
		ss << ht::generate_img(person_png);
		ss << cookie.nick;

		tlabel* label = dynamic_cast<tlabel*>(cookie.node->find("name", true));
		label->set_label(ss.str());
	}

	ttoggle_panel& toggle = *channel2_tree_->cursel();
	channel2_toggled(toggle);
}

std::vector<tchat_::tcookie>& tchat_::ready_branch(bool person, const tlobby_channel& channel)
{
	ttree* tree = person_tree_;

	std::map<std::string, std::string> tree_group_item;
	tcookie cookie;

	std::string nick;
	std::stringstream label;
	if (person) {
		nick = channel.name();

	} else {
		nick = tlobby_channel::get_nick(channel.cid);
		if (channel.err) {
			label << ht::generate_format("+R", color_to_uint32(font::BAD_COLOR));
		}
	}
	label << nick << "\n" << "--/--";

	std::string icon;
	if (person) {
		icon = null_str;
	} else {
		icon = std::string(channel_png) + "~GS()";
	}
	tree_group_item["icon"] = icon;
	tree_group_item["name"] = label.str();
	cookie = tcookie(signature_, &tree->get_root_node().insert_node("item", tree_group_item), nick, channel.cid, true);

	ttoggle_panel& toggle = *cookie.node;
	toggle.set_data(signature_ ++);

	std::map<int, std::vector<tcookie> >& cookies = person? person_cookies_: channel_cookies_;
	std::pair<std::map<int, std::vector<tcookie> >::iterator, bool> ins = 
		cookies.insert(std::make_pair(channel.cid, std::vector<tcookie>(1, cookie)));
	return ins.first->second;
}

std::pair<std::vector<tchat_::tcookie>*, tchat_::tcookie* > tchat_::contact_find(bool person, size_t signature)
{
	std::map<int, std::vector<tcookie> >& cookies = person? person_cookies_: channel_cookies_;

	std::pair<std::vector<tcookie>*, tcookie* > ret = std::make_pair(reinterpret_cast<std::vector<tcookie>*>(NULL), reinterpret_cast<tcookie*>(NULL));
	for (std::map<int, std::vector<tcookie> >::iterator it = cookies.begin(); it != cookies.end() && !ret.first; ++ it) {
		for (std::vector<tcookie>::iterator it2 = it->second.begin(); it2 != it->second.end() && !ret.first; ++ it2) {
			if (it2->signature == signature) {
				ret.first = &it->second;
				ret.second = &*it2;
			}
		}
	}
	return ret;
}

std::pair<std::vector<tchat_::tcookie>*, tchat_::tcookie* > tchat_::contact_find(bool person, int cid, int id, bool channel)
{
	std::map<int, std::vector<tcookie> >& cookies = person? person_cookies_: channel_cookies_;

	std::pair<std::vector<tcookie>*, tcookie* > ret = std::make_pair(reinterpret_cast<std::vector<tcookie>*>(NULL), reinterpret_cast<tcookie*>(NULL));
	if (cid != tlobby_channel::npos) {
		std::map<int, std::vector<tcookie> >::iterator it = cookies.find(cid);
		if (it == cookies.end()) {
			return ret;
		}
		for (std::vector<tcookie>::iterator it2 = it->second.begin(); it2 != it->second.end() && !ret.first; ++ it2) {
			if (it2->id == id && it2->channel == channel) {
				ret.first = &it->second;
				ret.second = &*it2;
			}
		}
	} else {
		for (std::map<int, std::vector<tcookie> >::iterator it = cookies.begin(); it != cookies.end() && !ret.first; ++ it) {
			for (std::vector<tcookie>::iterator it2 = it->second.begin(); it2 != it->second.end() && !ret.first; ++ it2) {
				if (it2->id == id && it2->channel == channel) {
					ret.first = &it->second;
					ret.second = &*it2;
				}
			}
		}
	}
	return ret;
}

std::pair<const std::vector<tchat_::tcookie>*, const tchat_::tcookie* > tchat_::contact_find(bool person, int cid, int id, bool channel) const
{
	const std::map<int, std::vector<tcookie> >& cookies = person? person_cookies_: channel_cookies_;

	std::pair<const std::vector<tcookie>*, const tcookie* > ret = std::make_pair(reinterpret_cast<std::vector<tcookie>*>(NULL), reinterpret_cast<tcookie*>(NULL));
	if (cid != tlobby_channel::npos) {
		std::map<int, std::vector<tcookie> >::const_iterator it = cookies.find(cid);
		if (it == cookies.end()) {
			return ret;
		}
		for (std::vector<tcookie>::const_iterator it2 = it->second.begin(); it2 != it->second.end() && !ret.first; ++ it2) {
			if (it2->id == id && it2->channel == channel) {
				ret.first = &it->second;
				ret.second = &*it2;
			}
		}
	} else {
		for (std::map<int, std::vector<tcookie> >::const_iterator it = cookies.begin(); it != cookies.end() && !ret.first; ++ it) {
			for (std::vector<tcookie>::const_iterator it2 = it->second.begin(); it2 != it->second.end() && !ret.first; ++ it2) {
				if (it2->id == id && it2->channel == channel) {
					ret.first = &it->second;
					ret.second = &*it2;
				}
			}
		}
	}
	return ret;
}

void tchat_::update_node_internal(const std::vector<tcookie>& cookies, const tcookie& cookie)
{
	std::stringstream icon_ss;
	std::stringstream ss;

	if (cookie.channel) {
		const tlobby_channel& channel = lobby->chat->get_channel(cookie.id);

		if (tlobby_channel::is_allocatable(channel.cid)) {
			icon_ss << channel_png;
			if (channel.users.empty()) {
				icon_ss << "~GS()";
			}
		}

		if (channel.err) {
			ss << ht::generate_format("+R", color_to_uint32(font::BAD_COLOR));
		}
		ss << cookie.nick << "\n";
		
		if (channel.users.empty()) {
			ss << "--/--";

		} else if (tlobby_channel::is_allocatable(channel.cid)) {
			ss << ht::generate_format(channel.users.size(), color_to_uint32(font::GRAY_COLOR));

		} else {
			size_t talkable = 0;
			for (std::vector<tcookie>::const_iterator it = cookies.begin(); it != cookies.end(); ++ it) {
				const tcookie& c2 = *it;
				if (c2.online && !c2.away) {
					talkable ++;
				}
			}

			ss << talkable << "/ " << (cookies.size() - 1);
		}
		if (cookie.unread) {
			ss << " (" << ht::generate_format(cookie.unread, color_to_uint32(font::GOOD_COLOR)) << ")";
		}

	} else {
		icon_ss << person_png;
		if (!cookie.online) {
			icon_ss << "~GS()";
		}

		ss << cookie.nick << "\n";
		if (cookie.unread) {
			ss << " (" << ht::generate_format(cookie.unread, color_to_uint32(font::GOOD_COLOR)) << ")";
		}

		update_node_internal(cookies, cookies.front());
	}

	tcontrol* icon = dynamic_cast<tcontrol*>(cookie.node->find("icon", true));
	icon->set_label(icon_ss.str());

	tlabel* label = dynamic_cast<tlabel*>(cookie.node->find("name", true));
	label->set_label(ss.str());
}

static const char* face_items[] = {
	"face0",
	"face1",
	"face2",
	"face3",
	"face4",
	"face5",
	"face6",
	"face7",
	"face8",
	"face9",
	"face10",
	"face11"
};
static int nb_items = sizeof(face_items) / sizeof(face_items[0]);

void tchat_::ready_face(twindow& window)
{
	tbutton* b;
	std::stringstream ss;
	for (int item = 0; item < nb_items; item ++) {
		b = find_widget<tbutton>(&window, face_items[item], false, false);
		ss.str("");
		ss << "face/default/" << item << ".png";
		b->set_label(ss.str());
	
		connect_signal_mouse_left_click(
			  *b
			, boost::bind(
				  &tchat_::insert_face
				, this
				, boost::ref(window)
				, item));
	}
}

void tchat_::reload_toolbar(twindow& window)
{
	funcs_.clear();
	for (int n = f_min; n <= f_max; n ++) {
		funcs_.push_back(n);
		tfunc& func = funcs_.back();

		tcontrol& widget = toolbar_->insert_item(null_str, func.image);
		widget.set_tooltip(func.tooltip);
		widget.set_cookie(n);
	}
}

void tchat_::refresh_toolbar(int type, int id)
{
	int n = 0;
	for (std::vector<tfunc>::const_iterator it = funcs_.begin(); it != funcs_.end(); ++ it, n ++) {
		const tfunc& func = *it;
		bool active = true;
		toolbar_->set_item_visible(n, func.type & type);
		if (func.id == f_copy) {
			active = current_session_ && (!current_session_->history.empty() || !current_session_->receiver->logs.empty());

		} else if (func.id == f_reply) {
			active = current_session_ && (!current_session_->history.empty() || !current_session_->receiver->logs.empty());
			if (active) {
				const std::string& my_nick = lobby->chat->me? lobby->chat->me->nick: lobby->nick();
				twidget* panel = history_->cursel();
				int index = (int)panel->cookie();
				const chat_logs::tlog& log = current_session_->log(index);
				active = log.nick != my_nick;
			}

		} else if (func.id == f_face) {
			active = false;

		} else {
			active = true;
		}
		toolbar_->item(n).set_active(active);
	}

	tcontrol* widget;
	if (type == ft_channel) {
		tlobby_channel& channel = lobby->chat->get_channel(id);
		widget = &toolbar_->item(toolbar_->get_at2(reinterpret_cast<void*>(f_part)));
		if ((!channel.err && channel.users.empty()) || channel.nick == game_config::app_channel) {
			widget->set_active(false);
		}
		if (channel.users.empty()) {
			widget = &toolbar_->item(toolbar_->get_at2(reinterpret_cast<void*>(f_explore)));
			widget->set_active(false);
		}

	} else if (type == ft_person || type == ft_chan_person) {
		bool favor = lobby->chat->is_favor_user(id);
		widget = &toolbar_->item(toolbar_->get_at2(reinterpret_cast<void*>(f_part_friend)));
		widget->set_active(favor && type == ft_person);
	}

	current_ft_ = type;
}

void tchat_::clear_branch(bool person, int type)
{
	std::map<int, std::vector<tcookie> >& cookies = person? person_cookies_: channel_cookies_;
	ttree* tree = person_tree_;

	std::vector<tcookie>& branch = cookies.find(type)->second;
	std::vector<tcookie>::iterator it = branch.begin();
	for (++ it; it != branch.end();) {
		ttree_node* node = it->node;
		tree->erase_node(*node);
		it = branch.erase(it);
	}
}

void tchat_::remove_branch(bool person, int type)
{
	std::map<int, std::vector<tcookie> >& cookies = person? person_cookies_: channel_cookies_;
	ttree* tree = person_tree_;

	std::map<int, std::vector<tcookie> >::iterator it = cookies.find(type);
	std::vector<tcookie>& branch = it->second;

	tree->erase_node(*branch.front().node);
	cookies.erase(it);
}

void tchat_::insert_user(bool person, std::vector<tcookie>& branch, const tlobby_user& user)
{
	if (person) {
		std::map<std::string, std::string> tree_group_item;

		tree_group_item["name"] = user.nick;
		ttree_node& branch_node = *branch.front().node;
		branch.push_back(tcookie(signature_, &branch_node.insert_node("item", tree_group_item), user.nick, user.uid, false));
		tcookie& cookie = branch.back();
		cookie.online = user.online;
		cookie.away = user.away;
		cookie.unread = user.unread;

		ttoggle_panel& toggle = *cookie.node;
		toggle.set_data(signature_ ++);
		
		update_node_internal(branch, cookie);

	} else {
		update_node_internal(branch, branch.front());
	}
}

void tchat_::erase_user(bool person, std::vector<tcookie>& branch, int uid)
{
	ttree* tree = person_tree_;

	for (std::vector<tcookie>::iterator it = branch.begin(); it != branch.end(); ++ it) {
		const tcookie& that = *it;
		if (!that.channel && that.id == uid) {
			ttree_node* node = it->node;
			tree->erase_node(*node);
			branch.erase(it);

			update_node_internal(branch, branch.front());
			return;
		}
	}
}

void tchat_::pre_show(twindow& window)
{	
	rtc::LogMessage::LogToDebug(rtc::LS_INFO);


	const SDL_Rect rect = screen_area();
	if (twidget::current_landscape) {
		if (twidget::should_conside_orientation(rect.w, rect.h)) {
			twidget::current_landscape = false;
			swap_resultion_ = twindow::set_orientation_resolution();
		}
	}
	if (!swap_resultion_) {
		pre_create_renderer();
	}
	if (twidget::current_landscape) {
		SDL_SetHint(SDL_HINT_ORIENTATIONS, "LandscapeLeft LandscapeRight");
	} else {
		SDL_SetHint(SDL_HINT_ORIENTATIONS, "Portrait");
	}
	portrait_ = !twidget::current_landscape;
	{
		int ii = 0;
		// portrait_ = true;
	}
/*
	if (window_) {
		window.invalidate_layout(nullptr);
		return;
	}
*/
	panel_ = find_widget<tstack>(&window, widget_id_, false, true);
	contact_layer_ = find_widget<tgrid>(&window, ctrlid_contact_layer, false, true);
	find_layer_ = find_widget<tgrid>(&window, ctrlid_find_layer, false, true);
	channel_layer_ = find_widget<tgrid>(&window, ctrlid_channel_layer, false, true);
	vrenderer_layer_ = find_widget<tgrid>(&window, ctrlid_channel_vrenderer, false, true);
	if (portrait_) {
		msg_layer_ = find_widget<tgrid>(&window, ctrlid_msg_layer, false, true);
	}

	notify_list_ = find_widget<tlistbox>(&window, ctrlid_notifies, false, true);
	person_tree_ = find_widget<ttree>(&window, ctrlid_persons, false, true);
	channel2_tree_ = find_widget<ttree>(&window, ctrlid_channel2, false, true);
	previous_page_ = find_widget<tbutton>(&window, ctrlid_previous, false, true);
	next_page_ = find_widget<tbutton>(&window, ctrlid_next, false, true);
	pagenum_ = find_widget<tlabel>(&window, ctrlid_pagenum, false, true);
	history_ = &find_widget<tlistbox>(window_, ctrlid_history, false);
	history_->set_scroll_to_end(true);
	input_ = &find_widget<tscroll_text_box>(window_, ctrlid_input, false);

	input_scale_ = &find_widget<tspacer>(window_, ctrlid_input_scale, false);
	send_ = &find_widget<tbutton>(window_, ctrlid_send, false);
	switch_to_chat_find_ = &find_widget<tbutton>(window_, ctrlid_switch_to_chat_find, false);
	switch_to_chat_channel_ = &find_widget<tbutton>(window_, ctrlid_switch_to_chat_channel, false);
	find_ = &find_widget<tbutton>(window_, ctrlid_find, false);
	join_channel_ = &find_widget<tbutton>(&window, ctrlid_join, false);
	chanlist_ = &find_widget<tlistbox>(&window, ctrlid_chanlist, false);
	chat_to_ = &find_widget<tbutton>(&window, ctrlid_chat_to, false);
	join_friend_ = &find_widget<tbutton>(&window, ctrlid_join_friend, false);

	person_tree_->set_did_node_changed(boost::bind(&tchat_::contact_channel_toggled, this, _1, _2));

	notify_list_->enable_select(false);
	notify_list_->set_did_row_changed(boost::bind(&tchat_::notify_did_click, this, _1, _2));
#ifdef _WIN32
	notify_list_->set_did_row_double_click(boost::bind(&tchat_::notify_did_double_click, this, _1, _2));
#endif
	chanlist_->set_did_row_changed(boost::bind(&tchat_::find_chan_toggled, this, boost::ref(window), _1));

	std::stringstream ss;
	
	tscroll_text_box& input = find_widget<tscroll_text_box>(window_, ctrlid_input, false);
	ttext_box& tb = find_widget<ttext_box>(input.content_grid(), "_text_box", false);
	input_tb_ = &tb;
	connect_signal_pre_key_press(tb, boost::bind(&tchat_::signal_handler_sdl_key_down, this, _3, _4, _5, _6, _7));
	window.keyboard_capture(&tb);

	tb.connect_signal<event::SDL_TEXT_INPUT>(
		boost::bind(
			&tchat_::enter_inputing
			, this
			, (int)true)
			, event::tdispatcher::front_child);

	tb.connect_signal<event::LEFT_BUTTON_DOWN>(
		boost::bind(
			&tchat_::enter_inputing
			, this
			, (int)true)
			, event::tdispatcher::front_child);

	history_->connect_signal<event::WHEEL_DOWN>(
		boost::bind(
			&tchat_::enter_inputing
			, this
			, (int)false)
			, event::tdispatcher::front_post_child);

	history_->connect_signal<event::WHEEL_UP>(
		boost::bind(
			&tchat_::enter_inputing
			, this
			, (int)false)
			, event::tdispatcher::front_post_child);

	connect_signal_mouse_left_click(
			  find_widget<tbutton>(&window, ctrlid_switch_to_find, false)
			, boost::bind(
				  &tchat_::switch_to_find
				, this
				, boost::ref(window)));

	if (portrait_) {
		connect_signal_mouse_left_click(
			  find_widget<tbutton>(msg_layer_, ctrlid_return, false)
			, boost::bind(
				  &tchat_::switch_to_home
				, this
				, boost::ref(window)));
	}
	connect_signal_mouse_left_click(
		find_widget<tbutton>(vrenderer_layer_, ctrlid_return, false)
		, boost::bind(
			&tchat_::leave_from_video
			, this
			, boost::ref(window)));

	connect_signal_mouse_left_click(
			  *previous_page_
			, boost::bind(
				  &tchat_::previous_page
				, this
				, boost::ref(window)));
	previous_page_->set_visible(twidget::HIDDEN);

	connect_signal_mouse_left_click(
			  *next_page_
			, boost::bind(
				  &tchat_::next_page
				, this
				, boost::ref(window)));
	next_page_->set_visible(twidget::HIDDEN);
	
	// find grid
	connect_signal_mouse_left_click(
			  *switch_to_chat_find_
			, boost::bind(
				  &tchat_::switch_to_home
				, this
				, boost::ref(window)));

	connect_signal_mouse_left_click(
			  *find_
			, boost::bind(
				  &tchat_::find
				, this
				, boost::ref(window)));

	connect_signal_mouse_left_click(
			  *join_channel_
			, boost::bind(
				  &tchat_::join_channel
				, this
				, boost::ref(window)));
	join_channel_->set_active(false);

	// channel grid
	connect_signal_mouse_left_click(
			  *switch_to_chat_channel_
			, boost::bind(
				  &tchat_::switch_to_home
				, this
				, boost::ref(window)));

	connect_signal_mouse_left_click(
			  *chat_to_
			, boost::bind(
				  &tchat_::chat_to
				, this
				, boost::ref(window)));

	connect_signal_mouse_left_click(
			  *join_friend_
			, boost::bind(
				  &tchat_::join_friend
				, this
				, boost::ref(window)));

	connect_signal_mouse_left_click(
			  *send_
			, boost::bind(
				  &tchat_::send
				, this
				, _3, _4));
	send_->set_active(false);

	{
		connect_signal_mouse_left_click(
			find_widget<tbutton>(&window, "_chat_caller", false)
			, boost::bind(
				&tchat_::chat_setup
				, this
				, boost::ref(window)
				, (int)true));

		connect_signal_mouse_left_click(
			find_widget<tbutton>(&window, "_chat_callee", false)
			, boost::bind(
				&tchat_::chat_setup
				, this
				, boost::ref(window)
				, (int)false));
	}

	person_tree_->set_no_indentation(true);

	for (std::map<int, tlobby_channel>::const_iterator it = lobby->chat->channels.begin(); it != lobby->chat->channels.end(); ++ it) {
		bool person = !tlobby_channel::is_allocatable(it->first);
		std::vector<tcookie>& branch = ready_branch(person, it->second);

		tlobby_channel& channel = lobby->chat->get_channel(it->first);
		for (std::vector<tlobby_user*>::const_iterator it = channel.users.begin(); it != channel.users.end(); ++ it) {
			const tlobby_user& user = **it;
			if (person && user.me) {
				continue;
			}
			insert_user(person, branch, user);
		}
	}

	input_tb_->set_src_pos(src_pos_);

	toolbar_ = find_widget<treport>(&window, ctrlid_tool_bar, false, true);
	toolbar_->set_did_item_click(boost::bind(&tchat_::did_item_click_report, this, _1, _2));
	reload_toolbar(window);

	catalog_ = find_widget<treport>(&window, ctrlid_contact_bar, false, true);
	catalog_->set_did_item_changed(boost::bind(&tchat_::did_item_changed_report, this, _1, _2));
	catalog_->set_boddy(find_widget<twidget>(&window, ctrlid_contact_panel, false, true));
	reload_catalog(window);
	refresh_toolbar(ft_none, tlobby_user::npos);

	switch_to_home(window);
	contact_switch_to_notify(window);

	// It is time to join tlobby::thandler queue.
	tlobby::thandler::join();

	window.invalidate_layout(nullptr);
}

void tchat_::visible_float_widgets(bool visible)
{
	window_->find_float_widget(ctrlid_pagenum)->set_visible(visible);

	if (visible) {
		window_->find_float_widget(ctrlid_previous)->set_visible(false);
		window_->find_float_widget(ctrlid_next)->set_visible(false);
	}
}

void tchat_::switch_to_home(twindow& window)
{
	if (!channel2_branch_.empty()) {
		channel2_tree_->clear();
		channel2_branch_.clear();
	}

	panel_->set_radio_layer(CONTACT_HOME);
	visible_float_widgets(true);
}

void tchat_::contact_switch_to_notify(twindow& window)
{
	find_widget<tgrid>(window_, ctrlid_notifies_layer, false).set_visible(twidget::VISIBLE);
	find_widget<tgrid>(window_, ctrlid_persons_layer, false).set_visible(twidget::INVISIBLE);

	notify_list_->clear();
	if (true) {
		utils::string_map list_item;
		std::map<std::string, std::string> list_item_item;

		list_item_item.insert(std::make_pair("portrait", "misc/chat.png"));
		list_item_item.insert(std::make_pair("nick", _("Test video chat")));
		list_item_item.insert(std::make_pair("msg", "Click it"));

		notify_list_->insert_row(list_item_item);
	}
	input_tb_->set_placeholder(_("If you want caller using it, input nick of peer at it"));
}

void tchat_::contact_switch_to_person(twindow& window)
{
	find_widget<tgrid>(window_, ctrlid_notifies_layer, false).set_visible(twidget::INVISIBLE);
	find_widget<tgrid>(window_, ctrlid_persons_layer, false).set_visible(twidget::VISIBLE);

	if (person_tree_->cursel()) {
		contact_channel_toggled(*person_tree_, *person_tree_->cursel());
	}
}

void tchat_::switch_to_find(twindow& window)
{
	panel_->set_radio_layer(FIND_LAYER);
	visible_float_widgets(false);

	find_chan_toggled(window, *chanlist_);
	find_->set_active(lobby->chat->ready());
}

void tchat_::app_OnMessage(rtc::Message* msg)
{
	chat_OnMessage(msg);
}

void tchat_::did_paper_draw_slice(bool remote, const cv::Mat& frame, const texture& frame_tex, const SDL_Rect& draw_rect, bool new_frame)
{
	SDL_Renderer* renderer = get_renderer();

	surface text_surf;
	if (remote) {
		SDL_RenderCopy(renderer, frame_tex.get(), NULL, &draw_rect);

		text_surf = font::get_rendered_text(_("Remote video"), 0, 48, font::BAD_COLOR);
		SDL_Rect dst = ::create_rect(draw_rect.x, draw_rect.y, text_surf->w, text_surf->h);
		render_surface(renderer, text_surf, NULL, &dst);

		surface surf = image::get_image("misc/chat.png");
		dst = ::create_rect(draw_rect.x, draw_rect.y + draw_rect.h - surf->h, surf->w, surf->h);
		render_surface(renderer, surf, NULL, &dst);

	} else {
		if (local_render_size_.x == nposm) {
			local_render_size_.x = frame.cols;
			local_render_size_.y = frame.rows;
		}

		if (local_render_size_.x * 2 > draw_rect.w) {
			local_render_size_.x /= 2;
			local_render_size_.y /= 2;
		}
		SDL_Rect dst;
		dst.w = local_render_size_.x;
		dst.h = local_render_size_.y;
		dst.x = draw_rect.x + original_local_offset_.x + current_local_offset_.x;
		dst.y = draw_rect.y + original_local_offset_.y + current_local_offset_.y;

		SDL_RenderCopy(renderer, frame_tex.get(), NULL, &dst);

		text_surf = font::get_rendered_text(_("Local video"), 0, 36, font::GOOD_COLOR);
		dst.w = text_surf->w;
		dst.h = text_surf->h;
		render_surface(renderer, text_surf, NULL, &dst);
	}
}

void tchat_::did_draw_vrenderer(ttrack& widget, const SDL_Rect& widget_rect, const bool bg_drawn, bool force)
{
	VALIDATE(widget_rect.w && widget_rect.h, null_str);
	if (deconstructed_) {
		return;
	}

	SDL_Renderer* renderer = get_renderer();
	ttrack::tdraw_lock lock(renderer, widget);

	VideoRenderer* local_sink = local_renderer_.get();
	VideoRenderer* remote_sink = remote_renderer_.get();

	bool require_render_local = bg_drawn || force;
	if (!require_render_local) {
		require_render_local = local_sink && local_sink->new_frame();
	}

	bool require_render_remote = bg_drawn || force;
	if (!require_render_remote) {
		require_render_remote = remote_sink != NULL && (remote_sink->new_frame() || require_render_local);
	}

	if (require_render_remote && !bg_drawn) {
		SDL_RenderCopy(renderer, widget.background_texture().get(), NULL, &widget_rect);
	}

	if (require_render_remote) {
		draw_slice(renderer, true, widget_rect, true);
		require_render_local = true; 
	}

	if (require_render_local) {
		draw_slice(renderer, false, widget_rect, true);
	}
}

void tchat_::did_control_drag_detect(ttrack& widget, const tpoint&, const tpoint& /*last_coordinate*/)
{
	original_local_offset_ += current_local_offset_;
	current_local_offset_.x = current_local_offset_.y = 0;

	if (local_render_size_.x == nposm) {
		return;
	}

	int width = widget.get_width();
	int height = widget.get_height();
	if (original_local_offset_.x < 0) {
		original_local_offset_.x = 0;
	} else if (original_local_offset_.x > width - local_render_size_.x) {
		original_local_offset_.x = width - local_render_size_.x;
	}
	if (original_local_offset_.y < 0) {
		original_local_offset_.y = 0;
	} else if (original_local_offset_.y > height - local_render_size_.y) {
		original_local_offset_.y = height - local_render_size_.y;
	}
}

void tchat_::did_drag_coordinate(ttrack& widget, const tpoint& first, const tpoint& last)
{
	if (is_null_coordinate(first)) {
		return;
	}
	current_local_offset_ = last - first;
	did_draw_vrenderer(*vrenderer_track_, vrenderer_track_->get_rect(), false, true);
}

void tchat_::switch_to_channel(twindow& window)
{
	panel_->set_radio_layer(CHANNEL_LAYER);
	visible_float_widgets(false);
}

void tchat_::switch_to_msg(twindow& window)
{
	panel_->set_radio_layer(MSG_LAYER);
	visible_float_widgets(false);
}

void tchat_::switch_to_video(twindow& window)
{
	panel_->set_radio_layer(VRENDERER_LAYER);
	visible_float_widgets(false);

	if (!vrenderer_track_) {
		set_did_draw_slice(boost::bind(&tchat_::did_paper_draw_slice, this, _1, _2, _3, _4, _5));

		ttrack* track = find_widget<ttrack>(&window, "vrenderer", false, true);
		track->set_did_draw(boost::bind(&tchat_::did_draw_vrenderer, this, _1, _2, _3, false));
		track->set_did_mouse_leave(boost::bind(&tchat_::did_control_drag_detect, this, _1, _2, _3));
		track->set_did_mouse_motion(boost::bind(&tchat_::did_drag_coordinate, this, _1, _2, _3));

		vrenderer_track_ = track;
	}
	vrenderer_track_->set_timer_interval(30);
}

void tchat_::leave_from_video(twindow& window)
{
	// close session
	if (state_ == CONNECTED) {
		if (peer_connection_.get()) {
			DeletePeerConnection();
			if (!local_only_) {
				// {"id":"stop"}
				Json::StyledWriter writer;
				Json::Value jmessage;

				jmessage["id"] = "stop";
				msg_2_signaling_server(control_socket_.get(), writer.write(jmessage));
			}
		}
		Close();
	}
	if (vrenderer_track_) {
		vrenderer_track_->set_timer_interval(0);
	}
	if (portrait_) {
		switch_to_msg(window);
	} else {
		switch_to_home(window);
	}
}

tchat_::tsession& tchat_::get_session(chat_logs::treceiver& receiver, bool allow_create)
{
	for (std::vector<tsession>::iterator it = sessions_.begin(); it != sessions_.end(); ++ it) {
		tsession& session = *it;
		if (session.receiver->id == receiver.id && session.receiver->channel == receiver.channel) {
			return session;
		}
	}

	VALIDATE(allow_create, "Must exist receiver!");
	sessions_.push_back(tsession(receiver));
	return sessions_.back();
}

void tchat_::switch_session(bool person, std::vector<tcookie>& branch, tcookie& cookie)
{
	// current support one session!
	sessions_.clear();
	current_session_ = NULL;

	user_to_title(cookie);

	if (cookie.unread) {
		cookie.unread = 0;
		update_node_internal(branch, cookie);
	}

	int id = cookie.id;
	bool channel = cookie.channel;

	chat_logs::treceiver& receiver = chat_logs::find_receiver(id, channel, true);
	tsession& session = get_session(receiver, true);
	current_session_ = &session;

	bool can_send = lobby->chat->ready();
	if (can_send) {
		if (!receiver.channel) {
			tlobby_user& user = lobby->chat->get_user(id);
			can_send = user.online;
		} else if (tlobby_channel::is_allocatable(id)) {
			tlobby_channel& channel = lobby->chat->get_channel(id);
			can_send = channel.joined();
		} else {
			can_send = false;
		}
	}
	send_->set_active(can_send);

	chat_2_scroll_label(*history_, *current_session_);
	if (person) {
		refresh_toolbar(ft_person, current_session_->receiver->id);
	} else {
		if (current_session_->receiver->channel) {
			refresh_toolbar(ft_channel, current_session_->receiver->id);
		} else {
			refresh_toolbar(ft_chan_person, current_session_->receiver->id);
		}
	}

	// focus back to input.
	window_->keyboard_capture(input_tb_);
}

void tchat_::did_item_changed_report(treport& report, ttoggle_button& widget)
{
	int catalog_tab = (int)widget.cookie();
	if (catalog_tab == 0) {
		contact_switch_to_notify(*window_);

	} else if (catalog_tab == 1) {
		contact_switch_to_person(*window_);
	}
}

void tchat_::did_item_click_report(treport& report, tbutton& widget)
{
	irc::server* serv = lobby->chat->serv();
	utils::string_map symbols;
	std::stringstream ss;

	bool require_reload = false;
	int id = (int)widget.cookie();
	if (id == f_netdiag) {
		netdiag(*window_);

	} else if (id == f_copy) {
		twidget* panel = history_->cursel();
		int index = (int)panel->cookie();
		const chat_logs::tlog& log = current_session_->log(index);
		std::string msg = log.msg;

		SDL_SetClipboardText(msg.c_str());

		gui2::show_transient_message(_("Content is copied to clipboard"), ss.str());

	} else if (id == f_reply) {
		twidget* panel = history_->cursel();
		int index = (int)panel->cookie();
		const chat_logs::tlog& log = current_session_->log(index);

		ss.str("");
		ss << log.nick << "";
		input_->set_label(ss.str());
		input_tb_->goto_end_of_line();

		input_->insert_img("misc/mini-reply.png");
		input_tb_->insert_str(" ");

		window_->keyboard_capture(input_tb_);
		input_tb_->goto_end_of_line();

	} else if (id == f_part) {
		tlobby_channel& channel = lobby->chat->get_channel(current_session_->receiver->id);

		symbols["part"] = ht::generate_format(_("Part"), color_to_uint32(font::YELLOW_COLOR));
		symbols["channel"] = ht::generate_format(channel.nick, color_to_uint32(font::BAD_COLOR));
		ss << vgettext2("After $part, you can not receive message from $channel. Do you want to $part?", symbols);
		int res = gui2::show_message("", ss.str(), gui2::tmessage::yes_no_buttons);
		if (res != gui2::twindow::OK) {
			return;
		}
		lobby->chat->erase_channel(current_session_->receiver->id);
		remove_branch(false, current_session_->receiver->id);

		// cannot part app-id channel.
		simulate_cookie_toggled(false, tlobby_channel::get_cid(game_config::app_channel), tlobby_channel::get_cid(game_config::app_channel), true);

	} else if (id == f_part_friend) {
		tlobby_channel& channel = lobby->chat->get_channel(tlobby_channel::t_friend);
		tlobby_user& user = channel.get_user(current_session_->receiver->id);

		symbols["nick"] = ht::generate_format(user.nick, color_to_uint32(font::YELLOW_COLOR));
		symbols["channel"] = ht::generate_format(channel.name(), color_to_uint32(font::GOOD_COLOR));
		ss << vgettext2("Do you want remove $nick from $channel?", symbols);
		int res = gui2::show_message("", ss.str(), gui2::tmessage::yes_no_buttons);
		if (res != gui2::twindow::OK) {
			return;
		}

		channel.erase_user(user.uid);

		serv->p_monitor(serv, current_session_->receiver->nick.c_str(), false);

		std::vector<tcookie>& branch = person_cookies_.find(tlobby_channel::t_friend)->second;
		size_t pos = -1;
		for (std::vector<tcookie>::iterator it = branch.begin(); it != branch.end(); ++ it) {
			const tcookie& cookie = *it;
			if (!cookie.channel && cookie.id == current_session_->receiver->id) {
				pos = std::distance(branch.begin(), it);
				break;
			}
		}
		VALIDATE(pos < branch.size(), "must exist user!");
		erase_user(true, branch, current_session_->receiver->id);
		if (pos >= branch.size()) {
			pos --;
		}
		if (pos) {
			simulate_cookie_toggled(true, tlobby_channel::t_friend, branch[pos].id, branch[pos].channel);

		} else {
			simulate_cookie_toggled(false, tlobby_channel::get_cid(game_config::app_channel), tlobby_channel::get_cid(game_config::app_channel), true);
			catalog_->select_item(at_channel);
		}

	} else if (id == f_explore) {
		tlobby_channel& channel = lobby->chat->get_channel(current_session_->receiver->id);

		switch_to_channel(*window_);
		generate_channel_tree(channel);

	}

	if (require_reload) {
		refresh_toolbar(current_ft_, current_session_->receiver->id);
	}
}

void tchat_::previous_page(twindow& window)
{
	current_session_->current_page ++;
	chat_2_scroll_label(*history_, *current_session_);
}

void tchat_::next_page(twindow& window)
{
	current_session_->current_page --;
	chat_2_scroll_label(*history_, *current_session_);
}

void tchat_::post_show()
{
/*
	{
		threading::lock lock(remote_sink_mutex_);
		StopRemoteRenderer();
	}
	{
		threading::lock lock(local_sink_mutex_);
		StopLocalRenderer();
	}
*/
	deconstructed_ = true;
	SDL_SetHint(SDL_HINT_ORIENTATIONS, "\0");

	if (swap_resultion_) {
		twidget::current_landscape = twidget::landscape_from_orientation(window_->get_orientation(), true);
		VALIDATE(twidget::current_landscape, "window must be landscape!");

		swap_resultion_ = false;
		twindow::set_orientation_resolution();
	}
}

bool tchat_::gui_ready() const
{
	return signature_ != 0;
}

void tchat_::reload_catalog(twindow& window)
{
	catalog_->clear();

	std::vector<std::string> catalogs;
	catalogs.push_back("misc/chat.png");
	catalogs.push_back("misc/contacts.png");

	std::stringstream ss;
	int index = 0;
	for (std::vector<std::string>::const_iterator it = catalogs.begin(); it != catalogs.end(); ++ it) {
		tcontrol& widget = catalog_->insert_item(null_str, null_str);
		widget.set_icon(*it);
		widget.set_cookie(index ++);
	}
	catalog_->select_item(0);
}

void tchat_::process_message(const std::string& chan, const std::string& from, const std::string& text)
{
	tlobby_user& user = lobby->chat->get_user(tlobby_user::get_uid(from));
	std::pair<std::vector<tcookie>*, tcookie* > pair;
	int receive_id;
	if (chan.empty()) {
		if (lobby->chat->is_favor_user(user.uid)) {
			pair = contact_find(true, tlobby_channel::npos, user.uid, false);
		} else {
			pair = contact_find(false, tlobby_channel::npos, user.uid, false);
			// here, if a tempraral chat, current not support. cookie.first is NULL.
		}
		receive_id = user.uid;

	} else {
		int cid = tlobby_channel::get_cid(chan);
		pair = contact_find(false, cid, cid, true);
		receive_id = cid;
	}

	if (!pair.first) {
		return;
	}

	chat_logs::add(receive_id, !chan.empty(), user, utils::is_utf8str(text)? text: err_encode_str);
	if (current_session_ && pair.second->channel == current_session_->receiver->channel && pair.second->id == current_session_->receiver->id) {
		int cursel = history_->cursel()->at();
		chat_2_scroll_label(*history_, *current_session_, cursel > 0? cursel - 1: cursel);

	} else {
		pair.second->unread ++;
		update_node_internal(*pair.first, *pair.second);
	}
}

void tchat_::process_userlist(const std::string& chan, const std::string& names)
{
	std::vector<std::string> vstr = utils::split(names, ' ');
	tlobby_channel& channel = lobby->chat->get_channel(tlobby_channel::get_cid(chan));

	if (!channel.users_receiving) {
		clear_branch(false, channel.cid);
	}

	std::vector<tcookie>& branch = channel_cookies_.find(channel.cid)->second;
	char* nick_prefixes = lobby->chat->serv()->nick_prefixes;
	std::string nopre_nick;
	for (std::vector<std::string>::const_iterator it = vstr.begin(); it != vstr.end(); ++ it) {
		const std::string& nick = *it;
		nopre_nick = *it;
		// Ignore prefixes so '!' won't cause issues
		if (strchr(nick_prefixes, nopre_nick[0])) {
			nopre_nick = nopre_nick.substr(1);
		}

		int uid = tlobby_user::get_uid(nopre_nick);
		const tlobby_user& user = channel.get_user(uid);
		insert_user(false, branch, user);
	}
}

void tchat_::process_userlist_end(const std::string& chan)
{
	tlobby_channel& channel = lobby->chat->get_channel(tlobby_channel::get_cid(chan));

	// std::vector<tcookie>& branch = channel_cookies_.find(channel.cid)->second;
	// branch.front().node->fold();

	if (current_ft_ == ft_channel && current_session_->receiver->id == channel.cid) {
		refresh_toolbar(current_ft_, current_session_->receiver->id);
	}
}

void tchat_::process_join(const std::string& chan, const std::string& nick)
{
	tlobby_channel& channel = lobby->chat->get_channel(tlobby_channel::get_cid(chan));
	tlobby_user& user = channel.get_user(tlobby_user::get_uid(nick));

	std::vector<tcookie>& branch = channel_cookies_.find(channel.cid)->second;
	insert_user(false, branch, user);

	if (lobby->chat->is_favor_user(user.uid)) {
		std::pair<std::vector<tchat_::tcookie>*, tchat_::tcookie* > ret = tchat_::contact_find(true, tlobby_channel::t_friend, user.uid, false);
		tcookie* cookie = ret.second;
		if (!cookie->online) {
			cookie->online = user.online;
			cookie->away = user.away;
			update_node_internal(*ret.first, *ret.second);
		}
	}
}

void tchat_::process_uparted(const std::string& chan)
{
}

void tchat_::process_part(const std::string& chan, const std::string& nick, const std::string& reason)
{
	tlobby_channel& channel = lobby->chat->get_channel(tlobby_channel::get_cid(chan));
	std::vector<tcookie>& branch = channel_cookies_.find(channel.cid)->second;
	int uid = tlobby_user::get_uid(nick, false);
	erase_user(false, branch, uid);
}

void tchat_::process_online(const char* nicks)
{
	std::vector<std::string> vstr;
	tlobby::tchat_sock::split_offline_online(nicks, vstr);

	std::pair<std::vector<tchat_::tcookie>*, tchat_::tcookie* > ret;
	for (std::vector<std::string>::const_iterator it = vstr.begin(); it != vstr.end(); ++ it) {
		const std::string& nick = *it;
		int uid = tlobby_user::get_uid(nick);
		tlobby_user& user = lobby->chat->get_user(uid);
		if (!user.valid()) {
			continue;
		}

		if (lobby->chat->is_favor_user(uid)) {
			ret = tchat_::contact_find(true, tlobby_channel::t_friend, uid, false);
			if (ret.first) {
				tcookie* cookie = ret.second;
				if (!cookie->online) {
					cookie->online = true;
					cookie->away = false;
					update_node_internal(*ret.first, *ret.second);
				}
			}
		}

	}
}

void tchat_::process_offline(const char* nicks)
{
	std::vector<std::string> vstr;
	tlobby::tchat_sock::split_offline_online(nicks, vstr);

	std::pair<std::vector<tchat_::tcookie>*, tchat_::tcookie* > ret;
	for (std::vector<std::string>::const_iterator it = vstr.begin(); it != vstr.end(); ++ it) {
		const std::string& nick = *it;
		int uid = tlobby_user::get_uid(nick);
		tlobby_user& user = lobby->chat->get_user(uid);
		if (!user.valid()) {
			continue;
		}

		if (lobby->chat->is_favor_user(uid)) {
			ret = tchat_::contact_find(true, tlobby_channel::t_friend, uid, false);
			if (ret.first) {
				tcookie* cookie = ret.second;
				if (cookie->online) {
					cookie->online = false;
					update_node_internal(*ret.first, *ret.second);
				}
			}
		}
	}
}

void tchat_::process_forbid_join(const std::string& chan, const std::string& reason)
{
	int cid = tlobby_channel::get_cid(chan);
	tlobby_channel& channel = lobby->chat->get_channel(cid);

	std::vector<tcookie>& branch = channel_cookies_.find(cid)->second;
	update_node_internal(branch, branch.front());
}

void tchat_::process_whois(const std::string& chan, const std::string& nick, bool online, bool away)
{
	int uid = tlobby_user::get_uid(nick);
	tlobby_user& user = lobby->chat->get_user(uid);

	std::pair<std::vector<tchat_::tcookie>*, tchat_::tcookie* > ret;
	if (irc::is_channel(lobby->chat->serv(), chan.c_str())) {
		ret = tchat_::contact_find(false, tlobby_channel::get_cid(chan), user.uid, false);
		if (ret.first) {
			tcookie* cookie = ret.second;
			if (cookie->online != online || (online && cookie->away != away)) {
				cookie->online = online;
				cookie->away = away;
				update_node_internal(*ret.first, *ret.second);
			}
		}
	}

	if (lobby->chat->is_favor_user(uid)) {
		ret = tchat_::contact_find(true, tlobby_channel::t_friend, user.uid, false);
		if (ret.first) {
			tcookie* cookie = ret.second;
			if (cookie->online != online || (online && cookie->away != away)) {
				cookie->online = online;
				cookie->away = away;
				update_node_internal(*ret.first, *ret.second);
			}
		}
	}

	if (ret.first && current_session_ && ret.second->id == current_session_->receiver->id && ret.second->channel == current_session_->receiver->channel) {
		send_->set_active(true);
	}
}

void tchat_::process_quit(const std::string& nick)
{
	tlobby_user& user = lobby->chat->get_user(tlobby_user::get_uid(nick));

	for (std::set<int>::const_iterator it = user.cids.begin(); it != user.cids.end(); ++ it) {
		int cid = *it;
		if (tlobby_channel::is_allocatable(cid)) {
			std::vector<tcookie>& branch = channel_cookies_.find(cid)->second;
			erase_user(false, branch, user.uid);
		} else {
			std::pair<std::vector<tchat_::tcookie>*, tchat_::tcookie* > ret = tchat_::contact_find(true, tlobby_channel::t_friend, user.uid, false);
			if (ret.first) {
				tcookie* cookie = ret.second;
				if (cookie->online) {
					cookie->online = false;
					update_node_internal(*ret.first, *ret.second);
				}
			}
		}
	}
}

void tchat_::notify_did_click(tlistbox& list, twidget& widget)
{
#ifdef _WIN32
#else
	if (portrait_) {
		switch_to_msg(*window_);
	}
#endif
}

void tchat_::notify_did_double_click(tlistbox& list, twidget& widget)
{
	if (portrait_) {
		switch_to_msg(*window_);
	}
}

void tchat_::find_chan_toggled(twindow& window, tlistbox& list)
{
	if (!chanlist_->cursel()) {
		return;
	}
	bool active = !list_chans_.empty() && chanlist_->rows();
	if (active) {
		const tlobby_channel& channel = lobby->chat->get_channel(tlobby_channel::get_cid(list_chans_[chanlist_->cursel()->at()], false));
		active = !channel.valid();
	}
	join_channel_->set_active(active);
}

void tchat_::process_chanlist_start()
{
	in_find_chan_ = true;
	list_chans_.clear();
	chanlist_->clear();
	tlabel& label = find_widget<tlabel>(window_, "_chat_find_result", false);
	label.set_label("0/0");

	switch_to_chat_find_->set_active(false);
	find_->set_label(_("Cancel"));
}

void tchat_::process_chanlist(const std::string& chan, int users, const std::string& topic)
{
	if (!in_find_chan_) {
		return;
	}
	if (users < cond_min_users_) {
		return;
	}

	if (std::find(list_chans_.begin(), list_chans_.end(), chan) == list_chans_.end()) {
		list_chans_.push_back(chan);
	} else {
		return;
	}
	if (list_chans_.size() <= 100) {
		std::map<std::string, std::string> list_item_item;

		list_item_item.insert(std::make_pair("number", str_cast(list_chans_.size())));
		list_item_item.insert(std::make_pair("nick", chan));
		list_item_item.insert(std::make_pair("users", str_cast(users)));
		list_item_item.insert(std::make_pair("topic", utils::is_utf8str(topic)? topic: err_encode_str));

		chanlist_->insert_row(list_item_item);
	}

	std::stringstream ss;
	ss << chanlist_->rows() << "/" << list_chans_.size();
	tlabel& label = find_widget<tlabel>(window_, "_chat_find_result", false);
	label.set_label(ss.str());
}

void tchat_::process_chanlist_end()
{
	if (!in_find_chan_) {
		return;
	}

	in_find_chan_ = false;

	switch_to_chat_find_->set_active(true);
	find_->set_label(_("Find"));
	find_->set_active(lobby->chat->ready());
	find_chan_toggled(*window_, *chanlist_);
}

void tchat_::process_network_status(bool connected)
{
	if (connected) {
	}
}

void tchat_::format_log_2_listbox(tlistbox& list, const tsession& sess, int cursel) const
{
	std::vector<chat_logs::tlog> logs;
	int start = sess.current_logs(logs);

	list.clear();

	std::map<std::string, std::string> list_item_item;

	std::stringstream ss;
	const std::string& my_nick = lobby->chat->me? lobby->chat->me->nick: lobby->nick();
	for (std::vector<chat_logs::tlog>::const_iterator it = logs.begin(); it != logs.end(); ++ it) {
		const chat_logs::tlog& log = *it;
		
		list_item_item.clear();
		ss.str("");

		bool me = log.nick == my_nick;
		if (me) {
			ss << format_time_date(log.t) << "    ";
			ss << ht::generate_format(log.nick, color_to_uint32(font::GOOD_COLOR));
		} else {
			ss << ht::generate_format(log.nick, color_to_uint32(font::BLUE_COLOR));
			ss << "    " << format_time_date(log.t);
		}

		list_item_item.insert(std::make_pair("lportrait", "misc/rose-36.png"));

		list_item_item.insert(std::make_pair("rportrait", "misc/rose-36.png"));

		list_item_item.insert(std::make_pair("sender", ss.str()));

		list_item_item.insert(std::make_pair("msg", log.msg));

		ttoggle_panel& panel = list.insert_row(list_item_item);

		panel.set_cookie(start ++);

		twidget& portrait = find_widget<twidget>(&panel, me? "lportrait": "rportrait", false);
		portrait.set_visible(twidget::HIDDEN);

		tgrid& grid_msg = find_widget<tgrid>(&panel, "_grid_msg", false);
		tgrid::tchild& child = grid_msg.child(0, 0);
		child.flags_ &= ~tgrid::HORIZONTAL_MASK;
		child.flags_ |= me? tgrid::HORIZONTAL_ALIGN_RIGHT: tgrid::HORIZONTAL_ALIGN_LEFT;

		tlabel& msg = find_widget<tlabel>(&panel, "msg", false);
		msg.set_canvas_variable("border", variant(me? "border6": "border5"));
		panel.set_canvas_highlight(false, true);
	}

	if (list.rows()) {
		if (cursel == nposm || cursel >= list.rows()) {
			cursel = list.rows() - 1;
		}
		list.select_row(cursel);
	}
}

void tchat_::chat_2_scroll_label(tlistbox& list, const tsession& sess, int cursel)
{
	format_log_2_listbox(list, sess, cursel);

	std::stringstream ss;
	if (current_session_) {
		ss << std::setfill('0') << std::setw(2) << (current_session_->pages() - current_session_->current_page);
		ss << "/";
		ss << std::setfill('0') << std::setw(2) << current_session_->pages();
	} else {
		ss << "--/--";
	}
	pagenum_->set_label(ss.str());

	window_->find_float_widget(ctrlid_previous)->set_visible(current_session_ && current_session_->can_previous());
	window_->find_float_widget(ctrlid_next)->set_visible(current_session_ && current_session_->can_next());
}

void tchat_::handle_status(int at, tsock::ttype type)
{
	if (at != tlobby::tag_chat) {
		return;
	}
	if (!gui_ready()) {
		return;
	}
	if (type == tsock::t_connected) {
		find_->set_active(true);
		return;

	} else if (type == tsock::t_disconnected) {
		for (std::map<int, tlobby_channel>::iterator it = lobby->chat->channels.begin(); it != lobby->chat->channels.end(); ++ it) {
			bool person = !tlobby_channel::is_allocatable(it->first);
			tlobby_channel& channel = it->second;
			if (person) {
				std::vector<tcookie>& branch = person_cookies_.find(it->first)->second;
				std::vector<tcookie>::iterator it2 = branch.begin();
				for (++ it2; it2 != branch.end(); ++ it2) {
					tcookie& cookie = *it2;
					if (cookie.online) {
						cookie.online = false;
						update_node_internal(branch, cookie);
					}
				}

			} else {
				clear_branch(person, it->first);
				std::vector<tcookie>& branch = channel_cookies_.find(it->first)->second;
				update_node_internal(branch, branch.front());
			}
		}
		
		if (dynamic_cast<ttoggle_button*>(&catalog_->item(at_channel))->get_value()) {
			simulate_cookie_toggled(false, tlobby_channel::get_cid(game_config::app_channel), tlobby_channel::get_cid(game_config::app_channel), true);
		} else {
			send_->set_active(false);
			find_->set_active(false);

			if (current_ft_ != ft_none) {
				refresh_toolbar(current_ft_, current_session_->receiver->id);
			}
		}
	}
}

bool tchat_::handle_raw(int at, tsock::ttype type, const char* param[])
{
	if (at != tlobby::tag_chat) {
		return false;
	}
	if (!gui_ready()) {
		return false;
	}

	bool halt = true;
	if (!strcasecmp(param[0], "NAMREPLAY")) {
		process_userlist(param[1], param[2]);

	} else if (!strcasecmp(param[0], "ENDOFNAMES")) {
		process_userlist_end(param[1]);

	} else if (!strcasecmp(param[0], "UJOINED")) {
		
	} else if (!strcasecmp(param[0], "UPARTED")) {
		process_uparted(param[1]);

	} else if (!strcasecmp(param[0], "PART")) {
		process_part(param[1], param[2], param[3]);

	} else if (!strcasecmp(param[0], "JOIN")) {
		process_join(param[1], param[2]);

	} else if (!strcasecmp(param[0], "WHOREPLAY")) {
		process_whois(param[1], param[2], utils::to_bool(param[3]), utils::to_bool(param[4]));

	} else if (!strcasecmp(param[0], "QUIT")) {
		process_quit(param[1]);

	} else if (!strcasecmp(param[0], "PRIVMSG")) {
		process_message(param[1], param[2], param[3]);

	} else if (!strcasecmp(param[0], "LISTSTART")) {
		process_chanlist_start();

	} else if (!strcasecmp(param[0], "LIST")) {
		process_chanlist(param[1], utils::to_int(param[2]), param[3]);

	} else if (!strcasecmp(param[0], "LISTEND")) {
		process_chanlist_end();

	} else if (!strcasecmp(param[0], "OFFLINE")) {
		process_offline(param[1]);

	} else if (!strcasecmp(param[0], "ONLINE")) {
		process_online(param[1]);

	} else if (!strcasecmp(param[0], "FORBIDJOIN")) {
		process_forbid_join(param[1], param[2]);

	} else {
		halt = false;
	}
	return halt;
}

void tchat_::swap_page(twindow& window, int layer, bool swap)
{
	if (!page_panel_) {
		return;
	}
	if (current_page_ == layer) {
		// desired page is the displaying page, do nothing.
		return;
	}

	bool require_layout_window = false;
	if (current_page_ == chat_page_) {
		require_layout_window = swap_resultion_;
		tchat_::post_show();
	} else {
		desire_swap_page(window, current_page_, false);
	}

	page_panel_->set_radio_layer(layer);

	if (layer == chat_page_) {
		tchat_::pre_show(*window_);

	} else {
		desire_swap_page(window, layer, true);
		if (require_layout_window) {
			window.invalidate_layout(nullptr);
		}
	}

	current_page_ = layer;
}

void tchat_::StartLogin(const std::string& server, int port) 
{
	server_ = server;
	Connect(server, port);
}

bool tchat_::app_relay_only()
{
	return find_widget<ttoggle_button>(window_, ctrlid_relayonly, false).get_value();
}

void tchat_::refresh_vrenderer_status(twindow& window) const
{
	std::stringstream ss;
	ss << "Connection: " << ice_connection_state_str(connection_state_) << "\n";
	ss << "Gathering:  " << ice_gathering_state_str(gathering_state_);

	tlabel& label = find_widget<tlabel>(vrenderer_layer_, ctrlid_vrenderer_status, false);
	label.set_label(ss.str());
}

//
// PeerConnectionObserver implementation.
//


void tchat_::app_OnIceConnectionChange()
{
	// thread context: _signalingThread
	refresh_vrenderer_status(*window_);
}

void tchat_::app_OnIceGatheringChange()
{
	// thread context: _signalingThread
	refresh_vrenderer_status(*window_);
}

void tchat_::OnRead(rtc::AsyncSocket* socket) 
{
	std::stringstream ss;
	size_t content_length = 0;
	bool first = true;
	const char* data;

	while ((data = read_2_buffer(socket, first, &content_length))) {
		first = false;

		Json::Reader reader;
		Json::Value json_object;
		if (!reader.parse(data, json_object)) {
			// _("Invalid data.");
			continue;
		}
		Json::Value& response_id = json_object["id"];
		if (response_id.isNull()) {
			// _("Unknown error.");
			continue;
		}
		std::string id = response_id.asString();
		std::string response = json_object["response"].asString();

		if (state_ == SIGNING_IN) {
			if (id != "resgisterResponse") {
				continue;
			}
			if (response == "accepted") {
				// register success
				// First response.  Let's store our server assigned ID.
				// The body of the response will be a list of already connected peers.

				if (caller_) {
					ConnectToPeer(peer_nick_, true, null_str);
				} else {
					switch_to_video(*window_);
					tlabel& label = find_widget<tlabel>(vrenderer_layer_, ctrlid_vrenderer_status, false);
					// label.set_label(_("Is waiting for the other to answer"));
					label.set_label(_("Calling"));
				}

				state_ = CONNECTED;

			} else {
				// rejected: user 'ancientcc' already registered
				ss << _("Call fail.");
				ss << "\n\n";
				ss << response;
				gui2::show_message("", ss.str());
				rtc::Thread::Current()->Post(RTC_FROM_HERE, this, MSG_SIGNALING_CLOSE, NULL);
				continue;
			}

		} else if (state_ == CONNECTED) {
			if (id == "iceCandidate") {
				Json::Value& candidate = json_object["candidate"];
				if (candidate.isObject()) {
					std::string sdp_mid = candidate[kCandidateSdpMidName].asString();
					int sdp_mlineindex = candidate[kCandidateSdpMlineIndexName].asInt();
					std::string sdp = candidate[kCandidateSdpName].asString();
					webrtc::SdpParseError error;
					std::unique_ptr<webrtc::IceCandidateInterface> candidate(
						webrtc::CreateIceCandidate(sdp_mid, sdp_mlineindex, sdp, &error));
					if (!candidate.get()) {
						RTC_LOG(WARNING) << "Can't parse received candidate message. " << "SdpParseError was: " << error.description;
						// should clear recv_data_vsize_ in the future!
						return;
					}
					const std::string& type = candidate->candidate().type();
					if (relay_only_ && type != cricket::RELAY_PORT_TYPE) {
						continue;
					}

					if (!peer_connection_->AddIceCandidate(candidate.get())) {
						RTC_LOG(WARNING) << "Failed to apply the received candidate";
						// should clear recv_data_vsize_ in the future!
						return;
					}
				}
			} else if (id == "stopCommunication") {
				VALIDATE(peer_connection_.get() != NULL, null_str);
				DeletePeerConnection();
				Close();
				leave_from_video(*window_);

			} else if (caller_) {
				if (id != "callResponse") {
					continue;
				}
				if (response == "accepted") {
					std::string sdp = json_object["sdpAnswer"].asString();
					// Replace message type from "offer" to "answer"
					sdp = webrtc_set_preferred_codec(sdp, preferred_codec_);
					webrtc::SessionDescriptionInterface* session_description(webrtc::CreateSessionDescription("answer", sdp, nullptr));
					peer_connection_->SetRemoteDescription(DummySetSessionDescriptionObserver::Create(), session_description);


					// I cannot understand, below peer_connection is NULL?
					// webrtc::PeerConnection* peer_connection = dynamic_cast<webrtc::PeerConnection*>(peer_connection_.get());
					// webrtc::WebRtcSession* session = peer_connection->session();

					// signal server, can send candidate to caller.
					// {"id":"answerProcessed"}
					Json::StyledWriter writer;
					Json::Value jmessage;

					jmessage["id"] = "answerProcessed";
					msg_2_signaling_server(control_socket_.get(), writer.write(jmessage));

					switch_to_video(*window_);
					
				} else {
					// rejected: user 'ancientcc' is not registered
					ss << _("Call fail.");
					ss << "\n\n";
					ss << response;
					gui2::show_message("", ss.str());
					rtc::Thread::Current()->Post(RTC_FROM_HERE, this, MSG_SIGNALING_CLOSE, NULL);
					continue;
				}
			} else {
				if (id == "incomingCall") {
					peer_nick_ = json_object["from"].asString();
					std::string offer = json_object[kSessionDescriptionSdpName].asString();
					ConnectToPeer(peer_nick_, false, offer);

					switch_to_video(*window_);

				} else if (id == "startCommunication") {
					// signal server, can send candidate to callee.
					// {"id":"startProcessed"}
					Json::StyledWriter writer;
					Json::Value jmessage;

					jmessage["id"] = "startProcessed";
					msg_2_signaling_server(control_socket_.get(), writer.write(jmessage));
				}
			}
		}
	}
}

tchat2::tchat2(const std::string& widget_id)
	: tchat_(widget_id, CHAT_PAGE)
{
}

void tchat2::pre_show()
{
	window_->set_escape_disabled(true);

	page_panel_ = find_widget<tstack>(window_, "panel", false, true);
	swap_page(*window_, CHAT_PAGE, false);

	join();
}

void tchat2::handle_status(int at, tsock::ttype type)
{
	if (at != tlobby::tag_chat) {
		return;
	}

	process_network_status(type == tsock::t_connected);

	tchat_::handle_status(at, type);
}

}
