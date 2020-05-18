#include "sdk_common.h"
#include "ble_srv_common.h"
#include "ble_cus.h"
#include <string.h>
#include "nrf_gpio.h"
#include "boards.h"
#include "nrf_log.h"
#include "bsp.h"

// Function Prototypes
static uint32_t custom_value_char_add(ble_cus_t * p_cus, const ble_cus_init_t * p_cus_init);
uint32_t ble_cus_init(ble_cus_t * p_cus, const ble_cus_init_t * p_cus_init);
static void on_connect(ble_cus_t * p_cus, ble_evt_t const * p_ble_evt);
static void on_disconnect(ble_cus_t * p_cus, ble_evt_t const * p_ble_evt);
static void on_write(ble_cus_t * p_cus, ble_evt_t const * p_ble_evt);
void ble_cus_on_ble_evt( ble_evt_t const * p_ble_evt, void * p_context);


/**@brief Function for adding the Custom Value characteristic.
 *
 * @param[in]   p_cus        Custom Service structure.
 * @param[in]   p_cus_init   Information needed to initialize the service.
 *
 * @return      NRF_SUCCESS on success, otherwise an error code.
 */
static uint32_t custom_value_char_add(ble_cus_t * p_cus, const ble_cus_init_t * p_cus_init)
{
		ret_code_t						err_code;
		ble_uuid_t						ble_uuid;
		ble_uuid128_t					cus_base_uuid = CUSTOM_SERVICE_UUID_BASE;
		ble_add_char_params_t	add_char_params;
		
		VERIFY_PARAM_NOT_NULL(p_cus);
    VERIFY_PARAM_NOT_NULL(p_cus_init);
		
		// Initialize the service structure.
    p_cus->data_handler = p_cus_init->data_handler;
		
		// Add a custom base UUID.
    err_code = sd_ble_uuid_vs_add(&cus_base_uuid, &p_cus->uuid_type);
    VERIFY_SUCCESS(err_code);

    ble_uuid.type = p_cus->uuid_type;
    ble_uuid.uuid = CUSTOM_SERVICE_UUID;
		
		// Add the service.
    err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY,
                                        &ble_uuid,
                                        &p_cus->service_handle);
    /**@snippet [Adding proprietary Service to the SoftDevice] */
    VERIFY_SUCCESS(err_code);
		
		memset(&add_char_params, 0, sizeof(add_char_params));
    add_char_params.uuid                     = CUSTOM_VALUE_CHAR_UUID;
    add_char_params.uuid_type                = p_cus->uuid_type;
    add_char_params.max_len                  = 2;
    add_char_params.init_len                 = sizeof(uint8_t);
    add_char_params.is_var_len               = true;
		add_char_params.char_props.read					 = 1;
    add_char_params.char_props.write         = 1;
    add_char_params.char_props.write_wo_resp = 0;
		add_char_params.char_props.notify				 = 1;
		add_char_params.is_defered_read					 = 0;
		add_char_params.is_defered_write				 = 1;

    add_char_params.read_access  = SEC_OPEN;
    add_char_params.write_access = SEC_OPEN;
		add_char_params.cccd_write_access = SEC_OPEN;
		
		err_code = characteristic_add(p_cus->service_handle, &add_char_params, &p_cus->custom_value_handles);
		if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    return NRF_SUCCESS;
}

uint32_t ble_cus_init(ble_cus_t * p_cus, const ble_cus_init_t * p_cus_init)
{
    if (p_cus == NULL || p_cus_init == NULL)
    {
        return NRF_ERROR_NULL;
    }

    uint32_t   err_code;
    ble_uuid_t ble_uuid;

    // Initialize service structure
		p_cus->evt_handler							 = p_cus_init->evt_handler;
    p_cus->conn_handle               = BLE_CONN_HANDLE_INVALID;

    // Add Custom Service UUID
    ble_uuid128_t base_uuid = {CUSTOM_SERVICE_UUID_BASE};
    err_code =  sd_ble_uuid_vs_add(&base_uuid, &p_cus->uuid_type);
    VERIFY_SUCCESS(err_code);
    
    ble_uuid.type = p_cus->uuid_type;
    ble_uuid.uuid = CUSTOM_SERVICE_UUID;

    // Add the Custom Service
//    err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &ble_uuid, &p_cus->service_handle);
//    if (err_code != NRF_SUCCESS)
//    {
//        return err_code;
//    }
//		
    // Add Custom Value characteristic
    return custom_value_char_add(p_cus, p_cus_init);
}

/**@brief Function for handling the Connect event.
 *
 * @param[in]   p_cus       Custom Service structure.
 * @param[in]   p_ble_evt   Event received from the BLE stack.
 */
static void on_connect(ble_cus_t * p_cus, ble_evt_t const * p_ble_evt)
{
    p_cus->conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
		NRF_LOG_INFO("peer %d connected", p_ble_evt->evt.gap_evt.conn_handle);
		ble_cus_evt_t evt;
	
		evt.evt_type = BLE_CUS_EVT_CONNECTED;
	
		p_cus -> evt_handler(p_cus, &evt);
}

/**@brief Function for handling the Disconnect event.
 *
 * @param[in]   p_cus       Custom Service structure.
 * @param[in]   p_ble_evt   Event received from the BLE stack.
 */
static void on_disconnect(ble_cus_t * p_cus, ble_evt_t const * p_ble_evt)
{
    UNUSED_PARAMETER(p_ble_evt);
    p_cus->conn_handle = BLE_CONN_HANDLE_INVALID;
	
	  ble_cus_evt_t evt;

    evt.evt_type = BLE_CUS_EVT_DISCONNECTED;

    p_cus->evt_handler(p_cus, &evt);
}

/**@brief Function for handling the Write event.
 *
 * @param[in]   p_cus       Custom Service structure.
 * @param[in]   p_ble_evt   Event received from the BLE stack.
 */

static void on_write(ble_cus_t * p_cus, ble_evt_t const * p_ble_evt)
{
   ble_gatts_evt_write_t const * p_evt_write = &p_ble_evt->evt.gatts_evt.params.write;
   
    // Custom Value Characteristic Written to.
    if (p_evt_write->handle == p_cus->custom_value_handles.value_handle)
    {
       bsp_indication_set(BSP_INDICATE_RCV_OK);
			 NRF_LOG_INFO("data received.....");
    }
		
		if ((p_evt_write->handle == p_cus->custom_value_handles.cccd_handle) && (p_evt_write->len == 2))
		{
			if (p_cus->evt_handler != NULL)
			{
				ble_cus_evt_t evt;
				
				if (ble_srv_is_notification_enabled(p_evt_write->data))
				{
					evt.evt_type = BLE_CUS_EVT_NOTIFICATION_ENABLED;
				}
				else
				{
					evt.evt_type = BLE_CUS_EVT_NOTIFICATION_DISABLED;					
				}
				
				// call application event handler
				p_cus->evt_handler(p_cus, &evt);
			}
		}
}

static void on_write_auth(ble_cus_t * p_cus, ble_evt_t const * p_ble_evt)
{
   ble_gatts_evt_rw_authorize_request_t	req = p_ble_evt->evt.gatts_evt.params.authorize_request;
		NRF_LOG_INFO("in authorized write function.....");
		if(req.type == BLE_GATTS_AUTHORIZE_TYPE_WRITE && req.request.write.handle == p_cus->custom_value_handles.value_handle)
		{
				ble_gatts_rw_authorize_reply_params_t auth_reply;
        memset(&auth_reply, 0, sizeof(auth_reply));
        auth_reply.type = BLE_GATTS_AUTHORIZE_TYPE_WRITE;
        auth_reply.params.write.update = 1;
				auth_reply.params.read.update  = 1;
						uint8_t value = *(uint8_t *)req.request.write.data;
			
            auth_reply.params.write.gatt_status = BLE_GATT_STATUS_SUCCESS;
            auth_reply.params.write.offset = 0;
            auth_reply.params.write.len = 2;
						auth_reply.params.write.p_data = &value;
				NRF_LOG_INFO("value received: 0x%x  p_data: 0x%x\r\n", value, *auth_reply.params.write.p_data);
				NRF_LOG_INFO("sending response.....");
				NRF_LOG_INFO("peer %u connected", p_ble_evt->evt.gap_evt.conn_handle);
				uint32_t err_code = sd_ble_gatts_rw_authorize_reply(p_cus->conn_handle, &auth_reply);
				APP_ERROR_CHECK(err_code);
				NRF_LOG_INFO("error code: %d", err_code);
				NRF_LOG_INFO("response sent.....");
			/*ble_gatts_value_t p_value;
      uint32_t d;
      p_value.len = 4;
      p_value.offset = 0;
      p_value.p_value = (uint8_t*)&d;
      err_code = sd_ble_gatts_value_get(p_ble_evt->evt.gatts_evt.conn_handle, req.request.write.handle, &p_value);
				APP_ERROR_CHECK(err_code);*/
		}
		
}

void ble_cus_on_ble_evt( ble_evt_t const * p_ble_evt, void * p_context)
{
    ble_cus_t * p_cus = (ble_cus_t *) p_context;
    
    if (p_cus == NULL || p_ble_evt == NULL)
    {
        return;
    }
    
    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_CONNECTED:
						NRF_LOG_INFO("in connect case.....");
						on_connect(p_cus, p_ble_evt);
						
            break;

        case BLE_GAP_EVT_DISCONNECTED:
						NRF_LOG_INFO("in disconnect case.....");
						on_disconnect(p_cus, p_ble_evt);
						
            break;

				case BLE_GATTS_EVT_WRITE:
						NRF_LOG_INFO("in write case.....");
						on_write(p_cus, p_ble_evt);
						
						break;
				
				case BLE_GATTS_EVT_RW_AUTHORIZE_REQUEST:
						NRF_LOG_INFO("in authorized write case.....");
						on_write_auth(p_cus, p_ble_evt);
						
						break;
								
        default:
            // No implementation needed.
            break;
    }
}

uint32_t ble_cus_custom_value_update(ble_cus_t * p_cus, uint8_t custom_value)
{

		//NRF_LOG_INFO("In ble_cus_custom_value_update. \r\n"); 
    if (p_cus == NULL)
    {
        return NRF_ERROR_NULL;
    }

    uint32_t err_code = NRF_SUCCESS;
    ble_gatts_value_t gatts_value;

    // Initialize value struct.
    memset(&gatts_value, 0, sizeof(gatts_value));

    gatts_value.len     = sizeof(uint8_t);
    gatts_value.offset  = 0;
    gatts_value.p_value = &custom_value;

    // Update database.
    err_code = sd_ble_gatts_value_set(p_cus->conn_handle,
                                      p_cus->custom_value_handles.value_handle,
                                      &gatts_value);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    // Send value if connected and notifying.
    if ((p_cus->conn_handle != BLE_CONN_HANDLE_INVALID)) 
    {
        ble_gatts_hvx_params_t hvx_params;

        memset(&hvx_params, 0, sizeof(hvx_params));

        hvx_params.handle = p_cus->custom_value_handles.value_handle;
        hvx_params.type   = BLE_GATT_HVX_NOTIFICATION;
        hvx_params.offset = gatts_value.offset;
        hvx_params.p_len  = &gatts_value.len;
        hvx_params.p_data = gatts_value.p_value;

        err_code = sd_ble_gatts_hvx(p_cus->conn_handle, &hvx_params);
        //NRF_LOG_INFO("sd_ble_gatts_hvx result: %x. \r\n", err_code); 
    }
    else
    {
        err_code = NRF_ERROR_INVALID_STATE;
        //NRF_LOG_INFO("sd_ble_gatts_hvx result: NRF_ERROR_INVALID_STATE. \r\n"); 
    }
	return err_code;
}
