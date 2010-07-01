#ifndef MSG_TYPES_H
#define MSG_TYPES_H

/**
 * @file msg_types.h
 * @brief Basic message type mappings.
 *
 */

/** MSG_type reserved for all message types (MONL only) */
#define MSG_TYPE_ANY  0x00

#define MSG_TYPE_MONL 0x09
#define MSG_TYPE_TOPOLOGY   0x10
#define MSG_TYPE_CHUNK      0x11
#define MSG_TYPE_SIGNALLING 0x12
#define MSG_TYPE_ML_KEEPALIVE 0x13


#endif	/* MSG_TYPES_H */
