/* test-gtkentrymask
 * Copyright (C) 2004-2005 Stéphane Raimbault <stephane.raimbault@gmail.com>
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

#include <gtk/gtk.h>
#include <gtkentrymask.h>

#define TAB_ENTRY_NB 20

typedef struct {
  GtkEntryMaskType type;
  gchar *s_label;
  gchar *mask;
  GtkWidget *entry;
} test_entry_mask_t;


void all_entry_get (GtkWidget *button,
                    const test_entry_mask_t *tab_entry)
{
  int i;

  for (i=0; i<TAB_ENTRY_NB; i++) {
    /* Or gtk_entrymask_get_valuetype (GTK_ENTRYMASK (tab_entry[i].entry)) */
    switch (tab_entry[i].type) {
    case ENTRYMASK_STRING:
      {
        const gchar *string;

        string = gtk_entry_get_text (GTK_ENTRY (tab_entry[i].entry));
        g_print ("%d - String : %s\n", i, string);
      }
      break;
    case ENTRYMASK_CURRENCY:
    case ENTRYMASK_NUMERIC:
      {
        gdouble currency;

        currency = gtk_entrymask_get_double (GTK_ENTRYMASK (tab_entry[i].entry));
        g_print ("%d - Currency : %f\n", i, currency);
      }
      break;
    case ENTRYMASK_TIME:
      {
        time_t time;
        struct tm *tm;

        time = gtk_entrymask_get_time_t (GTK_ENTRYMASK (tab_entry[i].entry));
        tm = localtime (&time);

        g_print("%d - Time %d:%d:%d\n", i, tm->tm_hour, tm->tm_min, tm->tm_sec);
      }
      break;
    case ENTRYMASK_DATE:
      {
        time_t time;
        struct tm *tm;

        time = gtk_entrymask_get_time_t (GTK_ENTRYMASK (tab_entry[i].entry));
        tm = localtime (&time);

        g_print ("%d - Date %04d-%02d-%02d\n", i, tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday);
      }
      break;
    case ENTRYMASK_DATETIME:
      {
        time_t time;
        struct tm *tm;

        time = gtk_entrymask_get_time_t (GTK_ENTRYMASK (tab_entry[i].entry));
        tm = localtime (&time);

        g_print ("%d - DateTime %04d-%02d-%02d %02d:%02d:%02d %s GMT%+ld - %s",
                 i, tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
                 tm->tm_hour, tm->tm_min, tm->tm_sec, tzname[0], timezone / 3600,
                 asctime (tm));
      }
      break;
    }

  }

  g_print("--End--\n");
}

int vbox_add_label_entry (GtkWidget *vbox, GtkWidget *label, GtkWidget *entry)
{
  GtkWidget *hbox;

  hbox = gtk_hbox_new(FALSE, 6);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

  gtk_widget_show (entry);
  gtk_box_pack_start(GTK_BOX(hbox), entry, TRUE, TRUE, 0);
}

int main (int argc, char **argv)
{
  guint i;
  GtkWidget *window;
  GtkWidget *vbox;
  GtkSizeGroup *size_group;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *button;
  test_entry_mask_t tab_entry[TAB_ENTRY_NB] = {
    { ENTRYMASK_STRING, "Phone Number:", "+99 (\\999)99-99-99", NULL },
    { ENTRYMASK_STRING, "USA Zip Code:", "<<000", NULL },
    { ENTRYMASK_STRING,"IP Address:", "000.000.000.000", NULL},
    { ENTRYMASK_CURRENCY, "Currency:", NULL, NULL },
    { ENTRYMASK_CURRENCY, "Currency (2):", "$###,###.##", NULL },
    { ENTRYMASK_CURRENCY, "Currency (3):", "###,##0.00", NULL },
    { ENTRYMASK_NUMERIC, "Numeric:", NULL, NULL },
    { ENTRYMASK_NUMERIC, "Numeric (2):", "+00000", NULL },
    { ENTRYMASK_NUMERIC, "Numeric (3):", "*,###", NULL },
    { ENTRYMASK_NUMERIC, "BUG - Numeric (4):", ",*", NULL },
    { ENTRYMASK_NUMERIC, "BUG - Numeric (5):", "##.0*", NULL },
    { ENTRYMASK_TIME, "Time:", NULL, NULL },
    { ENTRYMASK_TIME, "Time (2):", "HH:MM", NULL },
    { ENTRYMASK_TIME, "Time (3):", "HH:MM:SS", NULL },
    { ENTRYMASK_DATE, "Date:", NULL, NULL },
    { ENTRYMASK_DATE, "Date (2):", "mm/dd/yy", NULL },
    { ENTRYMASK_DATE, "Date (3):", "dd/mm/yyyy", NULL },
    { ENTRYMASK_DATETIME, "Date Time:", NULL, NULL },
    { ENTRYMASK_DATETIME, "Date Time (2):", "yyyy-mm-dd HH:MM:SS", NULL },
    { ENTRYMASK_DATETIME, "Date Time (3):", "dd/mm/yy HH:MM", NULL }
  };

  gtk_init (&argc, &argv);
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window), "Test GtkEntryMask");
  g_signal_connect(window, "destroy", G_CALLBACK (gtk_main_quit), NULL);

  /* VBOX */
  vbox = gtk_vbox_new (FALSE, 6);
  gtk_container_set_border_width(GTK_CONTAINER(vbox), 12);
  gtk_box_set_spacing(GTK_BOX(vbox), 18);

  size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
  gtk_container_add (GTK_CONTAINER (window), vbox);

  for (i=0; i<TAB_ENTRY_NB; i++) {
    label = gtk_label_new (tab_entry[i].s_label);
    gtk_size_group_add_widget (GTK_SIZE_GROUP (size_group), label);
    tab_entry[i].entry = gtk_entrymask_new_with_type (tab_entry[i].type, NULL, tab_entry[i].mask);
    vbox_add_label_entry (vbox, label, tab_entry[i].entry);
  }

  hbox = gtk_hbox_new(FALSE, 6);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 6);

  button = gtk_button_new_from_stock (GTK_STOCK_EXECUTE);
  gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, FALSE, 0);
  g_signal_connect(button, "clicked", G_CALLBACK (all_entry_get), tab_entry);

  button = gtk_button_new_from_stock (GTK_STOCK_QUIT);
  gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, FALSE, 0);
  g_signal_connect(button, "clicked", G_CALLBACK (gtk_main_quit), NULL);

  gtk_widget_show_all (window);
  gtk_main ();

  return 0;
}
