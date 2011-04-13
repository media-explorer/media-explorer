/*
 * Mex - a media explorer
 *
 * Copyright © 2010, 2011 Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses>
 */


#ifndef __MEX_SETTINGS_H__
#define __MEX_SETTINGS_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define MEX_TYPE_SETTINGS mex_settings_get_type()

#define MEX_SETTINGS(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MEX_TYPE_SETTINGS, MexSettings))

#define MEX_SETTINGS_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), MEX_TYPE_SETTINGS, MexSettingsClass))

#define MEX_IS_SETTINGS(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MEX_TYPE_SETTINGS))

#define MEX_IS_SETTINGS_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), MEX_TYPE_SETTINGS))

#define MEX_SETTINGS_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), MEX_TYPE_SETTINGS, MexSettingsClass))

typedef struct _MexSettings MexSettings;
typedef struct _MexSettingsClass MexSettingsClass;
typedef struct _MexSettingsPrivate MexSettingsPrivate;

struct _MexSettings
{
  GObject parent;

  MexSettingsPrivate *priv;
};

struct _MexSettingsClass
{
  GObjectClass parent_class;
};

GType           mex_settings_get_type         (void) G_GNUC_CONST;

MexSettings *   mex_settings_get_default      (void);
const gchar *   mex_settings_get_config_dir   (MexSettings *settings);
gchar *         mex_settings_find_config_file (MexSettings *settings,
                                               const gchar *config_file);

G_END_DECLS

#endif /* __MEX_SETTINGS_H__ */
