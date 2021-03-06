#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <netinet/tcp.h>
#include "nala.h"
#include "chat_server.h"

#define EPOLL_FD                            9
#define LISTENER_FD                         10
#define ERIK_FD                             11
#define KALLE_FD                            12
#define FIA_FD                              13
#define ERIK_TIMER_FD                       14
#define KALLE_TIMER_FD                      15
#define FIA_TIMER_FD                        16
#define KEEP_ALIVE_TIMER_FD                 17
#define LISA_FD                             18
#define LISA_TIMER_FD                       21

#define HEADER_SIZE sizeof(struct messi_header_t)

static inline int client_to_timer_fd(int client_fd)
{
    return (client_fd + 3);
}

static uint8_t connect_req_erik[] = {
    /* Header. */
    0x01, 0x00, 0x00, 0x08,
    /* Payload. */
    0x0a, 0x06, 0x0a, 0x04, 0x45, 0x72, 0x69, 0x6b
};

static uint8_t connect_req_kalle[] = {
    /* Header. */
    0x01, 0x00, 0x00, 0x09,
    /* Payload. */
    0x0a, 0x07, 0x0a, 0x05, 0x4b, 0x61, 0x6c, 0x6c, 0x65
};

static uint8_t connect_req_fia[] = {
    /* Header. */
    0x01, 0x00, 0x00, 0x07,
    /* Payload. */
    0x0a, 0x05, 0x0a, 0x03, 0x46, 0x69, 0x61
};

static uint8_t connect_rsp[] = {
    /* Header. */
    0x02, 0x00, 0x00, 0x02,
    /* Payload. */
    0x0a, 0x00
};

/**
 * user: Erik
 * text: Hello.
 */
static uint8_t message_ind_in[] = {
    /* Header. */
    0x01, 0x00, 0x00, 0x10,
    /* Payload. */
    0x12, 0x0e, 0x0a, 0x04, 0x45, 0x72, 0x69, 0x6b,
    0x12, 0x06, 0x48, 0x65, 0x6c, 0x6c, 0x6f, 0x2e
};

/**
 * user: Erik
 * text: Hello.
 */
static uint8_t message_ind_out[] = {
    /* Header. */
    0x02, 0x00, 0x00, 0x10,
    /* Payload. */
    0x12, 0x0e, 0x0a, 0x04, 0x45, 0x72, 0x69, 0x6b,
    0x12, 0x06, 0x48, 0x65, 0x6c, 0x6c, 0x6f, 0x2e
};

static struct chat_server_t server;
static struct chat_server_client_t clients[3];
static uint8_t clients_input_buffers[3][128];
static uint8_t message[128];
static uint8_t workspace_in[128];
static uint8_t workspace_out[128];

static void mock_prepare_make_non_blocking(int fd)
{
    fcntl_mock_once(fd, F_GETFL, 0, "");
    fcntl_mock_once(fd, F_SETFL, O_NONBLOCK, "");
}

static void mock_prepare_tcp_nodelay(int fd)
{
    int yes;

    yes = 1;
    setsockopt_mock_once(fd, IPPROTO_TCP, TCP_NODELAY, sizeof(yes), 0);
    setsockopt_mock_set_optval_in(&yes, sizeof(yes));
}

static void mock_prepare_read(int client_fd, uint8_t *buf_p, size_t size)
{
    read_mock_once(client_fd, HEADER_SIZE, HEADER_SIZE);
    read_mock_set_buf_out(&buf_p[0], HEADER_SIZE);

    if (size > HEADER_SIZE) {
        read_mock_once(client_fd, size - HEADER_SIZE, size - HEADER_SIZE);
        read_mock_set_buf_out(&buf_p[HEADER_SIZE], size - HEADER_SIZE);
    }
}

static void mock_prepare_read_try_again(int client_fd)
{
    read_mock_once(client_fd, HEADER_SIZE, -1);
    read_mock_set_errno(EAGAIN);
}

static void mock_prepare_write(int client_fd,
                               uint8_t *buf_p,
                               size_t size,
                               ssize_t res,
                               int error)
{
    write_mock_once(client_fd, size, res);
    write_mock_set_buf_in(buf_p, size);
    write_mock_set_errno(error);
}

static void on_connect_req(struct chat_server_t *self_p,
                           struct chat_server_client_t *client_p,
                           struct chat_connect_req_t *message_p)
{
    (void)client_p;

    printf("%s connected.\n", message_p->user_p);

    chat_server_init_connect_rsp(self_p);
    chat_server_reply(self_p);
}

void server_on_message_ind(struct chat_server_t *self_p,
                           struct chat_server_client_t *client_p,
                           struct chat_message_ind_t *message_in_p)
{
    (void)client_p;

    struct chat_message_ind_t *message_p;

    message_p = chat_server_init_message_ind(self_p);
    message_p->user_p = message_in_p->user_p;
    message_p->text_p = message_in_p->text_p;
    chat_server_broadcast(self_p);
}

static struct chat_server_client_t *connect_client(uint8_t *connect_req_buf_p,
                                                   size_t connect_req_size,
                                                   int client_fd)
{
    struct itimerspec timeout;
    int handle;

    /* TCP connect. */
    accept_mock_once(LISTENER_FD, client_fd);
    mock_prepare_make_non_blocking(client_fd);
    mock_prepare_tcp_nodelay(client_fd);
    timerfd_create_mock_once(CLOCK_MONOTONIC, 0, client_to_timer_fd(client_fd));
    memset(&timeout, 0, sizeof(timeout));
    timeout.it_value.tv_sec = 3;
    timerfd_settime_mock_once(client_to_timer_fd(client_fd), 0, 0);
    timerfd_settime_mock_set_new_value_in(&timeout, sizeof(timeout));
    epoll_ctl_mock_once(EPOLL_FD, EPOLL_CTL_ADD, client_to_timer_fd(client_fd), 0);
    epoll_ctl_mock_once(EPOLL_FD, EPOLL_CTL_ADD, client_fd, 0);
    handle = server_on_client_connected_mock_once();

    chat_server_process(&server, LISTENER_FD, EPOLLIN);

    /* Chat connect messages. */
    mock_prepare_read(client_fd, connect_req_buf_p, connect_req_size);
    mock_prepare_read_try_again(client_fd);
    write_mock_once(client_fd, sizeof(connect_rsp), sizeof(connect_rsp));
    write_mock_set_buf_in(&connect_rsp[0], sizeof(connect_rsp));

    chat_server_process(&server, client_fd, EPOLLIN);

    return (server_on_client_connected_mock_get_params_in(handle)->client_p);
}

static void mock_prepare_client_pending_disconnect(int client_fd)
{
    epoll_ctl_mock_once(EPOLL_FD, EPOLL_CTL_DEL, client_fd, 0);
    close_mock_once(client_fd, 0);
}

static void mock_prepare_destroy_pending_disconnect_client(int client_fd)
{
    close_mock_once(client_to_timer_fd(client_fd), 0);
    epoll_ctl_mock_once(EPOLL_FD,
                        EPOLL_CTL_DEL,
                        client_to_timer_fd(client_fd),
                        0);
    server_on_client_disconnected_mock_once();
}

static void disconnect_client(int client_fd)
{
    /* TCP disconnect. */
    read_mock_once(client_fd, HEADER_SIZE, -1);
    read_mock_set_errno(EPIPE);
    mock_prepare_client_pending_disconnect(client_fd);
    mock_prepare_destroy_pending_disconnect_client(client_fd);

    chat_server_process(&server, client_fd, EPOLLIN);
}

static struct chat_server_client_t *connect_erik(void)
{
    return (connect_client(connect_req_erik,
                           sizeof(connect_req_erik),
                           ERIK_FD));
}

static struct chat_server_client_t *connect_kalle(void)
{
    return (connect_client(connect_req_kalle,
                           sizeof(connect_req_kalle),
                           KALLE_FD));
}

static struct chat_server_client_t *connect_fia(void)
{
    return (connect_client(connect_req_fia,
                           sizeof(connect_req_fia),
                           FIA_FD));
}

static void disconnect_erik(void)
{
    disconnect_client(ERIK_FD);
}

static void disconnect_kalle(void)
{
    disconnect_client(KALLE_FD);
}

static void disconnect_fia(void)
{
    disconnect_client(FIA_FD);
}

void server_on_client_connected(struct chat_server_t *self_p,
                                struct chat_server_client_t *client_p)
{
    (void)self_p;
    (void)client_p;

    FAIL("Must be mocked.");
}

void server_on_client_disconnected(struct chat_server_t *self_p,
                                   struct chat_server_client_t *client_p)
{
    (void)self_p;
    (void)client_p;

    FAIL("Must be mocked.");
}

static void start_server_with_three_clients()
{
    int enable;

    ASSERT_EQ(chat_server_init(&server,
                               "tcp://127.0.0.1:6000",
                               &clients[0],
                               3,
                               &clients_input_buffers[0][0],
                               sizeof(clients_input_buffers[0]),
                               &message[0],
                               sizeof(message),
                               &workspace_in[0],
                               sizeof(workspace_in),
                               &workspace_out[0],
                               sizeof(workspace_out),
                               server_on_client_connected,
                               server_on_client_disconnected,
                               on_connect_req,
                               server_on_message_ind,
                               EPOLL_FD,
                               NULL), 0);

    /* Start creates a socket and starts listening for clients. */
    socket_mock_once(AF_INET, SOCK_STREAM, 0, LISTENER_FD);
    enable = 1;
    setsockopt_mock_once(LISTENER_FD, SOL_SOCKET, SO_REUSEADDR, sizeof(enable), 0);
    setsockopt_mock_set_optval_in(&enable, sizeof(enable));
    bind_mock_once(LISTENER_FD, sizeof(struct sockaddr_in), 0);
    listen_mock_once(LISTENER_FD, 5, 0);
    mock_prepare_make_non_blocking(LISTENER_FD);
    epoll_ctl_mock_once(EPOLL_FD, EPOLL_CTL_ADD, LISTENER_FD, 0);

    ASSERT_EQ(chat_server_start(&server), 0);
}

TEST(connect_and_disconnect_clients)
{
    start_server_with_three_clients();

    /* Connect and disconnect clients. */
    connect_erik();
    connect_kalle();
    connect_fia();
    disconnect_kalle();
    connect_kalle();
    disconnect_erik();
    disconnect_fia();

    /* Stop with Kalle connected. */
    epoll_ctl_mock_once(EPOLL_FD, EPOLL_CTL_DEL, LISTENER_FD, 0);
    close_mock_once(LISTENER_FD, 0);
    epoll_ctl_mock_once(EPOLL_FD, EPOLL_CTL_DEL, KALLE_FD, 0);
    close_mock_once(KALLE_FD, 0);
    epoll_ctl_mock_once(EPOLL_FD,
                        EPOLL_CTL_DEL,
                        client_to_timer_fd(KALLE_FD),
                        0);
    close_mock_once(client_to_timer_fd(KALLE_FD), 0);

    chat_server_stop(&server);
}

TEST(broadcast_message)
{
    start_server_with_three_clients();

    /* Connect clients. */
    connect_erik();
    connect_kalle();
    connect_fia();

    /* Broadcast a message as Fia. */
    mock_prepare_read(FIA_FD, &message_ind_in[0], sizeof(message_ind_in));
    mock_prepare_read_try_again(FIA_FD);
    write_mock_once(FIA_FD, sizeof(message_ind_out), sizeof(message_ind_out));
    write_mock_set_buf_in(&message_ind_out[0], sizeof(message_ind_out));
    write_mock_once(KALLE_FD, sizeof(message_ind_out), sizeof(message_ind_out));
    write_mock_set_buf_in(&message_ind_out[0], sizeof(message_ind_out));
    write_mock_once(ERIK_FD, sizeof(message_ind_out), sizeof(message_ind_out));
    write_mock_set_buf_in(&message_ind_out[0], sizeof(message_ind_out));

    chat_server_process(&server, FIA_FD, EPOLLIN);
}

TEST(broadcast_message_one_client_fails)
{
    struct chat_message_ind_t *message_p;

    start_server_with_three_clients();

    /* Connect clients. */
    connect_erik();
    connect_kalle();
    connect_fia();

    /* Broadcast a message as no one. Kalle will fail. */
    write_mock_once(FIA_FD, sizeof(message_ind_out), sizeof(message_ind_out));
    write_mock_set_buf_in(&message_ind_out[0], sizeof(message_ind_out));
    write_mock_once(KALLE_FD, sizeof(message_ind_out), -1);
    write_mock_set_errno(EIO);
    write_mock_set_buf_in(&message_ind_out[0], sizeof(message_ind_out));
    mock_prepare_client_pending_disconnect(KALLE_FD);
    write_mock_once(ERIK_FD, sizeof(message_ind_out), sizeof(message_ind_out));
    write_mock_set_buf_in(&message_ind_out[0], sizeof(message_ind_out));

    message_p = chat_server_init_message_ind(&server);
    message_p->user_p = "Erik";
    message_p->text_p = "Hello.";
    chat_server_broadcast(&server);

    /* Make the keep alive timer expire to trigger disconnection. */
    mock_prepare_destroy_pending_disconnect_client(KALLE_FD);

    chat_server_process(&server, client_to_timer_fd(KALLE_FD), EPOLLIN);
}

TEST(keep_alive)
{
    uint8_t ping[] = {
        /* Header. */
        0x03, 0x00, 0x00, 0x00
    };
    uint8_t pong[] = {
        /* Header. */
        0x04, 0x00, 0x00, 0x00
    };

    start_server_with_three_clients();

    /* Only one client needed. */
    connect_erik();

    /* Send a ping message, the keep alive timer should be restarted
       and a pong mesage should be sent. */
    mock_prepare_read(ERIK_FD, &ping[0], sizeof(ping));
    mock_prepare_read_try_again(ERIK_FD);

    timerfd_settime_mock_once(ERIK_TIMER_FD, 0, 0);

    write_mock_once(ERIK_FD, sizeof(pong), sizeof(pong));
    write_mock_set_buf_in(&pong[0], sizeof(pong));

    chat_server_process(&server, ERIK_FD, EPOLLIN);

    /* Make the keep alive timer expire. The client should be
       disconnected. */
    mock_prepare_client_pending_disconnect(ERIK_FD);
    mock_prepare_destroy_pending_disconnect_client(ERIK_FD);

    chat_server_process(&server, ERIK_TIMER_FD, EPOLLIN);
}

TEST(error_starting_client_keep_alive_timer_in_init)
{
    struct itimerspec timeout;

    start_server_with_three_clients();

    /* Make starting the timer fail. Verify that file descriptors are
       closed. */
    accept_mock_once(LISTENER_FD, ERIK_FD);
    mock_prepare_make_non_blocking(ERIK_FD);
    mock_prepare_tcp_nodelay(ERIK_FD);
    timerfd_create_mock_once(CLOCK_MONOTONIC, 0, ERIK_TIMER_FD);
    memset(&timeout, 0, sizeof(timeout));
    timeout.it_value.tv_sec = 3;
    timerfd_settime_mock_once(ERIK_TIMER_FD, 0, -1);
    close_mock_once(ERIK_TIMER_FD, 0);
    close_mock_once(ERIK_FD, 0);

    chat_server_process(&server, LISTENER_FD, EPOLLIN);
}

TEST(error_adding_client_fd_to_epoll)
{
    struct itimerspec timeout;

    start_server_with_three_clients();

    /* Make adding the client file descriptor to epoll instance
       fail. Verify that cleanup is ok. */
    accept_mock_once(LISTENER_FD, ERIK_FD);
    mock_prepare_make_non_blocking(ERIK_FD);
    mock_prepare_tcp_nodelay(ERIK_FD);
    timerfd_create_mock_once(CLOCK_MONOTONIC, 0, ERIK_TIMER_FD);
    memset(&timeout, 0, sizeof(timeout));
    timeout.it_value.tv_sec = 3;
    timerfd_settime_mock_once(ERIK_TIMER_FD, 0, 0);
    epoll_ctl_mock_once(EPOLL_FD, EPOLL_CTL_ADD, ERIK_TIMER_FD, 0);
    epoll_ctl_mock_once(EPOLL_FD, EPOLL_CTL_ADD, ERIK_FD, -1);

    epoll_ctl_mock_once(EPOLL_FD, EPOLL_CTL_DEL, ERIK_TIMER_FD, 0);
    close_mock_once(ERIK_TIMER_FD, 0);
    close_mock_once(ERIK_FD, 0);

    chat_server_process(&server, LISTENER_FD, EPOLLIN);
}

TEST(too_many_clients)
{
    start_server_with_three_clients();

    /* Maximum number of clients. */
    connect_erik();
    connect_kalle();
    connect_fia();

    /* Cannot connect another client. */
    accept_mock_once(LISTENER_FD, LISA_FD);
    mock_prepare_make_non_blocking(LISA_FD);
    mock_prepare_tcp_nodelay(LISA_FD);
    close_mock_once(LISA_FD, 0);

    chat_server_process(&server, LISTENER_FD, EPOLLIN);

    /* Reconnect a client. */
    disconnect_fia();
    connect_fia();
}

TEST(listener_bind_error)
{
    int enable;

    ASSERT_EQ(chat_server_init(&server,
                               "tcp://127.0.0.1:6000",
                               &clients[0],
                               3,
                               &clients_input_buffers[0][0],
                               sizeof(clients_input_buffers[0]),
                               &message[0],
                               sizeof(message),
                               &workspace_in[0],
                               sizeof(workspace_in),
                               &workspace_out[0],
                               sizeof(workspace_out),
                               NULL,
                               NULL,
                               on_connect_req,
                               server_on_message_ind,
                               EPOLL_FD,
                               NULL), 0);

    /* Start creates a socket and starts listening for clients. */
    socket_mock_once(AF_INET, SOCK_STREAM, 0, LISTENER_FD);
    enable = 1;
    setsockopt_mock_once(LISTENER_FD, SOL_SOCKET, SO_REUSEADDR, sizeof(enable), 0);
    setsockopt_mock_set_optval_in(&enable, sizeof(enable));
    bind_mock_once(LISTENER_FD, sizeof(struct sockaddr_in), -1);
    close_mock_once(LISTENER_FD, 0);

    ASSERT_EQ(chat_server_start(&server), -1);
}

static void read_first_chunk_of_message(int client_fd)
{
    read_mock_once(client_fd, 4, 1);
    read_mock_set_buf_out(&message_ind_in[0], 1);
    read_mock_once(client_fd, 3, 2);
    read_mock_set_buf_out(&message_ind_in[1], 2);
    read_mock_once(client_fd, 1, 1);
    read_mock_set_buf_out(&message_ind_in[3], 1);
    read_mock_once(client_fd, 16, 5);
    read_mock_set_buf_out(&message_ind_in[4], 5);
    read_mock_once(client_fd, 11, -1);
    read_mock_set_errno(EAGAIN);

    chat_server_process(&server, client_fd, EPOLLIN);
}

static void read_last_chunk_of_message(int client_fd)
{
    read_mock_once(client_fd, 11, 11);
    read_mock_set_buf_out(&message_ind_in[9], 11);
    mock_prepare_read_try_again(client_fd);

    /* The received message is broadcasted. */
    write_mock_once(FIA_FD, sizeof(message_ind_out), sizeof(message_ind_out));
    write_mock_set_buf_in(&message_ind_out[0], sizeof(message_ind_out));
    write_mock_once(ERIK_FD, sizeof(message_ind_out), sizeof(message_ind_out));
    write_mock_set_buf_in(&message_ind_out[0], sizeof(message_ind_out));

    chat_server_process(&server, client_fd, EPOLLIN);
}

static void send_message_ind(int client_fd, struct chat_server_client_t *client_p)
{
    struct chat_message_ind_t *message_p;

    write_mock_once(client_fd, sizeof(message_ind_out), sizeof(message_ind_out));
    write_mock_set_buf_in(&message_ind_out[0], sizeof(message_ind_out));

    message_p = chat_server_init_message_ind(&server);
    message_p->user_p = "Erik";
    message_p->text_p = "Hello.";
    chat_server_send(&server, client_p);
}

TEST(partial_message_read_from_multiple_clients_and_send)
{
    struct chat_server_client_t *erik_p;
    struct chat_server_client_t *fia_p;

    start_server_with_three_clients();
    erik_p = connect_erik();
    fia_p = connect_fia();

    /* Read the first part of the message for both clients. */
    read_first_chunk_of_message(ERIK_FD);
    read_first_chunk_of_message(FIA_FD);

    /* Send a message to each client to ensure separate input and
       output buffers. */
    send_message_ind(ERIK_FD, erik_p);
    send_message_ind(FIA_FD, fia_p);

    /* Read the end of the message. */
    read_last_chunk_of_message(ERIK_FD);
    read_last_chunk_of_message(FIA_FD);
}

TEST(send_message)
{
    struct chat_message_ind_t *message_p;
    struct chat_server_client_t *fia_p;

    start_server_with_three_clients();
    connect_erik();
    fia_p = connect_fia();
    connect_kalle();

    /* Send a message to Fia. */
    write_mock_once(FIA_FD, sizeof(message_ind_out), sizeof(message_ind_out));
    write_mock_set_buf_in(&message_ind_out[0], sizeof(message_ind_out));

    message_p = chat_server_init_message_ind(&server);
    message_p->user_p = "Erik";
    message_p->text_p = "Hello.";
    chat_server_send(&server, fia_p);
}

TEST(send_another_message_after_first_failed)
{
    struct chat_message_ind_t *message_p;
    struct chat_server_client_t *kalle_p;

    start_server_with_three_clients();
    connect_fia();
    kalle_p = connect_kalle();

    /* Send a message to Kalle. */
    write_mock_once(KALLE_FD, sizeof(message_ind_out), -1);
    write_mock_set_errno(EIO);
    mock_prepare_client_pending_disconnect(KALLE_FD);

    message_p = chat_server_init_message_ind(&server);
    message_p->user_p = "Erik";
    message_p->text_p = "Hello.";
    chat_server_send(&server, kalle_p);

    /* Send another message to Kalle. We do not know that the previous
       message could not be sent, and we still have a pointer to
       kalle_p, so the user might use it. */
    write_mock_once(-1, sizeof(message_ind_out), -1);
    write_mock_set_errno(EBADF);

    chat_server_send(&server, kalle_p);

    /* Make the keep alive timer expire. The client should be
       disconnected. */
    mock_prepare_destroy_pending_disconnect_client(KALLE_FD);

    chat_server_process(&server, KALLE_TIMER_FD, EPOLLIN);
}

TEST(reply_with_no_currect_client)
{
    struct chat_message_ind_t *message_p;

    start_server_with_three_clients();
    connect_fia();

    /* No current client. Reply will not do anything. */
    write_mock_none();

    message_p = chat_server_init_message_ind(&server);
    message_p->user_p = "Erik";
    message_p->text_p = "Hello.";
    chat_server_reply(&server);
}

TEST(disconnect_client_fia)
{
    struct chat_server_client_t *fia_p;

    start_server_with_three_clients();
    fia_p = connect_fia();

    /* Make the server disconnect the client. */
    mock_prepare_client_pending_disconnect(FIA_FD);

    chat_server_disconnect(&server, fia_p);

    /* Acutal disconnect next time process is called. */
    mock_prepare_destroy_pending_disconnect_client(FIA_FD);

    chat_server_process(&server, ERIK_FD, EPOLLIN);
}

static void on_message_ind_disconnect(struct chat_server_t *self_p,
                                      struct chat_server_client_t *client_p,
                                      struct chat_message_ind_t *message_in_p)
{
    (void)client_p;
    (void)message_in_p;

    chat_server_disconnect(self_p, NULL);
}

TEST(disconnect_current_client)
{
    start_server_with_three_clients();
    connect_fia();

    /* Make the server disconnect the client. */
    mock_prepare_read(FIA_FD, &message_ind_in[0], sizeof(message_ind_in));
    /* The client fd is set to -1 by pending disconnect function,
       ok? */
    read_mock_once(-1, HEADER_SIZE, -1);
    read_mock_set_errno(EIO);
    server_on_message_ind_mock_implementation(on_message_ind_disconnect);
    mock_prepare_client_pending_disconnect(FIA_FD);
    mock_prepare_destroy_pending_disconnect_client(FIA_FD);

    chat_server_process(&server, FIA_FD, EPOLLIN);
}

TEST(received_message_too_big)
{
    uint8_t big_message[] = {
        /* Header. Payload is never read. */
        0x01, 0x00, 0x00, 0xff
    };

    start_server_with_three_clients();
    connect_fia();

    /* Make the server disconnect the client by inputting a message
       that does not fit in the receive buffer. */
    mock_prepare_read(FIA_FD, &big_message[0], sizeof(big_message));
    mock_prepare_client_pending_disconnect(FIA_FD);
    mock_prepare_destroy_pending_disconnect_client(FIA_FD);

    chat_server_process(&server, FIA_FD, EPOLLIN);
}

TEST(output_buffering)
{
    struct chat_message_ind_t *message_p;
    struct chat_server_client_t *kalle_p;

    start_server_with_three_clients();
    kalle_p = connect_kalle();

    /* Send a message to Kalle. It is only partly sent, and the rest
       enqueued. */
    mock_prepare_write(KALLE_FD,
                       &message_ind_out[0],
                       sizeof(message_ind_out),
                       1,
                       0);
    mock_prepare_write(KALLE_FD,
                       &message_ind_out[1],
                       sizeof(message_ind_out) - 1,
                       -1,
                       EAGAIN);
    epoll_ctl_mock_once(EPOLL_FD, EPOLL_CTL_MOD, KALLE_FD, EPOLLIN | EPOLLOUT);

    message_p = chat_server_init_message_ind(&server);
    message_p->user_p = "Erik";
    message_p->text_p = "Hello.";
    chat_server_send(&server, kalle_p);

    /* Send another message to Kalle. It is enqueued. */
    message_p = chat_server_init_message_ind(&server);
    message_p->user_p = "Erik";
    message_p->text_p = "Hello.";
    chat_server_send(&server, kalle_p);

    /* Now set EPOLLOUT and transmit most of the enqueued data. */
    mock_prepare_write(KALLE_FD,
                       &message_ind_out[1],
                       sizeof(message_ind_out) - 1,
                       2,
                       0);
    mock_prepare_write(KALLE_FD,
                       &message_ind_out[3],
                       sizeof(message_ind_out) - 3,
                       sizeof(message_ind_out) - 3,
                       0);
    mock_prepare_write(KALLE_FD,
                       &message_ind_out[0],
                       sizeof(message_ind_out),
                       5,
                       0);
    mock_prepare_write(KALLE_FD,
                       &message_ind_out[5],
                       sizeof(message_ind_out) - 5,
                       -1,
                       EAGAIN);
    epoll_ctl_mock_none();

    chat_server_process(&server, KALLE_FD, EPOLLOUT);

    /* Transmit remaining data. */
    mock_prepare_write(KALLE_FD,
                       &message_ind_out[5],
                       sizeof(message_ind_out) - 5,
                       sizeof(message_ind_out) - 5,
                       0);
    epoll_ctl_mock_once(EPOLL_FD, EPOLL_CTL_MOD, KALLE_FD, EPOLLIN);

    chat_server_process(&server, KALLE_FD, EPOLLOUT);
}
