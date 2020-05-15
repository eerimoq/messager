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

#ifndef MESSI_H
#define MESSI_H

#include <stdint.h>
#include <arpa/inet.h>

/* Message types. */
#define MESSI_MESSAGE_TYPE_USER 1
#define MESSI_MESSAGE_TYPE_PING 2
#define MESSI_MESSAGE_TYPE_PONG 3

typedef int (*messi_epoll_ctl_t)(int epoll_fd, int op, int fd, uint32_t events);

struct messi_buffer_t {
    uint8_t *buf_p;
    size_t size;
};

struct messi_header_t {
    uint32_t type;
    uint32_t size;
} __attribute__ ((packed));

static inline void messi_header_ntoh(struct messi_header_t *header_p)
{
    header_p->type = ntohl(header_p->type);
    header_p->size = ntohl(header_p->size);
}

static inline void messi_header_hton(struct messi_header_t *header_p)
{
    header_p->type = htonl(header_p->type);
    header_p->size = htonl(header_p->size);
}

int messi_epoll_ctl_default(int epoll_fd, int op, int fd, uint32_t events);

int messi_make_non_blocking(int fd);

/**
 * Parse tcp://<host>:<port>. Returns zero(0) if successful.
 */
int messi_parse_tcp_uri(const char *uri_p,
                        char *host_p,
                        size_t host_size,
                        int *port_p);

#endif
