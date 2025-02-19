#include <assert.h>
#include <stdbool.h>
#include <string.h>

#include "../include/udx.h"

uv_loop_t loop;

udx_t asock;
udx_stream_t astream;

udx_t bsock;
udx_stream_t bstream;

udx_stream_write_t req;

bool ack_called = false;
bool read_called = false;
bool preconnect_called = false;

void
on_ack (udx_stream_write_t *r, int status, int unordered) {
  assert(&req == r);
  assert(status == 0);
  assert(unordered == 0);

  uv_stop(&loop);

  ack_called = true;
}

void
on_read (udx_stream_t *handle, ssize_t read_len, const uv_buf_t *buf) {
  assert(buf->len == 5);
  assert(buf->len == read_len);
  assert(memcmp(buf->base, "hello", 5) == 0);

  read_called = true;
}

void
on_preconnect (udx_t *sock, uint32_t id, struct sockaddr *addr) {
  int e;

  e = udx_stream_connect(&astream, sock, 2, addr, NULL);
  assert(e == 0);

  e = udx_stream_read_start(&astream, on_read);
  assert(e == 0);

  preconnect_called = true;
}

int
main () {
  int e;

  uv_loop_init(&loop);

  e = udx_init(&loop, &asock);
  assert(e == 0);

  e = udx_init(&loop, &bsock);
  assert(e == 0);

  struct sockaddr_in aaddr;
  uv_ip4_addr("127.0.0.1", 8081, &aaddr);
  e = udx_bind(&asock, (struct sockaddr *) &aaddr);
  assert(e == 0);

  struct sockaddr_in baddr;
  uv_ip4_addr("127.0.0.1", 8082, &baddr);
  e = udx_bind(&bsock, (struct sockaddr *) &baddr);
  assert(e == 0);

  e = udx_stream_init(&loop, &astream, 1);
  assert(e == 0);

  e = udx_stream_init(&loop, &bstream, 2);
  assert(e == 0);

  e = udx_preconnect(&asock, on_preconnect);
  assert(e == 0);

  e = udx_stream_connect(&bstream, &bsock, 1, (struct sockaddr *) &aaddr, NULL);
  assert(e == 0);

  uv_buf_t buf = uv_buf_init("hello", 5);
  udx_stream_write(&req, &bstream, &buf, 1, on_ack);

  uv_run(&loop, UV_RUN_DEFAULT);

  assert(ack_called && read_called && preconnect_called);

  return 0;
}
