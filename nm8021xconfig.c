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


#include <gio/gio.h>
#include <NetworkManager.h>
#include "nm-connection.h"


int
main (int argc, char *argv[])
{
    GVariantDict *creds = NULL, *settings = NULL;
    GVariantBuilder credsBuilder, settingsBuilder;


    g_variant_builder_init(&credsBuilder, G_VARIANT_TYPE("a{sv}"));
    g_variant_builder_add_value(&credsBuilder,
                                g_variant_new_dict_entry(
                                    g_variant_new_string("username"),
                                    g_variant_new_variant(g_variant_new_string("testing"))
                                )
                               );
    g_variant_builder_add_value(&credsBuilder,
                                g_variant_new_dict_entry(
                                    g_variant_new_string("password"),
                                    g_variant_new_variant(g_variant_new_string("testing"))
                                )
                               );
    creds = g_variant_dict_new(g_variant_builder_end(&credsBuilder));

    g_variant_builder_init(&settingsBuilder, G_VARIANT_TYPE("a{sv}"));
    g_variant_builder_add_value(&settingsBuilder,
                                g_variant_new_dict_entry(
                                    g_variant_new_string("eap_method"),
                                    g_variant_new_variant(g_variant_new_string("peap"))
                                )
                               );
    g_variant_builder_add_value(&settingsBuilder,
                                g_variant_new_dict_entry(
                                    g_variant_new_string("ca_cert_path"),
                                    g_variant_new_variant(g_variant_new_string("/etc/pki/ca-trust/extracted/pem/tls-ca-bundle.pem"))
                                )
                               );
    g_variant_builder_add_value(&settingsBuilder,
                                g_variant_new_dict_entry(
                                    g_variant_new_string("inner_method"),
                                    g_variant_new_variant(g_variant_new_string("mschapv2"))
                                )
                               );
    g_variant_builder_add_value(&settingsBuilder,
                                g_variant_new_dict_entry(
                                    g_variant_new_string("connection_name"),
                                    g_variant_new_variant(g_variant_new_string("en1p0s3"))
                                )
                               );
    settings = g_variant_dict_new(g_variant_builder_end(&settingsBuilder));


    return nm8021xconfig(creds, settings);
}

GVariant
*getSetting(const gchar *settingPath)
{
    g_debug("Getting setting: %s", settingPath);
    GDBusProxy *dbus_proxy;

    dbus_proxy =
        g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SYSTEM,
                                       G_DBUS_PROXY_FLAGS_NONE,
                                       NULL,
                                       NM_DBUS_SERVICE,
                                       settingPath,
                                       NM_DBUS_IFACE_SETTINGS_CONNECTION,
                                       NULL, NULL);
    g_assert (dbus_proxy != NULL);


    GError *err = NULL;
    GVariant *connectionSettings =
        g_dbus_proxy_call_sync(dbus_proxy,
                               "GetSettings", NULL,
                               G_DBUS_CALL_FLAGS_NONE,
                               -1, NULL, &err);
    if(connectionSettings == NULL)
        g_error("Error getting settings: %s\n", err->message);

    GVariant *tmpconnectionSettings = g_variant_get_child_value(connectionSettings, 0);
    return tmpconnectionSettings;
}

GVariant
*getSettingsList()
{
    g_debug("Getting setting list\n");
    GDBusProxy *dbus_proxy;
    dbus_proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SYSTEM,
                 G_DBUS_PROXY_FLAGS_NONE,
                 NULL,
                 NM_DBUS_SERVICE,
                 NM_DBUS_PATH_SETTINGS,
                 NM_DBUS_IFACE_SETTINGS,
                 NULL, NULL);
    g_assert (dbus_proxy != NULL);


    GError *err = NULL;
    GVariant *settingsList =
        g_dbus_proxy_call_sync(dbus_proxy,
                               "ListConnections", NULL,
                               G_DBUS_CALL_FLAGS_NONE,
                               -1, NULL, &err);
    if(settingsList == NULL)
        g_error("Error getting settings list: %s\n", err->message);

    GVariant *tmpsettingsList = g_variant_get_child_value(settingsList, 0);
    return tmpsettingsList;
}

GVariant
*getSettingName(const gchar *settingPath)
{
    g_debug("Getting setting name for: %s", settingPath);
    GDBusProxy *dbus_proxy;

    dbus_proxy =
        g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SYSTEM,
                                       G_DBUS_PROXY_FLAGS_NONE,
                                       NULL,
                                       NM_DBUS_SERVICE,
                                       settingPath,
                                       NM_DBUS_IFACE_SETTINGS_CONNECTION,
                                       NULL, NULL);
    g_assert (dbus_proxy != NULL);



    GVariant *setting = getSetting(settingPath);
    GVariantIter iter;
    GVariant *child;
    const gchar *key;
    GVariant *value;
    g_variant_iter_init(&iter, setting);
    while ((child = g_variant_iter_next_value(&iter)))
    {
        if (!g_strcmp0(g_variant_get_string(g_variant_get_child_value(child, 0), NULL), "connection"))
        {
            GVariantDict *connection_dict = g_variant_dict_new(g_variant_get_child_value(child, 1));
            GVariant *id = g_variant_dict_lookup_value(connection_dict, "id",NULL);
            return id;
        }
    }
    return NULL;
}

int
activateSettings (const gchar *connectionPath, GVariant *settings)
{
    g_debug("Activating the following settings for %s: %s",
            connectionPath,
            g_variant_print(settings, FALSE)
           );
    GDBusProxy *dbus_proxy;

    dbus_proxy =
        g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SYSTEM,
                                       G_DBUS_PROXY_FLAGS_NONE,
                                       NULL,
                                       NM_DBUS_SERVICE,
                                       connectionPath,
                                       NM_DBUS_IFACE_SETTINGS_CONNECTION,
                                       NULL, NULL);

    GError *err = NULL;
    GVariant *ret = NULL;
    ret = g_dbus_proxy_call_sync(dbus_proxy,
                                 "UpdateUnsaved", settings,
                                 G_DBUS_CALL_FLAGS_NONE,
                                 -1, NULL, &err);
    if (ret == NULL)
    {
        g_error ("Settings failed to apply!\nError: %s\n", err->message);
        return -1;
    }
    return 0;
}

int
nm8021xconfig(GVariantDict *credentials, GVariantDict *settings)
{
    const gchar *username = NULL, *password = NULL, *eapMethod = NULL, *caCertPath = NULL, *innerMethod = NULL, *conName = NULL;
    if (!credentials)

        return -1;
    if (g_variant_dict_contains(credentials, "username"))
    {
        username = g_variant_get_string(g_variant_dict_lookup_value(credentials, "username", NULL), NULL);
        g_debug("Username: %s", username);
    }
    else
    {
        return -1;
    }
    if (g_variant_dict_contains(credentials, "password"))
    {
        password = g_variant_get_string(g_variant_dict_lookup_value(credentials, "password", NULL), NULL);
        g_debug("Password: %s", password);
    }
    else
    {
        return -1;
    }


    if (settings)
    {
        if (g_variant_dict_contains(settings, "eap_method"))
            eapMethod = g_variant_get_string(g_variant_dict_lookup_value(settings, "eap_method", NULL), NULL);
        if (g_variant_dict_contains(settings, "ca_cert_path"))
            caCertPath = g_variant_get_string(g_variant_dict_lookup_value(settings, "ca_cert_path", NULL), NULL);
        if (g_variant_dict_contains(settings, "inner_method"))
            innerMethod = g_variant_get_string(g_variant_dict_lookup_value(settings, "inner_method", NULL), NULL);
        if (g_variant_dict_contains(settings, "connection_name"))
            conName = g_variant_get_string(g_variant_dict_lookup_value(settings, "connection_name", NULL), NULL);
    }
    //Try a common eap setup
    if (!eapMethod)
        eapMethod = "peap";
    if (!caCertPath)
        caCertPath = NULL;
    if (!innerMethod)
        innerMethod = "mschapv2";
    if (!conName)
        conName = "auto";

    if ((username == NULL) || (password == NULL))
        return 1;


#if !GLIB_CHECK_VERSION (2, 35, 0)
    /* Initialize GType system */
    g_type_init ();
#endif
    GVariant *settingsList = getSettingsList();
    GVariantIter iter0;
    g_variant_iter_init(&iter0, settingsList);
    GVariant *settingPath;
    while (( settingPath = g_variant_iter_next_value(&iter0)))
    {

        GVariant *name = getSettingName(g_variant_get_string(settingPath, NULL));
        g_debug("Working on: %s for %s\n",
                g_variant_get_string(settingPath, NULL),
                g_variant_get_string(name, NULL));
        if (g_strcmp0(conName, "auto") && g_strcmp0(conName, g_variant_get_string(name, NULL)))
        {
            g_debug("auto not set and connection doesn't match. Skipping...\n");
            continue;
        }
        GVariant *editedConnection, *setting;
        setting = getSetting(g_variant_get_string(settingPath, NULL));

        GVariantBuilder settingBuilder;
        g_variant_builder_init (&settingBuilder, G_VARIANT_TYPE ("a{sa{sv}}"));
        GVariant *ate021x = NULL;
        GVariantIter iter1;
        g_variant_iter_init (&iter1, setting);
        gchar *key;
        GVariant *value;
        while (g_variant_iter_loop (&iter1, "{&s@a{sv}}", &key, &value))
        {
            if (!g_strcmp0(key, "802-1x"))
            {
                ate021x = g_variant_ref_sink(value);
            }
            else
            {
                g_variant_builder_add_value(&settingBuilder, g_variant_new_dict_entry(g_variant_new_string(key), value));
            }

        }

        GVariantBuilder ate021xSettings;
        g_variant_builder_init(&ate021xSettings, G_VARIANT_TYPE("a{sv}"));
        if (ate021x != NULL)
        {
            GVariant *useLogonCreds = NULL;
            useLogonCreds = g_variant_lookup_value(ate021x, "use-logon-creds", G_VARIANT_TYPE("b"));
            if (useLogonCreds != NULL && g_variant_get_boolean(useLogonCreds))
            {
                g_message("Using logon credentials...\n");
                GVariantIter iter2;
                g_variant_iter_init( &iter2, ate021x );
                while(g_variant_iter_loop (&iter2, "{&s@v}", &key, &value))
                {
                    if (key != "identity" && key != "password")
                        g_variant_builder_add(&ate021xSettings, key, value);
                }
                g_variant_builder_add(&ate021xSettings, "{sv}", "identity", g_variant_new_string(username));
                g_variant_builder_add(&ate021xSettings, "{sv}", "password", g_variant_new_string(password));
            }
            else
            {
                g_debug("802.1x config already exists for %s. Skipping...",
                        g_variant_get_string(settingPath, NULL));
                continue;
            }
        }
        else
        {
            GVariantBuilder peaptypesbuilder;
            g_variant_builder_init(&peaptypesbuilder, G_VARIANT_TYPE("as"));
            g_variant_builder_add(&peaptypesbuilder, "s", eapMethod);
            GVariant *peaptypes = g_variant_builder_end(&peaptypesbuilder);
            g_variant_builder_add(&ate021xSettings, "{sv}", "eap", peaptypes);


            if (caCertPath)
            {
                if (g_str_has_prefix(caCertPath, "file://"))
                {
                    g_variant_builder_add(&ate021xSettings, "{sv}", "ca-cert", g_variant_new_bytestring(caCertPath));
                }
                else
                {
                    g_variant_builder_add(&ate021xSettings, "{sv}", "ca-cert", g_variant_new_bytestring(g_strconcat("file://", caCertPath)));
                }
            }
            g_variant_builder_add(&ate021xSettings, "{sv}", "system-ca-certs", g_variant_new_boolean(TRUE));
            g_variant_builder_add(&ate021xSettings, "{sv}", "phase2-auth", g_variant_new_string(innerMethod));
            g_variant_builder_add(&ate021xSettings, "{sv}", "identity", g_variant_new_string(username));
            g_variant_builder_add(&ate021xSettings, "{sv}", "password", g_variant_new_string(password));
        }

        ate021x = g_variant_builder_end(&ate021xSettings);

        g_variant_builder_add_value(&settingBuilder, g_variant_new_dict_entry(g_variant_new_string("802-1x"), ate021x));

        editedConnection = g_variant_builder_end(&settingBuilder);

        GVariantBuilder tempBuilder;
        g_variant_builder_init(&tempBuilder, G_VARIANT_TYPE("(a{sa{sv}})"));
        g_variant_builder_add_value(&tempBuilder, editedConnection);
        GVariant *temp = g_variant_builder_end(&tempBuilder);


        if (activateSettings(g_variant_get_string(settingPath, NULL), temp))
        {
            g_message ("Settings applied successfully to: %s!\n", g_variant_get_string(settingPath, NULL));
            GDBusProxy *dbus_proxy;
            dbus_proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SYSTEM,
                         G_DBUS_PROXY_FLAGS_NONE,
                         NULL,
                         NM_DBUS_SERVICE,
                         NM_DBUS_PATH_SETTINGS,
                         NM_DBUS_IFACE_SETTINGS,
                         NULL, NULL);
            g_assert (dbus_proxy != NULL);
            GError *err = NULL;
            GVariant *connectionSettings =
                g_dbus_proxy_call_sync(dbus_proxy,
                                       "ReloadConnections", NULL,
                                       G_DBUS_CALL_FLAGS_NONE,
                                       -1, NULL, &err);
            if(connectionSettings == NULL)
                g_error("Error reloading settings: %s\n", err->message);

        }
    }

    return 0;
}

