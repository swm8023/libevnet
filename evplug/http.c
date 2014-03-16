#include <stdio.h>
#include <string.h>

#include <evbase/log.h>
#include <evnet/filelog.h>
#include <evbase/evt.h>
#include <evbase/util.h>
#include <evnet/tcpsv.h>
#include <evnet/udpsv.h>
#include <evbase/thread.h>

char resp[]={"\
HTTP/1.1 200 OK\r\n\
Server: Apache-Coyote/1.1\r\n\
Set-Cookie: JSESSIONID=654E7C572DE63F77AAA9AC004A7C80B2; Path=/\r\n\
Content-Type: text/html;charset=GB18030\r\n\
Content-Length: 11\r\n\
Date: Sun, 16 Mar 2014 10:56:00 GMT\r\n\
\r\n\
hello world\
"};
int resplen;

int findrnrn(char *str, int len){
    str[len] = 0;
    char *p = strstr(str, "\r\n\r\n");
    if (p == NULL)
        return -1;
    else
        return p - str + 4;
}

static void on_read_comp(TCPCLT_P client, BUF_P buf, int rlen) {
    char buff[4096];
    if (findrnrn(buf->buf + buf->r_ind, rlen) != -1) {
        buff_readall(buf, buff, 4096);
        tcp_send(client, resp, resplen);
    }
}


int http_start(int port, int thread) {
    resplen = strlen(resp);
    EP_P pool = evt_pool_init(thread);
    TCPSRV_P server  = tcp_server_init(port, TCP_DEFAULT_FLAG);
    tcp_set_read_comp_cb(server, on_read_comp);
    tcp_server_bind_pool(server, pool);
    evt_pool_run(pool);
}