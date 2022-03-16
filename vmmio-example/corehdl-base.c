/*
 *
 * This file contains confidential information of Corellium LLC.
 * Unauthorized reproduction or distribution of this file is subject
 * to civil and criminal penalties.
 *
 * Copyright (C) 2019-21 Corellium LLC
 * All rights reserved.
 *
 */

#include "corehdl.h"

#define BUF_SIZE        32768

int corehdl_fd = -1, corehdl_dma_fd = -1;
static uint8_t corehdl_buf[2][BUF_SIZE];
static unsigned corehdl_buf_wp[2] = { 0 }, corehdl_buf_rp[2] = { 0 };
struct corehdl_if_s corehdl_if[MAX_IFACE*2];
static int corehdl_reset = 0;

int corehdl_connect_int(const char *target, int dma_fd)
{
    char *strp;
    char *port = NULL;
    unsigned portn;
    struct hostent *hent;
    struct sockaddr_in saddr;
    int res;

    if(!target)
        target = getenv("COREHDL_VM");
    if(!target) {
        fprintf(stderr, "[corehdl] Set environment variable COREHDL_VM to the address:port of the Corellium VM and try again.\n");
        return -EINVAL;
    }

    strp = malloc(strlen(target) + 1);
    if(!strp) {
        fprintf(stderr, "[corehdl] Memory allocation error.\n");
        return -ENOMEM;
    }
    strcpy(strp, target);

    port = strchr(strp, ':');
    if(port) {
        *(port++) = 0;
        portn = strtol(port, NULL, 0);
    } else
        portn = DFLT_PORT;

    hent = gethostbyname(strp);
    if(!hent) {
        fprintf(stderr, "[corehdl] Failed to resolve host %s: %s.\n", strp, hstrerror(h_errno));
        return -ENOENT;
    }

    corehdl_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(corehdl_fd < 0) {
        perror("[corehdl] Failed to create socket");
        return -errno;
    }

    memset(&saddr, 0, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(portn);
    saddr.sin_addr = *(struct in_addr *)hent->h_addr;

    if(connect(corehdl_fd, (struct sockaddr *)&saddr, sizeof(saddr)) != 0) {
        fprintf(stderr, "[corehdl] Failed to connect to %s:%d: %s.\n", strp, portn, strerror(errno));
        close(corehdl_fd);
        corehdl_fd = -1;
        return -errno;
    }

    res = 1;
    setsockopt(corehdl_fd, IPPROTO_TCP, TCP_NODELAY, (void *)&res, sizeof(res));

    if(dma_fd) {
        corehdl_dma_fd = socket(AF_INET, SOCK_STREAM, 0);
        if(corehdl_dma_fd < 0) {
            perror("[corehdl] Failed to create DMA socket");
            return -errno;
        }

        memset(&saddr, 0, sizeof(saddr));
        saddr.sin_family = AF_INET;
        saddr.sin_port = htons(portn);
        saddr.sin_addr = *(struct in_addr *)hent->h_addr;

        if(connect(corehdl_dma_fd, (struct sockaddr *)&saddr, sizeof(saddr)) != 0) {
            fprintf(stderr, "[corehdl] Failed to connect DMA to %s:%d: %s.\n", strp, portn, strerror(errno));
            close(corehdl_dma_fd);
            corehdl_dma_fd = -1;
            return -errno;
        }

        res = 1;
        setsockopt(corehdl_dma_fd, IPPROTO_TCP, TCP_NODELAY, (void *)&res, sizeof(res));
    }

    return 0;
}

void corehdl_connect(void)
{
    if(corehdl_connect_int(NULL, 0))
        exit(1);
}

void corehdl_claim_if(int ifn)
{
    uint64_t xfr[8] = { 0 };
    if(ifn < 0 || ifn >= MAX_IFACE)
        return;
    xfr[0] = 0x80000000 | ifn;
    xfr[1] = OP_RGN_CLAIM;
    write(corehdl_fd, xfr, 64);
}

int corehdl_recv_ready_int(int dma_fd)
{
    if ((dma_fd ? corehdl_fd : corehdl_dma_fd) < 0)
        return -ENOTCONN;

    return corehdl_buf_wp[dma_fd] - corehdl_buf_rp[dma_fd] < BUF_SIZE;
}

void corehdl_poll(void)
{
    if(corehdl_poll_int(0))
        exit(1);
}

int corehdl_poll_int(int dma_fd)
{
    uint64_t *xfr;
    int step, ifn0, ifn, op, len, xlen;
    int fd = dma_fd ? corehdl_dma_fd : corehdl_fd;

    if(fd < 0) {
        fprintf(stderr, "[corehdl] Not connected to VM yet.\n");
        return -ENOTCONN;
    }

    while(corehdl_buf_wp[dma_fd] - corehdl_buf_rp[dma_fd] < BUF_SIZE) {
        step = BUF_SIZE - (corehdl_buf_wp[dma_fd] - corehdl_buf_rp[dma_fd]);
        if(step > BUF_SIZE - (corehdl_buf_wp[dma_fd] % BUF_SIZE))
            step = BUF_SIZE - (corehdl_buf_wp[dma_fd] % BUF_SIZE);
        step = recv(fd, corehdl_buf[dma_fd] + (corehdl_buf_wp[dma_fd] % BUF_SIZE), step, MSG_DONTWAIT);
        if(step < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
            break;
        if(step <= 0) {
            fprintf(stderr, "[corehdl] Disconnected from host.\n");
            return -ECONNRESET;
        }
        corehdl_buf_wp[dma_fd] += step;
    }

    while(corehdl_buf_wp[dma_fd] - corehdl_buf_rp[dma_fd] >= 64) {
        xfr = (uint64_t *)(corehdl_buf[dma_fd] + (corehdl_buf_rp[dma_fd] % BUF_SIZE));
        len = (((xfr[0] >> 8) & 0xFF) + 1) * 64;
        if(corehdl_buf_wp[dma_fd] - corehdl_buf_rp[dma_fd] < len)
            break;
        ifn0 = xfr[0] & 0xFF;
        ifn = (xfr[0] & 0x80000000ul) ? (ifn0 + MAX_IFACE) : ifn0;
        op = xfr[1] & 0xFF;
        if(op == OP_PING) {
            xfr[0] &= ~0xFF00ul;
            write(fd, xfr, 64);
        } else if(op == OP_RESET) {
            if(!dma_fd)
                corehdl_reset = 1;
        } else if(ifn0 >= MAX_IFACE || !corehdl_if[ifn].ready) {
            if(ifn < MAX_IFACE) { /* return to initiator */
                xfr[0] &= ~0xFF00ul;
                write(fd, xfr, 64);
            }
        } else {
            if(corehdl_if[ifn].q_wp - corehdl_if[ifn].q_rp >= MAX_TXN_PER_IF)
                break;
            xlen = len > 128 ? 128 : len;
            step = corehdl_buf_rp[dma_fd] % BUF_SIZE;
            if(step + xlen > BUF_SIZE) {
                step = BUF_SIZE - step;
                memcpy(corehdl_if[ifn].q[corehdl_if[ifn].q_wp % MAX_TXN_PER_IF], xfr, step);
                memcpy(corehdl_if[ifn].q[corehdl_if[ifn].q_wp % MAX_TXN_PER_IF] + (step >> 3), corehdl_buf[dma_fd], xlen - step);
            } else
                memcpy(corehdl_if[ifn].q[corehdl_if[ifn].q_wp % MAX_TXN_PER_IF], xfr, xlen);
            corehdl_if[ifn].q_wp ++;
        }
        corehdl_buf_rp[dma_fd] += len;
    }
    return 0;
}

uint32_t corehdl_get_mask(uint64_t *xfr, int byteoffs, unsigned pair, unsigned size)
{
    uint32_t mask = xfr[1] >> 32;
    if(pair)
        mask = (mask & ((1u << (size >> 1)) - 1u)) | (mask >> (16 - (size >> 1)));
    return (byteoffs >= 32) ? 0 : (byteoffs < 0) ? (mask << -byteoffs) : (mask >> byteoffs);
}

uint64_t corehdl_get_data(uint64_t *xfr, int byteoffs, unsigned pair, unsigned size)
{
    uint64_t w[2] = { 0, 0 };
    int aoffs = (byteoffs < 0) ? 0 : byteoffs;
    if(pair && size < 16) {
        w[0] = (xfr[3] & ((1ul << (size << 2)) - 1ul)) | (xfr[4] << (size << 2));
    } else {
        if(aoffs < 40)
            w[0] = xfr[3 + (aoffs >> 3)];
        if(aoffs < 32)
            w[1] = xfr[4 + (aoffs >> 3)];
    }
    aoffs &= 7;
    w[0] = *(uint64_t *)((uint8_t *)w + aoffs);
    return (byteoffs < 0) ? w[0] << ((-byteoffs) * 8) : w[0];
}

void corehdl_put_data(uint64_t *xfr, int byteoffs, unsigned pair, unsigned size, uint64_t data, unsigned width)
{
    uint64_t w[2] = { 0, 0 }, lowoffs;
    if(byteoffs < 0) {
        data >>= (-byteoffs) * 8;
        byteoffs = 0;
    }
    if(pair && size < 16) {
        w[0] = (xfr[3] & ((1ul << (size << 2)) - 1ul)) | (xfr[4] << (size << 2));
    } else {
        if(byteoffs < 40)
            w[0] = xfr[3 + (byteoffs >> 3)];
        if(byteoffs < 32)
            w[1] = xfr[4 + (byteoffs >> 3)];
    }
    lowoffs = byteoffs & 7;
    switch(width) {
    case 1: *((uint8_t *)w + lowoffs) = data; break;
    case 2: *(uint16_t *)((uint8_t *)w + lowoffs) = data; break;
    case 4: *(uint32_t *)((uint8_t *)w + lowoffs) = data; break;
    case 8: *(uint64_t *)((uint8_t *)w + lowoffs) = data; break;
    }
    if(pair && size < 16) {
        xfr[3] = w[0];
        xfr[4] = w[0] >> (size << 2);
    } else {
        if(byteoffs < 40)
            xfr[3 + (byteoffs >> 3)] = w[0];
        if(byteoffs < 32)
            xfr[4 + (byteoffs >> 3)] = w[1];
    }
}

void corehdl_unpack_bt(struct corehdl_if_s *cif, int ibt, uint64_t *xfr, int raw)
{
    unsigned i, j, p, size, nsb;
    uint8_t *packed;
    int64_t delta;
    uint64_t prev = 0;

    if(ibt >= MAX_BTIF)
        return;

    if(!raw) {
        size = (xfr[1] >> 16) & 0x3F;
        p = (size + 7) >> 3;
        j = xfr[1] & 0xFFu;
        if(j == OP_READP && j == OP_WRITEP && j == OP_WRITEAP)
            p *= 2;
        xfr = &xfr[p+3];
    }

    packed = (uint8_t *)xfr;
    size = packed[0];
    for(i=1,j=0; i<size && j<MAX_BT; j++) {
        p = (packed[i] & 7) + 2;
        memcpy(&delta, packed + i + 1, p - 1);
        delta <<= 5;
        delta |= packed[i] >> 3;
        nsb = p * 8 - 3;
        if(nsb < 64) {
            delta <<= 64 - nsb;
            delta >>= 64 - nsb;
        }
        delta <<= 2;
        prev = cif->btrace[ibt].bt[j] = prev + delta;
        i += p;
    }
    cif->btrace[ibt].nbt = j;
}

void corehdl_unim_bt(const int ifn, const int ibt, const int ent, long long *res)
{
    struct corehdl_if_s *cif;
    if(ifn >= MAX_IFACE || !corehdl_if[ifn].ready || ibt >= MAX_BTIF || ent >= MAX_BT)
        return;
    cif = &corehdl_if[ifn];
    if(ent < cif->btrace[ibt].nbt)
        *res = cif->btrace[ibt].bt[ent];
    else
        *res = -1ll;
}

void corehdl_rstctrl_setup(void)
{
    if(corehdl_fd < 0)
        corehdl_connect();
}

void corehdl_rstctrl_transact(const int rsti, int *rsto)
{
    corehdl_poll();
    if(rsti)
        corehdl_reset = 0;
    *rsto = corehdl_reset;
}

#if IVERILOG_VPI

static int corehdl_unim_bt_compiletf(char *user_data)
{
    return 0;
}

static int corehdl_unim_bt_calltf(char *user_data)
{
    int ifn = tf_getp(1);
    int ibt = tf_getp(2);
    int ent = tf_getp(3);
    long long res = -1l;
    corehdl_unim_bt(ifn, ibt, ent, &res);
    tf_putlongp(4, res, res >> 32);
    return 0;
}

static int corehdl_rstctrl_setup_compiletf(char *user_data)
{
    return 0;
}

static int corehdl_rstctrl_setup_calltf(char *user_data)
{
    corehdl_rstctrl_setup();
    return 0;
}

static int corehdl_rstctrl_transact_compiletf(char *user_data)
{
    return 0;
}

static int corehdl_rstctrl_transact_calltf(char *user_data)
{
    int rsti = tf_getp(1);
    int rsto;
    corehdl_rstctrl_transact(rsti, &rsto);
    tf_putp(2, rsto);
    return 0;
}

void corehdl_register(void)
{
    s_vpi_systf_data tf_data;

    tf_data.type      = vpiSysTask;
    tf_data.tfname    = "$corehdl_rstctrl_setup";
    tf_data.calltf    = corehdl_rstctrl_setup_calltf;
    tf_data.compiletf = corehdl_rstctrl_setup_compiletf;
    tf_data.sizetf    = 0;
    tf_data.user_data = 0;
    vpi_register_systf(&tf_data);

    tf_data.type      = vpiSysTask;
    tf_data.tfname    = "$corehdl_rstctrl_transact";
    tf_data.calltf    = corehdl_rstctrl_transact_calltf;
    tf_data.compiletf = corehdl_rstctrl_transact_compiletf;
    tf_data.sizetf    = 0;
    tf_data.user_data = 0;
    vpi_register_systf(&tf_data);

    tf_data.type      = vpiSysTask;
    tf_data.tfname    = "$corehdl_unim_bt";
    tf_data.calltf    = corehdl_unim_bt_calltf;
    tf_data.compiletf = corehdl_unim_bt_compiletf;
    tf_data.sizetf    = 0;
    tf_data.user_data = 0;
    vpi_register_systf(&tf_data);

    corehdl_wbm_register();
    corehdl_wbs_register();
    corehdl_axim_register();
    corehdl_axis_register();
    corehdl_apbm_register();
    corehdl_ahbm_register();
    corehdl_ahbs_register();
}

void (*vlog_startup_routines[])() = {
    corehdl_register,
    0 };

#endif
