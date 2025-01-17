/*
 * maps_unzip.c
 *
 * Unzip all ZIP files inside the maps directory and delete ZIP afterwards.
 *
 *  Created on: Feb 27, 2023
 *      Author: tilmann@bubecks.de
 */

#include <string.h>

#include "lib/miniz/miniz.h"
#include "common.h"
#include "gui/statusbar.h"
#include "drivers/rtc.h"
#include "etc/format.h"

bool unzip_zipfile(char *target_dir, char *zip_file_path, bool gui)
{
	mz_zip_archive zip;
	mz_zip_zero_struct(&zip);
	mz_zip_archive_file_stat file_stat;
	char mess[128];
	char path[PATH_LEN];
    lv_obj_t * msg_ptr = NULL;

	bool res = mz_zip_reader_init_file(&zip, zip_file_path, 0);
	if (res)
	{
	    if (gui)
	    {
            sprintf(mess, _("Unzipping\n%s"), zip_file_path);
            msg_ptr = statusbar_msg_add(STATUSBAR_MSG_PROGRESS, mess);
	    }

		int fileCount = (int)mz_zip_reader_get_num_files(&zip);
		for ( int i = 0; i < fileCount; i++ )
		{
			if (!mz_zip_reader_file_stat(&zip, i, &file_stat)) continue;
			if (mz_zip_reader_is_file_a_directory(&zip, i)) continue; // skip directories for now
			sprintf(path, "%s/%s", target_dir, file_stat.m_filename);
			if ( !file_exists(path) || file_size_from_name(path) != file_stat.m_uncomp_size)
			{
                if (msg_ptr)
			        statusbar_msg_update_progress(msg_ptr, (i * 100) / fileCount);

				if (!mz_zip_reader_extract_to_file(&zip, i, path, 0))
				{
					sprintf(mess, _("ZIP %s corrupted"), zip_file_path);
					statusbar_msg_add(STATUSBAR_MSG_ERROR, mess);
					ERR("unzip(%s) -> %s failed with %d", zip_file_path, path, zip.m_last_error);
					res = false;
					break;
				}
			}
		}

        if (msg_ptr)
            statusbar_msg_close(msg_ptr);
	}

	mz_zip_reader_end(&zip);

	return res;
}

static void maps_unzip_dir(char *dir_path)
{
	REDDIR * dir = red_opendir(dir_path);
	if (dir != NULL)
	{
		while (true)
		{
			REDDIRENT * entry = red_readdir(dir);
			if (entry == NULL)
				break;

			char *pos = strcasestr(entry->d_name, ".zip");
			if ( pos != NULL)
			{
				char file_path[PATH_LEN] = {0};
				str_join(file_path, 3, dir_path, "/", entry->d_name);

				if (unzip_zipfile(dir_path, file_path, true))
				{
					red_unlink(file_path);                         // Remove ZIP file, if successfull
				}
			}
		}
		red_closedir(dir);
	}
}

static void maps_unzip_task(void * param)
{
	// Wait some time to let other things done before we start
	osDelay(10 * 1000);

	maps_unzip_dir(PATH_MAP_DIR);
	maps_unzip_dir(PATH_TOPO_DIR);

	RedTaskUnregister();
	vTaskDelete(NULL);
}

void maps_unzip_start_task()
{
	// Start in an own task, as it needs more stack and should run in the background.
	xTaskCreate((TaskFunction_t)maps_unzip_task, "maps_unzip", 1024 * 8, NULL, osPriorityLow, NULL);
}


