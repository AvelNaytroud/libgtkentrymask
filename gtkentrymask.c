/* gtkentrymask - gtkentry with type, mask and i18n
 * Copyright (C) 2001 Santiago Capel
 * Copyright (C) 2003 Damien Létard
 * Copyright (C) 2003-2005 Stéphane Raimbault <stephane raimbault@gmail.com>
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


#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <locale.h>
#include <langinfo.h>
#include <time.h>
#include <gdk/gdkkeysyms.h>
#include <gdk/gdki18n.h>
#include <gtk/gtkmain.h>
#include <gtk/gtkselection.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkstyle.h>
#include <gtk/gtkeditable.h>
#include <gtk/gtkcontainer.h>

#include "gtkentrymask.h"


#define DEBUG_MSG 0
#if DEBUG_MSG > 0
#define DEBUG_MSG0 g_print
#else
#define DEBUG_MSG0(format, args...)
#endif
#if DEBUG_MSG > 1
#define DEBUG_MSG1 g_print
#else
#define DEBUG_MSG1(format, args...)
#endif
#if DEBUG_MSG > 2
#define DEBUG_MSG2 g_print
#else
#define DEBUG_MSG2(format, args...)
#endif
#if DEBUG_MSG > 3
#define DEBUG_MSG3 g_print
#else
#define DEBUG_MSG3(format, args...)
#endif
#if DEBUG_MSG > 4
#define DEBUG_MSG4 g_print
#else
#define DEBUG_MSG4(format, args...)
#endif


enum {
  PROP_0,
  PROP_MAX_LENGTH,
  PROP_VISIBILITY
};

/* GObject, GtkObject methods
 */
static void   gtk_entrymask_class_init      (GtkEntryMaskClass    *klass);
static void   gtk_entrymask_editable_init   (GtkEditableClass      *iface);
static void   gtk_entrymask_init            (GtkEntryMask         *entrymask);
static void   gtk_entrymask_set_property    (GObject           *object,
                                             guint              prop_id,
                                             const GValue      *value,
                                             GParamSpec        *pspec);
static void   gtk_entrymask_get_property    (GObject         *object,
                                             guint            prop_id,
                                             GValue          *value,
                                             GParamSpec      *pspec);

static void   gtk_entrymask_finalize        (GObject         *object);

/* GtkWidget methods
 */
static gint   gtk_entrymask_focus_in        (GtkWidget *widget,
                                             GdkEventFocus *event);
static gint   gtk_entrymask_focus_out       (GtkWidget *widget,
                                             GdkEventFocus *event);

/* GtkEditable method implementations
 */
static void   gtk_entrymask_insert_text     (GtkEditable       *editable,
                                             const gchar       *new_text,
                                             gint               new_text_length,
                                             gint              *position);
static void   gtk_entrymask_delete_text     (GtkEditable       *editable,
                                             gint               start_pos,
                                             gint               end_pos);
static gchar *gtk_set_locale_number_mask    (gchar *mask,
                                             gchar *grouping,
                                             gint digits);
static gchar *gtk_set_locale_date_mask      (gchar *mask, gint wich_format);
static void   gtk_entrymask_format_text     (GtkEntryMask *entrymask);
static void   gtk_entrymask_format_currency (GtkEntryMask *entrymask);
static void   gtk_entrymask_format_date     (GtkEntryMask *entrymask);
static time_t gtk_entrymask_unformat_date   (GtkEntryMask *entrymask);

/* Internal routines
 */
static gchar *do_mask                        (gint valuetype,
                                              const gchar *mask,
                                              const gchar *origtext,
                                              gint start_pos,
                                              gint *position,
                                              gchar blank_char);


static GtkEntryClass *parent_class = NULL;

/* Internationalization information, obtained with localeconv() */
static GdkWChar lc_decimal_point;
static GdkWChar lc_thousands_sep;
static GdkWChar lc_positive_sign;
static GdkWChar lc_negative_sign;
static GdkWChar lc_mon_decimal_point;
static GdkWChar lc_mon_thousands_sep;
#define MAX_FORMAT_BUF 128
static gchar lc_date_mask[MAX_FORMAT_BUF];
static gchar lc_time_mask[MAX_FORMAT_BUF];
static gchar lc_datetime_mask[MAX_FORMAT_BUF];
static gchar lc_numeric_mask[MAX_FORMAT_BUF];
static gchar lc_currency_mask[MAX_FORMAT_BUF];

#define LC_DECIMAL_POINT ((valuetype==ENTRYMASK_CURRENCY)?lc_mon_decimal_point:lc_decimal_point)
#define LC_THOUSANDS_SEP ((valuetype==ENTRYMASK_CURRENCY)?lc_mon_thousands_sep:lc_thousands_sep)
#define LC_NEGATIVE_SIGN (lc_negative_sign)
#define LC_POSITIVE_SIGN (lc_positive_sign)

#define LITERAL_MASK_CHAR '\\'


/*
 * For information on how to extend an existing widget, see:
 *          http://developer.gnome.org/doc/GGAD/cha-widget.html
 * This widget, gtk_xentrymask, has been extended from the gtk_entry widget,
 * by means, mainly, of replacing gtk_entry to gtk_entrymask.
 * Also, gtk_spin_button has been a good source of inspiration
 */

GType
gtk_entrymask_get_type (void)
{
  static GType entrymask_type = 0;

  if (!entrymask_type)
    {
      static const GTypeInfo entrymask_info =
        {
          sizeof (GtkEntryMaskClass),
          NULL,                 /* base_init */
          NULL,                 /* base_finalize */
          (GClassInitFunc) gtk_entrymask_class_init,
          NULL,                 /* class_finalize */
          NULL,                 /* class_data */
          sizeof (GtkEntryMask),
          0,            /* n_preallocs */
          (GInstanceInitFunc) gtk_entrymask_init,
        };

      static const GInterfaceInfo editable_info =
        {
          (GInterfaceInitFunc) gtk_entrymask_editable_init, /* interface_init */
          NULL, /* interface_finalize */
          NULL  /* interface_data */
        };

      entrymask_type = g_type_register_static (GTK_TYPE_ENTRY, "GtkEntryMask",
                                               &entrymask_info, 0);

      g_type_add_interface_static (entrymask_type,
                                   GTK_TYPE_EDITABLE,
                                   &editable_info);
    }

  return entrymask_type;
}


static void
gtk_entrymask_class_init (GtkEntryMaskClass *class)
{
  GObjectClass   *gobject_class = G_OBJECT_CLASS (class);
  GtkWidgetClass *widget_class;
  GtkEntryClass  *entry_class;

  widget_class = (GtkWidgetClass*) class;
  entry_class = (GtkEntryClass*) class;
  parent_class = g_type_class_peek_parent (class);

  gobject_class->finalize = gtk_entrymask_finalize;
  gobject_class->set_property = gtk_entrymask_set_property;
  gobject_class->get_property = gtk_entrymask_get_property;

  widget_class->focus_in_event = gtk_entrymask_focus_in;
  widget_class->focus_out_event = gtk_entrymask_focus_out;

  /* Get the locale symbols and masks for numbers and dates */
  {
    struct lconv *lc;

    lc = localeconv ();

    if (*(lc->mon_decimal_point) && *(lc->decimal_point))
      {
        lc_mon_decimal_point = *(lc->mon_decimal_point);
        lc_decimal_point = *(lc->decimal_point);
      }
    else if (*(lc->decimal_point))
      {
        lc_decimal_point = *(lc->decimal_point);
        lc_mon_decimal_point = *(lc->decimal_point);
      }
    else if (*(lc->mon_decimal_point))
      {
        lc_decimal_point = *(lc->mon_decimal_point);
        lc_mon_decimal_point = *(lc->mon_decimal_point);
      }
    else
      {
        lc_decimal_point = ',';
        lc_mon_decimal_point = ',';
      }

    if (*(lc->mon_thousands_sep) && *(lc->thousands_sep))
      {
        lc_mon_thousands_sep = *(lc->mon_thousands_sep);
        lc_thousands_sep = *(lc->thousands_sep);
      }
    else if (*(lc->thousands_sep))
      {
        lc_thousands_sep = *(lc->thousands_sep);
        lc_mon_thousands_sep = *(lc->thousands_sep);
      }
    else if (*(lc->mon_thousands_sep))
      {
        lc_thousands_sep = *(lc->mon_thousands_sep);
        lc_mon_thousands_sep = *(lc->mon_thousands_sep);
      }
    else
      {
        lc_thousands_sep = '.';
        lc_mon_thousands_sep = '.';
      }

    if (*(lc->positive_sign))
      lc_positive_sign = *(lc->positive_sign);
    else
      lc_positive_sign = '+';

    if (*(lc->negative_sign))
      lc_negative_sign = *(lc->negative_sign);
    else
      lc_negative_sign = '-';

    gtk_set_locale_date_mask (lc_date_mask, D_FMT);
    gtk_set_locale_date_mask (lc_time_mask, T_FMT);
    strcpy (lc_datetime_mask, lc_date_mask);
    strcat (lc_datetime_mask, " ");
    strcat (lc_datetime_mask, lc_time_mask);
    gtk_set_locale_number_mask (lc_currency_mask,
                                lc->mon_grouping, lc->frac_digits);
    gtk_set_locale_number_mask (lc_numeric_mask,
                                lc->mon_grouping, lc->frac_digits);
  }
}


static void
gtk_entrymask_editable_init (GtkEditableClass *iface)
{
  iface->insert_text = gtk_entrymask_insert_text;
  iface->delete_text = gtk_entrymask_delete_text;
}


/* Init specific to each object of type gtk_entrymask */
static void
gtk_entrymask_init (GtkEntryMask *entrymask)
{
  entrymask->masking = 0;
  entrymask->formating = 0;
  entrymask->forced = 0;
  entrymask->blank_char = '_';
}


/* Finalize specific to each object of type gtk_entrymask
 * This function is an exact copy fo the one in gtk_entry
 */
static void
gtk_entrymask_finalize (GObject *object)
{
  g_return_if_fail (object != NULL);
  g_return_if_fail (GTK_IS_ENTRYMASK (object));

  G_OBJECT_CLASS (parent_class)->finalize (object);
}


/* TODO Add more arguments */
static void
gtk_entrymask_set_property (GObject        *object,
                            guint           prop_id,
                            const GValue   *value,
                            GParamSpec     *pspec)
{
  GtkEntryMask *entrymask;

  entrymask = GTK_ENTRYMASK (object);

  switch (prop_id)
    {
    case PROP_MAX_LENGTH:
      gtk_entrymask_set_max_length (entrymask,
                                    g_value_get_uint(value));
      break;
    case PROP_VISIBILITY:
      gtk_entrymask_set_visibility (entrymask,
                                    g_value_get_boolean(value));
      break;
    default:
      break;
    }
}

static void
gtk_entrymask_get_property (GObject         *object,
                            guint            prop_id,
                            GValue          *value,
                            GParamSpec      *pspec)
{
  GtkEntryMask *entrymask;

  entrymask = GTK_ENTRYMASK (object);

  switch (prop_id)
    {
    case PROP_MAX_LENGTH:
      g_value_set_uint (value, entrymask->entry.text_max_length);
      break;
    case PROP_VISIBILITY:
      g_value_set_boolean (value, gtk_entry_get_visibility(GTK_ENTRY(entrymask)));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

GtkWidget*
gtk_entrymask_new (void)
{
  return GTK_WIDGET (gtk_type_new (GTK_TYPE_ENTRYMASK));
}


/* Create a new gtk_entrymask with a type, a format and a mask */
GtkWidget*
gtk_entrymask_new_with_type (GtkEntryMaskType type,
                             const gchar *format,
                             const gchar *mask)
{
  GtkEntryMask *entrymask;

  entrymask = gtk_type_new (GTK_TYPE_ENTRYMASK);
  entrymask->valuetype = type;
  entrymask->format = format;
  entrymask->mask = mask;

  return GTK_WIDGET (entrymask);
}


/* The following functions just call the parent's implementation */
/* returns a reference to the text */
gchar*
gtk_entrymask_get_text (GtkEntryMask *entrymask)
{
  return (char*)gtk_entry_get_text (GTK_ENTRY(entrymask));
}

/* Generic set_text function */
void
gtk_entrymask_set_text (GtkEntryMask *entrymask,
                        const gchar *text)
{
  DEBUG_MSG3("sta: set_text(%s)\n", text);
  if (!GTK_WIDGET_HAS_FOCUS(entrymask) && !entrymask->formating)
    {
      gtk_entry_set_text (GTK_ENTRY (entrymask), text);
      gtk_entrymask_format_text (entrymask);
    }
  else
    {
      gtk_entry_set_text (GTK_ENTRY (entrymask), text);
    }
  DEBUG_MSG3("end: set_text(%s)\n", text);
}

void
gtk_entrymask_set_text_forced (GtkEntryMask *entrymask,
                               const gchar *text)
{
  entrymask->forced++;
  gtk_entry_set_text (GTK_ENTRY (entrymask), text);
  entrymask->forced--;
}

void
gtk_entrymask_set_position (GtkEntryMask *entrymask,
                            gint position)
{
  gtk_entry_set_position (GTK_ENTRY (entrymask), position);
}

void
gtk_entrymask_set_visibility (GtkEntryMask *entrymask,
                              gboolean visible)
{
  gtk_entry_set_visibility (GTK_ENTRY (entrymask), visible);
}

void
gtk_entrymask_set_editable(GtkEntryMask *entrymask,
                           gboolean  editable)
{
  gtk_entry_set_editable (GTK_ENTRY (entrymask), editable);
}

guint16
gtk_entrymask_get_max_length (GtkEntryMask *entrymask)
{
  g_return_val_if_fail (entrymask != NULL, 0);
  g_return_val_if_fail (GTK_IS_ENTRYMASK (entrymask),0);

  return entrymask->entry.text_max_length;
}

void
gtk_entrymask_set_max_length (GtkEntryMask *entrymask,
                              guint16       max)
{
  gtk_entry_set_max_length (GTK_ENTRY (entrymask), max);
}


/* This functions set and get values specific to gtk_entrymask */

void
gtk_entrymask_set_valuetype (GtkEntryMask   *entrymask,
                             GtkEntryMaskType type)
{
  g_return_if_fail (entrymask != NULL);
  g_return_if_fail (GTK_IS_ENTRYMASK (entrymask));
  entrymask->valuetype = type;
}

GtkEntryMaskType
gtk_entrymask_get_valuetype (GtkEntryMask   *entrymask)
{
  g_return_val_if_fail (entrymask != NULL, 0);
  g_return_val_if_fail (GTK_IS_ENTRYMASK (entrymask), 0);

  return entrymask->valuetype;
}

void
gtk_entrymask_set_mask (GtkEntryMask   *entrymask,
                        const gchar *mask)
{
  g_return_if_fail (entrymask != NULL);
  g_return_if_fail (GTK_IS_ENTRYMASK (entrymask));

  entrymask->mask = mask;
}

const gchar *
gtk_entrymask_get_mask (GtkEntryMask   *entrymask)
{
  g_return_val_if_fail (entrymask != NULL, NULL);
  g_return_val_if_fail (GTK_IS_ENTRYMASK (entrymask), NULL);

  return entrymask->mask;
}

void
gtk_entrymask_set_format (GtkEntryMask   *entrymask,
                          const gchar *format)
{
  g_return_if_fail (entrymask != NULL);
  g_return_if_fail (GTK_IS_ENTRYMASK (entrymask));

  entrymask->format = format;
}

const gchar *
gtk_entrymask_get_format (GtkEntryMask   *entrymask)
{
  g_return_val_if_fail (entrymask != NULL, NULL);
  g_return_val_if_fail (GTK_IS_ENTRYMASK (entrymask), NULL);

  return entrymask->format;
}

void
gtk_entrymask_set_blank_char (GtkEntryMask *entrymask,
                              const gchar blank_char)
{
  entrymask->blank_char = blank_char;
}

gchar
gtk_entrymask_get_blank_char (GtkEntryMask *entrymask)
{
  return entrymask->blank_char;
}

/* The following functions set/get the text of the gtk_entrymask
   in various formats */

/* As integer */
void
gtk_entrymask_set_integer(GtkEntryMask *entrymask,
                          gint val)
{
  gchar buffer[32];

  sprintf(buffer, "%d", val);
  gtk_entrymask_set_text(entrymask, buffer);
}

gint gtk_entrymask_get_integer(GtkEntryMask *entrymask)
{
  return atoi(gtk_entrymask_get_text(entrymask));
}

/* As double */
void gtk_entrymask_set_double(GtkEntryMask *entrymask, gdouble val)
{
  gchar buffer[128], *pbuffer=buffer;
  GtkEntryMaskType valuetype = entrymask->valuetype;

  sprintf (buffer, "%f", val);
  /* if the locale is not set, we don't know which decimal point sprintf will use */
  while (*pbuffer) {
    if (*pbuffer == ',' || *pbuffer == '.') {
      *pbuffer = LC_DECIMAL_POINT;
    }
    pbuffer++;
  }
  gtk_entrymask_set_text (entrymask, buffer);
}

gdouble
gtk_entrymask_get_double(GtkEntryMask *entrymask)
{
  gchar buffer[128], *pbuffer=buffer, *ptext;
  GtkEntryMaskType valuetype = entrymask->valuetype;

  /* Remove invalid characters */
  ptext = gtk_entrymask_get_text (entrymask);
  while (*ptext) {
    if (isdigit (*ptext)
        || *ptext == LC_DECIMAL_POINT
        || *ptext == LC_NEGATIVE_SIGN)
      *pbuffer++ = *ptext;
    ptext++;
  }
  *pbuffer = '\0';

  return atof(buffer);
}


/* As time_t */
/* Problem: time_t ranges from 13-dec-1901 to 19-jan-2038 */
void
gtk_entrymask_set_time_t(GtkEntryMask *entrymask, time_t adate)
{
  const gchar *pmask;
  gchar buffer[MAX_FORMAT_BUF]="", itemformat[]="%0_d";
  gint l, days, months, years, hours, minutes, seconds;
  struct tm lt = *localtime (&adate);
  gint module=1;

  days=months=years=hours=minutes=seconds=0;
  if (entrymask->mask == NULL || !(*entrymask->mask))
    {
      switch (entrymask->valuetype)
        {
        case ENTRYMASK_DATE:
          pmask = lc_date_mask;
          break;
        case ENTRYMASK_TIME:
          pmask = lc_time_mask;
          break;
        default: /* ENTRYMASK_DATETIME */
          pmask = lc_datetime_mask;
        }
    }
  else
    pmask = entrymask->mask;
  pmask--;

  do
    {
      pmask++;
      switch (*pmask) {
      case 'd':
      case 'D':
        days++;
        break;
      case 'm':
        months++;
        break;
      case 'y':
      case 'Y':
        years++;
        module*=10;
        break;
      case 'h':
      case 'H':
        hours++;
        break;
      case 'M':
        minutes++;
        break;
      case 's':
      case 'S':
        seconds++;
        break;
      default:
        if (days) {
          itemformat[2] = (gchar)days + '0';
          sprintf(buffer+strlen(buffer), itemformat, lt.tm_mday);
          days = 0;
        }

        if (months) {
          itemformat[2] = (gchar)months + '0';
          sprintf(buffer+strlen(buffer), itemformat, lt.tm_mon+1);
          months = 0;
        }

        if (years) {
          itemformat[2] = (gchar)years + '0';
          sprintf(buffer+strlen(buffer), itemformat, (lt.tm_year + 1900)%module);
          years = 0;
        }

        if (hours) {
          itemformat[2] = (gchar)hours + '0';
          sprintf(buffer+strlen(buffer), itemformat, lt.tm_hour);
          hours = 0;
        }

        if (minutes) {
          itemformat[2] = (gchar)minutes + '0';
          sprintf(buffer+strlen(buffer), itemformat, lt.tm_min);
          minutes = 0;
        }

        if (seconds) {
          itemformat[2] = (gchar)seconds + '0';
          sprintf(buffer+strlen(buffer), itemformat, lt.tm_sec);
          seconds = 0;
        }
        l = strlen(buffer);
        buffer[l] = *pmask;
        buffer[l+1] = '\0';
      }
    } while (*pmask);

  gtk_entrymask_set_text(entrymask, buffer);
}

time_t
gtk_entrymask_get_time_t(GtkEntryMask *entrymask)
{
  const gchar *pmask;
  gchar *ptext;
  struct tm lt = {0};

  if (entrymask->mask == NULL || !(*entrymask->mask))
    {
      switch (entrymask->valuetype)
        {
        case ENTRYMASK_DATE:
          pmask = lc_date_mask;
          break;
        case ENTRYMASK_TIME:
          pmask = lc_time_mask;
          break;
        default: /* ENTRYMASK_DATETIME */
          pmask = lc_datetime_mask;
        }
    }
  else
    pmask = entrymask->mask;
  ptext = gtk_entrymask_get_text (entrymask);

  while (*pmask && *ptext)
    {
      switch (*pmask)
        {
        case 'd':
        case 'D':
          lt.tm_mday = lt.tm_mday * 10 + (gint)((*ptext) - '0');
          break;
        case 'm':
          lt.tm_mon = lt.tm_mon * 10 + (gint)(*ptext - '0');
          break;
        case 'y':
        case 'Y':
          lt.tm_year = lt.tm_year * 10 + (gint)(*ptext - '0');
          break;
        case 'h':
        case 'H':
          lt.tm_hour = lt.tm_hour * 10 + (gint)((*ptext) - '0');
          break;
        case 'M':
          lt.tm_min = lt.tm_min * 10 + (gint)((*ptext) - '0');
          break;
        case 's':
        case 'S':
          lt.tm_sec = lt.tm_sec * 10 + (gint)((*ptext) - '0');
          break;
        }
      pmask++;
      ptext++;
    }

  if (lt.tm_mon > 0)
    lt.tm_mon--;

  if (lt.tm_year == 0) /* Just time, nor date */
    lt.tm_year = 70;
  else if (lt.tm_year < 30)
    lt.tm_year += 100;
  else if (lt.tm_year > 100)
    lt.tm_year -= 1900;

  return mktime(&lt);
}


/* Formats the text of an entry, usally after focus_out */
static void
gtk_entrymask_format_text (GtkEntryMask *entrymask)
{
  GtkEntryMaskType valuetype;
  const gchar *format, *text;

  text = gtk_entrymask_get_text(entrymask);
  DEBUG_MSG3("sta: format_text(%s)\n", text);
  format = entrymask->format;
  entrymask->formating++;

  if (format && *format)
    {
      switch (valuetype = entrymask->valuetype)
        {
        case ENTRYMASK_DATE:
        case ENTRYMASK_TIME:
        case ENTRYMASK_DATETIME:
          gtk_entrymask_format_date(entrymask);
          break;
        case ENTRYMASK_NUMERIC:
        case ENTRYMASK_CURRENCY:
          if (strchr(format, '$')
              || valuetype == ENTRYMASK_CURRENCY)
            {
              gtk_entrymask_format_currency(entrymask);
            }

          if (strchr(format, 'B')
              && gtk_entrymask_get_double(entrymask) == 0.0)
            {
              gtk_entrymask_set_text_forced(entrymask,"");
            }
          break;

        default:
          if (strchr(format, '<'))
            {
              g_strup(gtk_entrymask_get_text(entrymask));
              gtk_entrymask_set_text_forced(entrymask, gtk_entrymask_get_text(entrymask));
            }
          else if (strchr(format, '>'))
            g_strdown(gtk_entrymask_get_text(entrymask));

          gtk_entrymask_set_text_forced(entrymask, gtk_entrymask_get_text(entrymask));
        }
    }

  DEBUG_MSG3("end: format_text(%s)\n", gtk_entrymask_get_text(entrymask));
  entrymask->formating--;
}


/*****************************************************************************
 * Processing of numeric masks
 *
 * #,9 -> Digit. If the first character is zero, it is replaced with a blank
 * 0 -> Digit. If the first character is zero, the zero is remained
 * . -> Decimal point position
 * , -> Thousands separator
 * + -> The number doesn't admit the negative sign
 * * -> Repeat the mask pattern
 *
 ****************************************************************************/

/* Check whether a character is valid for a mask */
/* Valuetype can be ENTRYMASK_NUMERIC and ENTRYMASK_CURRENCY */
static int masknum_match(GtkEntryMaskType valuetype, char text, char mask)
{
  switch (mask)
    {
    case '0':
      return isdigit(text);
    case '#':
    case '9':
      return isdigit(text) || text==' ';
    case ',':
      return text == LC_THOUSANDS_SEP;
    case '.':
      return text == LC_DECIMAL_POINT;
    default:
      return(text == mask);
    }
}

/* Returns the default character for a mask character */
static char masknum_defaultchar(GtkEntryMaskType valuetype, char mask)
{
  switch(mask) {
  case '#':
  case '9':
    return '0';
  case ',':
    return LC_THOUSANDS_SEP;
  case '.':
    return LC_DECIMAL_POINT;
  default:
    return mask;
  }
}

/* Repeats the mask pattern up to a given length */
gchar *masknum_dupmask(gchar *start, gchar *end, gint maxlen)
{
  gchar *start_save, *pend;

  DEBUG_MSG2("masknum_dupmask(start='%s', end='%s', maxlen=%d\n", start, end, maxlen);

  start_save = start;
  if (end < start)
    {
      start = end;
      end = start_save;
      start_save = start;
    }
  else if (end == start)
    {
      *start = '#';
      end++;
      maxlen--;
    }
  pend = end;

  while (maxlen > 0)
    {
      start = start_save;
      while (start < end)
        {
          *pend++ = *start++;
          maxlen--;
        }
    }
  DEBUG_MSG2("masknum_dupmask end:'%s'\n", start_save);

  return pend;
}

/* Formats a numeric text according to a mask.
 * Tracks the position of the cursor.
 */
static gchar *masknum_mask(GtkEntryMaskType valuetype,
                           const char *origmask,
                           const char *origtext,
                           int startpos, int *position)
{
  const gchar *porig;
  gchar mask[MAX_FORMAT_BUF], *pmask, *puntomask;
  gchar text[MAX_FORMAT_BUF], *ptext, *puntotext;
  gchar entera[MAX_FORMAT_BUF], *pentera;
  gchar decimal[MAX_FORMAT_BUF], *pdecimal;
  gchar *result;
  int yahaypunto, enelpunto, pospunto, longinsert, aborrarlen;
  int bneg, ltext, lmask, pos, decpos;
  int quitandoceros, repdec, repent, sign, ndec, borrando, dupmask;

  DEBUG_MSG3("\nmasknum_mask(text='%s' mask='%s' position=%d startpos=%d\n",
             origtext, origmask, *position, startpos);

  borrando = (startpos == *position);
  repdec = 0;
  repent = 0;
  ndec=0;
  sign = 1;
  DEBUG_MSG2("Procesar mascara original:'%s'\n", origmask);
  puntomask = NULL;
  pmask = mask;
  lmask = 0;
  dupmask=0;

  while (*origmask)
    {
      if (*origmask == '*' && puntomask == NULL)
        repent = 1;
      else if (*origmask == '*' && puntomask != NULL)
        {
          repdec = 1;
          /* Duplicar mascara hasta unos 8 decimales */
          if (*(pmask-1) == '.')
            pmask = masknum_dupmask(pmask, pmask, 8);
          else
            pmask = masknum_dupmask(pmask-1, pmask, 8 - (pmask - puntomask) + 1);
          lmask = strlen(mask);
        }

      else if (*origmask == '+')
        sign = 0;
      else if (*origmask == '.')
        {
          if (repent && !dupmask)
            {
              /* Duplicar mascara hasta unos 20 digitos enteros */
              pmask = masknum_dupmask(mask, pmask, 20);
              *pmask = '\0';
              lmask = strlen(mask);
              DEBUG_MSG2("Creada mascara entera:'%s', longitud:%d\n", mask, lmask);
              dupmask = 1;
            }
          puntomask = pmask;
          *pmask++ = *origmask;
          lmask++;
        }
      else
        {
          if (puntomask != NULL)
            ndec++;
          *pmask++ = *origmask;
          lmask++;
        }
      origmask++;
    }

  if (repent && !dupmask)
    {
      /* No ha encontrado un punto */
      pmask = masknum_dupmask(mask, pmask, 20);
      *pmask='\0';
      lmask = strlen(mask);
    }
  *pmask = '\0';
  DEBUG_MSG2("Procesada mascara original:'%s', longitud:%d\n", mask, lmask);

  result = g_malloc (MAX_FORMAT_BUF);
  longinsert = *position - startpos;
  puntotext = strchr (origtext, LC_DECIMAL_POINT);

  if (puntotext == NULL)
    pospunto = strlen (origtext);
  else
    pospunto = puntotext - origtext;
  enelpunto = *position - pospunto;
  porig = origtext;
  DEBUG_MSG2("enelpunto=%d, longinsert=%d\n", enelpunto, longinsert);

  if (enelpunto != 0 &&
      !(longinsert == strlen(origtext) &&
        startpos == 0))
    {
      /* Sobreescribir, si no es un set_text */
      porig = origtext;
      ptext = text;
      pos = 0;
      aborrarlen = longinsert;
      DEBUG_MSG2("Sobreescribir:*position=%d, longinsert=%d\n",
                 *position, longinsert);

      while (*porig)
        {
          if (pos >= startpos && pos < *position)
            {
              /* No sobreescribir si el caracter no va a ser valido */
              if ((*porig == LC_THOUSANDS_SEP)
                  || (*porig == LC_DECIMAL_POINT
                      && *(porig+1) != LC_DECIMAL_POINT))
                {
                  DEBUG_MSG3("no copio pos=%d, *porig=%c. Es una coma o punto\n", pos, *porig);
                  *position -= 1;
                  aborrarlen--;
                }
              else if (*porig != LC_DECIMAL_POINT
                       && *porig != LC_THOUSANDS_SEP
                       && mask[pos]!=','
                       && !masknum_match(valuetype, *porig, mask[pos]))
                {
                  DEBUG_MSG3("no copio pos=%d, *porig=%c. No es char valido\n", pos, *porig);
                  aborrarlen--;
                  *position -= 1;
                }
              else
                {
                  DEBUG_MSG3("copio pos=%d, *porig=%c\n", pos, *porig);
                  *ptext++ = *porig;
                }
            }
          else if ((pos >= *position && pos < *position + aborrarlen))
            {
              DEBUG_MSG3("elimino pos=%d, *porig=%c\n", pos, *porig);
              if (*porig == LC_THOUSANDS_SEP)
                {
                  aborrarlen++;
                }
            }
          else
            {
              DEBUG_MSG3("copio pos=%d, *porig=%c\n", pos, *porig);
              *ptext++ = *porig;
            }
          pos++;
          porig++;
        }
      *ptext='\0';
      DEBUG_MSG2("Sobreescrito: %s\n", text);
      porig = text;
    }

  decpos = 0;
  bneg = 0;
  yahaypunto = 0;
  quitandoceros = 1;
  DEBUG_MSG2("Eliminar caracteres no validos del texto\n");
  ptext = text;
  pos = 0;

  while (*porig)
    {
      if (isdigit(*porig) && !(*porig == '0' && quitandoceros))
        {
          *ptext++ = *porig;
          quitandoceros=0;
        }
      else
        {
#if 0
          /* Esto no hace falta porque ya hemos sobreescrito */
          /* Se ha pulsado . en vez del punto decimal local */
          if (*porig == '.'
              && (*(porig+1) == LC_DECIMAL_POINT /*|| *(porig+1)=='\0'*/))
            {
              *position+=1;
            }
          else
#endif
            if (*porig == LC_DECIMAL_POINT)
              {
                if (yahaypunto == 0)
                  {
                    yahaypunto = 1;
                    *ptext++ = LC_DECIMAL_POINT;
                    quitandoceros = 0;
                  }
              }
            else if (*porig == LC_NEGATIVE_SIGN)
              {
                bneg = !bneg;
                *position-=1;
              } else if (*porig == LC_POSITIVE_SIGN)
                {
                  bneg = 0;
                  *position-=1;
                }
            else
              {
                *position-=1;
              }
        }
      porig++;
      pos++;
    }
  *ptext='\0';
  DEBUG_MSG2("Texto limpio='%s', position=%d\n", text, *position);

  if (!mask || !*mask)
    {
      if (bneg && sign)
        {
          *result = LC_NEGATIVE_SIGN;
          *position+=1;
          strncpy(result + 1, text, MAX_FORMAT_BUF-1);
        }
      else
        strncpy(result, text, MAX_FORMAT_BUF-1);

      result[MAX_FORMAT_BUF-1]='\0';
      DEBUG_MSG2("Mascara=Null, result='%s'\n", result);

      return result;
    }

  ltext = strlen(text);
  DEBUG_MSG3("ltext=%d, lmask=%d\n",ltext,lmask);
  puntotext = strchr(text, LC_DECIMAL_POINT);
  puntomask = strchr(mask, '.');
  DEBUG_MSG2("puntotext='%s', puntomask='%s'\n",puntotext,puntomask);
  pdecimal = decimal;

  if (puntomask)
    {
      DEBUG_MSG2("Formatear parte decimal\n");
      if (!puntotext)
        if (borrando)
          {
            ptext = text + *position;
          }
        else
          {
            ptext = text + ltext - ((ltext == longinsert)?0:ndec);
          }
      else
        ptext = puntotext + 1;
      pmask = puntomask + 1;
      *pdecimal++ = LC_DECIMAL_POINT;

      while (*ptext && *pmask)
        {
          DEBUG_MSG2("match(ptext='%s' pmask='%s')\n", ptext, pmask);
          if (masknum_match(valuetype, *ptext, *pmask))
            {
              DEBUG_MSG3("Match!\n");
              *pdecimal++ = *ptext++;
              if (*pmask)
                pmask++;
            }
          else if (*pmask == ',')
            {
              DEBUG_MSG3("Raro que haya comas en parte decimal\n");
              *pdecimal++ = LC_THOUSANDS_SEP;
              pmask++;
            }
          else
            {
              ptext++;
            }
          DEBUG_MSG3("creando parte decimal='%s'\n", decimal);
        }

      while (*pmask)
        {
          if (*pmask == '#')
            pmask++;
          else if (!repdec)
            *pdecimal++ = masknum_defaultchar(valuetype, *pmask++);
        }
    }
  *pdecimal = '\0';
  DEBUG_MSG2("decimal='%s'\n", decimal);

  DEBUG_MSG2("Formatear parte entera\n");
  DEBUG_MSG2("puntotext='%s', puntomask='%s'\n",puntotext,puntomask);
  pentera = entera + MAX_FORMAT_BUF-1;
  *pentera--= '\0';

  if (!puntomask) {
    pmask = mask + lmask - 1;
  } else {
    pmask = puntomask - 1;
  }

  if (!puntotext)
    if (borrando) {
      ptext = text + *position - 1;
    } else {
      ptext = text + ltext - ((ltext == longinsert)?0:ndec) - 1;
    }
  else
    ptext = puntotext - 1;
  DEBUG_MSG3("pmask='%s' ptext='%s'\n", pmask, ptext);

  while (ptext >= text && pmask >= mask && pentera > entera)
    {
      DEBUG_MSG3("match(ptext='%s' pmask='%s')\n", ptext, pmask);
      if (masknum_match(valuetype, *ptext, *pmask))
        {
          DEBUG_MSG3("Match!\n");
          *pentera-- = *ptext--;
          pmask--;
        }
      else if (*pmask == ',')
        {
          *pentera-- = LC_THOUSANDS_SEP;
          pmask--;
          *position+=1;
        }
      else
        {
          ptext--;
        }
      DEBUG_MSG3("entera='%s' position=%d\n", pentera + 1, *position);
    }

  DEBUG_MSG2("Anadir lo que queda de mascara\n");
  while (pmask >= mask && pentera > entera) {
    if (*pmask == '0')
      {
        *pentera-- = '0';
      }
    else if (*pmask == ',' && *(pmask-1) == '0')
      {
        *pentera-- = LC_THOUSANDS_SEP;
      }
    else if (!repent)
      {
        *pentera-- = ' ';
      }
    else
      {
        *position -= 1;
      }
    pmask--;
    *position+=1;
  }
  DEBUG_MSG2("entera='%s' position=%d\n", pentera + 1, *position);
  DEBUG_MSG2("Caracteres sobrantes de text\n");

  while (ptext >= text) {
    ptext--;
    *position-=1;
  }
  DEBUG_MSG2("entera='%s' position=%d\n", pentera + 1, *position);
  pentera++;

  if (*pentera == LC_THOUSANDS_SEP) {
    *position-=1;
    pentera++;
    DEBUG_MSG3("quitando coma inicial: position=%d\n", *position);
  }

  if (bneg && sign) {
    *(--pentera) = LC_NEGATIVE_SIGN;
    *position += 1;
  }

  strncpy(result, pentera, MAX_FORMAT_BUF);
  result[MAX_FORMAT_BUF-1] = '\0';
  strncat(result, decimal, MAX_FORMAT_BUF);
  result[MAX_FORMAT_BUF-1] = '\0';

  if (*position < 0)
    *position = 0;

  if (result[*position] == ' ')
    {
      *position += 1;
      while (result[*position] == ' ')
        *position += 1;
    }

  if (*position > strlen(result))
    *position = strlen(result);

  DEBUG_MSG1("Exit:masknum_mask:result='%s' position=%d\n\n", result, *position);

  return result;
}


/*****************************************************************************
 * Processing of date and string masks
 *
 * General:
 * # -> Digit, space, + or -
 * 9 -> Digit or space
 * 0 -> Digit or zero
 * A -> Alphanumeric character
 *
 * Dates and times:
 *
 * 0   -> Digit or zero
 * d,D -> Day of the month
 * m   -> Month
 * y,Y -> Year
 * h,H -> Hour
 * M   -> Minutes
 * s,S -> Seconds
 *
 * \   -> The next character is treated as literal
 *
 * Formats:
 * Numeric:
 *     $   Currency symbol
 *     B   Blank if zero
 * Dates:
 *     same as strftime
 *
 ****************************************************************************/


/* Returns the length of a mask */
static gint maskalfa_len(const char *pmask)
{
  gint len=0;

  while (*pmask)
    {
      if (*pmask++ == LITERAL_MASK_CHAR)
        pmask++;
      len++;
    }

  return len;
}

/* Returns the default character for a mask character */
static char maskalfa_defaultchar(GtkEntryMaskType valuetype,
                                 const char *pmask,
                                 gint pos, gchar blank_char)
{
  gint inbackslash;

  pmask--;
  do {
    pmask++;
    if (*pmask == LITERAL_MASK_CHAR)
      {
        inbackslash=1;
        pmask++;
      }
    else
      inbackslash=0;
    pos--;
  } while (*pmask && pos>=0);

  if (inbackslash)
    return *pmask;
  else switch(*pmask)
    {
    case 'd':
    case 'D':
    case 'y':
    case 'Y':
    case 'h':
    case 'H':
    case 'm':
    case 'M':
    case 's':
    case 'S':
      if (valuetype == ENTRYMASK_DATE
          || valuetype == ENTRYMASK_TIME
          || valuetype == ENTRYMASK_DATETIME)
        return '0';
      else
        return *pmask;
    case '0':
      return blank_char;
    case '9':
    case '#':
    case 'A':
    case '>':
    case '<':
      return blank_char;
    default:
      return *pmask;
    }
}

/* Check whether a character is valid for a mask */
static int maskalfa_match(GtkEntryMaskType valuetype,
                          char text, const char *pmask,
                          gint pos, gchar blank_char)
{
  gint inbackslash;

  pmask--;
  do
    {
      pmask++;
      if (*pmask == LITERAL_MASK_CHAR) {
        inbackslash = 1;
        pmask++;
      } else
        inbackslash=0;
      pos--;
    } while (*pmask && pos>=0);

  if (inbackslash)
    return (text == *pmask);
  else
    {
      if (text == maskalfa_defaultchar(valuetype, pmask, 0, blank_char))
        return 1;
      else switch (*pmask)
        {
        case 'd':
        case 'D':
        case 'm':
        case 'M':
        case 'y':
        case 'Y':
        case 'h':
        case 'H':
        case 's':
        case 'S':
          if (valuetype == ENTRYMASK_DATE
              || valuetype == ENTRYMASK_TIME
              || valuetype == ENTRYMASK_DATETIME)
            return isdigit(text);
          else
            return (text == *pmask);
        case '9':
          return isdigit(text) || text == ' ';
        case '#':
          return isdigit(text) || text == ' '
            || text == LC_POSITIVE_SIGN || text == LC_NEGATIVE_SIGN;
        case '0':
          return isdigit(text);
        case '>':
        case '<':
        case 'A':
          return isalpha(text);
        default:
          return (text == *pmask);
        }
    }
}

/* gets the nth character in a mask, if it is not literal */
static gchar
maskalfa_nth(GtkEntryMaskType valuetype, const char *pmask, gint pos)
{
  gint inbackslash;

  pmask--;
  do
    {
      pmask++;
      if (*pmask == LITERAL_MASK_CHAR)
        {
          inbackslash=1;
          pmask++;
        }
      else
        inbackslash=0;
      pos--;
    }
  while(*pmask && pos >= 0);

  if (inbackslash)
    return '\0';
  else
    return *pmask;
}


/* Returns whether a character in a mask is editable */
static int
maskalfa_iseditable(GtkEntryMaskType valuetype, const char *pmask, gint pos)
{
  gint inbackslash;

  pmask--;
  do
    {
      pmask++;
      if (*pmask == LITERAL_MASK_CHAR)
        {
          inbackslash=1;
          pmask++;
        }
      else
        inbackslash=0;
      pos--;
    }
  while (*pmask && pos>=0);

  if (inbackslash)
    return 0;
  else switch (*pmask)
    {
    case 'd':
    case 'D':
    case 'm':
    case 'M':
    case 'y':
    case 'Y':
    case 'h':
    case 'H':
    case 's':
    case 'S':
      if (valuetype == ENTRYMASK_DATE
          || valuetype == ENTRYMASK_TIME
          || valuetype == ENTRYMASK_DATETIME)
        return 1;
      else
        return 0;
    case '9':
    case '#':
    case '0':
    case 'A':
    case '<':
    case '>':
      return 1;
    default:
      return 0;
    }
}

/* Returns the first editable character in a mask */
static gint maskalfa_firsteditable(GtkEntryMaskType valuetype, const char *pmask)
{
  gint pos=0;
  while (!maskalfa_iseditable(valuetype, pmask, pos) && pos < strlen(pmask))
    pos++;

  return pos;
}

/* Returns the previous editable character in a mask */
static gint maskalfa_previouseditable(GtkEntryMaskType valuetype, const char *pmask, gint pos)
{
  while (!maskalfa_iseditable(valuetype, pmask, pos) && pos)
    pos--;

  return pos;
}

/* Returns the next editable character in a mask */
static gint maskalfa_nexteditable(GtkEntryMaskType valuetype, const char *pmask, gint pos)
{
  while (!maskalfa_iseditable(valuetype, pmask, pos) && pos < strlen(pmask))
    pos++;

  return pos;
}

/* Returns the last editable character in a mask */
static gint maskalfa_lasteditable(GtkEntryMaskType valuetype, const char *pmask)
{
  gint pos = strlen(pmask);
  while (!maskalfa_iseditable(valuetype, pmask, pos) && pos)
    pos--;

  return pos;
}

/* Formats a text according to its type and mask(string, date and time) */
static gchar *maskalfa_mask(GtkEntryMaskType valuetype, const char *mask,
                            const char *origtext, int startpos,
                            int *position, gchar blank_char)
{
  const char *porig;
  gchar *text, *ptext;
  gint postext, posmask, borrandomask, borrando, endpos, masklen;
  gchar *result, *presult;

  DEBUG_MSG1("\nmaskalfa_mask(text='%s' mask='%s' position=%d startpos=%d\n", origtext, mask, *position, startpos);

  text = g_malloc (MAX_FORMAT_BUF);
  result = g_malloc (MAX_FORMAT_BUF);
  presult = result;
  borrandomask = (startpos == *position && !maskalfa_iseditable (valuetype, mask,startpos));
  borrando = (startpos == *position);
  DEBUG_MSG1("borrando=%d, borrandomask=%d, mascara:%c\n",
             borrando, borrandomask, mask[startpos]);
  porig = origtext;
  ptext = text;
  postext=0;
  if (borrando)
    { /* desplazar a la izda solo caracteres sin mascara */
      DEBUG_MSG1("Borrando\n");
      while (*porig && postext < startpos)
        {
          *ptext++ = *porig++;
          postext++;
        }
      *ptext='\0';
      DEBUG_MSG2("texto limpio='%s'\n", text);
      masklen = maskalfa_len (mask);
      posmask = masklen - (strlen(origtext) - postext);
      DEBUG_MSG2("postext=%d, posmask=%d, masklen='%d'\n", postext, posmask, masklen);
      while (*porig)
        {
          if (maskalfa_iseditable (valuetype, mask, posmask))
            *ptext++ = *porig++;
          else
            porig++;
          posmask++;
        }
      *ptext='\0';
      ptext = text;
    }
  else
    {
      /* sobreescribir */
      DEBUG_MSG1("Sobreescribiendo\n");
      endpos = *position + (*position - startpos);
      while (*porig) {
        /* No sobreescribir si el caracter no va a ser valido */
        if (postext >= startpos && postext < *position)
          {
            if (!maskalfa_match (valuetype, *porig, mask, postext, blank_char))
              {
                DEBUG_MSG3("no copio pos=%d, *porig=%c. No match\n", postext, *porig);
                *position -= 1;
                endpos--;
              }
            else
              {
                DEBUG_MSG3("copio pos=%d, *porig=%c\n", postext, *porig);
                *ptext++ = *porig;
              }
          }
        else if (postext >= *position && postext < endpos)
          {
            DEBUG_MSG3("no copio pos=%d, *porig=%c. sobreescribo\n", postext, *porig);
          }
        else {
          DEBUG_MSG3("copio pos=%d, *porig=%c\n", postext, *porig);
          *ptext++ = *porig;
        }
        porig++;
        postext++;
      }
      *ptext = '\0';
      ptext = text;
    }
  DEBUG_MSG2("Texto limpio='%s', position=%d\n", text, *position);

  postext=posmask=0;
  while (*ptext && maskalfa_defaultchar (valuetype, mask, posmask, blank_char))
    {
      DEBUG_MSG3("match(ptext='%s' mask='%s', posmask=%d, defchar=%c)\n",
                 ptext, mask, posmask, maskalfa_defaultchar(valuetype, mask, posmask, blank_char));
      if (maskalfa_match(valuetype, *ptext, mask, posmask, blank_char))
        {
          DEBUG_MSG4("Match!\n");
          if (maskalfa_nth (valuetype, mask, posmask) == '>')
            {
              *presult++ = toupper(*ptext);
              ptext++;
            }
          else if (maskalfa_nth (valuetype, mask, posmask) == '<')
            {
              *presult++ = tolower(*ptext);
              ptext++;
            }
          else
            *presult++ = *ptext++;
          postext++;
          posmask++;
        }
      else if (!maskalfa_iseditable (valuetype, mask, posmask))
        {
          DEBUG_MSG3("No es editable!\n");
          *presult++ = maskalfa_defaultchar (valuetype, mask, posmask, blank_char);
          posmask++;
          if (postext <= startpos && !borrandomask)
            {
              *position += 1;
              DEBUG_MSG3("incrementar position=%d, pos=%d\n", *position, posmask);
            }
        }
      else
        {
          ptext++;
          postext++;
          if (postext <= startpos && !borrandomask)
            {
              *position -=1;
              DEBUG_MSG3("decrementar position=%d, pos=%d\n", *position, posmask);
            }
        }
      *presult = '\0';
      DEBUG_MSG3("creando texto='%s'\n", result);
    }

  while (maskalfa_defaultchar (valuetype, mask, posmask, blank_char))
    {
      *presult++ = maskalfa_defaultchar(valuetype, mask, posmask, blank_char);
      posmask++;
    }

  *presult = '\0';
  if (*position < 0)
    *position = 0;
  if (borrandomask)
    {
      if (*position > 0 && !maskalfa_iseditable (valuetype, mask,*position-1))
        *position = maskalfa_previouseditable (valuetype, mask, *position) + 1;
    }
  else if (!maskalfa_iseditable(valuetype, mask, *position))
    {
      *position = maskalfa_nexteditable (valuetype, mask, *position);
    }

  if (*position < maskalfa_firsteditable (valuetype, mask))
    {
      *position = maskalfa_firsteditable (valuetype, mask);
    }
  else if (*position > maskalfa_lasteditable (valuetype, mask))
    {
      *position=maskalfa_lasteditable (valuetype, mask) + 1;
    }
  g_free(text);
  DEBUG_MSG1("Exit:maskalfa_mask:result='%s' position=%d\n\n", result, *position);

  return result;
}


/* Generic function to format a text according to its type and mask */
static gchar *
do_mask(gint valuetype, const gchar *mask, const gchar *origtext,
        gint start_pos, gint *position, gchar blank_char)
{
  switch (valuetype)
    {
    case ENTRYMASK_DATE:
      if (!mask || !*mask)
        mask = lc_date_mask;
      return maskalfa_mask (valuetype, mask, origtext, start_pos, position, blank_char);
    case ENTRYMASK_TIME:
      if (!mask || !*mask)
        mask = lc_time_mask;;
      return maskalfa_mask (valuetype, mask, origtext, start_pos, position, blank_char);
    case ENTRYMASK_DATETIME:
      if (!mask || !*mask)
        mask = lc_datetime_mask;
      return maskalfa_mask (valuetype, mask, origtext, start_pos, position, blank_char);
    case ENTRYMASK_NUMERIC:
      if (!mask || !*mask)
        mask = lc_numeric_mask;
      return masknum_mask (valuetype, mask, origtext, start_pos, position);
    case ENTRYMASK_CURRENCY:
      if (!mask || !*mask)
        mask = lc_currency_mask;
      return masknum_mask (valuetype, mask, origtext, start_pos, position);
    default: /*  ENTRYMASK_STRING */
      return maskalfa_mask (valuetype, mask, origtext, start_pos, position, blank_char);
    }
}

/* Create default monetary/number mask. See <locale.h> for description */
static gchar *
gtk_set_locale_number_mask (gchar *mask, gchar *grouping, gint digits)
{
  int i;
  gchar *pmask = mask;

  if (!grouping || !*grouping)
    {
      strcpy (mask,"*,###");
      pmask = mask + strlen (mask);
    }
  else
    {
      gchar *grp = grouping + (strlen (grouping)-1);
      *pmask++ = '*';
      while (grp >= grouping) {
        if (*grp != CHAR_MAX) /* locale not set */
          *pmask++ = ',';
        for (i=0; i<*grp; i++)
          *pmask++ = '#';
        grp--;
      }
    }
  if (digits > 0 && digits != CHAR_MAX)
    {
      *pmask++ = '.';
      while (digits--)
        *pmask++ = '#';
    }
  *pmask = '\0';

  return mask;
}


/* Create default date/time mask. See <locale.h> for description */
static gchar *
gtk_set_locale_date_mask (gchar *mask, gint wich_format)
{
  gchar *pmask;
  gchar *nl_format;
  gchar format[MAX_FORMAT_BUF], *pformat;
  gint inpercent, embedded;
  gint hasday, hasmonth, hasyear, hashour, hasminute, hassecond;

  nl_format = nl_langinfo (wich_format);
  DEBUG_MSG0("nl_format=%s\n", nl_format);

  if (!nl_format || !*nl_format)
    {
      switch (wich_format) {
      case D_FMT:
        strcpy (mask, "dd/mm/yy");
        break;
      case T_FMT:
        strcpy (mask, "HH/MM/SS");
        break;
      case D_T_FMT:
        strcpy (mask, "dd/mm/yy HH/MM/SS");
        break;
      default:
        *mask ='\0';
      }
    } else {
      /* expand embedded date or time formats %c, %x, %X and %T */
      pformat = format;
      do
        {
          inpercent = 0;
          embedded = 0;
          while (*nl_format)
            {
              if (*nl_format == '%')
                {
                  if (inpercent)
                    {
                      *pformat++ = '%';
                      *pformat++ = *nl_format;
                      inpercent=0;
                    }
                  else
                    {
                      inpercent = 1;
                    }
                }
              else if (!inpercent)
                {
                  *pformat++ = *nl_format;
                }
              else
                { /* within percent */
                  inpercent=0;
                  switch (*nl_format)
                    {
                    case 'c': /* embedded datetime as %x %X */
                      strcpy (pformat, nl_langinfo(D_FMT));
                      strcat (pformat, " ");
                      strcat (pformat, nl_langinfo(T_FMT));
                      pformat = pformat + strlen(pformat);
                      embedded = 1;
                      break;
                    case 'C': /* embedded long format locale datetime */
                      strcpy (pformat, nl_langinfo(D_T_FMT));
                      pformat = pformat + strlen(pformat);
                      embedded = 1;
                      break;
                    case 'D': /* embedded date as %m/%d/%y */
                      strcpy (pformat, "%m/%d/%y");
                      pformat = format + strlen(format);
                      embedded = 1;
                      break;
                    case 'x': /* embedded locale date */
                      strcpy (pformat, nl_langinfo(D_FMT));
                      pformat = pformat + strlen(pformat);
                      embedded = 1;
                      break;
                    case 'X': /* embedded locale time */
                      strcpy (pformat, nl_langinfo(T_FMT));
                      pformat = pformat + strlen(pformat);
                      embedded = 1;
                      break;
                    case 'T': /* embedded time as %H:%M:%S */
                      strcpy (pformat, "%H:%M:%S");
                      pformat = format + strlen(format);
                      embedded = 1;
                      break;
                    case 'R':           /* GNU extension.  */
                      strcpy (pformat, "%H:%M");
                      pformat = format + strlen(format);
                      embedded = 1;
                      break;
                    case 'r':           /* POSIX.2 extension.  */
                      strcpy (pformat, "%H:%M:%S"); /*"%I:%M:%S %p" originally */
                      pformat = format + strlen(format);
                      embedded = 1;
                      break;
                    default:
                      *pformat++ = '%';
                      *pformat++ = *nl_format;
                    }
                }
              nl_format++;
            }
        }
      while (embedded != 0);

      *pformat = '\0';

      pformat = format;
      pmask = mask;
      hasday=hasmonth=hasyear=hashour=hasminute=hassecond=0;
      inpercent = 0;
      while (*pformat) {
        if (*pformat == '%')
          {
            if (inpercent)
              {
                *pmask++ = *pformat++;
                inpercent = 0;
              }
            else
              {
                inpercent = 1;
              }
          }
        else if (!inpercent)
          {
            *pmask++ = *pformat;
          }
        else
          { /* within percent */
            inpercent=0;
            switch(*pformat) {
            case 'y':
              if (!hasyear)
                {
                  *pmask++ = 'y';
                  *pmask++ = 'y';
                  hasyear=1;
                }
              break;
            case 'Y':
              if (!hasyear)
                {
                  *pmask++ = 'y';
                  *pmask++ = 'y';
                  *pmask++ = 'y';
                  *pmask++ = 'y';
                  hasyear=1;
                }
              break;
            case 'm':
            case 'b':
              if (!hasmonth) {
                *pmask++ = 'm';
                *pmask++ = 'm';
                hasmonth=1;
              }
              break;
            case 'd':
              if (!hasday) {
                *pmask++ = 'd';
                *pmask++ = 'd';
                hasday=1;
              }
              break;
            case 'H':
            case 'I':
              if (!hashour) {
                *pmask++ = 'H';
                *pmask++ = 'H';
                hashour=1;
              }
              break;
            case 'M':
              if (!hasminute) {
                *pmask++ = 'M';
                *pmask++ = 'M';
                hashour=1;
              }
              break;
            case 'S':
              if (!hassecond) {
                *pmask++ = 'S';
                *pmask++ = 'S';
                hassecond=1;
              }
              break;
            case 'U':
            case 'W':
            case 'w':
            case 'p':
            case 'a':
            case 'A':
            case 'j':
            case 'B':
            case 'Z':
              /* not used */
              break;
            }
          }
        pformat++;
      }
      *pmask='\0';
    }
  return mask;
}

/* Formats the text of a currency or numeric entry */
static void
gtk_entrymask_format_currency (GtkEntryMask *entrymask)
{
  struct lconv *lc;
  gchar buffer[MAX_FORMAT_BUF], *pbuffer;
  gchar *text;
  gdouble value = gtk_entrymask_get_double (entrymask);

  pbuffer = buffer;
  lc = localeconv ();
  if (lc->n_sign_posn == CHAR_MAX) /* locale not set */
    return;
  if (value < 0.0)
    {
      gtk_entrymask_set_double (entrymask, -value);
      text = gtk_entrymask_get_text (entrymask);
      switch (lc->n_sign_posn)
        {
          /* Positive and negative sign positions:
             0 Parentheses surround the quantity and currency_symbol.
             1 The sign string precedes the quantity and currency_symbol.
             2 The sign string follows the quantity and currency_symbol.
             3 The sign string immediately precedes the currency_symbol.
             4 The sign string immediately follows the currency_symbol.  */
        case 0:
          *pbuffer++ = '(';
          if (lc->n_cs_precedes)
            {
              strcat (pbuffer, lc->currency_symbol);
            }
          if (lc->n_sep_by_space)
            strcat (pbuffer, " ");
          strcat (pbuffer, text);
          strcat (pbuffer, ")");
          break;
        case 1:
          strcpy (pbuffer, lc->negative_sign);
          if (lc->n_cs_precedes)
            {
              strcat (pbuffer, lc->currency_symbol);
            }
          if (lc->n_sep_by_space)
            strcat (pbuffer, " ");
          strcat (pbuffer, text);
          break;
        case 2:
          strcpy (pbuffer, text);
          strcat (pbuffer, lc->currency_symbol);
          strcat (pbuffer, lc->negative_sign);
          break;
        case 3:
          if (lc->n_cs_precedes) {
            strcpy (pbuffer, lc->negative_sign);
            strcat (pbuffer, lc->currency_symbol);
            if (lc->n_sep_by_space)
              strcat (pbuffer, " ");
            strcat (pbuffer, text);
          } else {
            strcpy (pbuffer, text);
            if (lc->n_sep_by_space)
              strcat (pbuffer, " ");
            strcat (pbuffer, lc->negative_sign);
            strcat (pbuffer, lc->currency_symbol);
          }
          break;
        case 4:
          if (lc->n_cs_precedes) {
            strcat (pbuffer, lc->currency_symbol);
            strcpy (pbuffer, lc->negative_sign);
            if (lc->n_sep_by_space)
              strcat (pbuffer, " ");
            strcat (pbuffer, text);
          } else {
            strcpy (pbuffer, text);
            if (lc->n_sep_by_space)
              strcat (pbuffer, " ");
            strcat (pbuffer, lc->currency_symbol);
            strcat (pbuffer, lc->negative_sign);
          }
          break;
        }
    } else { /* positive values */
      gtk_entrymask_set_double(entrymask, value);
      text = gtk_entrymask_get_text(entrymask);
      switch (lc->p_sign_posn) {
        /* Positive and negative sign positions:
           0 Parentheses surround the quantity and currency_symbol.
           1 The sign string precedes the quantity and currency_symbol.
           2 The sign string follows the quantity and currency_symbol.
           3 The sign string immediately precedes the currency_symbol.
           4 The sign string immediately follows the currency_symbol.  */
      case 0:
        *pbuffer++ = '(';
        if (lc->p_cs_precedes)
          {
            strcat (pbuffer, lc->currency_symbol);
          }
        if (lc->p_sep_by_space)
          strcat (pbuffer, " ");
        strcat (pbuffer, text);
        strcat (pbuffer, ")");
        break;
      case 1:
        strcpy (pbuffer, lc->positive_sign);
        if (lc->p_cs_precedes)
          {
            strcat (pbuffer, lc->currency_symbol);
          }
        if (lc->p_sep_by_space)
          strcat (pbuffer, " ");
        strcat (pbuffer, text);
        break;
      case 2:
        strcpy (pbuffer, text);
        strcat (pbuffer, lc->currency_symbol);
        strcat (pbuffer, lc->positive_sign);
        break;
      case 3:
        if (lc->p_cs_precedes)
          {
            strcpy (pbuffer, lc->positive_sign);
            strcat (pbuffer, lc->currency_symbol);
            if (lc->p_sep_by_space)
              strcat (pbuffer, " ");
            strcat (pbuffer, text);
          }
        else
          {
            strcpy (pbuffer, text);
            if (lc->p_sep_by_space)
              strcat (pbuffer, " ");
            strcat (pbuffer, lc->positive_sign);
            strcat (pbuffer, lc->currency_symbol);
          }
        break;
      case 4:
        if (lc->p_cs_precedes)
          {
            strcat (pbuffer, lc->currency_symbol);
            strcpy (pbuffer, lc->positive_sign);
            if (lc->p_sep_by_space)
              strcat (pbuffer, " ");
            strcat (pbuffer, text);
          }
        else
          {
            strcpy (pbuffer, text);
            if (lc->p_sep_by_space)
              strcat (pbuffer, " ");
            strcat (pbuffer, lc->currency_symbol);
            strcat (pbuffer, lc->positive_sign);
          }
        break;
      }
    }
  gtk_entrymask_set_text_forced (entrymask, buffer);
}


/* Formats the text of a time or date entry */
static void
gtk_entrymask_format_date(GtkEntryMask *entrymask)
{
  char buffer[MAX_FORMAT_BUF];
  time_t time = gtk_entrymask_get_time_t (entrymask);
  struct tm *tm = localtime (&time);

  strftime (buffer, MAX_FORMAT_BUF, entrymask->format, tm);
  gtk_entrymask_set_text_forced (entrymask, buffer);
}

/* Unformats the text of a time or date entry, used in focus_in */
static time_t
gtk_entrymask_unformat_date(GtkEntryMask *entrymask)
{
  struct tm tm = {0};
  const gchar *text = gtk_entrymask_get_text (entrymask);
  tm.tm_year = 30; /* Fictitious values for the date, for just-time dates */
  tm.tm_mon = 0;
  tm.tm_mday = 1;
  strptime ((char*)text, entrymask->format, &tm);
  return mktime (&tm);
}



/* Overriden function to insert text in the entry widget */
static void
gtk_entrymask_insert_text (GtkEditable *editable,
                           const gchar *new_text,
                           gint         new_text_length,
                           gint        *position)
{
  gchar *maskedtext = NULL;
  gchar *origtext;
  GtkEntryMask *entrymask;
  GtkEditableClass *parent_editable_iface;

  g_return_if_fail (editable != NULL);
  g_return_if_fail (GTK_IS_ENTRYMASK (editable));
  entrymask = GTK_ENTRYMASK (editable);

  parent_editable_iface = g_type_interface_peek (parent_class, GTK_TYPE_EDITABLE);

  parent_editable_iface->insert_text (editable, new_text,
                                      new_text_length, position);

  if (entrymask->masking || entrymask->forced) {
    return;
  }
  if (entrymask->valuetype == ENTRYMASK_STRING
      && (!entrymask->mask || !*entrymask->mask))
    return; /* There is nothing to mask */
  entrymask->masking++;

  origtext = gtk_entrymask_get_text(entrymask);
  DEBUG_MSG2("insert_text: texto original=%s\n", origtext);
  maskedtext = do_mask(entrymask->valuetype, entrymask->mask,
                       origtext, *position - new_text_length, position,
                       entrymask->blank_char);
  DEBUG_MSG2("insert_text: texto masked='%s'\n", maskedtext);
  if (strcmp(maskedtext, origtext) != 0)
    gtk_entry_set_text(GTK_ENTRY(editable), maskedtext);
  if (maskedtext)
    g_free(maskedtext);

  entrymask->masking--;
}



/* Overriden function to delete text from the entry widget */
static void
gtk_entrymask_delete_text (GtkEditable *editable,
                           gint         start_pos,
                           gint         end_pos)
{
  gchar *maskedtext = NULL;
  gchar *origtext;
  GtkEntryMask *entrymask;
  gint position;
  GtkEditableClass *parent_editable_iface;

  g_return_if_fail (editable != NULL);
  g_return_if_fail (GTK_IS_ENTRYMASK (editable));
  entrymask = GTK_ENTRYMASK (editable);

  parent_editable_iface = g_type_interface_peek (parent_class, GTK_TYPE_EDITABLE);

  parent_editable_iface->delete_text (editable, start_pos, end_pos);

  if (entrymask->masking || entrymask->forced)
    {
      return;
    }
  if (entrymask->valuetype == ENTRYMASK_STRING
      && (!entrymask->mask || !*entrymask->mask))
    return; /* There is nothing to mask */

  entrymask->masking++;
  position = start_pos;
  origtext  = gtk_entrymask_get_text(entrymask);
  maskedtext = do_mask(entrymask->valuetype, entrymask->mask,
                       origtext, position, &position, entrymask->blank_char);

  if (strcmp(maskedtext, origtext) != 0)
    gtk_entry_set_text(GTK_ENTRY(editable), maskedtext);

  gtk_entrymask_set_position(entrymask, position);

  if (maskedtext)
    free(maskedtext);

  entrymask->masking--;
}


/* Overriden function to place the cursor in the correct position
   and to unformat the text */
static gint
gtk_entrymask_focus_in(GtkWidget *widget,
                       GdkEventFocus *event)
{
  GtkEntryMask *entrymask;
  GtkEntryMaskType valuetype;
  const gchar *posdec, *text, *format;

  entrymask = GTK_ENTRYMASK (widget);
  text = gtk_entrymask_get_text(entrymask);
  format = entrymask->format;
  switch ((valuetype=entrymask->valuetype))
    {
    case ENTRYMASK_DATE:
    case ENTRYMASK_TIME:
    case ENTRYMASK_DATETIME:
      if (format) {
        entrymask->formating++;
        gtk_entrymask_set_time_t(entrymask, gtk_entrymask_unformat_date(entrymask));
        entrymask->formating--;
      }
      gtk_entrymask_set_position(entrymask, 0);
      break;
    case ENTRYMASK_NUMERIC:
    case ENTRYMASK_CURRENCY:
      if ((format && strchr(format, '$'))
          || valuetype == ENTRYMASK_CURRENCY)
        {
          entrymask->formating++;
          gtk_entrymask_set_double(entrymask, gtk_entrymask_get_double(entrymask));
          entrymask->formating--;
        }
      text = gtk_entrymask_get_text(entrymask);
      posdec = strchr(text, LC_DECIMAL_POINT);
      if (posdec)
        {
          gtk_entrymask_set_position(entrymask, posdec - text);
        }
      break;
    case ENTRYMASK_STRING:
      if (entrymask->mask && *entrymask->mask)
        {
          gint pos=0;
          if (*text=='\0')
            { /* Perdemos el primer caracter si no esta mask en texto */
              gchar *maskedtext = do_mask(valuetype, entrymask->mask, text, 0, &pos, entrymask->blank_char);
              entrymask->formating++;
              gtk_entrymask_set_text(entrymask, maskedtext);
              entrymask->formating--;
              g_free(maskedtext);
            }
          gtk_entrymask_set_position(entrymask,
                                     maskalfa_firsteditable(valuetype,
                                                            entrymask->mask));
        }
      break;
    }

  return (GTK_WIDGET_CLASS (parent_class)->focus_in_event (widget, event));
}


/* Overriden function to format the text acordingly to the format property */
static gint
gtk_entrymask_focus_out(GtkWidget *widget,
                        GdkEventFocus *event)
{
  GtkEntryMask *entrymask = GTK_ENTRYMASK (widget);

  gtk_entrymask_format_text(entrymask);

  return (GTK_WIDGET_CLASS (parent_class)->focus_out_event (widget, event));
}
