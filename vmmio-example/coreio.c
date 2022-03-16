/*
 *
 * This file contains confidential information of Corellium LLC.
 * Unauthorized reproduction or distribution of this file is subject
 * to civil and criminal penalties.
 *
 * Copyright (C) 2021 Corellium LLC
 * All rights reserved.
 *
 */

#include <stdio.h>
#include <stdint.h>
#include <sys/select.h>
#include <sys/time.h>
#include <time.h>

#include "coreio.h"
#include "corehdl.h"

static struct {
    coreio_func_t func;
    void *funcp;
} coreio_vmmio[MAX_IFACE];
static unsigned coreio_vmmio_mask = 0;

static struct coreio_dma_op {
    struct coreio_dma_op *prev, *next;
    unsigned mid;
    unsigned sid;
    uint64_t addr;
    size_t len;
    uint8_t *buf;
    unsigned flags;
    void (*cpl)(void *, int);
    void *cplp;
    unsigned wr;
    unsigned outst;
    int err;
} coreio_dma_ops = { &coreio_dma_ops, &coreio_dma_ops };
static unsigned coreio_dmaif_mask = 0;

#define NDMAPKTS 1024
static struct coreio_dma_pkt {
    struct coreio_dma_op *op;
    uint8_t *buf;
    size_t size;
} coreio_dma_pkts[NDMAPKTS];
static struct {
    uint32_t q[NDMAPKTS];
    uint32_t wp, rp;
} coreio_dma_pktalloc;

int coreio_connect(const char *target)
{
    unsigned i;
    for(i=0; i<NDMAPKTS; i++)
        coreio_dma_pktalloc.q[i] = i;
    coreio_dma_pktalloc.wp = NDMAPKTS;
    coreio_dma_pktalloc.rp = 0;
    return corehdl_connect_int(target, 1);
}

static int coreio_dma_create(unsigned mid, unsigned sid, uint64_t addr, size_t len, void *buf, unsigned flags, void (*cpl)(void *, int), void *cplp, unsigned wr)
{
    struct coreio_dma_op *op;

    if(mid >= MAX_IFACE)
        return -EINVAL;
    if(sid >= 16)
        return -EINVAL;
    if(!len)
        return -EINVAL;

    op = calloc(1, sizeof(*op));
    if(!op)
        return -ENOMEM;

    op->mid = mid;
    op->sid = sid;
    op->addr = addr;
    op->len = len;
    op->buf = buf;
    op->flags = flags & 15;
    op->cpl = cpl;
    op->cplp = cplp;
    op->wr = wr;
    op->prev = coreio_dma_ops.prev;
    op->next = &coreio_dma_ops;
    op->prev->next = op;
    op->next->prev = op;

    corehdl_if[mid + MAX_IFACE].ready = 1;
    coreio_dmaif_mask |= 1u << mid;

    return 0;
}

static void coreio_dma_kick(void)
{
    struct coreio_dma_op *op;
    unsigned pid;
    uint64_t xfr[8];
    uint64_t size, pmask;

    if(corehdl_dma_fd < 0)
        return;

    while(1) {
        if(coreio_dma_ops.next == &coreio_dma_ops)
            break;
        op = coreio_dma_ops.next;

        if(coreio_dma_pktalloc.wp == coreio_dma_pktalloc.rp)
            break;
        pid = coreio_dma_pktalloc.q[coreio_dma_pktalloc.rp % NDMAPKTS];
        coreio_dma_pktalloc.rp ++;

        size = 32 - (op->addr & 31);
        if(size > op->len)
            size = op->len;
        pmask = ~((-1ul) << size);

        xfr[0] = ((uint64_t)pid << 32) | 0x80000000ull | op->mid;
        xfr[1] = (op->wr ? OP_WRITEA : OP_READ) | (size << 16) | (op->sid << 24) | (op->wr ? (pmask << 32) : 0) | (op->flags << 28);
        xfr[2] = op->addr;
        memset(&xfr[3], 0xAA, 32);
        xfr[7] = 0;
        if(op->wr)
            memcpy(&xfr[3], op->buf, size);
        write(corehdl_dma_fd, xfr, 64);

        coreio_dma_pkts[pid].op = op;
        coreio_dma_pkts[pid].buf = op->buf;
        coreio_dma_pkts[pid].size = size;

        op->outst ++;
        op->addr += size;
        op->buf += size;
        op->len -= size;
        if(!op->len) {
            op->next->prev = op->next;
            op->prev->next = op->prev;
            op->next = op->prev = NULL;
        }
    }
}

static void coreio_dma_recv(uint64_t *xfr)
{
    struct coreio_dma_op *op;
    unsigned pid;

    if((xfr[1] & 0xFF) != OP_WRITEA && (xfr[1] & 0xFF) != OP_READ)
        return;
    if((xfr[0] & 0x80000000u) != 0x80000000u)
        return;

    pid = xfr[0] >> 32;
    if(pid >= NDMAPKTS || !coreio_dma_pkts[pid].op)
        return;
    op = coreio_dma_pkts[pid].op;

    if(xfr[0] & 0x40000000u) {
        if(!op->err)
            op->err = -EFAULT;
    } else
        if(!op->wr)
            memcpy(coreio_dma_pkts[pid].buf, &xfr[3], coreio_dma_pkts[pid].size);

    coreio_dma_pkts[pid].op = NULL;
    coreio_dma_pkts[pid].buf = NULL;
    coreio_dma_pkts[pid].size = 0;
    coreio_dma_pktalloc.q[coreio_dma_pktalloc.wp % NDMAPKTS] = pid;
    coreio_dma_pktalloc.wp ++;

    op->outst --;
    if(!op->outst) {
        if(op->cpl)
            op->cpl(op->cplp, op->err);
        free(op);
    }
}

static void coreio_dma_signal(void *param, int err)
{
    int *done = param;
    *done = err ? err : 1;
}

static void coreio_dma_block(int *done)
{
    struct corehdl_if_s *cif;
    int res, nfds, skip;
    unsigned mask, ifn;
    fd_set readfds;

    skip = 1;
    while(!*done) {
        if(!skip) {
            FD_ZERO(&readfds);
            FD_SET(corehdl_dma_fd, &readfds);
            nfds = corehdl_dma_fd + 1;
            select(nfds, &readfds, NULL, NULL, NULL);
        }

        res = corehdl_poll_int(1);
        if(res) {
            *done = res;
            break;
        }
        mask = coreio_dmaif_mask;
        res = 0;
        while(mask) {
            ifn = __builtin_ctz(mask);
            mask &= mask - 1;
            cif = &corehdl_if[ifn + MAX_IFACE];
            while(cif->q_wp != cif->q_rp) {
                coreio_dma_recv(cif->q[cif->q_rp % MAX_TXN_PER_IF]);
                cif->q_rp ++;
                res = 1;
                skip = 1;
            }
        }
        if(res)
            coreio_dma_kick();
    }
}

int coreio_dma_read(unsigned mid, unsigned sid, uint64_t addr, size_t len, void *buf, unsigned flags, void (*cpl)(void *, int), void *cplp)
{
    int done = 0;
    int res = coreio_dma_create(mid, sid, addr, len, buf, flags, cpl ? cpl : coreio_dma_signal, cpl ? cplp : &done, 0);
    if(res)
        return res;
    coreio_dma_kick();
    if(!cpl) {
        coreio_dma_block(&done);
        return (done < 0) ? done : 0;
    }
    return 0;
}

int coreio_dma_write(unsigned mid, unsigned sid, uint64_t addr, size_t len, void *buf, unsigned flags, void (*cpl)(void *, int), void *cplp)
{
    int done = 0;
    int res = coreio_dma_create(mid, sid, addr, len, buf, flags, cpl ? cpl : coreio_dma_signal, cpl ? cplp : &done, 1);
    if(res)
        return res;
    coreio_dma_kick();
    if(!cpl) {
        coreio_dma_block(&done);
        return (done < 0) ? done : 0;
    }
    return 0;
}

int coreio_irq_update(unsigned rid, unsigned iid, unsigned state)
{
    struct corehdl_if_s *cif = &corehdl_if[rid];
    uint64_t xfr[8];

    if(corehdl_fd < 0)
        return -ENOTCONN;
    if(iid >= MAX_IRQ)
        return -EINVAL;

    state = !!state;
    if(((cif->irqvld >> iid) & 1) == state)
        return 0;
    cif->irqvld ^= 1u << iid;

    xfr[0] = 0x80000000u | rid;
    xfr[1] = OP_IRQ_UPDATE;
    xfr[2] = iid;
    xfr[3] = (cif->irqvld >> iid) & 1u;
    xfr[4] = xfr[5] = xfr[6] = xfr[7] = 0;
    write(corehdl_fd, xfr, 64);
    return 0;
}

int coreio_register(unsigned rid, const coreio_func_t *func, void *funcp)
{
    uint64_t xfr[8];

    if(rid >= MAX_IFACE)
        return -EINVAL;
    if(corehdl_fd < 0)
        return -ENOTCONN;
    if(coreio_vmmio[rid].func.read)
        return -EBUSY;
    coreio_vmmio[rid].func = *func;
    coreio_vmmio[rid].funcp = funcp;
    coreio_vmmio_mask |= 1u << rid;
    corehdl_if[rid].ready = 1;

    xfr[0] = 0x80000000u | rid;
    xfr[1] = OP_RGN_CLAIM;
    xfr[2] = xfr[3] = xfr[4] = xfr[5] = xfr[6] = xfr[7] = 0;
    write(corehdl_fd, xfr, 64);
    return 0;
}

static void coreio_mmio_recv(unsigned rid, uint64_t *xfr)
{
    unsigned op = xfr[1] & 0xFF;
    const coreio_func_t *func = &coreio_vmmio[rid].func;
    void *funcp = coreio_vmmio[rid].funcp;
    unsigned size;

    if(corehdl_fd < 0)
        return;

    if(!func)
        return;

    size = (xfr[1] >> 16) & 63;
    switch(op) {
    case OP_READ:
        if(func->read)
            if(func->read(funcp, xfr[2], size, &xfr[3], (xfr[1] >> 28) & 15))
                xfr[0] |= 0x40000000u;
        write(corehdl_fd, xfr, 64);
        break;
    case OP_READP:
        if(func->readp) {
            if(func->readp(funcp, xfr[2], size, &xfr[3], &xfr[4 + (size >> 3)], (xfr[1] >> 28) & 15))
                xfr[0] |= 0x40000000u;
        } else if(func->read)
            if(func->read(funcp, xfr[2], size, &xfr[3], (xfr[1] >> 28) & 15) ||
               func->read(funcp, xfr[2], size, &xfr[4 + (size >> 3)], (xfr[1] >> 28) & 15))
                xfr[0] |= 0x40000000u;
        write(corehdl_fd, xfr, 64);
        break;
    case OP_WRITE:
    case OP_WRITEA:
        if(func->write)
            if(func->write(funcp, xfr[2], size, &xfr[3], (xfr[1] >> 28) & 15))
                xfr[0] |= 0x40000000u;
        if(op == OP_WRITEA)
            write(corehdl_fd, xfr, 64);
        break;
    case OP_WRITEP:
    case OP_WRITEAP:
        if(func->writep) {
            if(func->writep(funcp, xfr[2], size, &xfr[3], &xfr[4 + (size >> 3)], (xfr[1] >> 28) & 15))
                xfr[0] |= 0x40000000u;
        } else if(func->write)
            if(func->write(funcp, xfr[2], size, &xfr[3], (xfr[1] >> 28) & 15) ||
               func->write(funcp, xfr[2], size, &xfr[4 + (size >> 3)], (xfr[1] >> 28) & 15))
                xfr[0] |= 0x40000000u;
        if(op == OP_WRITEAP)
            write(corehdl_fd, xfr, 64);
        break;
    }
}

int coreio_preparefds(int nfds, fd_set *readfds, fd_set *writefds)
{
    if(corehdl_recv_ready_int(0) > 0) {
        FD_SET(corehdl_fd, readfds);
        if(corehdl_fd >= nfds)
            nfds = corehdl_fd + 1;
    }
    if(corehdl_recv_ready_int(1) > 0) {
        FD_SET(corehdl_dma_fd, readfds);
        if(corehdl_dma_fd >= nfds)
            nfds = corehdl_dma_fd + 1;
    }
    return nfds;
}

int coreio_processfds(fd_set *readfds, fd_set *writefds)
{
    struct corehdl_if_s *cif;
    unsigned mask, ifn;
    int res, cont = 1;

    if(corehdl_fd < 0)
        return -ENOTCONN;

    if(FD_ISSET(corehdl_fd, readfds)) {
        res = corehdl_poll_int(0);
        if(res)
            return res;
    }
    if(FD_ISSET(corehdl_dma_fd, readfds)) {
        res = corehdl_poll_int(1);
        if(res)
            return res;
    }

    while(cont) {
        cont = 0;
        mask = coreio_vmmio_mask;
        while(mask) {
            ifn = __builtin_ctz(mask);
            mask &= mask - 1;
            cif = &corehdl_if[ifn];
            while(cif->q_wp != cif->q_rp) {
                coreio_mmio_recv(ifn, cif->q[cif->q_rp % MAX_TXN_PER_IF]);
                cif->q_rp ++;
                cont = 1;
            }
        }

        mask = coreio_dmaif_mask;
        res = 0;
        while(mask) {
            ifn = __builtin_ctz(mask);
            mask &= mask - 1;
            cif = &corehdl_if[ifn + MAX_IFACE];
            while(cif->q_wp != cif->q_rp) {
                coreio_dma_recv(cif->q[cif->q_rp % MAX_TXN_PER_IF]);
                cif->q_rp ++;
                res = 1;
                cont = 1;
            }
        }
        if(res)
            coreio_dma_kick();

        if(cont) {
            res = corehdl_poll_int(0);
            if(res)
                return res;
            res = corehdl_poll_int(1);
            if(res)
                return res;
        }
    }

    return 0;
}

static uint64_t coreio_get_microtime(void)
{
    struct timespec tsp;
    clock_gettime(CLOCK_MONOTONIC, &tsp);
    return tsp.tv_sec * 1000000ul + (tsp.tv_nsec / 1000ul);
}

int coreio_mainloop(long long usec)
{
    long long now_us = coreio_get_microtime();
    long long end_us = now_us + usec;
    fd_set readfds, writefds;
    struct timeval tv = { 0, 0 };
    int res;

    while(usec < 0 || end_us >= now_us) {
        if(usec >= 0) {
            tv.tv_sec = (end_us - now_us) / 1000000ull;
            tv.tv_usec = (end_us - now_us) % 1000000ull;
        }
        FD_ZERO(&writefds);
        FD_ZERO(&readfds);
        int nfds = coreio_preparefds(0, &readfds, &writefds);
        if(nfds < 0)
            return nfds;
        select(nfds, &readfds, &writefds, NULL, usec >= 0 ? &tv : NULL);
        res = coreio_processfds(&readfds, &writefds);
        if(res)
            return res;
        now_us = coreio_get_microtime();
    }
    return 0;
}

void coreio_disconnect(void)
{
    close(corehdl_fd);
    close(corehdl_dma_fd);
    corehdl_fd = -1;
    corehdl_dma_fd = -1;
}
