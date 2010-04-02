/* gtkentrymask - gtkentry with type, mask and i18n
 * Copyright (C) 2001 Santiago Capel
 * Copyright (C) 2003 Damien Létard
 * Copyright (C) 2003-2005 Stéphane Raimbault <stephane.raimbault@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __GTK_ENTRYMASK_H__
#define __GTK_ENTRYMASK_H__

#include <time.h>

#include <gdk/gdk.h>
#include <gtk/gtkentry.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GTK_TYPE_ENTRYMASK              (gtk_entrymask_get_type ())
#define GTK_ENTRYMASK(obj)              (GTK_CHECK_CAST ((obj), GTK_TYPE_ENTRYMASK, GtkEntryMask))
#define GTK_ENTRYMASK_CLASS(klass)      (GTK_CHECK_CLASS_CAST ((klass), GTK_TYPE_ENTRYMASK, GtkEntryMaskClass))
#define GTK_IS_ENTRYMASK(obj)               (GTK_CHECK_TYPE ((obj), GTK_TYPE_ENTRYMASK))
#define GTK_IS_ENTRYMASK_CLASS(klass)       (GTK_CHECK_CLASS_TYPE ((klass), GTK_TYPE_ENTRYMASK))


typedef int GtkEntryMaskType;
#define ENTRYMASK_STRING          0
#define ENTRYMASK_NUMERIC         1
#define ENTRYMASK_CURRENCY        2
#define ENTRYMASK_DATE            3
#define ENTRYMASK_TIME            4
#define ENTRYMASK_DATETIME        5


typedef struct _GtkEntryMask       GtkEntryMask;
typedef struct _GtkEntryMaskClass  GtkEntryMaskClass;

struct _GtkEntryMask
  {
    GtkEntry entry;

    GtkEntryMaskType valuetype;
    const gchar *mask;
    const gchar *format;
    gchar blank_char;
    gint8 formating;
    gint8 masking;
    gint8 forced;
  };

struct _GtkEntryMaskClass
  {
    GtkEntryClass parent_class;
  };


GType                    gtk_entrymask_get_type         (void);
GtkWidget*       gtk_entrymask_new              (void);
GtkWidget*       gtk_entrymask_new_with_type    (GtkEntryMaskType type,
                                                         const gchar *format,
                                                 const gchar *mask);
void             gtk_entrymask_set_text         (GtkEntryMask  *entrymask,
                                                 const gchar   *text);
void             gtk_entrymask_set_text_forced (GtkEntryMask *entrymask,
                                                const gchar *text);
/* returns a reference to the text */
gchar*           gtk_entrymask_get_text         (GtkEntryMask  *entrymask);
void             gtk_entrymask_set_position     (GtkEntryMask  *entrymask,
                                                 gint           position);
void             gtk_entrymask_set_visibility   (GtkEntryMask   *entrymask,
                                                 gboolean       visible);
void             gtk_entrymask_set_editable     (GtkEntryMask   *entrymask,
                                                 gboolean       editable);
/* text is truncated if needed */
void             gtk_entrymask_set_max_length   (GtkEntryMask   *entrymask,
                                                 guint16        max);
void             gtk_entrymask_set_valuetype    (GtkEntryMask   *entrymask,
                                                 GtkEntryMaskType type);
void             gtk_entrymask_set_mask        (GtkEntryMask   *entrymask,
                                                const gchar *mask);
void             gtk_entrymask_set_format      (GtkEntryMask   *entrymask,
                                                const gchar *format);
GtkEntryMaskType gtk_entrymask_get_valuetype   (GtkEntryMask   *entrymask);
const gchar *    gtk_entrymask_get_format       (GtkEntryMask   *entrymask);
const gchar *    gtk_entrymask_get_mask         (GtkEntryMask   *entrymask);
guint16          gtk_entrymask_get_max_length   (GtkEntryMask   *entrymask);

void             gtk_entrymask_set_blank_char   (GtkEntryMask *entrymask,
                                                 const gchar blank_char);
gchar            gtk_entrymask_get_blank_char   (GtkEntryMask *entrymask);
void             gtk_entrymask_set_integer      (GtkEntryMask *entrymask,
                                                 gint val);
gint             gtk_entrymask_get_integer      (GtkEntryMask *entrymask);
void             gtk_entrymask_set_double       (GtkEntryMask *entrymask,
                                                 gdouble val);
gdouble          gtk_entrymask_get_double       (GtkEntryMask *entrymask);
void             gtk_entrymask_set_time_t       (GtkEntryMask *entrymask,
                                                 time_t adate);
time_t           gtk_entrymask_get_time_t       (GtkEntryMask *entrymask);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __GTK_ENTRYMASK_H__ */
