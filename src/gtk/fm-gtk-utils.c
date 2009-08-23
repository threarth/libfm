/*
 *      fm-gtk-utils.c
 *      
 *      Copyright 2009 PCMan <pcman@debian>
 *      
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *      
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *      
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *      MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib/gi18n.h>

#include "fm-gtk-utils.h"
#include "fm-file-ops-job.h"
#include "fm-progress-dlg.h"
#include "fm-path-entry.h"

void fm_show_error(GtkWindow* parent, const char* msg)
{
    GtkWidget* dlg = gtk_message_dialog_new_with_markup(parent, 0, 
                                GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, msg);
    gtk_window_set_title((GtkWindow*)dlg, _("Error"));
    gtk_dialog_run((GtkDialog*)dlg);
    gtk_widget_destroy(dlg);
}

int fm_yes_no(GtkWindow* parent, const char* question)
{
    int ret;
    GtkWidget* dlg = gtk_message_dialog_new_with_markup(parent, 0, 
                                GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO, question);
    ret = gtk_dialog_run((GtkDialog*)dlg);
    gtk_widget_destroy(dlg);
    return ret;
}

int fm_ok_cancel(GtkWindow* parent, const char* question)
{
    int ret;
    GtkWidget* dlg = gtk_message_dialog_new_with_markup(parent, 0, 
                                GTK_MESSAGE_QUESTION, GTK_BUTTONS_OK_CANCEL, question);
    ret = gtk_dialog_run((GtkDialog*)dlg);
    gtk_widget_destroy(dlg);
    return ret;
}

int fm_ask(GtkWindow* parent, const char* question, ...)
{
    int ret;
    va_list args;
    va_start (args, question);
    ret = fm_ask_valist(parent, question, args);
    va_end (args);
    return ret;
}

int fm_askv(GtkWindow* parent, const char* question, const char** options)
{
    int ret;
    guint id = 1;
    GtkWidget* dlg = gtk_message_dialog_new_with_markup(parent, 0, 
                                GTK_MESSAGE_QUESTION, 0, question);
    /* FIXME: need to handle defualt button and alternative button
     * order problems. */
    while(*options)
    {
        /* FIXME: handle button image and stock buttons */
        GtkWidget* btn = gtk_dialog_add_button(dlg, *options, id);
        ++options;
        ++id;
    }
    ret = gtk_dialog_run((GtkDialog*)dlg);
    if(ret >= 1)
        ret -= 1;
    else
        ret == -1;
    gtk_widget_destroy(dlg);
    return ret;
}

int fm_ask_valist(GtkWindow* parent, const char* question, va_list options)
{
    GArray* opts = g_array_sized_new(TRUE, TRUE, sizeof(char*), 6);
    gint ret;
    const char* opt = va_arg(options, const char*);
    while(opt)
    {
        g_array_append_val(opts, opt);
        opt = va_arg (options, const char *);
    }
    ret = fm_askv(parent, question, opts->data);
    g_array_free(opts, TRUE);
    return ret;
}

enum {
    INPUT_STR,
    INPUT_PATH,
    INPUT_RENAME
};

char* _fm_get_user_input(GtkWindow* parent, const char* title, const char* msg, const char* default_text, int type)
{
    char* str = NULL;
    GtkWidget* dlg = gtk_dialog_new_with_buttons(title, parent, 0,
                                GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);
    GtkWidget* label = gtk_label_new(msg);
    GtkWidget* entry;
    if(type == INPUT_PATH)
        entry = fm_path_entry_new();
    else
        entry = gtk_entry_new();
    gtk_box_pack_start(GTK_DIALOG(dlg)->vbox, label, FALSE, TRUE, 6);
    gtk_box_pack_start(GTK_DIALOG(dlg)->vbox, entry, FALSE, TRUE, 6);
    gtk_container_set_border_width(GTK_DIALOG(dlg)->vbox, 10);
    gtk_widget_show_all(dlg);

    if(default_text && default_text[0])
    {
        gtk_entry_set_text(entry, default_text);
        /* only select filename part without extension name. */
        if(type == INPUT_RENAME && default_text[1])
        {
            /* FIXME: handle the special case for *.tar.gz or *.tar.bz2 */
            const char* dot = g_utf8_strrchr(default_text, -1, '.');
/*
            const char* dot = default_text;
            while( dot = g_utf8_strchr(dot + 1, -1, '.') )
            {
                gboolean uncertain;
                if(g_content_type_guess(dot, NULL, 0, &uncertain))
                {
                    gtk_editable_select_region(entry, 0, g_utf8_pointer_to_offset(default_text, dot));
                    break;
                }
            }
*/
            if(dot)
                gtk_editable_select_region(entry, 0, g_utf8_pointer_to_offset(default_text, dot));
            else
                gtk_editable_select_region(entry, 0, -1);
        }
    }

    while(gtk_dialog_run(dlg) == GTK_RESPONSE_OK)
    {
        const char* pstr = gtk_entry_get_text(entry);
        if( pstr && *pstr )
        {
            str = g_strdup(pstr);
            break;
        }
    }
    gtk_widget_destroy(dlg);
    return str;
}

char* fm_get_user_input(GtkWindow* parent, const char* title, const char* msg, const char* default_text)
{
    return _fm_get_user_input(parent, title, msg, default_text, FALSE);
}

FmPath* fm_get_user_input_path(GtkWindow* parent, const char* title, const char* msg, FmPath* default_path)
{
    char *str, *path_str = NULL;
    if(default_path)
        path_str = fm_path_to_str(default_path);
    str=_fm_get_user_input(parent, title, msg, path_str, FALSE);
    FmPath* path = fm_path_new(str);
    g_free(path_str);
    g_free(str);
    return path;
}

FmPath* fm_select_folder(GtkWindow* parent)
{
    FmPath* path;
    GtkFileChooser* chooser;
    chooser = (GtkFileChooser*)gtk_file_chooser_dialog_new(_("Please select a folder"), 
                                        parent, GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
                                        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                        GTK_STOCK_OK, GTK_RESPONSE_OK,
                                        NULL);
    gtk_dialog_set_alternative_button_order((GtkDialog*)chooser, 
                                        GTK_RESPONSE_CANCEL,
                                        GTK_RESPONSE_OK, NULL);
    if( gtk_dialog_run((GtkDialog*)chooser) == GTK_RESPONSE_OK )
    {
        char* file = gtk_file_chooser_get_filename(chooser);
        if(!file)
            file = gtk_file_chooser_get_uri(chooser);
        path = fm_path_new(file);
        g_free(file);
    }
    else
        path = NULL;
    gtk_widget_destroy((GtkWidget*)chooser);
    return path;
}


struct MountData
{
    GMainLoop *loop;
    GError* err;
};

void on_mounted(GFile *gf, GAsyncResult *res, struct MountData* data)
{
    GError* err = NULL;
    if( !g_file_mount_enclosing_volume_finish(gf, res, &err) )
        data->err = err;
    else
    {
        if(err)
            g_error_free(err);
    }
    g_main_loop_quit(data->loop);
}

gboolean fm_mount_path(GtkWindow* parent, FmPath* path)
{
    gboolean ret = FALSE;
    struct MountData* data = g_new0(struct MountData, 1);
    GFile* gf = fm_path_to_gfile(path);
    GMountOperation* op = gtk_mount_operation_new(parent);
    GCancellable* cancel = g_cancellable_new();
    data->loop = g_main_loop_new (NULL, TRUE);

    g_file_mount_enclosing_volume(gf, 0, op, cancel, (GAsyncReadyCallback)on_mounted, data);

    if (g_main_loop_is_running(data->loop))
    {
        GDK_THREADS_LEAVE();
        g_main_loop_run(data->loop);
        GDK_THREADS_ENTER();
    }
    g_main_loop_unref(data->loop);

    if(data->err)
    {
        fm_show_error(parent, data->err->message);
        g_error_free(data->err);
    }
    else
        ret = TRUE;

    g_free(data);
    g_object_unref(cancel);
    g_object_unref(op);
    g_object_unref(gf);
    return ret;
}

/* File operations */
/* FIXME: only show the progress dialog if the job isn't finished 
 * in 1 sec. */

void fm_copy_files(FmPathList* files, FmPath* dest_dir)
{
	GtkWidget* dlg;
	FmJob* job = fm_file_ops_job_new(FM_FILE_OP_COPY, files);
	fm_file_ops_job_set_dest(job, dest_dir);
	dlg = fm_progress_dlg_new(job);
	gtk_window_present(dlg);
	fm_job_run_async(job);
}

void fm_move_files(FmPathList* files, FmPath* dest_dir)
{
	GtkWidget* dlg;
	FmJob* job = fm_file_ops_job_new(FM_FILE_OP_MOVE, files);
	fm_file_ops_job_set_dest(job, dest_dir);
	dlg = fm_progress_dlg_new(job);
	gtk_window_present(dlg);
	fm_job_run_async(job);
}

void fm_trash_files(FmPathList* files)
{
	GtkWidget* dlg;
	FmJob* job = fm_file_ops_job_new(FM_FILE_OP_TRASH, files);
	dlg = fm_progress_dlg_new(job);
	gtk_window_present(dlg);
	fm_job_run_async(job);
}

void fm_delete_files(FmPathList* files)
{
	GtkWidget* dlg;
	FmJob* job = fm_file_ops_job_new(FM_FILE_OP_DELETE, files);
	dlg = fm_progress_dlg_new(job);
	gtk_window_present(dlg);
	fm_job_run_async(job);
}

void fm_move_or_copy_files_to(FmPathList* files, gboolean is_move)
{
    FmPath* dest = fm_select_folder(NULL);
    if(dest)
    {
        if(is_move)
            fm_move_files(files, dest);
        else
            fm_copy_files(files, dest);
        fm_path_unref(dest);
    }
}

/*
void fm_rename_files(FmPathList* files)
{
    
}
*/

void fm_rename_file(FmPath* file)
{
    GFile* gf = fm_path_to_gfile(file), *parent, *dest;
    GError* err = NULL;
    char* new_name = _fm_get_user_input(NULL, _("Rename File"), _("Please enter a new name:"), file->name, INPUT_RENAME);
    if( !new_name )
        return;
    parent = g_file_get_parent(gf);
    dest = g_file_get_child(parent, new_name);
    g_object_unref(parent);
    if(!g_file_move(gf, dest,
                G_FILE_COPY_ALL_METADATA|
                G_FILE_COPY_NO_FALLBACK_FOR_MOVE|
                G_FILE_COPY_NOFOLLOW_SYMLINKS,
                NULL, /* make this cancellable later. */
                NULL, NULL, &err))
    {
        fm_show_error(NULL, err->message);
        g_error_free(err);
    }
    g_object_unref(dest);
    g_object_unref(gf);
}

