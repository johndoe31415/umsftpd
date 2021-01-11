/**
 *	umsftpd - User-mode SFTP application.
 *	Copyright (C) 2021-2021 Johannes Bauer
 *
 *	This file is part of umsftpd.
 *
 *	umsftpd is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; this program is ONLY licensed under
 *	version 3 of the License, later versions are explicitly excluded.
 *
 *	umsftpd is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with umsftpd; if not, write to the Free Software
 *	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *	Johannes Bauer <JohannesBauer@gmx.de>
**/

#include <stdio.h>
#include <stdbool.h>
#include <libssh/callbacks.h>
#include <libssh/server.h>
#include <libssh/sftp.h>
#include "logging.h"

struct ssh_handle_t {
	unsigned int hid;
	ssh_event event;
	ssh_session session;
	ssh_channel channel;
	sftp_session sftp;
	struct {
		bool authenticated;
		bool sftp_requested;
		unsigned int authentication_attempts;
	} params;
};

static int subsystem_request(ssh_session session, ssh_channel channel, const char *subsystem, void *userdata) {
	struct ssh_handle_t *handle = (struct ssh_handle_t*) userdata;
	/* TODO: What if subsystem unprintable? */

	logmsg(LLVL_TRACE, "HID %u - subsystem requested: %s", handle->hid, subsystem);
	if (!strcmp(subsystem, "sftp")) {
		handle->params.sftp_requested = true;
		return SSH_OK;
	} else {
		return SSH_ERROR;
	}
}

static int auth_password(ssh_session session, const char *user, const char *pass, void *userdata) {
	struct ssh_handle_t *handle = (struct ssh_handle_t*) userdata;

	handle->params.authenticated = true;
	handle->params.authentication_attempts++;
	logmsg(LLVL_TRACE, "HID %u - authentication for user %s successful", handle->hid, user);
	return SSH_AUTH_SUCCESS;
}

static ssh_channel channel_open(ssh_session session, void *userdata) {
	struct ssh_handle_t *handle = (struct ssh_handle_t*) userdata;

	if (handle->channel) {
		logmsg(LLVL_ERROR, "HID %u - already has a channel allocated; refusing to create another one", handle->hid);
		return NULL;
	}

	logmsg(LLVL_TRACE, "HID %u - creating a new channel", handle->hid);
	handle->channel = ssh_channel_new(session);
	return handle->channel;
}

static void process_client_message(struct ssh_handle_t *handle, sftp_client_message message) {
	switch (message->type) {
		case SSH_FXP_REALPATH: {
			logmsg(LLVL_TRACE, "HID %u - client requested SSH_FXP_REALPATH of %s", handle->hid, message->filename);
			sftp_reply_name(message, "/foo", NULL);
			return;
		}

		case SSH_FXP_OPENDIR: {
			logmsg(LLVL_TRACE, "HID %u - client requested SSH_FXP_OPENDIR of %s", handle->hid, message->filename);
#if 0
			struct file_handle_t *file_handle = get_free_file_handle(handle);
			if (file_handle) {
				file_handle->allocated = true;

				ssh_string dir_handle = sftp_handle_alloc(handle->sftp, file_handle);
				sftp_reply_handle(message, dir_handle);
			} else {
				logmsg(LLVL_ERROR, "HID %u - out of file handles", handle->hid);
			}
#endif
			return;
		}

		case SSH_FXP_READDIR: {
#if 0
			struct file_handle_t *file_handle = (struct file_handle_t*)sftp_handle(message->sftp, message->handle);
			logmsg(LLVL_TRACE, "HID %u - client requested SSH_FXP_READDIR of %p", handle->hid, file_handle);

			if (!file_handle->eof) {
				sftp_reply_names_add(message, "file", "longname", NULL);
				sftp_reply_names(message);
				file_handle->eof = true;
			} else {
				sftp_reply_status(message, SSH_FX_EOF, "EOF");
			}
#endif
			return;
		}

		case SSH_FXP_CLOSE: {
			struct file_handle_t *file_handle = (struct file_handle_t*)sftp_handle(message->sftp, message->handle);
			logmsg(LLVL_TRACE, "HID %u - client requested SSH_FXP_CLOSE of %p", handle->hid, file_handle);
			//release_file_handle(file_handle);
			sftp_reply_status(message, SSH_FX_OK, "OK");
			return;
		}

		case SSH_FXP_LSTAT: {
			struct sftp_attributes_struct attrs = {
				.flags = SSH_FILEXFER_ATTR_SIZE | SSH_FILEXFER_ATTR_UIDGID | SSH_FILEXFER_ATTR_PERMISSIONS | SSH_FILEXFER_ATTR_ACMODTIME,
				.name = "foo",
				.longname = "bar",
				.uid = 1000,
				.gid = 2000,
				.type = SSH_FILEXFER_TYPE_DIRECTORY,
				.permissions = 16877,
			};
			logmsg(LLVL_TRACE, "HID %u - client requested SSH_FXP_LSTAT of %s", handle->hid, message->filename);
			sftp_reply_attr(message, &attrs);
			return;
		}

		default:
			logmsg(LLVL_TRACE, "HID %u - client requested unknown type %u", handle->hid, message->type);
			sftp_reply_status(message, SSH_FX_FAILURE, "Unknown type");
			return;

	}
}

static void handle_session_event_loop(struct ssh_handle_t *handle) {
	logmsg(LLVL_TRACE, "HID %u - entering session event loop", handle->hid);

	do {
		/* Poll the main event which takes care of the session, the channel and
		 * even our child process's stdout/stderr (once it's started). */
		if (ssh_event_dopoll(handle->event, -1) == SSH_ERROR) {
			ssh_channel_close(handle->channel);
		}

		if (handle->params.sftp_requested && (handle->sftp == NULL) && (handle->session != NULL) && (handle->channel != NULL)) {
			handle->sftp = sftp_server_new(handle->session, handle->channel);
			logmsg(LLVL_TRACE, "HID %u - creating new SFTP server session", handle->hid);
			if (!handle->sftp) {
				logmsg(LLVL_ERROR, "HID %u - error creating new SFTP server session", handle->hid);
				return;

			}
			logmsg(LLVL_TRACE, "HID %u - successfully created new SFTP server session", handle->hid);


			logmsg(LLVL_TRACE, "HID %u - initializing SFTP server", handle->hid);
			if (sftp_server_init(handle->sftp)) {
				logmsg(LLVL_ERROR, "HID %u - failed to initialize SFTP server", handle->hid);
				return;
			}
			logmsg(LLVL_TRACE, "HID %u - successfully initialized SFTP server", handle->hid);
			continue;
		}

		if (handle->sftp) {
			sftp_client_message msg = sftp_get_client_message(handle->sftp);
			if (!msg) {
				logmsg(LLVL_ERROR, "HID %u - unable to receive client message: %s", handle->hid, ssh_get_error(handle->session));
				break;
			} else {
				process_client_message(handle, msg);
			}
		}
	} while (ssh_channel_is_open(handle->channel));
}

static void handle_session(struct ssh_handle_t *handle) {
	logmsg(LLVL_TRACE, "HID %d - creating on new session", handle->hid);

	//server_cb.auth_pubkey_function = auth_publickey;
	//ssh_set_auth_methods(session, SSH_AUTH_METHOD_PASSWORD | SSH_AUTH_METHOD_PUBLICKEY);
	ssh_set_auth_methods(handle->session, SSH_AUTH_METHOD_PASSWORD);

	struct ssh_channel_callbacks_struct channel_cb = {
		.userdata = handle,
		.channel_subsystem_request_function = subsystem_request
	};

	struct ssh_server_callbacks_struct server_cb = {
		.userdata = handle,
		.auth_password_function = auth_password,
		.channel_open_request_session_function = channel_open,
	};

	ssh_callbacks_init(&server_cb);
	ssh_callbacks_init(&channel_cb);
	ssh_set_server_callbacks(handle->session, &server_cb);

	if (ssh_handle_key_exchange(handle->session) != SSH_OK) {
		logmsg(LLVL_ERROR, "HID %u - failed to exchange keys: %s", handle->hid, ssh_get_error(handle->session));
		return;
	} else {
		logmsg(LLVL_TRACE, "HID %u - successfully finished key exchange", handle->hid);
	}
	ssh_event_add_session(handle->event, handle->session);

	while ((!handle->params.authenticated) || (handle->channel == NULL)) {
		logmsg(LLVL_TRACE, "HID %u - polling for authentication events", handle->hid);
		if (ssh_event_dopoll(handle->event, 100) == SSH_ERROR) {
			fprintf(stderr, "Polling error: %s\n", ssh_get_error(handle->session));
			return;
		}
	}

	ssh_set_channel_callbacks(handle->channel, &channel_cb);
	handle_session_event_loop(handle);
}

static void _handle_session(ssh_event event, ssh_session session) {
	static unsigned int hid;
	struct ssh_handle_t handle = {
		.hid = ++hid,
		.event = event,
		.session = session,
	};

	handle_session(&handle);

	ssh_event_free(event);
	ssh_disconnect(session);
	ssh_free(session);
}

static bool start_server(void) {
	int result = ssh_init();
	if (result < 0) {
		logmsg(LLVL_CRITICAL, "ssh_init() failed.");
		return false;
	}

	ssh_bind sshbind = ssh_bind_new();
	if (sshbind == NULL) {
		logmsg(LLVL_CRITICAL, "ssh_bind_new() failed.");
		return false;
	}

	{
		unsigned int port = 12345;
		ssh_bind_options_set(sshbind, SSH_BIND_OPTIONS_BINDPORT, &port);
	}
	//	ssh_bind_options_set(sshbind, SSH_BIND_OPTIONS_RSAKEY, "ssh_host_rsa_key");
	//	ssh_bind_options_set(sshbind, SSH_BIND_OPTIONS_DSAKEY, "ssh_host_dsa_key");
	//	ssh_bind_options_set(sshbind, SSH_BIND_OPTIONS_ECDSAKEY, "ssh_host_ecdsa_key");
	ssh_bind_options_set(sshbind, SSH_BIND_OPTIONS_HOSTKEY, "ssh_host_ed25519_key");

	if (ssh_bind_listen(sshbind) < 0) {
		fprintf(stderr, "%s\n", ssh_get_error(sshbind));
		return 1;
	}

	while (true) {
		ssh_session session = ssh_new();
		if (session == NULL) {
			logmsg(LLVL_CRITICAL, "Failed to allocate session.");
			break;
		}

		ssh_event event = ssh_event_new();
		if (event == NULL) {
			logmsg(LLVL_CRITICAL, "Failed to create event.");
			break;
		}

		if (ssh_bind_accept(sshbind, session) != SSH_ERROR) {
			_handle_session(event, session);
		} else {
			logmsg(LLVL_ERROR, "Error finishing connection request.");
			ssh_event_free(event);
			ssh_disconnect(session);
			ssh_free(session);
		}
	}
	return true;
}


int main(int argc, char **argv) {
	llvl_set(LLVL_TRACE);
	start_server();
}
