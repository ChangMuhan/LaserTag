/**
 * protocol.h
 * 枪靶双端通信协议
 * 负责人: C (通信显示负责人)
 */

#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>
#include <stdbool.h>

/* ===== 消息类型 ===== */
typedef enum {
    MSG_ARM = 0,        // 枪端: 射击准备
    MSG_READY,          // 靶端: 可以发射
    MSG_RESULT,         // 靶端: 射击结果
    MSG_RESET,          // 任一端: 新局重置
    MSG_STATE,          // 任一端: 状态同步
    MSG_HEARTBEAT,      // 心跳保活
    MSG_ACK             // 确认应答
} msg_type_t;

/* ===== 消息结构 ===== */
#pragma pack(push, 1)

// 消息头
typedef struct {
    uint8_t start_marker;       // 起始标记 0xAA
    uint8_t msg_type;           // 消息类型
    uint8_t length;             // 负载长度
    uint8_t checksum;           // 校验和
} msg_header_t;

// ARM: 射击准备
typedef struct {
    msg_header_t header;
    uint16_t session_id;        // 局号
    uint16_t shot_id;           // 射击编号
} msg_arm_t;

// READY: 可以发射
typedef struct {
    msg_header_t header;
    uint16_t session_id;
    uint16_t shot_id;
} msg_ready_t;

// RESULT: 射击结果
typedef struct {
    msg_header_t header;
    uint16_t session_id;
    uint16_t shot_id;
    uint8_t hit;                // 1=命中, 0=未命中
    uint16_t score;             // 当前总分
} msg_result_t;

// RESET: 新局重置
typedef struct {
    msg_header_t header;
    uint16_t session_id;        // 新的局号
} msg_reset_t;

// STATE: 状态同步
typedef struct {
    msg_header_t header;
    uint16_t session_id;
    uint16_t score;
    uint8_t ammo;
    uint8_t conn_status;        // 连接状态
} msg_state_t;

// HEARTBEAT: 心跳
typedef struct {
    msg_header_t header;
    uint32_t timestamp;
} msg_heartbeat_t;

// ACK: 确认
typedef struct {
    msg_header_t header;
    uint8_t ack_msg_type;       // 确认哪种消息
    uint16_t ack_id;            // 确认的ID
} msg_ack_t;

#pragma pack(pop)

/* ===== 协议常量 ===== */
#define MSG_START_MARKER        0xAA
#define MSG_MAX_SIZE            64
#define MSG_TIMEOUT_MS          1000

/* ===== 函数接口 ===== */

/**
 * 计算校验和 (简单累加)
 */
uint8_t protocol_calc_checksum(const uint8_t *data, uint8_t len);

/**
 * 验证消息校验和
 */
bool protocol_verify_checksum(const uint8_t *data, uint8_t len);

/**
 * 构建ARM消息
 */
void protocol_build_arm(msg_arm_t *msg, uint16_t session_id, uint16_t shot_id);

/**
 * 构建READY消息
 */
void protocol_build_ready(msg_ready_t *msg, uint16_t session_id, uint16_t shot_id);

/**
 * 构建RESULT消息
 */
void protocol_build_result(msg_result_t *msg, uint16_t session_id,
                          uint16_t shot_id, bool hit, uint16_t score);

/**
 * 构建RESET消息
 */
void protocol_build_reset(msg_reset_t *msg, uint16_t session_id);

/**
 * 构建STATE消息
 */
void protocol_build_state(msg_state_t *msg, uint16_t session_id,
                         uint16_t score, uint8_t ammo, uint8_t conn_status);

/**
 * 解析接收到的消息
 * 返回: 消息类型, 失败返回-1
 */
int protocol_parse_message(const uint8_t *data, uint8_t len, void *msg_out);

#endif // PROTOCOL_H
