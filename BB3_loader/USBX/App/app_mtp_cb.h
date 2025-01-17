/*
 * app_mtp_cb.h
 *
 *  Created on: Apr 5, 2022
 *      Author: horinek
 */

#ifndef APP_APP_MTP_CB_H_
#define APP_APP_MTP_CB_H_

#include "common.h"

#include "ux_api.h"
#include "ux_device_class_pima.h"

void mtp_assign_parameters(UX_SLAVE_CLASS_PIMA_PARAMETER * parameter);
void app_mtp_thread_entry(ULONG arg);
bool mtp_session_is_open();
void mtp_session_close_force();

#endif /* APP_APP_MTP_CB_H_ */
