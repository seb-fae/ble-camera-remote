/***************************************************************************//**
 * @file
 * @brief Core application logic.
 *******************************************************************************
 * # License
 * <b>Copyright 2020 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: Zlib
 *
 * The licensor of this software is Silicon Laboratories Inc.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 ******************************************************************************/
#include "em_common.h"
#include "app_assert.h"
#include "sl_bluetooth.h"
#include "gatt_db.h"
#include "app.h"
#include "sl_simple_button_instances.h"

// The advertising set handle allocated from Bluetooth stack.
static uint8_t advertising_set_handle = 0xff;
static uint8_t inputReportData[] = {0};
static uint8_t bstate = 0;

void sl_button_on_change(const sl_button_t * handle)
{
  if (handle == &sl_button_btn0)
    {
      if ( sl_button_get_state(handle) == SL_SIMPLE_BUTTON_RELEASED)
      {
        bstate = 0;
        printf("button released\n");
      }
      else
      {
        bstate = 1;
        printf("button pressed\n");
      }
      sl_bt_external_signal(1);
    }
}
/**************************************************************************//**
 * Application Init.
 *****************************************************************************/
SL_WEAK void app_init(void)
{
  /////////////////////////////////////////////////////////////////////////////
  // Put your additional application init code here!                         //
  // This is called once during start-up.                                    //
  /////////////////////////////////////////////////////////////////////////////
}

/**************************************************************************//**
 * Application Process Action.
 *****************************************************************************/
SL_WEAK void app_process_action(void)
{
  /////////////////////////////////////////////////////////////////////////////
  // Put your additional application code here!                              //
  // This is called infinitely.                                              //
  // Do not call blocking functions from here!                               //
  /////////////////////////////////////////////////////////////////////////////
}


static uint8_t app_connection = 0;

/**************************************************************************//**
 * Bluetooth stack event handler.
 * This overrides the dummy weak implementation.
 *
 * @param[in] evt Event coming from the Bluetooth stack.
 *****************************************************************************/
void sl_bt_on_event(sl_bt_msg_t *evt)
{
  sl_status_t sc;
  bd_addr address;
  uint8_t address_type;
  uint8_t system_id[8];

  switch (SL_BT_MSG_ID(evt->header)) {
    // -------------------------------
    // This event indicates the device has started and the radio is ready.
    // Do not call any stack command before receiving this boot event!
    case sl_bt_evt_system_boot_id:

      sl_bt_sm_configure(0, sm_io_capability_noinputnooutput);
      sl_bt_sm_set_bondable_mode(1);

      // Create an advertising set.
      sc = sl_bt_advertiser_create_set(&advertising_set_handle);
      app_assert_status(sc);

      // Set advertising interval to 100ms.
      sc = sl_bt_advertiser_set_timing(
        advertising_set_handle,
        160, // min. adv. interval (milliseconds * 1.6)
        160, // max. adv. interval (milliseconds * 1.6)
        0,   // adv. duration
        0);  // max. num. adv. events
      app_assert_status(sc);
      // Start general advertising and enable connections.
      sc = sl_bt_advertiser_start(
        advertising_set_handle,
        sl_bt_advertiser_general_discoverable,
        sl_bt_advertiser_connectable_scannable);
      app_assert_status(sc);
      printf("Start\n");
      break;

    // -------------------------------
    // This event indicates that a new connection was opened.
    case sl_bt_evt_connection_opened_id:
      sl_bt_sm_increase_security(evt->data.evt_connection_opened.connection);
     break;

    case sl_bt_evt_sm_bonded_id:
     printf("successful bonding\r\n");
     break;

    case sl_bt_evt_sm_bonding_failed_id:
     printf("bonding failed, reason 0x%2X\r\n", evt->data.evt_sm_bonding_failed.reason);
     /* Previous bond is broken, delete it and close connection, host must retry at least once */
     sl_bt_sm_delete_bondings();
     sl_bt_connection_close(evt->data.evt_sm_bonding_failed.connection);

     break;

    case sl_bt_evt_system_external_signal_id:
      memset(inputReportData, 0, sizeof(inputReportData));

      inputReportData[0] = bstate;

      sc = sl_bt_gatt_server_send_notification(app_connection, gattdb_report, 1, inputReportData);
      app_assert_status(sc);

      printf("sent \r\n");

      break;

    case sl_bt_evt_gatt_server_characteristic_status_id:
         if (gattdb_report == evt->data.evt_gatt_server_characteristic_status.characteristic) {
           // client characteristic configuration changed by remote GATT client
           if (sl_bt_gatt_server_client_config == (sl_bt_gatt_server_characteristic_status_flag_t)evt->data.evt_gatt_server_characteristic_status.status_flags) {
             if ((sl_bt_gatt_client_config_flag_t)evt->data.evt_gatt_server_characteristic_status.client_config_flags != sl_bt_gatt_disable)

                 app_connection = evt->data.evt_gatt_server_characteristic_status.connection;
                 printf("notification enabled %d\r\n", app_connection);
           }
         }
         break;

    // -------------------------------
    // This event indicates that a connection was closed.
    case sl_bt_evt_connection_closed_id:
      // Restart advertising after client has disconnected.
      sc = sl_bt_advertiser_start(
        advertising_set_handle,
        sl_bt_advertiser_general_discoverable,
        sl_bt_advertiser_connectable_scannable);
      app_assert_status(sc);
      break;

    ///////////////////////////////////////////////////////////////////////////
    // Add additional event handlers here as your application requires!      //
    ///////////////////////////////////////////////////////////////////////////

    // -------------------------------
    // Default event handler.
    default:
      break;
  }
}
