/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Copyright 2015 Paul Rawson
 */

#define PAM_SM_AUTH
#define PAM_SM_ACCOUNT
#define PAM_SM_SESSION
#define PAM_SM_PASSWORD

#include <security/pam_modules.h>
#include <stdio.h>


PAM_EXTERN int pam_sm_authenticate (pam_handle_t *pamh, int flags, int argc, const char **argv) {
	const void *username;
	const void *password;
	pam_get_item(pamh, PAM_USER, &username);
	pam_get_item(pamh, PAM_AUTHTOK, &password);
	if (password == NULL)
		pam_get_authtok(pamh, PAM_AUTHTOK, &password, "Password: ");
	nmconfig (username, password);
	return PAM_SUCCESS;
}

PAM_EXTERN int pam_sm_setcred (pam_handle_t *pamh, int flags, int argc, const char **argv) {
	return PAM_SUCCESS;
}

PAM_EXTERN int pam_sm_acct_mgmt (pam_handle_t *pamh, int flags, int argc, const char **argv) {
	return PAM_IGNORE;
}

PAM_EXTERN int pam_sm_open_session (pam_handle_t *pamh, int flags, int argc, const char **argv) {
	return PAM_IGNORE;
}

PAM_EXTERN int pam_sm_chauthtok (pam_handle_t *pamh, int flags, int argc, const char **argv) {
	return PAM_IGNORE;
}


