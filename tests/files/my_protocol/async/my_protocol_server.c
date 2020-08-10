/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020, Erik Moqvist
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

/* This file was generated by Messi. */

#include "messi.h"
#include "my_protocol_server.h"

static void client_reset_input(struct my_protocol_server_client_t *self_p)
{
    self_p->input.state = my_protocol_server_client_input_state_header_t;
    self_p->input.size = 0;
    self_p->input.left = sizeof(struct messi_header_t);
}

static void on_client_connected_default(struct my_protocol_server_t *self_p,
                                        struct my_protocol_server_client_t *client_p)
{
    (void)self_p;
    (void)client_p;
}

static void on_client_disconnected_default(struct my_protocol_server_t *self_p,
                                           struct my_protocol_server_client_t *client_p)
{
    (void)self_p;
    (void)client_p;
}

static void on_stcp_client_connected(struct async_stcp_server_client_t *stcp_p)
{
    struct my_protocol_server_client_t *self_p;

    self_p = async_container_of(stcp_p, typeof(*self_p), stcp);

    client_reset_input(self_p);
    self_p->server_p->on_client_connected(self_p->server_p, self_p);
}

static void on_stcp_client_disconnected(struct async_stcp_server_client_t *stcp_p)
{
    struct my_protocol_server_client_t *self_p;

    self_p = async_container_of(stcp_p, typeof(*self_p), stcp);

    self_p->server_p->on_client_disconnected(self_p->server_p, self_p);
}

static int handle_message_user(struct my_protocol_server_t *self_p,
                               struct my_protocol_server_client_t *client_p)
{
    int res;
    struct my_protocol_client_to_server_t *message_p;
    uint8_t *payload_buf_p;
    size_t payload_size;

    self_p->input.message_p = my_protocol_client_to_server_new(
        &self_p->input.workspace.buf_p[0],
        self_p->input.workspace.size);
    message_p = self_p->input.message_p;

    if (message_p == NULL) {
        return (-1);
    }

    payload_buf_p = &client_p->input.data.buf_p[sizeof(struct messi_header_t)];
    payload_size = client_p->input.size - sizeof(struct messi_header_t);

    res = my_protocol_client_to_server_decode(message_p, payload_buf_p, payload_size);

    if (res != (int)payload_size) {
        return (-1);
    }

    self_p->current_client_p = client_p;

    switch (message_p->messages.choice) {

    case my_protocol_client_to_server_messages_choice_foo_req_e:
        self_p->on_foo_req(
            self_p,
            client_p,
            message_p->messages.value.foo_req_p);
        break;

    case my_protocol_client_to_server_messages_choice_bar_ind_e:
        self_p->on_bar_ind(
            self_p,
            client_p,
            message_p->messages.value.bar_ind_p);
        break;

    case my_protocol_client_to_server_messages_choice_fie_rsp_e:
        self_p->on_fie_rsp(
            self_p,
            client_p,
            message_p->messages.value.fie_rsp_p);
        break;

    default:
        break;
    }

    self_p->current_client_p = NULL;

    return (0);
}

static int handle_message_ping(struct my_protocol_server_client_t *client_p)
{
    struct messi_header_t header;

    async_timer_start(&client_p->keep_alive_timer);
    header.type = MESSI_MESSAGE_TYPE_PONG;
    messi_header_set_size(&header, 0);
    async_stcp_server_client_write(&client_p->stcp, &header, sizeof(header));

    return (0);
}

static int handle_message(struct my_protocol_server_t *self_p,
                          struct my_protocol_server_client_t *client_p,
                          uint32_t type)
{
    int res;

    switch (type) {

    case MESSI_MESSAGE_TYPE_CLIENT_TO_SERVER_USER:
        res = handle_message_user(self_p, client_p);
        break;

    case MESSI_MESSAGE_TYPE_PING:
        res = handle_message_ping(client_p);
        break;

    default:
        res = -1;
        break;
    }

    return (res);
}

static void on_stcp_client_input(struct async_stcp_server_client_t *stcp_p)
{
    int res;
    ssize_t size;
    struct messi_header_t *header_p;
    struct my_protocol_server_client_t *self_p;

    self_p = async_container_of(stcp_p, typeof(*self_p), stcp);
    header_p = (struct messi_header_t *)self_p->input.data.buf_p;

    while (true) {
        size = async_stcp_server_client_read(
            &self_p->stcp,
            &self_p->input.data.buf_p[self_p->input.size],
            self_p->input.left);

        if (size == 0) {
            break;
        }

        self_p->input.size += size;
        self_p->input.left -= size;

        if (self_p->input.left > 0) {
            continue;
        }

        if (self_p->input.state == my_protocol_server_client_input_state_header_t) {
            self_p->input.left = messi_header_get_size(header_p);

            if ((self_p->input.left + sizeof(*header_p))
                > self_p->input.data.size) {
                /* client_pending_disconnect(client_p, self_p); */
                break;
            }

            self_p->input.state = my_protocol_server_client_input_state_payload_t;
        }

        if (self_p->input.left == 0) {
            res = handle_message(self_p->server_p, self_p, header_p->type);

            if (res == 0) {
                client_reset_input(self_p);
            } else {
                //client_pending_disconnect(client_p, self_p);
                break;
            }
        }
    }
}

void my_protocol_server_new_output_message(struct my_protocol_server_t *self_p)
{
    self_p->output.message_p = my_protocol_server_to_client_new(
        &self_p->output.workspace.buf_p[0],
        self_p->output.workspace.size);
}

static void on_foo_req_default(
    struct my_protocol_server_t *self_p,
    struct my_protocol_server_client_t *client_p,
    struct my_protocol_foo_req_t *message_p)
{
    (void)self_p;
    (void)client_p;
    (void)message_p;
}

static void on_bar_ind_default(
    struct my_protocol_server_t *self_p,
    struct my_protocol_server_client_t *client_p,
    struct my_protocol_bar_ind_t *message_p)
{
    (void)self_p;
    (void)client_p;
    (void)message_p;
}

static void on_fie_rsp_default(
    struct my_protocol_server_t *self_p,
    struct my_protocol_server_client_t *client_p,
    struct my_protocol_fie_rsp_t *message_p)
{
    (void)self_p;
    (void)client_p;
    (void)message_p;
}

static int encode_user_message(struct my_protocol_server_t *self_p)
{
    int payload_size;
    struct messi_header_t *header_p;

    payload_size = my_protocol_server_to_client_encode(
        self_p->output.message_p,
        &self_p->output.encoded.buf_p[sizeof(*header_p)],
        self_p->output.encoded.size - sizeof(*header_p));

    if (payload_size < 0) {
        return (payload_size);
    }

    header_p = (struct messi_header_t *)self_p->output.encoded.buf_p;
    header_p->type = MESSI_MESSAGE_TYPE_SERVER_TO_CLIENT_USER;
    messi_header_set_size(header_p, payload_size);

    return (payload_size + sizeof(*header_p));
}

int my_protocol_server_init(
    struct my_protocol_server_t *self_p,
    const char *server_uri_p,
    struct my_protocol_server_client_t *clients_p,
    int clients_max,
    uint8_t *clients_input_bufs_p,
    size_t client_input_size,
    uint8_t *message_buf_p,
    size_t message_size,
    uint8_t *workspace_in_buf_p,
    size_t workspace_in_size,
    uint8_t *workspace_out_buf_p,
    size_t workspace_out_size,
    my_protocol_server_on_client_connected_t on_client_connected,
    my_protocol_server_on_client_disconnected_t on_client_disconnected,
    my_protocol_server_on_foo_req_t on_foo_req,
    my_protocol_server_on_bar_ind_t on_bar_ind,
    my_protocol_server_on_fie_rsp_t on_fie_rsp,
    struct async_t *async_p)
{
    int res;
    int i;

    if (on_foo_req == NULL) {
        on_foo_req = on_foo_req_default;
    }

    if (on_bar_ind == NULL) {
        on_bar_ind = on_bar_ind_default;
    }

    if (on_fie_rsp == NULL) {
        on_fie_rsp = on_fie_rsp_default;
    }

    if (on_client_connected == NULL) {
        on_client_connected = on_client_connected_default;
    }

    if (on_client_disconnected == NULL) {
        on_client_disconnected = on_client_disconnected_default;
    }

    res = messi_parse_tcp_uri(server_uri_p,
                              &self_p->server.address[0],
                              sizeof(self_p->server.address),
                              &self_p->server.port);

    if (res != 0) {
        return (res);
    }

    self_p->on_foo_req = on_foo_req;
    self_p->on_bar_ind = on_bar_ind;
    self_p->on_fie_rsp = on_fie_rsp;
    self_p->input.workspace.buf_p = workspace_in_buf_p;
    self_p->input.workspace.size = workspace_in_size;
    self_p->output.encoded.buf_p = message_buf_p;
    self_p->output.encoded.size = message_size;
    self_p->output.workspace.buf_p = workspace_out_buf_p;
    self_p->output.workspace.size = workspace_out_size;
    self_p->async_p = async_p;
    async_stcp_server_init(&self_p->stcp,
                           &self_p->server.address[0],
                           self_p->server.port,
                           NULL,
                           on_stcp_client_connected,
                           on_stcp_client_disconnected,
                           on_stcp_client_input,
                           async_p);

    for (i = 0; i < clients_max; i++) {
        clients_p[i].input.data.buf_p = &clients_input_bufs_p[i * client_input_size];
        clients_p[i].input.data.size = client_input_size;
        async_stcp_server_add_client(&self_p->stcp, &clients_p[i].stcp);
    }

    return (0);
}

void my_protocol_server_start(struct my_protocol_server_t *self_p)
{
    async_stcp_server_start(&self_p->stcp);
}

void my_protocol_server_stop(struct my_protocol_server_t *self_p)
{
    async_stcp_server_stop(&self_p->stcp);
}

void my_protocol_server_send(struct my_protocol_server_t *self_p,
                      struct my_protocol_server_client_t *client_p)
{
    int res;

    res = encode_user_message(self_p);

    if (res < 0) {
        return;
    }

    async_stcp_server_client_write(&client_p->stcp,
                                   self_p->output.encoded.buf_p,
                                   res);
}

void my_protocol_server_reply(struct my_protocol_server_t *self_p)
{
    if (self_p->current_client_p != NULL) {
        my_protocol_server_send(self_p, self_p->current_client_p);
    }
}

void my_protocol_server_broadcast(struct my_protocol_server_t *self_p)
{
    int res;
    struct my_protocol_server_client_t *client_p;

    /* Create the message. */
    res = encode_user_message(self_p);

    if (res < 0) {
        return;
    }

    /* Send it to all clients. */
    client_p = self_p->clients.connected_list_p;

    while (client_p != NULL) {
        async_stcp_server_client_write(&client_p->stcp,
                                       self_p->output.encoded.buf_p,
                                       res);
        client_p = client_p->next_p;
    }
}

void my_protocol_server_disconnect(
    struct my_protocol_server_t *self_p,
    struct my_protocol_server_client_t *client_p)
{
    if (client_p == NULL) {
        client_p = self_p->current_client_p;
    }

    if (client_p == NULL) {
        return;
    }

    // client_pending_disconnect(client_p, self_p);
}

struct my_protocol_foo_rsp_t *my_protocol_server_init_foo_rsp(
    struct my_protocol_server_t *self_p)
{
    my_protocol_server_new_output_message(self_p);
    my_protocol_server_to_client_messages_foo_rsp_alloc(self_p->output.message_p);

    return (self_p->output.message_p->messages.value.foo_rsp_p);
}

struct my_protocol_fie_req_t *my_protocol_server_init_fie_req(
    struct my_protocol_server_t *self_p)
{
    my_protocol_server_new_output_message(self_p);
    my_protocol_server_to_client_messages_fie_req_alloc(self_p->output.message_p);

    return (self_p->output.message_p->messages.value.fie_req_p);
}

