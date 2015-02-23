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
main (int argc, char *argv[]) {
 return nmconfig("testing", "testing");
}

int nmconfig(gchar *username, gchar *password) {

	if ((username == NULL) || (password == NULL))
		return 1;
	GDBusProxy *nm_proxy, *nm_active_proxy, *nm_settings_proxy, *nm_settings_connection_proxy;

#if !GLIB_CHECK_VERSION (2, 35, 0)
	/* Initialize GType system */
	g_type_init ();
#endif

	nm_proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SYSTEM,
	                                       G_DBUS_PROXY_FLAGS_NONE,
	                                       NULL,
	                                       NM_DBUS_SERVICE,
	                                       NM_DBUS_PATH,
	                                       NM_DBUS_INTERFACE,
	                                       NULL, NULL);
	g_assert (nm_proxy != NULL);
	GVariant *PrimaryConnectionPath = g_dbus_proxy_get_cached_property (nm_proxy, "PrimaryConnection" );

	nm_active_proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SYSTEM,
                                               G_DBUS_PROXY_FLAGS_NONE,
                                               NULL,
                                               NM_DBUS_SERVICE,
                                               g_variant_get_string(PrimaryConnectionPath, NULL),
                                               NM_DBUS_INTERFACE_ACTIVE_CONNECTION,
                                               NULL, NULL);
	g_assert (nm_active_proxy != NULL);
	GVariant *PrimaryConnectionSettingsPath = g_dbus_proxy_get_cached_property (nm_active_proxy, "Connection");

	nm_settings_proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SYSTEM,
                                               G_DBUS_PROXY_FLAGS_NONE,
                                               NULL,
                                               NM_DBUS_SERVICE,
                                               NM_DBUS_PATH_SETTINGS,
                                               NM_DBUS_IFACE_SETTINGS,
                                               NULL, NULL);
	g_assert (nm_settings_proxy != NULL);
	
	nm_settings_connection_proxy = 
		g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SYSTEM,
                                               G_DBUS_PROXY_FLAGS_NONE,
                                               NULL,
                                               NM_DBUS_SERVICE,
                                               g_variant_get_string(
							PrimaryConnectionSettingsPath, 
							NULL),
                                               NM_DBUS_IFACE_SETTINGS_CONNECTION,
                                               NULL, NULL);
	g_assert (nm_settings_connection_proxy != NULL);

	GError *err = NULL;
	GVariant *ret = NULL;
	ret = g_dbus_proxy_call_sync(nm_settings_proxy,
					"ReloadConnections", NULL,
					G_DBUS_CALL_FLAGS_NONE,
					-1, NULL, &err);
	if (err != NULL)
		g_print( "Error: %s\n", err->message );
	err = NULL;
	ret = NULL;

	GVariant *PrimaryConnectionSettings = 
			g_dbus_proxy_call_sync(nm_settings_connection_proxy, 
						"GetSettings", NULL, 
						G_DBUS_CALL_FLAGS_NONE, 
						-1, NULL, NULL);

	GVariant *editedConnection;

	GVariantBuilder conBuilder;
	g_variant_builder_init (&conBuilder, G_VARIANT_TYPE ("a{sa{sv}}"));

	GVariant *ate021x = NULL;

	GVariantIter iter;
	g_variant_iter_init (&iter, g_variant_get_child_value(
					PrimaryConnectionSettings,0));
	gchar *key;
	GVariant *value;
	while (g_variant_iter_loop (&iter, "{&s@a{sv}}", &key, &value))
	{
		if (!g_strcmp0(key, "802-1x")) {
			ate021x = g_variant_ref_sink(value);
		} else {
			g_variant_builder_add_value(&conBuilder, g_variant_new_dict_entry(g_variant_new_string(key), value));
		}

	}

	GVariantBuilder ate021xSettings;
	if (ate021x != NULL){
		GVariant *useLogonCreds = NULL;
		useLogonCreds = g_variant_lookup_value(ate021x, "use-logon-creds", G_VARIANT_TYPE("b"));
		if (useLogonCreds == NULL || !g_variant_get_boolean(useLogonCreds)) {
			g_print ( "The primary connection is already configured for 802.1x!\n" );
			return 1;
		} else {
			g_print("Using logon credentials...\n");
			g_variant_iter_init( &iter, ate021x );
			while(g_variant_iter_loop (&iter, "{&s@v}", &key, &value)){
				if (key != "identity" && key != "password")
					g_variant_builder_add(&ate021xSettings, key, value);
			}
		}
	} else {
		// Try a common EAP setup
		g_variant_builder_init(&ate021xSettings, G_VARIANT_TYPE("a{sv}"));
		GVariantBuilder peaptypesbuilder;
		g_variant_builder_init(&peaptypesbuilder, G_VARIANT_TYPE("as"));
		g_variant_builder_add(&peaptypesbuilder, "s", "peap");
		GVariant *peaptypes = g_variant_builder_end(&peaptypesbuilder);
		g_variant_builder_add(&ate021xSettings, "{sv}", "eap", peaptypes);
		g_variant_builder_add(&ate021xSettings, "{sv}", "system-ca-certs", g_variant_new_boolean(TRUE));
		g_variant_builder_add(&ate021xSettings, "{sv}", "phase2-auth", g_variant_new_string("mschapv2"));
	}
	g_variant_builder_add(&ate021xSettings, "{sv}", "identity", g_variant_new_string(username));
	g_variant_builder_add(&ate021xSettings, "{sv}", "password", g_variant_new_string(password));
	ate021x = g_variant_builder_end(&ate021xSettings);
	g_variant_builder_add_value(&conBuilder, g_variant_new_dict_entry(g_variant_new_string("802-1x"), ate021x));

	editedConnection = g_variant_builder_end(&conBuilder);


	GVariantBuilder tempBuilder;
	g_variant_builder_init(&tempBuilder, G_VARIANT_TYPE("(a{sa{sv}})"));
	g_variant_builder_add_value(&tempBuilder, editedConnection);
	GVariant *temp = g_variant_builder_end(&tempBuilder);

	ret = g_dbus_proxy_call_sync(nm_settings_connection_proxy,
				"UpdateUnsaved", temp,
				G_DBUS_CALL_FLAGS_NONE,
				-1, NULL, &err);

	if (ret == NULL) {
		g_print ("Settings failed to apply!\nError: %s\n", err->message);
	} else {
		g_print ("Settings applied successfully!\n");

		GVariant *devices = g_dbus_proxy_get_cached_property(nm_active_proxy, "Devices");
		gchar *device;
		g_variant_iter_init(&iter, devices);
		while (g_variant_iter_loop(&iter, "&o", &device)) {
			GVariantBuilder activateConnectionBuilder;
			g_variant_builder_init( &activateConnectionBuilder, G_VARIANT_TYPE("(ooo)"));
			g_variant_builder_add_value(&activateConnectionBuilder, PrimaryConnectionSettingsPath);
			g_variant_builder_add_value(&activateConnectionBuilder, g_variant_new_object_path(device));
			g_variant_builder_add_value(&activateConnectionBuilder, g_variant_new_object_path("/"));

			GVariant *activateConnection = g_variant_builder_end(&activateConnectionBuilder);

		        ret = g_dbus_proxy_call_sync(nm_proxy,
        		                        "ActivateConnection", activateConnection,
        	        	                G_DBUS_CALL_FLAGS_NONE,
                        		        -1, NULL, NULL);
		}

	}

	return 0;
}

