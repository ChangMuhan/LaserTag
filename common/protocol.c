/**
 * protocol.c
 * 通信协议实现
 */

#include "protocol.h"
#include <string.h>

/**
 * 计算校验和 (简单累加取反)
 */
uint8_t protocol_calc_checksum(const uint8_t *data, uint8_t len) {
    uint8_t sum = 0;
    for (uint8_t i = 0; i < len; i++) {
        sum += data[i];
    }
    return ~sum;
}

/**
 * 验证消息校验和
 */
bool protocol_verify_checksum(const uint8_t *data, uint8_t len) {
    if (len < sizeof(msg_header_t)) {
        return false;
    }

    // 计算除校验和字段外的所有字节
    uint8_t calc_sum = 0;
    for (uint8_t i = 0; i < 3; i++) {  // start_marker, msg_type, length
        calc_sum += data[i];
    }
    for (uint8_t i = 4; i < len; i++) {  // 跳过checksum字段
        calc_sum += data[i];
    }

    return (data[3] == (uint8_t)(~calc_sum));
}

/**
 * 构建ARM消息
 */
void protocol_build_arm(msg_arm_t *msg, uint16_t session_id, uint16_t shot_id) {
    memset(msg, 0, sizeof(msg_arm_t));
    msg->header.start_marker = MSG_START_MARKER;
    msg->header.msg_type = MSG_ARM;
    msg->header.length = sizeof(msg_arm_t) - sizeof(msg_header_t);
    msg->session_id = session_id;
    msg->shot_id = shot_id;

    // 计算校验和
    uint8_t *data = (uint8_t *)msg;
    uint8_t sum = data[0] + data[1] + data[2];  // header前3字节
    sum += (session_id & 0xFF) + (session_id >> 8);
    sum += (shot_id & 0xFF) + (shot_id >> 8);
    msg->header.checksum = ~sum;
}

/**
 * 构建READY消息
 */
void protocol_build_ready(msg_ready_t *msg, uint16_t session_id, uint16_t shot_id) {
    memset(msg, 0, sizeof(msg_ready_t));
    msg->header.start_marker = MSG_START_MARKER;
    msg->header.msg_type = MSG_READY;
    msg->header.length = sizeof(msg_ready_t) - sizeof(msg_header_t);
    msg->session_id = session_id;
    msg->shot_id = shot_id;

    uint8_t *data = (uint8_t *)msg;
    uint8_t sum = data[0] + data[1] + data[2];
    sum += (session_id & 0xFF) + (session_id >> 8);
    sum += (shot_id & 0xFF) + (shot_id >> 8);
    msg->header.checksum = ~sum;
}

/**
 * 构建RESULT消息
 */
void protocol_build_result(msg_result_t *msg, uint16_t session_id,
                          uint16_t shot_id, bool hit, uint16_t score) {
    memset(msg, 0, sizeof(msg_result_t));
    msg->header.start_marker = MSG_START_MARKER;
    msg->header.msg_type = MSG_RESULT;
    msg->header.length = sizeof(msg_result_t) - sizeof(msg_header_t);
    msg->session_id = session_id;
    msg->shot_id = shot_id;
    msg->hit = hit ? 1 : 0;
    msg->score = score;

    uint8_t *data = (uint8_t *)msg;
    uint8_t sum = data[0] + data[1] + data[2];
    sum += (session_id & 0xFF) + (session_id >> 8);
    sum += (shot_id & 0xFF) + (shot_id >> 8);
    sum += msg->hit;
    sum += (score & 0xFF) + (score >> 8);
    msg->header.checksum = ~sum;
}

/**
 * 构建RESET消息
 */
void protocol_build_reset(msg_reset_t *msg, uint16_t session_id) {
    memset(msg, 0, sizeof(msg_reset_t));
    msg->header.start_marker = MSG_START_MARKER;
    msg->header.msg_type = MSG_RESET;
    msg->header.length = sizeof(msg_reset_t) - sizeof(msg_header_t);
    msg->session_id = session_id;

    uint8_t *data = (uint8_t *)msg;
    uint8_t sum = data[0] + data[1] + data[2];
    sum += (session_id & 0xFF) + (session_id >> 8);
    msg->header.checksum = ~sum;
}

/**
 * 构建STATE消息
 */
void protocol_build_state(msg_state_t *msg, uint16_t session_id,
                         uint16_t score, uint8_t ammo, uint8_t conn_status) {
    memset(msg, 0, sizeof(msg_state_t));
    msg->header.start_marker = MSG_START_MARKER;
    msg->header.msg_type = MSG_STATE;
    msg->header.length = sizeof(msg_state_t) - sizeof(msg_header_t);
    msg->session_id = session_id;
    msg->score = score;
    msg->ammo = ammo;
    msg->conn_status = conn_status;

    uint8_t *data = (uint8_t *)msg;
    uint8_t sum = data[0] + data[1] + data[2];
    sum += (session_id & 0xFF) + (session_id >> 8);
    sum += (score & 0xFF) + (score >> 8);
    sum += ammo + conn_status;
    msg->header.checksum = ~sum;
}

/**
 * 解析接收到的消息
 * 返回: 消息类型, 失败返回-1
 */
int protocol_parse_message(const uint8_t *data, uint8_t len, void *msg_out) {
    if (len < sizeof(msg_header_t) || data[0] != MSG_START_MARKER) {
        return -1;
    }

    // 验证校验和
    if (!protocol_verify_checksum(data, len)) {
        return -1;
    }

    msg_type_t type = (msg_type_t)data[1];

    // 根据类型复制数据
    switch (type) {
        case MSG_ARM:
            if (len >= sizeof(msg_arm_t)) {
                memcpy(msg_out, data, sizeof(msg_arm_t));
                return MSG_ARM;
            }
            break;

        case MSG_READY:
            if (len >= sizeof(msg_ready_t)) {
                memcpy(msg_out, data, sizeof(msg_ready_t));
                return MSG_READY;
            }
            break;

        case MSG_RESULT:
            if (len >= sizeof(msg_result_t)) {
                memcpy(msg_out, data, sizeof(msg_result_t));
                return MSG_RESULT;
            }
            break;

        case MSG_RESET:
            if (len >= sizeof(msg_reset_t)) {
                memcpy(msg_out, data, sizeof(msg_reset_t));
                return MSG_RESET;
            }
            break;

        case MSG_STATE:
            if (len >= sizeof(msg_state_t)) {
                memcpy(msg_out, data, sizeof(msg_state_t));
                return MSG_STATE;
            }
            break;

        default:
            return -1;
    }

    return -1;
}
