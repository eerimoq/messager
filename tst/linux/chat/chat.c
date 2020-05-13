/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Erik Moqvist
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/**
 * This file was generated by pbtools.
 */

#include <limits.h>
#include "chat.h"

#if CHAR_BIT != 8
#    error "Number of bits in a char must be 8."
#endif

void chat_connect_req_init(
    struct chat_connect_req_t *self_p,
    struct pbtools_heap_t *heap_p)
{
    self_p->base.heap_p = heap_p;
    self_p->user_p = "";
}

void chat_connect_req_encode_inner(
    struct pbtools_encoder_t *encoder_p,
    struct chat_connect_req_t *self_p)
{
    pbtools_encoder_write_string(encoder_p, 1, self_p->user_p);
}

void chat_connect_req_decode_inner(
    struct pbtools_decoder_t *decoder_p,
    struct chat_connect_req_t *self_p)
{
    int wire_type;

    while (pbtools_decoder_available(decoder_p)) {
        switch (pbtools_decoder_read_tag(decoder_p, &wire_type)) {

        case 1:
            pbtools_decoder_read_string(decoder_p, wire_type, &self_p->user_p);
            break;

        default:
            pbtools_decoder_skip_field(decoder_p, wire_type);
            break;
        }
    }
}

void chat_connect_req_encode_repeated_inner(
    struct pbtools_encoder_t *encoder_p,
    int field_number,
    struct chat_connect_req_repeated_t *repeated_p)
{
    pbtools_encode_repeated_inner(
        encoder_p,
        field_number,
        (struct pbtools_repeated_message_t *)repeated_p,
        sizeof(struct chat_connect_req_t),
        (pbtools_message_encode_inner_t)chat_connect_req_encode_inner);
}

void chat_connect_req_decode_repeated_inner(
    struct pbtools_decoder_t *decoder_p,
    struct pbtools_repeated_info_t *repeated_info_p,
    struct chat_connect_req_repeated_t *repeated_p)
{
    pbtools_decode_repeated_inner(
        decoder_p,
        repeated_info_p,
        (struct pbtools_repeated_message_t *)repeated_p,
        sizeof(struct chat_connect_req_t),
        (pbtools_message_init_t)chat_connect_req_init,
        (pbtools_message_decode_inner_t)chat_connect_req_decode_inner);
}

struct chat_connect_req_t *
chat_connect_req_new(
    void *workspace_p,
    size_t size)
{
    return (pbtools_message_new(
                workspace_p,
                size,
                sizeof(struct chat_connect_req_t),
                (pbtools_message_init_t)chat_connect_req_init));
}

int chat_connect_req_encode(
    struct chat_connect_req_t *self_p,
    uint8_t *encoded_p,
    size_t size)
{
    return (pbtools_message_encode(
                &self_p->base,
                encoded_p,
                size,
                (pbtools_message_encode_inner_t)chat_connect_req_encode_inner));
}

int chat_connect_req_decode(
    struct chat_connect_req_t *self_p,
    const uint8_t *encoded_p,
    size_t size)
{
    return (pbtools_message_decode(
                &self_p->base,
                encoded_p,
                size,
                (pbtools_message_decode_inner_t)chat_connect_req_decode_inner));
}

void chat_message_ind_init(
    struct chat_message_ind_t *self_p,
    struct pbtools_heap_t *heap_p)
{
    self_p->base.heap_p = heap_p;
    self_p->user_p = "";
    self_p->text_p = "";
}

void chat_message_ind_encode_inner(
    struct pbtools_encoder_t *encoder_p,
    struct chat_message_ind_t *self_p)
{
    pbtools_encoder_write_string(encoder_p, 2, self_p->text_p);
    pbtools_encoder_write_string(encoder_p, 1, self_p->user_p);
}

void chat_message_ind_decode_inner(
    struct pbtools_decoder_t *decoder_p,
    struct chat_message_ind_t *self_p)
{
    int wire_type;

    while (pbtools_decoder_available(decoder_p)) {
        switch (pbtools_decoder_read_tag(decoder_p, &wire_type)) {

        case 1:
            pbtools_decoder_read_string(decoder_p, wire_type, &self_p->user_p);
            break;

        case 2:
            pbtools_decoder_read_string(decoder_p, wire_type, &self_p->text_p);
            break;

        default:
            pbtools_decoder_skip_field(decoder_p, wire_type);
            break;
        }
    }
}

void chat_message_ind_encode_repeated_inner(
    struct pbtools_encoder_t *encoder_p,
    int field_number,
    struct chat_message_ind_repeated_t *repeated_p)
{
    pbtools_encode_repeated_inner(
        encoder_p,
        field_number,
        (struct pbtools_repeated_message_t *)repeated_p,
        sizeof(struct chat_message_ind_t),
        (pbtools_message_encode_inner_t)chat_message_ind_encode_inner);
}

void chat_message_ind_decode_repeated_inner(
    struct pbtools_decoder_t *decoder_p,
    struct pbtools_repeated_info_t *repeated_info_p,
    struct chat_message_ind_repeated_t *repeated_p)
{
    pbtools_decode_repeated_inner(
        decoder_p,
        repeated_info_p,
        (struct pbtools_repeated_message_t *)repeated_p,
        sizeof(struct chat_message_ind_t),
        (pbtools_message_init_t)chat_message_ind_init,
        (pbtools_message_decode_inner_t)chat_message_ind_decode_inner);
}

struct chat_message_ind_t *
chat_message_ind_new(
    void *workspace_p,
    size_t size)
{
    return (pbtools_message_new(
                workspace_p,
                size,
                sizeof(struct chat_message_ind_t),
                (pbtools_message_init_t)chat_message_ind_init));
}

int chat_message_ind_encode(
    struct chat_message_ind_t *self_p,
    uint8_t *encoded_p,
    size_t size)
{
    return (pbtools_message_encode(
                &self_p->base,
                encoded_p,
                size,
                (pbtools_message_encode_inner_t)chat_message_ind_encode_inner));
}

int chat_message_ind_decode(
    struct chat_message_ind_t *self_p,
    const uint8_t *encoded_p,
    size_t size)
{
    return (pbtools_message_decode(
                &self_p->base,
                encoded_p,
                size,
                (pbtools_message_decode_inner_t)chat_message_ind_decode_inner));
}

void chat_client_to_server_messages_connect_req_init(
    struct chat_client_to_server_t *self_p)
{
    self_p->messages.choice = chat_client_to_server_messages_choice_connect_req_e;
    chat_connect_req_init(
        &self_p->messages.value.connect_req,
        self_p->base.heap_p);
}

void chat_client_to_server_messages_message_ind_init(
    struct chat_client_to_server_t *self_p)
{
    self_p->messages.choice = chat_client_to_server_messages_choice_message_ind_e;
    chat_message_ind_init(
        &self_p->messages.value.message_ind,
        self_p->base.heap_p);
}

void chat_client_to_server_messages_encode(
    struct pbtools_encoder_t *encoder_p,
    struct chat_client_to_server_messages_oneof_t *self_p)
{
    switch (self_p->choice) {

    case chat_client_to_server_messages_choice_connect_req_e:
        pbtools_encoder_sub_message_encode_always(
            encoder_p,
            1,
            &self_p->value.connect_req.base,
            (pbtools_message_encode_inner_t)chat_connect_req_encode_inner);
        break;

    case chat_client_to_server_messages_choice_message_ind_e:
        pbtools_encoder_sub_message_encode_always(
            encoder_p,
            2,
            &self_p->value.message_ind.base,
            (pbtools_message_encode_inner_t)chat_message_ind_encode_inner);
        break;

    default:
        break;
    }
}

static void chat_client_to_server_messages_connect_req_decode(
    struct pbtools_decoder_t *decoder_p,
    int wire_type,
    struct chat_client_to_server_t *self_p)
{
    chat_client_to_server_messages_connect_req_init(self_p);
    pbtools_decoder_sub_message_decode(
        decoder_p,
        wire_type,
        &self_p->messages.value.connect_req.base,
        (pbtools_message_decode_inner_t)chat_connect_req_decode_inner);
}

static void chat_client_to_server_messages_message_ind_decode(
    struct pbtools_decoder_t *decoder_p,
    int wire_type,
    struct chat_client_to_server_t *self_p)
{
    chat_client_to_server_messages_message_ind_init(self_p);
    pbtools_decoder_sub_message_decode(
        decoder_p,
        wire_type,
        &self_p->messages.value.message_ind.base,
        (pbtools_message_decode_inner_t)chat_message_ind_decode_inner);
}

void chat_client_to_server_init(
    struct chat_client_to_server_t *self_p,
    struct pbtools_heap_t *heap_p)
{
    self_p->base.heap_p = heap_p;
    self_p->messages.choice = 0;
}

void chat_client_to_server_encode_inner(
    struct pbtools_encoder_t *encoder_p,
    struct chat_client_to_server_t *self_p)
{
    chat_client_to_server_messages_encode(encoder_p, &self_p->messages);
}

void chat_client_to_server_decode_inner(
    struct pbtools_decoder_t *decoder_p,
    struct chat_client_to_server_t *self_p)
{
    int wire_type;

    while (pbtools_decoder_available(decoder_p)) {
        switch (pbtools_decoder_read_tag(decoder_p, &wire_type)) {

        case 1:
            chat_client_to_server_messages_connect_req_decode(
                decoder_p,
                wire_type,
                self_p);
            break;

        case 2:
            chat_client_to_server_messages_message_ind_decode(
                decoder_p,
                wire_type,
                self_p);
            break;

        default:
            pbtools_decoder_skip_field(decoder_p, wire_type);
            break;
        }
    }
}

void chat_client_to_server_encode_repeated_inner(
    struct pbtools_encoder_t *encoder_p,
    int field_number,
    struct chat_client_to_server_repeated_t *repeated_p)
{
    pbtools_encode_repeated_inner(
        encoder_p,
        field_number,
        (struct pbtools_repeated_message_t *)repeated_p,
        sizeof(struct chat_client_to_server_t),
        (pbtools_message_encode_inner_t)chat_client_to_server_encode_inner);
}

void chat_client_to_server_decode_repeated_inner(
    struct pbtools_decoder_t *decoder_p,
    struct pbtools_repeated_info_t *repeated_info_p,
    struct chat_client_to_server_repeated_t *repeated_p)
{
    pbtools_decode_repeated_inner(
        decoder_p,
        repeated_info_p,
        (struct pbtools_repeated_message_t *)repeated_p,
        sizeof(struct chat_client_to_server_t),
        (pbtools_message_init_t)chat_client_to_server_init,
        (pbtools_message_decode_inner_t)chat_client_to_server_decode_inner);
}

struct chat_client_to_server_t *
chat_client_to_server_new(
    void *workspace_p,
    size_t size)
{
    return (pbtools_message_new(
                workspace_p,
                size,
                sizeof(struct chat_client_to_server_t),
                (pbtools_message_init_t)chat_client_to_server_init));
}

int chat_client_to_server_encode(
    struct chat_client_to_server_t *self_p,
    uint8_t *encoded_p,
    size_t size)
{
    return (pbtools_message_encode(
                &self_p->base,
                encoded_p,
                size,
                (pbtools_message_encode_inner_t)chat_client_to_server_encode_inner));
}

int chat_client_to_server_decode(
    struct chat_client_to_server_t *self_p,
    const uint8_t *encoded_p,
    size_t size)
{
    return (pbtools_message_decode(
                &self_p->base,
                encoded_p,
                size,
                (pbtools_message_decode_inner_t)chat_client_to_server_decode_inner));
}

void chat_connect_rsp_init(
    struct chat_connect_rsp_t *self_p,
    struct pbtools_heap_t *heap_p)
{
    self_p->base.heap_p = heap_p;

}

void chat_connect_rsp_encode_inner(
    struct pbtools_encoder_t *encoder_p,
    struct chat_connect_rsp_t *self_p)
{
    (void)encoder_p;
    (void)self_p;
}

void chat_connect_rsp_decode_inner(
    struct pbtools_decoder_t *decoder_p,
    struct chat_connect_rsp_t *self_p)
{
    (void)self_p;

    int wire_type;

    while (pbtools_decoder_available(decoder_p)) {
        switch (pbtools_decoder_read_tag(decoder_p, &wire_type)) {


        default:
            pbtools_decoder_skip_field(decoder_p, wire_type);
            break;
        }
    }
}

void chat_connect_rsp_encode_repeated_inner(
    struct pbtools_encoder_t *encoder_p,
    int field_number,
    struct chat_connect_rsp_repeated_t *repeated_p)
{
    pbtools_encode_repeated_inner(
        encoder_p,
        field_number,
        (struct pbtools_repeated_message_t *)repeated_p,
        sizeof(struct chat_connect_rsp_t),
        (pbtools_message_encode_inner_t)chat_connect_rsp_encode_inner);
}

void chat_connect_rsp_decode_repeated_inner(
    struct pbtools_decoder_t *decoder_p,
    struct pbtools_repeated_info_t *repeated_info_p,
    struct chat_connect_rsp_repeated_t *repeated_p)
{
    pbtools_decode_repeated_inner(
        decoder_p,
        repeated_info_p,
        (struct pbtools_repeated_message_t *)repeated_p,
        sizeof(struct chat_connect_rsp_t),
        (pbtools_message_init_t)chat_connect_rsp_init,
        (pbtools_message_decode_inner_t)chat_connect_rsp_decode_inner);
}

struct chat_connect_rsp_t *
chat_connect_rsp_new(
    void *workspace_p,
    size_t size)
{
    return (pbtools_message_new(
                workspace_p,
                size,
                sizeof(struct chat_connect_rsp_t),
                (pbtools_message_init_t)chat_connect_rsp_init));
}

int chat_connect_rsp_encode(
    struct chat_connect_rsp_t *self_p,
    uint8_t *encoded_p,
    size_t size)
{
    return (pbtools_message_encode(
                &self_p->base,
                encoded_p,
                size,
                (pbtools_message_encode_inner_t)chat_connect_rsp_encode_inner));
}

int chat_connect_rsp_decode(
    struct chat_connect_rsp_t *self_p,
    const uint8_t *encoded_p,
    size_t size)
{
    return (pbtools_message_decode(
                &self_p->base,
                encoded_p,
                size,
                (pbtools_message_decode_inner_t)chat_connect_rsp_decode_inner));
}

void chat_server_to_client_messages_connect_rsp_init(
    struct chat_server_to_client_t *self_p)
{
    self_p->messages.choice = chat_server_to_client_messages_choice_connect_rsp_e;
    chat_connect_rsp_init(
        &self_p->messages.value.connect_rsp,
        self_p->base.heap_p);
}

void chat_server_to_client_messages_message_ind_init(
    struct chat_server_to_client_t *self_p)
{
    self_p->messages.choice = chat_server_to_client_messages_choice_message_ind_e;
    chat_message_ind_init(
        &self_p->messages.value.message_ind,
        self_p->base.heap_p);
}

void chat_server_to_client_messages_encode(
    struct pbtools_encoder_t *encoder_p,
    struct chat_server_to_client_messages_oneof_t *self_p)
{
    switch (self_p->choice) {

    case chat_server_to_client_messages_choice_connect_rsp_e:
        pbtools_encoder_sub_message_encode_always(
            encoder_p,
            1,
            &self_p->value.connect_rsp.base,
            (pbtools_message_encode_inner_t)chat_connect_rsp_encode_inner);
        break;

    case chat_server_to_client_messages_choice_message_ind_e:
        pbtools_encoder_sub_message_encode_always(
            encoder_p,
            2,
            &self_p->value.message_ind.base,
            (pbtools_message_encode_inner_t)chat_message_ind_encode_inner);
        break;

    default:
        break;
    }
}

static void chat_server_to_client_messages_connect_rsp_decode(
    struct pbtools_decoder_t *decoder_p,
    int wire_type,
    struct chat_server_to_client_t *self_p)
{
    chat_server_to_client_messages_connect_rsp_init(self_p);
    pbtools_decoder_sub_message_decode(
        decoder_p,
        wire_type,
        &self_p->messages.value.connect_rsp.base,
        (pbtools_message_decode_inner_t)chat_connect_rsp_decode_inner);
}

static void chat_server_to_client_messages_message_ind_decode(
    struct pbtools_decoder_t *decoder_p,
    int wire_type,
    struct chat_server_to_client_t *self_p)
{
    chat_server_to_client_messages_message_ind_init(self_p);
    pbtools_decoder_sub_message_decode(
        decoder_p,
        wire_type,
        &self_p->messages.value.message_ind.base,
        (pbtools_message_decode_inner_t)chat_message_ind_decode_inner);
}

void chat_server_to_client_init(
    struct chat_server_to_client_t *self_p,
    struct pbtools_heap_t *heap_p)
{
    self_p->base.heap_p = heap_p;
    self_p->messages.choice = 0;
}

void chat_server_to_client_encode_inner(
    struct pbtools_encoder_t *encoder_p,
    struct chat_server_to_client_t *self_p)
{
    chat_server_to_client_messages_encode(encoder_p, &self_p->messages);
}

void chat_server_to_client_decode_inner(
    struct pbtools_decoder_t *decoder_p,
    struct chat_server_to_client_t *self_p)
{
    int wire_type;

    while (pbtools_decoder_available(decoder_p)) {
        switch (pbtools_decoder_read_tag(decoder_p, &wire_type)) {

        case 1:
            chat_server_to_client_messages_connect_rsp_decode(
                decoder_p,
                wire_type,
                self_p);
            break;

        case 2:
            chat_server_to_client_messages_message_ind_decode(
                decoder_p,
                wire_type,
                self_p);
            break;

        default:
            pbtools_decoder_skip_field(decoder_p, wire_type);
            break;
        }
    }
}

void chat_server_to_client_encode_repeated_inner(
    struct pbtools_encoder_t *encoder_p,
    int field_number,
    struct chat_server_to_client_repeated_t *repeated_p)
{
    pbtools_encode_repeated_inner(
        encoder_p,
        field_number,
        (struct pbtools_repeated_message_t *)repeated_p,
        sizeof(struct chat_server_to_client_t),
        (pbtools_message_encode_inner_t)chat_server_to_client_encode_inner);
}

void chat_server_to_client_decode_repeated_inner(
    struct pbtools_decoder_t *decoder_p,
    struct pbtools_repeated_info_t *repeated_info_p,
    struct chat_server_to_client_repeated_t *repeated_p)
{
    pbtools_decode_repeated_inner(
        decoder_p,
        repeated_info_p,
        (struct pbtools_repeated_message_t *)repeated_p,
        sizeof(struct chat_server_to_client_t),
        (pbtools_message_init_t)chat_server_to_client_init,
        (pbtools_message_decode_inner_t)chat_server_to_client_decode_inner);
}

struct chat_server_to_client_t *
chat_server_to_client_new(
    void *workspace_p,
    size_t size)
{
    return (pbtools_message_new(
                workspace_p,
                size,
                sizeof(struct chat_server_to_client_t),
                (pbtools_message_init_t)chat_server_to_client_init));
}

int chat_server_to_client_encode(
    struct chat_server_to_client_t *self_p,
    uint8_t *encoded_p,
    size_t size)
{
    return (pbtools_message_encode(
                &self_p->base,
                encoded_p,
                size,
                (pbtools_message_encode_inner_t)chat_server_to_client_encode_inner));
}

int chat_server_to_client_decode(
    struct chat_server_to_client_t *self_p,
    const uint8_t *encoded_p,
    size_t size)
{
    return (pbtools_message_decode(
                &self_p->base,
                encoded_p,
                size,
                (pbtools_message_decode_inner_t)chat_server_to_client_decode_inner));
}
