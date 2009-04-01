/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* Cherokee
 *
 * Authors:
 *      Alvaro Lopez Ortega <alvaro@alobbs.com>
 *
 * Copyright (C) 2001-2009 Alvaro Lopez Ortega
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */ 

#include "common-internal.h"
#include "logger_custom.h"

#include "util.h"
#include "connection.h"
#include "connection-protected.h"
#include "module.h"
#include "server.h"
#include "server-protected.h"
#include "header.h"
#include "header-protected.h"

/* Plug-in initialization
 */
PLUGIN_INFO_LOGGER_EASIEST_INIT (custom);

/* The macros
 */
static ret_t
add_ip_remote (cherokee_template_t       *template,
	       cherokee_template_token_t *token,
	       cherokee_buffer_t         *output,
	       void                      *param)
{
	cuint_t                prev_len;
	cherokee_connection_t *conn      = CONN(param);

	UNUSED (template);
	UNUSED (token);
	
	prev_len = output->len;

	cherokee_buffer_ensure_addlen (output, CHE_INET_ADDRSTRLEN);
	cherokee_socket_ntop (&conn->socket,
			      (output->buf + output->len),
			      (output->size - output->len) -1);

	output->len += strlen(output->buf + prev_len);
	return ret_ok;
}

static ret_t
add_ip_local (cherokee_template_t       *template,
	      cherokee_template_token_t *token,
	      cherokee_buffer_t         *output,
	      void                      *param)
{
	cherokee_connection_t *conn = CONN(param);

	UNUSED (template);
	UNUSED (token);
	
	if (! cherokee_buffer_is_empty (&conn->bind->ip)) {
		cherokee_buffer_add_buffer (output, &conn->bind->ip);
	} else {
		cherokee_buffer_add_str (output, "-");
	}
	
	return ret_ok;
}

static ret_t
add_status (cherokee_template_t       *template,
	    cherokee_template_token_t *token,
	    cherokee_buffer_t         *output,
	    void                      *param)
{
	cherokee_connection_t *conn = CONN(param);

	UNUSED (template);
	UNUSED (token);

	cherokee_buffer_add_ulong10 (output, conn->error_code);
	return ret_ok;
}

static ret_t
add_transport (cherokee_template_t       *template,
	       cherokee_template_token_t *token,
	       cherokee_buffer_t         *output,
	       void                      *param)
{
	cherokee_connection_t *conn = CONN(param);

	UNUSED (template);
	UNUSED (token);

	if (conn->socket.is_tls) {
		cherokee_buffer_add_str (output, "https");
	} else {
		cherokee_buffer_add_str (output, "http");
	}

	return ret_ok;
}

static ret_t
add_protocol (cherokee_template_t       *template,
	      cherokee_template_token_t *token,
	      cherokee_buffer_t         *output,
	      void                      *param)
{
	cherokee_connection_t *conn = CONN(param);

	UNUSED (template);
	UNUSED (token);

	switch (conn->header.version) {
	case http_version_11:
		cherokee_buffer_add_str (output, "HTTP/1.1");
		break;
	case http_version_10:
		cherokee_buffer_add_str (output, "HTTP/1.0");
		break;
	case http_version_09:
		cherokee_buffer_add_str (output, "HTTP/0.9");
		break;
	default:
		cherokee_buffer_add_str (output, "Unknown");
	}

	return ret_ok;
}

static ret_t
add_port_server (cherokee_template_t       *template,
		 cherokee_template_token_t *token,
		 cherokee_buffer_t         *output,
		 void                      *param)
{
	cherokee_connection_t *conn = CONN(param);

	UNUSED (template);
	UNUSED (token);

	cherokee_buffer_add_buffer (output, &conn->bind->server_port);
	return ret_ok;
}

static ret_t
add_query_string (cherokee_template_t       *template,
		  cherokee_template_token_t *token,
		  cherokee_buffer_t         *output,
		  void                      *param)
{
	cherokee_connection_t *conn = CONN(param);

	UNUSED (template);
	UNUSED (token);

	if (! cherokee_buffer_is_empty(&conn->query_string)) {
		cherokee_buffer_add_buffer (output, &conn->query_string);
	} else {
		cherokee_buffer_add_str (output, "-");
	}

	return ret_ok;
}

static ret_t
add_request_first_line (cherokee_template_t       *template,
			cherokee_template_token_t *token,
			cherokee_buffer_t         *output,
			void                      *param)
{
	char                  *p;
	char                  *end;
	cherokee_connection_t *conn = CONN(param);

	UNUSED (template);
	UNUSED (token);
	
	end = (conn->header.input_buffer->buf +
	       conn->header.input_buffer->len);

	p =  conn->header.input_buffer->buf;
	p += conn->header.request_off;

	while ((*p != CHR_CR) && (*p != CHR_LF) && (p < end))
		p++;
	
	cherokee_buffer_add (output,
			     conn->header.input_buffer->buf,
			     p - conn->header.input_buffer->buf);

	return ret_ok;
}

static ret_t
add_now (cherokee_template_t       *template,
	 cherokee_template_token_t *token,
	 cherokee_buffer_t         *output,
	 void                      *param)
{
	UNUSED (template);
	UNUSED (token);

	output = output;
	param  = param;

	// Time when accepted
	return ret_ok;
}

static ret_t
add_time_secs (cherokee_template_t       *template,
	       cherokee_template_token_t *token,
	       cherokee_buffer_t         *output,
	       void                      *param)
{
	UNUSED (template);
	UNUSED (token);

	output = output;
	param  = param;

	// Elapse
	return ret_ok;
}

static ret_t
add_time_nsecs (cherokee_template_t       *template,
		cherokee_template_token_t *token,
		cherokee_buffer_t         *output,
		void                      *param)
{
	UNUSED (template);
	UNUSED (token);

	output = output;
	param  = param;

	// Elapse
	return ret_ok;
}

static ret_t
add_user_remote (cherokee_template_t       *template,
		 cherokee_template_token_t *token,
		 cherokee_buffer_t         *output,
		 void                      *param)
{
	cherokee_connection_t *conn = CONN(param);

	UNUSED (template);
	UNUSED (token);

	if ((conn->validator) && 
	    (! cherokee_buffer_is_empty (&conn->validator->user)))
	{
		cherokee_buffer_add_buffer (output, &conn->validator->user);
	} else {
		cherokee_buffer_add_str (output, "-");
	}

	return ret_ok;
}

static ret_t
add_request (cherokee_template_t       *template,
	     cherokee_template_token_t *token,
	     cherokee_buffer_t         *output,
	     void                      *param)
{
	cherokee_connection_t *conn = CONN(param);

	UNUSED (template);
	UNUSED (token);

	cherokee_buffer_add_buffer (output, &conn->request);
	return ret_ok;
}

static ret_t
add_request_original (cherokee_template_t       *template,
		      cherokee_template_token_t *token,
		      cherokee_buffer_t         *output,
		      void                      *param)
{
	cherokee_connection_t *conn = CONN(param);

	UNUSED (template);
	UNUSED (token);
	
	if (cherokee_buffer_is_empty (&conn->request_original)) {
		cherokee_buffer_add_buffer (output, &conn->request);
	} else  {
		cherokee_buffer_add_buffer (output, &conn->request_original);
	}

	return ret_ok;
}

static ret_t
add_vserver_name (cherokee_template_t       *template,
		  cherokee_template_token_t *token,
		  cherokee_buffer_t         *output,
		  void                      *param)
{
	cherokee_connection_t *conn = CONN(param);

	UNUSED (template);
	UNUSED (token);

	cherokee_buffer_add_buffer (output, &CONN_VSRV(conn)->name);
	return ret_ok;
}


static ret_t
_set_template (cherokee_logger_custom_t *logger,
	       cherokee_template_t      *template)
{
	ret_t ret;
	const struct {
		const char *name;
		void       *func;
	} *p, macros[] = {
		{"ip_remote",          add_ip_remote},
		{"ip_local",           add_ip_local},
		{"protocol",           add_protocol},
		{"transport",          add_transport},
		{"port_server",        add_port_server},
		{"query_string",       add_query_string},
		{"request_first_line", add_request_first_line},
		{"status",             add_status},
		{"now",                add_now},
		{"time_secs",          add_time_secs},
		{"time_nsecs",         add_time_nsecs},
		{"user_remote",        add_user_remote},
		{"request",            add_request},
		{"request_original",   add_request_original},
		{"vserver_name",       add_vserver_name},
		{NULL, NULL}
	};

	for (p=macros; p->name; p++) {
		ret = cherokee_template_set_token (template, p->name, 
						   (cherokee_tem_repl_func_t) p->func,
						   logger, NULL);
		if (unlikely (ret != ret_ok)) {
			return ret;
		}
	}

	return ret_ok;
}


static ret_t
_init_template (cherokee_logger_custom_t *logger,
		cherokee_template_t      *template,
		cherokee_config_node_t   *config,
		const char               *key_config)
{
	ret_t              ret;
	cherokee_buffer_t *tmp;

	ret = cherokee_template_init (template);
	if (ret != ret_ok)
		return ret;

	ret = _set_template (logger, template);
	if (ret != ret_ok)
		return ret;

	ret = cherokee_config_node_read (config, key_config, &tmp);
	if (ret != ret_ok) {
		PRINT_ERROR ("Custom Logger: A template is needed for logging connections: %s\n", key_config);
		return ret_error;
	}

	ret = cherokee_template_parse (template, tmp);
	if (ret != ret_ok) {
		PRINT_ERROR ("Couldn't parse custom log: '%s'\n", tmp->buf);
		return ret_error;
	}

	return ret_ok;
}


ret_t
cherokee_logger_custom_new (cherokee_logger_t         **logger,
			    cherokee_virtual_server_t  *vsrv,
			    cherokee_config_node_t     *config)
{
	ret_t                   ret;
	cherokee_config_node_t *subconf;
	CHEROKEE_NEW_STRUCT (n, logger_custom);

	/* Init the base class object
	 */
	cherokee_logger_init_base (LOGGER(n), PLUGIN_INFO_PTR(custom));

	MODULE(n)->init           = (logger_func_init_t) cherokee_logger_custom_init;
	MODULE(n)->free           = (logger_func_free_t) cherokee_logger_custom_free;

	LOGGER(n)->flush          = (logger_func_flush_t) cherokee_logger_custom_flush;
	LOGGER(n)->reopen         = (logger_func_reopen_t) cherokee_logger_custom_reopen;
	LOGGER(n)->write_error    = (logger_func_write_error_t) cherokee_logger_custom_write_error;
	LOGGER(n)->write_access   = (logger_func_write_access_t) cherokee_logger_custom_write_access;
	LOGGER(n)->write_string   = (logger_func_write_string_t) cherokee_logger_custom_write_string;
	LOGGER(n)->write_error_fd = (logger_func_write_error_fd_t)  cherokee_logger_custom_write_error_fd;

	/* Init properties
	 */
	ret = cherokee_config_node_get (config, "access", &subconf);
	if (ret == ret_ok) {
		ret = cherokee_server_get_log_writer (VSERVER_SRV(vsrv), subconf, &n->writer_access);
		if (ret != ret_ok) {
			return ret_error;
		}
	}

	ret = cherokee_config_node_get (config, "error", &subconf);
	if (ret == ret_ok) {
		ret = cherokee_server_get_log_writer (VSERVER_SRV(vsrv), subconf, &n->writer_error);
		if (ret != ret_ok) {
			return ret_error;
		}
	}

	/* Templates
	 */
	ret = _init_template (n, &n->template_conn, config, "access_template");
	if (ret != ret_ok)
		return ret;

	ret = _init_template (n, &n->template_error, config, "error_template");
	if (ret != ret_ok)
		return ret;

	/* Return the object
	 */
	*logger = LOGGER(n);
	return ret_ok;
}

ret_t 
cherokee_logger_custom_init (cherokee_logger_custom_t *logger)
{
	ret_t ret;

	ret = cherokee_logger_writer_open (logger->writer_access);
	if (ret != ret_ok)
		return ret;

	ret = cherokee_logger_writer_open (logger->writer_error);
	if (ret != ret_ok)
		return ret;

	return ret_ok;
}

ret_t
cherokee_logger_custom_free (cherokee_logger_custom_t *logger)
{
	cherokee_template_mrproper (&logger->template_conn);
	cherokee_template_mrproper (&logger->template_error);

	return ret_ok;
}

ret_t
cherokee_logger_custom_flush (cherokee_logger_custom_t *logger)
{
	return cherokee_logger_writer_flush (logger->writer_access);
}

ret_t
cherokee_logger_custom_reopen (cherokee_logger_custom_t *logger)
{
	ret_t ret1;
	ret_t ret2;

	ret1 = cherokee_logger_writer_reopen (logger->writer_access);
	ret2 = cherokee_logger_writer_reopen (logger->writer_error);

	if (ret1 != ret_ok)
		return ret1;

	return ret2;
}

ret_t
cherokee_logger_custom_write_error_fd (cherokee_logger_custom_t *logger, int fd)
{
	if ((logger->writer_error->fd != -1) &&
	    (logger->writer_error->fd != fd))
	{
		dup2 (logger->writer_error->fd, fd);
	}

	return ret_ok;
}

ret_t
cherokee_logger_custom_write_access (cherokee_logger_custom_t *logger,
				     cherokee_connection_t    *conn)
{
	ret_t              ret;
	cherokee_buffer_t *log;
	
	/* Get the buffer
	 */
	ret = cherokee_logger_writer_get_buf (logger->writer_access, &log);
	if (unlikely (ret != ret_ok))
		return ret;

	/* Render the template
	 */
	ret = cherokee_template_render (&logger->template_conn, log, conn);
	if (unlikely (ret != ret_ok))
		return ret;

	cherokee_buffer_add_char (log, '\n');

	/* Flush buffer if full
	 */  
	if (log->len < logger->writer_access->max_bufsize)
		return ret_ok;

	ret = cherokee_logger_writer_flush (logger->writer_access);
	if (unlikely (ret != ret_ok))
		return ret;

	return ret_ok;
}

ret_t
cherokee_logger_custom_write_error (cherokee_logger_custom_t *logger,
				    cherokee_connection_t    *conn)
{
	ret_t              ret;
	cherokee_buffer_t *log;

	/* Get the buffer
	 */
	ret = cherokee_logger_writer_get_buf (logger->writer_error, &log);
	if (unlikely (ret != ret_ok))
		return ret;

	/* Render the template
	 */
	ret = cherokee_template_render (&logger->template_error, log, conn);
	if (unlikely (ret != ret_ok))
		return ret;

	cherokee_buffer_add_char (log, '\n');

	/* It's an error. Flush it!
	 */
	ret = cherokee_logger_writer_flush (logger->writer_error);
	if (unlikely (ret != ret_ok))
		return ret;

	return ret_ok;
}

ret_t
cherokee_logger_custom_write_string (cherokee_logger_custom_t *logger,
				     const char               *string)
{
	ret_t              ret;
	cherokee_buffer_t *log;

	ret = cherokee_logger_writer_get_buf (logger->writer_access, &log);
	if (unlikely (ret != ret_ok))
		return ret;

	ret = cherokee_buffer_add (log, string, strlen(string));
 	if (unlikely (ret != ret_ok))
		return ret;

	/* Flush buffer if full
	 */  
  	if (log->len < logger->writer_access->max_bufsize)
		return ret_ok;

	ret = cherokee_logger_writer_flush (logger->writer_access);
	if (unlikely (ret != ret_ok))
		return ret;

	return ret_ok;
}
