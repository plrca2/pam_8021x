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
#include <gio/gio.h>


PAM_EXTERN int pam_sm_authenticate (pam_handle_t *pamh, int flags, int argc, const char **argv)
{
    const void *username;
    const void *password;
    pam_get_item(pamh, PAM_USER, &username);
    pam_get_item(pamh, PAM_AUTHTOK, &password);
    if (password == NULL)
        pam_get_authtok(pamh, PAM_AUTHTOK, &password, "Password: ");

    GVariantDict *creds = NULL, *settings = NULL;
    GVariantBuilder credsBuilder, settingsBuilder;


    g_variant_builder_init(&credsBuilder, G_VARIANT_TYPE("a{sv}"));
    g_variant_builder_add_value(&credsBuilder,
                                g_variant_new_dict_entry(
                                    g_variant_new_string("username"),
                                    g_variant_new_variant(g_variant_new_string(username))
                                )
                               );
    g_variant_builder_add_value(&credsBuilder,
                                g_variant_new_dict_entry(
                                    g_variant_new_string("password"),
                                    g_variant_new_variant(g_variant_new_string(password))
                                )
                               );

    creds = g_variant_dict_new(g_variant_builder_end(&credsBuilder));

    g_variant_builder_init(&settingsBuilder, G_VARIANT_TYPE("a{sv}"));
    int i = 0;
    for (i = 0; i < argc; i++)
    {
        gchar **option = g_strsplit(argv[i], "=", 2);
        g_variant_builder_add(&settingsBuilder,
                              "{sv}",
                              g_locale_to_utf8(
                                  option[0],
                                  -1,
                                  NULL,
                                  NULL,
                                  NULL
                              ),
                              g_variant_new_string(
                                  g_locale_to_utf8(
                                      option[1],
                                      -1,
                                      NULL,
                                      NULL,
                                      NULL
                                  )
                              )
                             );
        g_strfreev(option);
    }

    GVariant *settingsv = g_variant_builder_end(&settingsBuilder);
    g_debug("pam settings: %s", g_variant_print(settingsv, FALSE));
    settings = g_variant_dict_new(settingsv);

    g_debug("Configuring NetworkManager for 802.1x\n");
    nm8021xconfig(creds, settings);
    return PAM_SUCCESS;
}

PAM_EXTERN int pam_sm_setcred (pam_handle_t *pamh, int flags, int argc, const char **argv)
{
    return PAM_SUCCESS;
}

PAM_EXTERN int pam_sm_acct_mgmt (pam_handle_t *pamh, int flags, int argc, const char **argv)
{
    return PAM_IGNORE;
}

PAM_EXTERN int pam_sm_open_session (pam_handle_t *pamh, int flags, int argc, const char **argv)
{
    return PAM_IGNORE;
}

PAM_EXTERN int pam_sm_chauthtok (pam_handle_t *pamh, int flags, int argc, const char **argv)
{
    return PAM_IGNORE;
}


