#include "wii_controller.h"

bd_addr_t wii_remote_bda;
//bd_addr_t wii_bda;
QueueHandle_t transfer_queue_handle;

void wii_mitm_flush_queue();

void find_wii_remote()
{
    printf("scanning for wii remote...\n");
    post_bt_packet(create_hci_inquiry_packet(GAP_IAC_LIMITED_INQUIRY, 30, 0));
}

void handle_wii_mitm_inquiry_result(uint8_t* packet, uint16_t size)
{
    static DISCOVERED_DEVICE* discovered_devices;

    uint8_t num_responses = packet[3];
    uint8_t* p = packet + 4;
    for (int i = 0; i < num_responses; i++)
    {
        bd_addr_t addr;
        //read_bda(p, addr);
        memcpy(addr, p, BDA_SIZE);
        uint8_t psrm = p[6];
        uint32_t cod = read_uint24(p + 9);
        uint16_t clock_offset = read_uint16_be(p + 12) & 0x7fff;

        if (discovered_devices != NULL)
        {
            DISCOVERED_DEVICE* dd = discovered_devices;
            if (memcmp(addr, dd->addr, BDA_SIZE) == 0)
            {
                continue;
            }
        }

        printf("query addr %s psrm %u cod %06lx clock_offset %04x\n", bda_to_string(addr), psrm, cod, clock_offset);

        post_bt_packet(create_hci_remote_name_request_packet(addr, psrm, true, clock_offset));

        if (discovered_devices == NULL)
        {
            discovered_devices = malloc(sizeof(DISCOVERED_DEVICE));
            memcpy(discovered_devices->addr, addr, BDA_SIZE);
            discovered_devices->next = NULL;
        }
        else
        {
            DISCOVERED_DEVICE* fd = discovered_devices;
            while (fd->next != NULL)
            {
                fd = fd->next;
            }
            fd->next = malloc(sizeof(DISCOVERED_DEVICE));
            memcpy(fd->next->addr, addr, BDA_SIZE);
            fd->next->next = NULL;
        }
    }
}

void handle_wii_mitm_inquiry_complete(uint8_t* packet, uint16_t size)
{
    if (wii_controller.state == WII_MITM_DISCOVERY)
    {
        find_wii_remote();
    }
}

void handle_wii_mitm_remote_name_request_complete(uint8_t* packet, uint16_t size)
{
    uint8_t status = packet[3];
    bd_addr_t addr;
    //read_bda(packet + 4, addr);
    memcpy(addr, packet + 4, BDA_SIZE);
    char* name = (char*)(packet + 10);

    printf("found device status 0x%02x %s name %s\n", status, bda_to_string(addr), name);

    if (wii_controller.state == WII_MITM_DISCOVERY)
    {
        if (strcmp(name, WII_REMOTE_NAME) == 0)
        {
            printf("connecting to %s (%s)...\n", bda_to_string(addr), name);

            post_bt_packet(create_hci_inquiry_cancel_packet());
			post_bt_packet(create_hci_create_connection_packet(addr, 0x18, 0, false, 0, 0));
            memcpy(wii_remote_bda, addr, BDA_SIZE);
            wii_controller.state = WII_MITM_DISCOVERED;
        }
    }
}

void handle_wii_mitm_connection_request(HCI_CONNECTION_REQUEST_EVENT_PACKET* packet)
{
    uint32_t cod = uint24_bytes_to_uint32(packet->class_of_device);
    printf("connection request from %s cod %06lx type %u\n", bda_to_string(packet->addr), cod, packet->link_type);

    if ((wii_controller.state == WII_MITM_PAIRING_PENDING || wii_controller.state == WII_MITM_CONNECTION_PENDING)&&
        packet->link_type == HCI_LINK_TYPE_ACL &&
        cod == WII_COD)
    {
        printf("accepting wii connection from %s...\n", bda_to_string(wii_addr));
        memcpy(wii_addr, packet->addr, BDA_SIZE);
        nvs_set_blob(wii_controller.nvs_handle, WII_ADDR_BLOB_NAME, wii_addr, BDA_SIZE);
        wii_controller.state = WII_MITM_CONNECTING_WII;
        post_bt_packet(create_hci_accept_connection_request_packet(packet->addr, HCI_ROLE_SLAVE));
    }
    else if (wii_controller.state == WII_MITM_CONNECTION_PENDING &&
        packet->link_type == HCI_LINK_TYPE_ACL &&
        cod == WII_REMOTE_COD)
    {
        printf("accepting wii remote connection from %s...\n", bda_to_string(packet->addr));
        wii_controller.state = WII_MITM_CONNECTING_WII_REMOTE;
        post_bt_packet(create_hci_accept_connection_request_packet(packet->addr, HCI_ROLE_MASTER));
    }
    else
    {
        printf("rejecting unknown connection...\n");
        post_bt_packet(create_hci_reject_connection_request_packet(packet->addr, ERROR_CODE_CONNECTION_REJECTED_DUE_TO_UNACCEPTABLE_BD_ADDR));
    }
}

void handle_wii_mitm_connection_complete(HCI_CONNECTION_COMPLETE_EVENT_PACKET* packet)
{
    if (packet->status == 0)
    {
        if (wii_controller.state == WII_MITM_CONNECTING_WII_REMOTE)
        {
            if (memcmp(packet->addr, wii_addr, BDA_SIZE) == 0)
            {
                wii_controller.wii_con_handle = packet->con_handle;
                printf("wii connected con_handle 0x%x...\n", packet->con_handle);
            }
            else
            {
                wii_controller.wii_remote_con_handle = packet->con_handle;
                printf("wii remote connected con_handle 0x%x...\n", packet->con_handle);
                printf("connecting to wii at %s...\n", bda_to_string(wii_addr));
                post_bt_packet(create_hci_create_connection_packet(wii_addr, 0x8, 0, false, 0, HCI_ROLE_SLAVE));
            }

            if (wii_controller.wii_con_handle != INVALID_CON_HANDLE &&
                wii_controller.wii_remote_con_handle != INVALID_CON_HANDLE)
            {
                wii_mitm_flush_queue();
            }
        }
        else if (wii_controller.state == WII_MITM_CONNECTING_WII)
        {
            wii_controller.wii_con_handle = packet->con_handle;
            //wii_controller.state = WII_MITM_CONNECTED;
            wii_controller.state = WII_MITM_WAIT_FIRST_PACKET;
            printf("wii connected con_handle 0x%x...\n", packet->con_handle);
        }
        else if (wii_controller.state == WII_MITM_DISCOVERED)
        {
            wii_controller.wii_remote_con_handle = packet->con_handle;

            //wii_controller.state = WII_MITM_PAIRING_PENDING;
            //post_bt_packet(create_hci_authentication_requested_packet(packet->con_handle));

            printf("wii remote connected con_handle 0x%x...\n", packet->con_handle);
            //printf("ready to pair\n");

            printf("wii 0x%x and wii remote 0x%x connected\n", wii_controller.wii_con_handle, wii_controller.wii_remote_con_handle);
            wii_mitm_flush_queue();

            //post_bt_packet(create_hci_authentication_requested_packet(packet->con_handle));
        }
    }
}

void wii_mitm_map_connections(BT_PACKET_ENVELOPE* env)
{
    HCI_ACL_PACKET* acl_packet = (HCI_ACL_PACKET*)env->packet;

    if (acl_packet->con_handle == wii_controller.wii_con_handle)
    {
        //printf("transfer acl packet from wii 0x%x to wii remote 0x%x\n", acl_packet->con_handle, wii_controller.wii_remote_con_handle);
        acl_packet->con_handle = wii_controller.wii_remote_con_handle;
    }
    else if (acl_packet->con_handle == wii_controller.wii_remote_con_handle)
    {
        //printf("transfer acl packet from wii remote 0x%x to wii 0x%x\n", acl_packet->con_handle, wii_controller.wii_con_handle);
        acl_packet->con_handle = wii_controller.wii_con_handle;
    }
    else
    {
        abort();
    }
}

// void send_acl_packet(BT_PACKET_ENVELOPE* env)
// {
//     xSemaphoreTake(all_controller_buffers_sem, portMAX_DELAY);
//     dump_packet(OUTPUT_PACKET, env->packet, env->size);

//     while (!esp_vhci_host_check_send_available());
//     esp_vhci_host_send_packet(env->packet, env->size);

//     HCI_ACL_PACKET* acl_packet = (HCI_ACL_PACKET*)env->packet;

//     uint16_t con_handles[1] = { acl_packet->con_handle };
//     uint16_t num_complete[1] = { 1 };
//     BT_PACKET_ENVELOPE* hncp_env = create_hci_host_number_of_completed_packets_packet(1, con_handles, num_complete);

//     while (!esp_vhci_host_check_send_available());
//     esp_vhci_host_send_packet(hncp_env->packet, hncp_env->size);

//     free(hncp_env);
//     free(env);
// }

void wii_mitm_flush_queue()
{
    BT_PACKET_ENVELOPE* env;

    while (xQueueReceive(transfer_queue_handle, &env, 0))
    {
        wii_mitm_map_connections(env);
        post_bt_packet(env);
        //send_acl_packet(env);
    }
}

void wii_mitm_transfer_packet(uint8_t* packet, uint16_t size)
{
    BT_PACKET_ENVELOPE* env = create_packet_envelope(size);
    //env->io_direction = OUTPUT_PACKET;
    memcpy(env->packet, packet, size);

    if (wii_controller.wii_con_handle != INVALID_CON_HANDLE &&
        wii_controller.wii_remote_con_handle != INVALID_CON_HANDLE)
    {
        wii_mitm_flush_queue();

        wii_mitm_map_connections(env);
        post_bt_packet(env);
        //send_acl_packet(env);
    }
    else
    {
        printf("packet transfer pending\n");
        if (wii_controller.state == WII_MITM_WAIT_FIRST_PACKET)
        {
            wii_controller.state = WII_MITM_DISCOVERY;
            find_wii_remote();
            printf("pair wii remote NOW!\n");
        }

        xQueueSend(transfer_queue_handle, &env, portMAX_DELAY);
    }
}

void handle_wii_mitm_pin_code_request(HCI_PIN_CODE_REQUEST_EVENT_PACKET* packet)
{
    printf("pin code request from %s...\n", bda_to_string(packet->addr));

    uint8_t pin[BDA_SIZE];

    memcpy(pin, wii_addr, BDA_SIZE);
    printf("sending pin code %02x %02x %02x %02x %02x %02x\n", pin[0], pin[1], pin[2], pin[3], pin[4], pin[5]);

    post_bt_packet(create_hci_pin_code_request_reply_packet(packet->addr, pin, BDA_SIZE));
}

void handle_wii_mitm_remote_link_key_request(HCI_LINK_KEY_REQUEST_EVENT_PACKET* packet)
{
    //reverse_bda(packet->addr);

    printf("link key request from %s...\n", bda_to_string(packet->addr));

    switch (wii_controller.state)
    {
        case WII_MITM_CONNECTING_WII_REMOTE:
        {
            uint8_t link_key[HCI_LINK_KEY_SIZE];
            size_t size = HCI_LINK_KEY_SIZE;
            esp_err_t err = nvs_get_blob(wii_controller.nvs_handle, LINK_KEY_BLOB_NAME, link_key, &size);
            if (err == ESP_OK && size == HCI_LINK_KEY_SIZE)
            {
                printf("returning stored link key");
                for (int i = 0; i < HCI_LINK_KEY_SIZE; i++)
                {
                    printf(" %02x", link_key[i]);
                }
                printf("\n");
                post_bt_packet(create_hci_link_key_request_reply_packet(packet->addr, link_key));
            }
            break;
        }
        default:
            break;
    }
}

void wii_mitm_packet_handler(uint8_t* packet, uint16_t size)
{
    HCI_ACL_PACKET* acl_packet = (HCI_ACL_PACKET*)packet;
    HCI_EVENT_PACKET* event_packet = (HCI_EVENT_PACKET*)packet;

    switch (acl_packet->type)
    {
        case HCI_EVENT_PACKET_TYPE:
            switch (event_packet->event_code)
            {
                case HCI_EVENT_CONNECTION_REQUEST:
                    handle_wii_mitm_connection_request((HCI_CONNECTION_REQUEST_EVENT_PACKET*)packet);
                    break;
                case HCI_EVENT_INQUIRY_RESULT:
                    handle_wii_mitm_inquiry_result(packet, size);
                    break;
                case HCI_EVENT_INQUIRY_COMPLETE:
                    handle_wii_mitm_inquiry_complete(packet, size);
                    break;
                case HCI_EVENT_REMOTE_NAME_REQUEST_COMPLETE:
                    handle_wii_mitm_remote_name_request_complete(packet, size);
                    break;
                case HCI_EVENT_CONNECTION_COMPLETE:
                    handle_wii_mitm_connection_complete((HCI_CONNECTION_COMPLETE_EVENT_PACKET*)packet);
                    break;
                case HCI_EVENT_PIN_CODE_REQUEST:
                    handle_wii_mitm_pin_code_request((HCI_PIN_CODE_REQUEST_EVENT_PACKET*)packet);
                    break;
                case HCI_EVENT_LINK_KEY_REQUEST:
                    handle_wii_mitm_remote_link_key_request((HCI_LINK_KEY_REQUEST_EVENT_PACKET*)packet);
                    break;
                // case HCI_EVENT_LINbK_KEY_REQUEST:
                //     handle_wii_mitm_link_key_request((HCI_LINK_KEY_REQUEST_EVENT_PACKET*)packet);
                //     break;
                // case HCI_EVENT_AUTHENTICATION_COMPLETE:
                //     handle_wii_mitm_authentication_complete((HCI_AUTHENTICATION_COMPLETE_EVENT_PACKET*)packet);
                //     break;
                default:
                    break;
            }
            break;
        case HCI_ACL_PACKET_TYPE:
            wii_mitm_transfer_packet(packet, size);
            break;
        default:
            break;
    }
}

void wii_mitm()
{
    post_bt_packet(create_hci_write_class_of_device_packet(WII_REMOTE_COD));
    post_bt_packet(create_hci_write_local_name(WII_REMOTE_NAME));
    post_bt_packet(create_hci_write_default_link_policy_settings_packet(HCI_LINK_POLICY_ENABLE_ROLE_SWITCH | 
                                                                        HCI_LINK_POLICY_ENABLE_SNIFF_MODE | 
                                                                        HCI_LINK_POLICY_ENABLE_SNIFF_MODE | 
                                                                        HCI_LINK_POLICY_ENABLE_HOLD_MODE |
                                                                        HCI_LINK_POLICY_ENABLE_PARK_STATE));
    post_bt_packet(create_hci_secure_connections_host_support_packet(1));
    post_bt_packet(create_hci_current_iac_lap_packet(GAP_IAC_LIMITED_INQUIRY));
    post_bt_packet(create_hci_write_scan_enable_packet(HCI_PAGE_SCAN_ENABLE | HCI_INQUIRY_SCAN_ENABLE));
    post_bt_packet(create_hci_write_pin_type_packet(HCI_FIXED_PIN_TYPE));
    post_bt_packet(create_hci_host_buffer_size_packet(HOST_ACL_BUFFER_SIZE, HOST_SCO_BUFFER_SIZE, HOST_NUM_ACL_BUFFERS, HOST_NUM_SCO_BUFFERS));
    post_bt_packet(create_hci_set_controller_to_host_flow_control_packet(HCI_FLOW_CONTROL_ACL));

    //post_bt_packet(create_hci_write_authentication_enable(1));

    transfer_queue_handle = xQueueCreate(10, sizeof(BT_PACKET_ENVELOPE*));

    wii_controller.state = WII_MITM_CONNECTION_PENDING;

    size_t size = BDA_SIZE;
    esp_err_t ret = nvs_get_blob(wii_controller.nvs_handle, WII_ADDR_BLOB_NAME, wii_addr, &size);
    if (ret == ESP_OK && size == BDA_SIZE)
    {
        printf("stored wii at %s\n", bda_to_string(wii_addr));

    }

    //find_wii_remote();
}
