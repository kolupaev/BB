/*
 * info.c
 *
 *  Created on: Apr 30, 2021
 *      Author: horinek
 */


#include "gui/gui_list.h"

#include "system.h"

#include "fc/fc.h"

#include "drivers/rev.h"
#include "drivers/esp/download/slot.h"
#include "drivers/esp/protocol.h"
#include "drivers/power/pwr_mng.h"

#include "gui/dialog.h"
#include "gui/tasks/filemanager.h"
#include "gui/statusbar.h"
#include "gui/statusbar.h"

#include "etc/format.h"
#include "etc/bootloader.h"


REGISTER_TASK_I(firmware,
    char new_fw[32];
    uint8_t slot_id;
);

void firmware_update_progress_cb(uint8_t res, void * data)
{
    if (res == dialog_res_cancel)
    {
        esp_http_stop(local->slot_id);
    }
}

void firmware_update_worker(char * path)
{
    red_unlink(UPDATE_FILE);
    copy_file(path, UPDATE_FILE);
    system_reboot();

    tfree(path);
    RedTaskUnregister();
    vTaskDelete(NULL);
}

void firmware_update_apply_cb(uint8_t res, void * data)
{
    UNUSED(data);

    char * opt_data = dialog_get_opt_data();

    if (res == dialog_res_yes)
    {
        char * path = tmalloc(PATH_LEN);

        snprintf(path, PATH_LEN, "%s/%s", PATH_FW_DIR, opt_data);
        dialog_show(_("Updating"), _("Please wait"), dialog_progress, NULL);
        dialog_progress_spin();
        xTaskCreate((TaskFunction_t)firmware_update_worker, "firmware_update_worker", 1024 * 2, path, 24, NULL);
    }

    tfree(opt_data);
}

void firmware_update_get_file_cb(uint8_t res, download_slot_t * ds)
{
    INFO("info_update_get_file_cb cb %u", res);

    if (res == DOWNLOAD_SLOT_PROGRESS)
    {
        dialog_progress_set_progress((ds->pos * 100) / ds->size);
    }
    else if (res == DOWNLOAD_SLOT_COMPLETE)
    {
        dialog_close();
        download_slot_file_data_t * data = (download_slot_file_data_t *)ds->data;
        char path[64];
        char tmp_path[TEMP_NAME_LEN];

        get_tmp_path(tmp_path, data->tmp_id);

        snprintf(path, sizeof(path), "%s/%s", PATH_FW_DIR, local->new_fw);

        red_unlink(path);
        red_rename(tmp_path, path);
        red_unlink(tmp_path);

        dialog_show(_("Start update process?"), "", dialog_yes_no, firmware_update_apply_cb);
        char * opt_data = tmalloc(strlen(local->new_fw) + 1);
        strcpy(opt_data, local->new_fw);
        dialog_add_opt_data(opt_data);
    }
    else
    {
        dialog_downloads_error(res);
    }
}

void firmware_update_question_cb(uint8_t res, void * data)
{
    if (res == dialog_res_yes)
    {
        char url[128];

        fc_landing();

        snprintf(url, sizeof(url), "%s/%s/fw/%s", config_get_text(&config.system.server_url), config_get_select_text(&config.system.fw_channel), local->new_fw);

        local->slot_id = esp_http_get(url, DOWNLOAD_SLOT_TYPE_FILE, firmware_update_get_file_cb);
        dialog_show(_("Downloading firmware"), "", dialog_progress, firmware_update_progress_cb);
        dialog_progress_spin();
        dialog_progress_set_subtitle(local->new_fw);
    }
}

void firmware_update_info_cb(uint8_t res, download_slot_t * ds)
{
	if (res == DOWNLOAD_SLOT_PROGRESS)
		return;

    if (res == DOWNLOAD_SLOT_COMPLETE)
    {
        dialog_close();

        char msg[64];

        if (read_value(ds->data, "firmware", local->new_fw, sizeof(local->new_fw)))
        {
            char rev_str[20];
            rev_get_sw_string(rev_str);
            if (strncmp(rev_str, local->new_fw, strlen(rev_str)) == 0 || strlen(local->new_fw) == 0)
            {
                dialog_show(_("Up to date"), _("You are using the latest firmware"), dialog_confirm, NULL);
            }
            else
            {
                snprintf(msg, sizeof(msg), _("Download firmware\n%s?"), local->new_fw);
                dialog_show(_("Firmware update"), msg, dialog_yes_no, firmware_update_question_cb);
            }
        }
        else
        {
            dialog_downloads_error(DOWNLOAD_SLOT_NOT_FOUND);
        }
    }
    else
    {
        dialog_downloads_error(res);
    }
}

static bool firmware_update_cb(lv_obj_t * obj, lv_event_t event)
{
    if (event == LV_EVENT_CLICKED)
    {
		char url[128];

		snprintf(url, sizeof(url), "%s/%s/", config_get_text(&config.system.server_url), config_get_select_text(&config.system.fw_channel));

		local->slot_id = esp_http_get(url, DOWNLOAD_SLOT_TYPE_PSRAM, firmware_update_info_cb);
		dialog_show(_("Checking for updates"), "", dialog_progress, firmware_update_progress_cb);
		dialog_progress_spin();
    }
    return true;
}

bool manual_install_fm_cb(uint8_t event, char * path)
{
    if (event == FM_CB_SELECT)
    {
        char text[64];
        path = strrchr(path, '/');

        if (path == NULL)
            return true;
        path++;

        snprintf(text, sizeof(text), _("Install version\n%s"), path);

        dialog_show(_("Start update?"), text, dialog_yes_no, firmware_update_apply_cb);
        char * opt_data = tmalloc(strlen(path) + 1);
        strcpy(opt_data, path);
        dialog_add_opt_data(opt_data);
    }
	return true;
}

static bool manual_install_cb(lv_obj_t * obj, lv_event_t event)
{
	if (event == LV_EVENT_CLICKED)
	{
		gui_switch_task(&gui_filemanager, LV_SCR_LOAD_ANIM_MOVE_LEFT);
		filemanager_open(PATH_FW_DIR, 0, &gui_firmware, FM_FLAG_HIDE_DIR | FM_FLAG_SORT_NAME, manual_install_fm_cb);

		//supress default handler
		return false;
	}

	return true;
}

static bool firmware_serial_release_note_cb(lv_obj_t * obj, lv_event_t event)
{
    if (event == LV_EVENT_CLICKED)
    {
    	release_note_show();
    }
    return true;
}

lv_obj_t * firmware_init(lv_obj_t * par)
{
    help_set_base("System/Firmware");

    lv_obj_t * list = gui_list_create(par, _("Device firmware"), &gui_system, NULL);

    char rev_str[20];
    char value[32];
    lv_obj_t * obj;

    rev_get_sw_string(rev_str);
    snprintf(value, sizeof(value), _("Firmware ver. %s"), rev_str);
    obj = gui_list_info_add_entry(list, _h("Release note"), value);
    gui_config_entry_add(obj, CUSTOM_CB, firmware_serial_release_note_cb);

    gui_list_auto_entry(list, _h("Check for updates"), CUSTOM_CB, firmware_update_cb);
    gui_list_auto_entry(list, _h("Notify for new fw"), &config.system.check_for_updates, NULL);

    gui_list_auto_entry(list, _h("Firmware channel"), &config.system.fw_channel, NULL);

    gui_list_auto_entry(list, _h("Manual firmware install"), CUSTOM_CB, manual_install_cb);

    snprintf(value, sizeof(value), "%lu", nvm->bootloader);
    gui_list_info_add_entry(list, _h("Bootloader version"), value);

    return list;
}
