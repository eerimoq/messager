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
 *
 * This file is part of the Messi project.
 */

#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include "my_protocol_server.h"

static struct my_protocol_server_client_t *alloc_client(struct my_protocol_server_t *self_p)
{
    struct my_protocol_server_client_t *client_p;

    client_p = self_p->clients.free_list_p;

    if (client_p != NULL) {
        /* Remove from free list. */
        self_p->clients.free_list_p = client_p->next_p;

        /* Add to used list. */
        client_p->next_p = self_p->clients.used_list_p;

        if (self_p->clients.used_list_p != NULL) {
            self_p->clients.used_list_p->prev_p = client_p;
        }

        self_p->clients.used_list_p = client_p;
    }

    return (client_p);
}

static void free_client(struct my_protocol_server_t *self_p,
                        struct my_protocol_server_client_t *client_p)
{
    /* Remove from used list. */
    if (client_p == self_p->clients.used_list_p) {
        self_p->clients.used_list_p = client_p->next_p;
    } else {
        client_p->prev_p->next_p = client_p->next_p;
    }

    if (client_p->next_p != NULL) {
        client_p->next_p->prev_p = client_p->prev_p;
    }

    /* Add to fre list. */
    client_p->next_p = self_p->clients.free_list_p;
    self_p->clients.free_list_p = client_p;
}

static int epoll_ctl_add(struct my_protocol_server_t *self_p, int fd)
{
    return (self_p->epoll_ctl(self_p->epoll_fd, EPOLL_CTL_ADD, fd, EPOLLIN));
}

static int epoll_ctl_del(struct my_protocol_server_t *self_p, int fd)
{
    return (self_p->epoll_ctl(self_p->epoll_fd, EPOLL_CTL_DEL, fd, 0));
}

static void close_fd(struct my_protocol_server_t *self_p, int fd)
{
    epoll_ctl_del(self_p, fd);
    close(fd);
}

static void client_reset_input(struct my_protocol_server_client_t *self_p)
{
    self_p->input.state = my_protocol_server_client_input_state_header_t;
    self_p->input.size = 0;
    self_p->input.left = sizeof(struct my_protocol_common_header_t);
}

static int client_start_keep_alive_timer(struct my_protocol_server_client_t *self_p)
{
    struct itimerspec timeout;

    memset(&timeout, 0, sizeof(timeout));
    timeout.it_value.tv_sec = 3;

    return (timerfd_settime(self_p->keep_alive_timer_fd, 0, &timeout, NULL));
}

static int client_init(struct my_protocol_server_client_t *self_p,
                       struct my_protocol_server_t *server_p,
                       int client_fd)
{
    int res;

    self_p->client_fd = client_fd;
    client_reset_input(self_p);
    self_p->keep_alive_timer_fd = timerfd_create(CLOCK_MONOTONIC, 0);

    if (self_p->keep_alive_timer_fd == -1) {
        return (-1);
    }

    res = client_start_keep_alive_timer(self_p);

    if (res == -1) {
        goto out1;
    }

    res = epoll_ctl_add(server_p, self_p->keep_alive_timer_fd);

    if (res == -1) {
        goto out1;
    }

    res = epoll_ctl_add(server_p, client_fd);

    if (res == -1) {
        goto out2;
    }

    return (0);

 out2:
    epoll_ctl_del(server_p, self_p->keep_alive_timer_fd);

 out1:
    close(self_p->keep_alive_timer_fd);

    return (-1);
}

static void client_destroy(struct my_protocol_server_client_t *self_p,
                           struct my_protocol_server_t *server_p)
{
    close_fd(server_p, self_p->client_fd);
    close_fd(server_p, self_p->keep_alive_timer_fd);
    free_client(server_p, self_p);
}

static void process_listener(struct my_protocol_server_t *self_p, uint32_t events)
{
    (void)events;

    int res;
    int client_fd;
    struct my_protocol_server_client_t *client_p;

    client_fd = accept(self_p->listener_fd, NULL, 0);

    if (client_fd == -1) {
        return;
    }

    res = my_protocol_common_make_non_blocking(client_fd);

    if (res == -1) {
        goto out1;
    }

    client_p = alloc_client(self_p);

    if (client_p == NULL) {
        goto out1;
    }

    res = client_init(client_p, self_p, client_fd);

    if (res != 0) {
        goto out2;
    }

    return;

 out2:
    free_client(self_p, client_p);

 out1:
    close(client_fd);
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

    payload_buf_p = &client_p->input.buf_p[sizeof(struct my_protocol_common_header_t)];
    payload_size = client_p->input.size - sizeof(struct my_protocol_common_header_t);

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
            &message_p->messages.value.foo_req);
        break;

    case my_protocol_client_to_server_messages_choice_bar_ind_e:
        self_p->on_bar_ind(
            self_p,
            client_p,
            &message_p->messages.value.bar_ind);
        break;

    case my_protocol_client_to_server_messages_choice_fie_rsp_e:
        self_p->on_fie_rsp(
            self_p,
            client_p,
            &message_p->messages.value.fie_rsp);
        break;

    default:
        break;
    }

    return (0);
}

static int handle_message_ping(struct my_protocol_server_client_t *client_p)
{
    int res;
    ssize_t size;
    struct my_protocol_common_header_t header;

    res = client_start_keep_alive_timer(client_p);

    if (res != 0) {
        return (res);
    }

    header.type = MY_PROTOCOL_COMMON_MESSAGE_TYPE_PONG;
    header.size = 0;
    my_protocol_common_header_hton(&header);

    size = write(client_p->client_fd, &header, sizeof(header));

    if (size != sizeof(header)) {
        return (-1);
    }

    return (0);
}

static int handle_message(struct my_protocol_server_t *self_p,
                          struct my_protocol_server_client_t *client_p,
                          uint32_t type)
{
    int res;

    switch (type) {

    case MY_PROTOCOL_COMMON_MESSAGE_TYPE_USER:
        res = handle_message_user(self_p, client_p);
        break;

    case MY_PROTOCOL_COMMON_MESSAGE_TYPE_PING:
        res = handle_message_ping(client_p);
        break;

    default:
        res = -1;
        break;
    }

    return (res);
}

static void process_client_socket(struct my_protocol_server_t *self_p,
                                  struct my_protocol_server_client_t *client_p)
{
    int res;
    ssize_t size;
    struct my_protocol_common_header_t *header_p;

    header_p = (struct my_protocol_common_header_t *)client_p->input.buf_p;

    while (true) {
        size = read(client_p->client_fd,
                    &client_p->input.buf_p[client_p->input.size],
                    client_p->input.left);

        if (size <= 0) {
            if (!((size == -1) && (errno == EAGAIN))) {
                client_destroy(client_p, self_p);
            }

            break;
        }

        client_p->input.size += size;
        client_p->input.left -= size;

        if (client_p->input.left > 0) {
            continue;
        }

        if (client_p->input.state == my_protocol_server_client_input_state_header_t) {
            my_protocol_common_header_ntoh(header_p);
            client_p->input.left = header_p->size;
            client_p->input.state = my_protocol_server_client_input_state_payload_t;
        }

        if (client_p->input.left == 0) {
            res = handle_message(self_p, client_p, header_p->type);

            if (res == 0) {
                client_reset_input(client_p);
            } else {
                client_destroy(client_p, self_p);
            }
        }
    }
}

static void process_client_keep_alive_timer(struct my_protocol_server_t *self_p,
                                            struct my_protocol_server_client_t *client_p)
{
    client_destroy(client_p, self_p);
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
    struct my_protocol_common_header_t *header_p;

    payload_size = my_protocol_server_to_client_encode(
        self_p->output.message_p,
        &self_p->message.data.buf_p[sizeof(*header_p)],
        self_p->message.data.size - sizeof(*header_p));

    if (payload_size < 0) {
        return (payload_size);
    }

    header_p = (struct my_protocol_common_header_t *)self_p->message.data.buf_p;
    header_p->type = MY_PROTOCOL_COMMON_MESSAGE_TYPE_USER;
    header_p->size = payload_size;
    my_protocol_common_header_hton(header_p);

    return (payload_size + sizeof(*header_p));
}

int my_protocol_server_init(
    struct my_protocol_server_t *self_p,
    const char *address_p,
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
    my_protocol_server_on_foo_req_t on_foo_req,
    my_protocol_server_on_bar_ind_t on_bar_ind,
    my_protocol_server_on_fie_rsp_t on_fie_rsp,
    int epoll_fd,
    my_protocol_epoll_ctl_t epoll_ctl)
{
    (void)clients_max;

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

    if (epoll_ctl == NULL) {
        epoll_ctl = my_protocol_common_epoll_ctl_default;
    }

    self_p->address_p = address_p;
    self_p->on_foo_req = on_foo_req;
    self_p->on_bar_ind = on_bar_ind;
    self_p->on_fie_rsp = on_fie_rsp;
    self_p->epoll_fd = epoll_fd;
    self_p->epoll_ctl = epoll_ctl;

    /* Lists of clients. */
    self_p->clients.free_list_p = &clients_p[0];
    self_p->clients.input_buffer_size = client_input_size;

    for (i = 0; i < clients_max - 1; i++) {
        clients_p[i].next_p = &clients_p[i + 1];
        clients_p[i].input.buf_p = &clients_input_bufs_p[i * client_input_size];
    }

    clients_p[i].next_p = NULL;
    clients_p[i].input.buf_p = &clients_input_bufs_p[i * client_input_size];
    self_p->clients.used_list_p = NULL;

    self_p->message.data.buf_p = message_buf_p;
    self_p->message.data.size = message_size;
    self_p->input.workspace.buf_p = workspace_in_buf_p;
    self_p->input.workspace.size = workspace_in_size;
    self_p->output.workspace.buf_p = workspace_out_buf_p;
    self_p->output.workspace.size = workspace_out_size;

    return (0);
}

int my_protocol_server_start(struct my_protocol_server_t *self_p)
{
    int res;
    int listener_fd;
    struct sockaddr_in addr;
    int enable;

    listener_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (listener_fd == -1) {
        return (1);
    }

    enable = 1;

    res = setsockopt(listener_fd,
                     SOL_SOCKET,
                     SO_REUSEADDR,
                     &enable,
                     sizeof(enable));

    if (res != 0) {
        goto out;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(6000);
    inet_aton("127.0.0.1", (struct in_addr *)&addr.sin_addr.s_addr);

    res = bind(listener_fd, (struct sockaddr *)&addr, sizeof(addr));

    if (res == -1) {
        goto out;
    }

    res = listen(listener_fd, 5);

    if (res == -1) {
        goto out;
    }

    res = my_protocol_common_make_non_blocking(listener_fd);

    if (res == -1) {
        goto out;
    }

    res = epoll_ctl_add(self_p, listener_fd);

    if (res == -1) {
        goto out;
    }

    self_p->listener_fd = listener_fd;

    return (0);

 out:
    close(listener_fd);

    return (-1);
}

void my_protocol_server_stop(struct my_protocol_server_t *self_p)
{
    struct my_protocol_server_client_t *client_p;
    struct my_protocol_server_client_t *next_client_p;

    close_fd(self_p, self_p->listener_fd);
    client_p = self_p->clients.used_list_p;

    while (client_p != NULL) {
        close_fd(self_p, client_p->client_fd);
        close_fd(self_p, client_p->keep_alive_timer_fd);
        next_client_p = client_p->next_p;
        client_p->next_p = self_p->clients.free_list_p;
        self_p->clients.free_list_p = client_p;
        client_p = next_client_p;
    }
}

void my_protocol_server_process(struct my_protocol_server_t *self_p, int fd, uint32_t events)
{
    struct my_protocol_server_client_t *client_p;

    if (fd == self_p->listener_fd) {
        process_listener(self_p, events);
    } else {
        client_p = self_p->clients.used_list_p;

        while (client_p != NULL) {
            if (fd == client_p->client_fd) {
                process_client_socket(self_p, client_p);
                break;
            } else if (fd == client_p->keep_alive_timer_fd) {
                process_client_keep_alive_timer(self_p, client_p);
                break;
            }

            client_p = client_p->next_p;
        }
    }
}

void my_protocol_server_send(struct my_protocol_server_t *self_p)
{
    int res;

    res = encode_user_message(self_p);

    if (res < 0) {
        return;
    }
}

void my_protocol_server_reply(struct my_protocol_server_t *self_p)
{
    int res;

    res = encode_user_message(self_p);

    if (res < 0) {
        return;
    }

    write(self_p->current_client_p->client_fd, self_p->message.data.buf_p, res);
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
    client_p = self_p->clients.used_list_p;

    while (client_p != NULL) {
        write(client_p->client_fd, self_p->message.data.buf_p, res);
        client_p = client_p->next_p;
    }
}

struct my_protocol_foo_rsp_t *
my_protocol_server_init_foo_rsp(struct my_protocol_server_t *self_p)
{
    self_p->output.message_p = my_protocol_server_to_client_new(
        &self_p->output.workspace.buf_p[0],
        self_p->output.workspace.size);
    my_protocol_server_to_client_messages_foo_rsp_init(self_p->output.message_p);

    return (&self_p->output.message_p->messages.value.foo_rsp);
}

struct my_protocol_fie_req_t *
my_protocol_server_init_fie_req(struct my_protocol_server_t *self_p)
{
    self_p->output.message_p = my_protocol_server_to_client_new(
        &self_p->output.workspace.buf_p[0],
        self_p->output.workspace.size);
    my_protocol_server_to_client_messages_fie_req_init(self_p->output.message_p);

    return (&self_p->output.message_p->messages.value.fie_req);
}

