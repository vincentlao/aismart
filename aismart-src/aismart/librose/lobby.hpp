/* $Id: multiplayer.hpp 47847 2010-12-05 21:12:23Z shadowmaster $ */
/*
   Copyright (C) 2005 - 2010 Philippe Plantier <ayin@anathas.org>
   Part of the Battle for Wesnoth Project http://www.wesnoth.org

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#ifndef LOBBY_HPP_INCLUDED
#define LOBBY_HPP_INCLUDED

#include "sdl_utils.hpp"
#include "events.hpp"
#include "config.hpp"
#include <time.h>
#include "ichat.hpp"
#include "gui/dialogs/dialog.hpp"

#include "rtc_base/sigslot.h"
#include "rtc_base/physicalsocketserver.h"

namespace irc {
struct ircnet;
struct server;
}

#define DEFAULT_RECONNECT_PROHABIT	20000
#define DEFAULT_HEARTBEAT_THRESHOLD	120000 // heartbeat maybe to 1 minute.

class tlobby;

struct tnoresponse_msg
{
	static std::string form_cookie(int major, const std::string& minor)
	{
		std::stringstream ss;
		ss << major << "-" << minor;
		return ss.str();
	}

	tnoresponse_msg(Uint32 t, const std::string& cookie)
		: t(t)
		, cookie(cookie)
	{}

	bool operator==(const std::string& that) const { return cookie == that; }
	bool operator!=(const std::string& that) const { return !operator==(that); }

	Uint32 t;
	std::string cookie;
};

class tsock: public sigslot::has_slots<>, private boost::noncopyable
{
	friend class tlobby;
public:
	enum tstate {s_none, s_created, s_consulting, s_ready};
	enum ttype {t_data, t_connected, t_disconnected};
	explicit tsock(int tag);
	virtual ~tsock();

	virtual void process() {}
	// true: continue, false: halt
	virtual bool receive_probed() { return true; }
	virtual size_t queue_data(const config& buf, const std::string& packet_type);

	bool valid() const { return at_ >= 0; }
	const std::string& tag() const { return tag_; }
	rtc::AsyncSocket* conn() const { return socket_.get(); }
	virtual void set_host(const std::string& host, int port);
	const std::string host() const { return host_; }
	int port() const { return port_; }
	tstate state() const { return state_; }
	virtual void reset_connect();
	bool check_time_overflow(Uint32 threshold);
	void process_error(const std::string& error_str);
	virtual bool ready() const { return state_ == s_ready; }

	void set_connect_result(const std::string& error);

	virtual bool connect(int family, int type, const std::string& host, int port);
	virtual void pre_disconnect() {}
	virtual void post_disconnect();

	virtual bool insert_noresponse_msg(int major, const std::string& minor);
	virtual bool erase_noresponse_msg(int major, const std::string& minor);

protected:
	void resize_raw_data(int size);

private:
	void OnConnect(rtc::AsyncSocket* socket);
	void OnRead(rtc::AsyncSocket* socket);
	void OnClose(rtc::AsyncSocket* socket, int err);

	virtual void mini_connectd() {}
	virtual void mini_read() {}
	virtual void mini_close(int err) {}

public:
	bool raw_data_only;
	bool require_stats;
	int connected_at_;
	std::string error_;
	Uint32 msg_send_time;
	Uint32 msg_send_gap;

protected:
	int at_;
	std::string tag_;
	std::unique_ptr<rtc::AsyncSocket> socket_;
	std::string host_;
	int port_;
	tstate state_;
	Uint32 last_active_time_;
	Uint32 next_create_time_;
	bool accept_;
	std::vector<tnoresponse_msg> noresponse_msgs_;

	int reconnect_prohabit_;
	int heartbeat_threshold_;

	int max_noresponse_msgs_;
	int noresponse_threshold_;

	config data_;
	char* raw_data_;
	int raw_data_size_;
	int raw_data_vsize_;
};

class tlobby_user 
{
public:
	static const int npos;
	static std::map<std::string, int> uids;
	static int get_uid(const std::string& nick, bool must_exist = true);
	static const std::string& get_nick(int uid);

	tlobby_user(int uid, const std::string& nick)
		: uid(uid)
		, nick(nick)
		, cids()
		, unread(0)
		, online(false)
		, away(false)
		, me(false)
	{}

	bool valid() const { return uid != npos; }

	int uid;
	std::string nick;
	std::set<int> cids;
	int unread;
	bool online;
	bool away;
	bool me;
};
extern tlobby_user null_user;

class tlobby_channel
{
public:
	static const int npos;
	static const int t_me;
	static const int t_friend;
	static const int min_allocatable = 100;
	static std::map<std::string, int> cids;
	static int get_cid(const std::string& nick, bool must_exist = true);
	static const std::string& get_nick(int cid);
	static bool is_allocatable(int cid) { return cid >= min_allocatable; }

	tlobby_channel(int cid, const std::string& nick, const std::string& key = null_str)
		: cid(cid)
		, nick(nick)
		, key(key)
		, topic()
		, users()
		, users_receiving(false)
		, who_reqeusting(0)
		, err(false)
	{}
	~tlobby_channel();

	tlobby_user& insert_user(int uid, const std::string& nick);
	void erase_user(int uid);

	tlobby_user& get_user(int uid) const;

	std::string name() const;
	bool valid() const { return cid != npos; }
	bool joined() const { return !users.empty(); }

	int cid;
	std::string nick;
	std::string key;
	std::string topic;
	std::vector<tlobby_user*> users;
	bool users_receiving;
	Uint32 who_reqeusting;
	bool err;
};
extern tlobby_channel null_channel;

namespace chat_logs {

struct tlog {
	tlog(int uid, const std::string& nick, const std::string& msg, time_t t)
		: uid(uid)
		, nick(nick)
		, msg(msg)
		, t(t)
	{}

	int uid;
	std::string nick;
	std::string msg;
	time_t t;
};

struct treceiver {
	treceiver(int id = tlobby_channel::npos, bool channel = true)
		: id(id)
		, channel(channel)
	{
		if (id != tlobby_channel::npos) {
			nick = channel? tlobby_channel::get_nick(id): tlobby_user::get_nick(id);
		}
	}

	bool valid() const { return id != tlobby_channel::npos; }
	void insert_log(int uid, const std::string& nick, const std::string& msg, time_t t)
	{
		logs.push_back(tlog(uid, nick, msg, t));
	}

	int id;
	bool channel;
	std::string nick;
	std::vector<tlog> logs;
};

struct thistory_log {
	thistory_log(int uid, const std::string& nick, time_t from, time_t to, int offset, int size)
		: uid(uid)
		, nick(nick)
		, from(from)
		, to(to)
		, offset(offset)
		, size(size)
	{}

	bool operator==(const thistory_log& that) const { return nick == that.nick; }
	bool operator!=(const thistory_log& that) const { return !operator==(that); }

	bool operator<(const thistory_log& that) const { return nick < that.nick; }

	int uid;
	std::string nick;
	time_t from;
	time_t to;
	int offset;
	int size;
};
extern std::set<thistory_log> history_logs;

treceiver& find_receiver(int id, bool channel, bool allow_create = false);
void add(int id, bool channel, const tlobby_user& sender, const std::string& msg);

void restore_from_logfile();
void user_from_logfile(const thistory_log& user, std::vector<tlog>& logs);
void save_temp_logfile();
void combine_logfile();

}

class tlobby
{
	friend class tsock;
public:
	static const int tag_chat = 0;
	static const int tag_http = 1;
	static const int tag_transit = 2;
	static const int min_app_tag = 3;

	class thandler
	{
	public:
		thandler();
		virtual ~thandler();
		virtual void handle_status(int at, tsock::ttype type) {}
		virtual bool handle_raw(int at, tsock::ttype type, const char* param[]) { return false; }
		virtual bool handle_raw2(int at, tsock::ttype type, const char* data, int len) { return false; }
		virtual bool handle(int tag, tsock::ttype type, const config& data) { return false; }
		void join();

	private:
		bool has_joined_;
	};

	class tlog_handler
	{
	public:
		tlog_handler();
		virtual ~tlog_handler();
		virtual void handle(const tsock& sock, const std::string& msg) = 0;
		void join();

	private:
		bool has_joined_;
	};
	
	struct tlog
	{
		tlog(tsock::tstate state, const std::string& msg, time_t t)
			: state(state)
			, msg(msg)
			, t(t)
		{}
		
		tsock::tstate state;
		std::string msg;
		time_t t;
	};

	class tchat_sock: public tsock, public ichat
	{
	public:
		enum {msg_join, msg_userlist, msg_who, msg_chanlist, msg_monitor};

		static const int ping_interval = 60000;
		static void split_offline_online(const char* nicks, std::vector<std::string>& result);

		tchat_sock();
		~tchat_sock();

		const std::string& nick() const { return nick_; }
		irc::server* serv() const { return serv_; }
		void process();
		void pre_disconnect();
		void set_host(const std::string& host, int port);
		void reset_connect();

		// impletement ichat
		void tcp_send_len(char* buf, int len);
		void handle_command(const char* param[]);

		tlobby_channel& get_channel(int cid);
		const tlobby_channel& get_channel(int cid) const;
		tlobby_channel& insert_channel(int cid, const std::string& nick);
		void erase_channel(int cid);

		bool is_favor_user(int uid) const;
		tlobby_user& favor_user(int uid) const;

		bool is_valid_user(int uid) const { return users_.count(uid); }

		tlobby_user& get_user(int uid);
		tlobby_user& insert_user(int uid, const std::string& nick, int cid);
		void erase_user(int uid, int cid);

		bool erase_noresponse_msg(int major, const std::string& minor);

		bool join_channel(tlobby_channel& channel, bool force);
		void find_channel(const std::string& chan, int min_users);

		void save_preferences();

	protected:
		void do_task();
		irc::ircnet generate_ircnet() const;
		void send_ping();

		void process_chanlist_end();
		void process_online(const char* nicks);
		void process_offline(const char* nicks);
		bool process_message(const std::string& chan, const std::string& from, const std::string& text);
		bool process_userlist_th(const std::string& chan, const std::string& names);
		void process_userlist_bh(const std::string& chan);
		bool process_userlist_end(const std::string& chan);
		void process_ujoined(const std::string& chan);
		void process_uparted(const std::string& chan);
		bool process_part_th(const std::string& chan, const std::string& nick, const std::string& reason);
		void process_part_bh(const std::string& chan, const std::string& nick, const std::string& reason);
		bool process_join(const std::string& chan, const std::string& nick);
		bool process_whois(const std::string& chan, const std::string& nick, bool online, bool away);
		bool process_quit_th(const std::string& nick);
		void process_quit_bh(const std::string& nick);
		void process_forbid_join(const std::string& chan, const std::string& reason);

	private:
		void mini_connectd() override;
		void mini_read() override;
		void mini_close(int err) override;

	public:
		std::map<int, tlobby_channel> channels;
		tlobby_user* me;

	private:
		std::map<int, tlobby_user> users_;
		std::string nick_;
		irc::server* serv_;
		bool pong_receiving_;
		bool online_offline_received_;
		
		Uint32 last_task_time_;
		Uint32 task_threshold_;
	};

	class thttp_sock: public tsock
	{
	public:
		static int http_2_cfg(const char* http, const int size, config& cfg);

		thttp_sock()
			: tsock(tag_http)
			, response_size_(0)
		{}
		void process();
		bool ready() const { return socket_.get() != nullptr; }
		void reset_connect();

		virtual std::string form_url(const std::string& task) const { return task; }
		virtual std::string form_request(const std::string& task, size_t content_length) const;

		bool network_connect_dialog(bool quiet);
		bool network_receive_dialog(int hidden_ms = 3);
		bool network_send_dialog(const char* buf, int len, int hidden_ms = 3);

		int response_size() const { return response_size_; }
		const char* response_buf() const { return response_size_? raw_data_: nullptr; }

	private:
		void mini_connectd() override;
		void mini_read() override;
		void mini_close(int err) override;

	private:
		int response_size_;
	};

	class ttransit_sock: public tsock
	{
	public:
		ttransit_sock()
			: tsock(tag_transit)
			, remote_handle_(0)
		{
			raw_data_only = false;
		}
		void process();
		bool connect(int family, int type, const std::string& host, int port);
		void pre_disconnect();
		void post_disconnect();
		bool receive_probed();

		void send_data(const config& cfg);

	private:
		bool is_pending_remote_handle() const;
		void default_handle(const config& data);

		void mini_connectd() override {}
		void mini_read() override {}
		void mini_close(int err) override {}

	private:
		// The remote handle is the handle assigned to this connection by the remote host.
		// Is 0 before a handle has been assigned.
		int remote_handle_;
	};

	tlobby(tchat_sock* _chat, thttp_sock* _http, ttransit_sock* _transit);
	virtual ~tlobby();

	tsock& sock(int tag) const { return *socks_[tag]; }

	void set_nick(const std::string& nick);
	const std::string& nick() const { return nick_; }

	void set_nick2(const std::string& leader);

	bool insert_accept_sock(tsock* sock);

	void pump();
	void broadcast_handle_status(int at, tsock::ttype type);
	void broadcast_handle_raw(int at, tsock::ttype type, const char* param[]);

	void add_log(const tsock& sock, const std::string& msg);
	void clear_log() { logs_.clear(); }
	std::string format_log_str() const;

	const std::vector<tlog>& logs() const { return logs_; }

	// in normal, app should not call it.
	void disable_chat();

private:
	virtual tsock* get_accept_sock();

public:
	tchat_sock* chat;
	thttp_sock* http;
	ttransit_sock* transit;

protected:
	std::vector<tsock*> socks_;
	std::vector<thandler*> handlers_;

	std::vector<tlog_handler*> log_handlers_;
	std::vector<tlog> logs_;
	std::string nick_;
	bool enable_chat_;
};

extern tlobby* lobby;

#endif
