/*
 * Base64 encoding/decoding (RFC1341)
 * Copyright (c) 2005, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#ifndef _BASE64_H_e9d284c2f319e5639e261724276ea80c98e49e4e64299a3a990196ea
#define _BASE64_H_e9d284c2f319e5639e261724276ea80c98e49e4e64299a3a990196ea

#include <stddef.h>
#include <stdint.h>

uint8_t *base64_encode(uint8_t *src, size_t len, size_t *out_len);
uint8_t *base64_decode(uint8_t *src, size_t len, size_t *out_len);

#endif /* BASE64_H */
