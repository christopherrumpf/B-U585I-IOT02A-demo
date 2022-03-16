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

#ifndef _COREHDL_H
#define _COREHDL_H

#define _BSD_SOURCE 1
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>

#if IVERILOG_VPI
#include <iverilog/vpi_user.h>
#include <iverilog/veriuser.h>
#endif

#define MAX_IFACE       8
#define MAX_IRQ         8
#define MAX_TXN_PER_IF  16
#define AXIS_WQ         64
#define AXIS_TQ         16
#define AHBM_FR         32
#define MAX_BT          16
#define MAX_BTIF        2

#define DFLT_PORT       4800

#define OP_PING         0x00
#define OP_READ         0x01
#define OP_WRITE        0x02
#define OP_READP        0x03
#define OP_WRITEP       0x04
#define OP_WRITEA       0x05
#define OP_WRITEAP      0x06
#define OP_IRQ_UPDATE   0x41
#define OP_RGN_CLAIM    0xF0
#define OP_RESET        0xFF

extern int corehdl_fd, corehdl_dma_fd;
extern struct corehdl_if_s {
    unsigned ready, width, rwidth, rstlast, req_gen;
    unsigned l2w, l2rw;
    uint64_t q[MAX_TXN_PER_IF][16];
    unsigned q_wp, q_rp;
    unsigned step, nstep;
    unsigned irqvld;
    struct {
        uint64_t bt[MAX_BT];
        unsigned nbt;
    } btrace[MAX_BTIF];
    union {
        struct {
            long long adr_o;
            long long dat_o;
            int we_o;
            int stb_o;
            int cyc_o;
            int sel_o;
        } wbm;
        struct {
            int ack_o;
            long long dat_o;
        } wbs;
        struct {
            unsigned lite;
            unsigned nsplit, issue, itr;

            unsigned num_rids, num_wids;
            unsigned widm, ridm, widwp, widrp, ridwp, ridrp;
            unsigned *wid, *rid;
            struct corehdl_axim_tm {
                int tr, offs, num, cnt;
            } *wtm, *rtm;

            unsigned ftrm, ftrwp, ftrrp;
            unsigned *ftr;
            struct corehdl_axim_tr {
                unsigned mask;
                uint64_t xfr[16];
            } *tr;

            int awid, awlen, awsize, awburst, awlock, awcache, awprot, awqos, awregion, awvalid;
            long long awaddr;

            int arid, arlen, arsize, arburst, arlock, arcache, arprot, arqos, arregion, arvalid;
            long long araddr;

            long long wdata0, wdata1, wdata2, wdata3;
            int wstrb, wlast, wvalid;
        } axim;
        struct {
            struct corehdl_axis_wq {
                long long wdata[4];
                int wstrb, wlast;
            } wq[AXIS_WQ];
            unsigned wqwp, wqrp;

            struct corehdl_axis_tq {
                int wr, id, len, size, burst;
                unsigned char region, prot;
                long long addr;
            } tq[AXIS_TQ];
            unsigned tqwp, tqrp;

            int awready, wready;
            int bid, bresp, bvalid;
            int arready;
            int rid;
            long long rdata0, rdata1, rdata2, rdata3;
            int rresp, rlast, rvalid;
        } axis;
        struct {
            unsigned setup_phase;

            long long paddr, pwdata;
            int pprot, psel, penable, pwrite, pstrb;
        } apbm;
        struct {
            struct corehdl_ahbm_frag {
                unsigned char l2sz, offs;
                unsigned char hburst, htrans;
            } fr[AHBM_FR];
            uint64_t dxfr[8], exfr[8];
            int dvalid, doffs, dsize, dpair, dlast, dfirst;
            int evalid, eoffs, esize, epair, elast;

            long long haddr;
            int hwrite, hsize, hburst, hprot, htrans, hmastlock;
            long long hwdata0, hwdata1, hwdata2, hwdata3;
        } ahbm;
        struct {
            uint64_t wxfr[8];
            uint64_t rxfr[8], raddr;
            unsigned rbidx;

            int hready, hresp;
            long long hrdata0, hrdata1, hrdata2, hrdata3;
        } ahbs;
    };
} corehdl_if[MAX_IFACE*2];

void corehdl_connect(void);
int corehdl_connect_int(const char *target, int dma_fd);
void corehdl_claim_if(int ifn);
void corehdl_poll(void);
int corehdl_poll_int(int dma_fd);
int corehdl_recv_ready_int(int dma_fd);
uint32_t corehdl_get_mask(uint64_t *xfr, int byteoffs, unsigned pair, unsigned size);
uint64_t corehdl_get_data(uint64_t *xfr, int byteoffs, unsigned pair, unsigned size);
void corehdl_put_data(uint64_t *xfr, int byteoffs, unsigned pair, unsigned size, uint64_t data, unsigned width);
void corehdl_unpack_bt(struct corehdl_if_s *cif, int ibt, uint64_t *xfr, int raw);

void corehdl_rstctrl_setup(void);
void corehdl_rstctrl_transact(const int rsti, int *rsto);
void corehdl_unim_bt(const int ifn, const int ibt, const int ent, long long *res);
void corehdl_wbm_setup(const int ifn, const int data_width);
void corehdl_wbm_transact(const int ifn, const int rst, long long *adr_o, const long long dat_i, long long *dat_o, int *we_o, int *stb_o, const int ack_i, int *cyc_o, int *sel_o, const int irqvld);
void corehdl_wbs_setup(const int ifn, const int data_width);
void corehdl_wbs_transact(const int ifn, const int rst, const long long adr_i, long long *dat_o, const long long dat_i, const int we_i, const int stb_i, int *ack_o, const int cyc_i, const int sel_i);
void corehdl_axim_setup(const int ifn, const int lite, const int write_width, const int num_wids, const int read_width, const int num_rids);
void corehdl_axim_transact(const int ifn, const int rst,
    int *awid, long long *awaddr, int *awlen, int *awsize, int *awburst, int *awlock, int *awcache, int *awprot, int *awqos, int *awregion, int *awvalid, const int awready,
    long long *wdata0, long long *wdata1, long long *wdata2, long long *wdata3, int *wstrb, int *wlast, int *wvalid, const int wready,
    const int bid, const int bresp, const int bvalid, int *bready,
    int *arid, long long *araddr, int *arlen, int *arsize, int *arburst, int *arlock, int *arcache, int *arprot, int *arqos, int *arregion, int *arvalid, const int arready,
    const int rid, const long long rdata0, const long long rdata1, const long long rdata2, const long long rdata3, const int rresp, const int rlast, const int rvalid, int *rready,
    const int irqvld);
void corehdl_axis_setup(const int ifn, const int write_width, const int read_width);
void corehdl_axis_transact(const int ifn, const int rst,
    const int awid, const long long awaddr, const int awlen, const int awsize, const int awburst, const int awlock, const int awcache, const int awprot, const int awqos, const int awregion, const int awvalid, int *awready,
    const long long wdata0, const long long wdata1, const long long wdata2, const long long wdata3, const int wstrb, const int wlast, const int wvalid, int *wready,
    int *bid, int *bresp, int *bvalid, const int bready,
    const int arid, const long long araddr, const int arlen, const int arsize, const int arburst, const int arlock, const int arcache, const int arprot, const int arqos, const int arregion, const int arvalid, int *arready,
    int *rid, long long *rdata0, long long *rdata1, long long *rdata2, long long *rdata3, int *rresp, int *rlast, int *rvalid, const int rready);
void corehdl_apbm_setup(const int ifn, const int data_width);
void corehdl_apbm_transact(const int ifn, const int rst, long long *paddr, int *pprot, int *psel, int *penable, int *pwrite, long long *pwdata, int *pstrb, const int pready, const long long prdata, const int pslverr, const int irqvld);
void corehdl_ahbm_setup(const int ifn, const int data_width);
void corehdl_ahbm_transact(const int ifn, const int rst,
    long long *haddr, int *hwrite, int *hsize, int *hburst, int *hprot, int *htrans, int *hmastlock, long long *hwdata0, long long *hwdata1, long long *hwdata2, long long *hwdata3,
    const int hready, const int hresp, const long long hrdata0, const long long hrdata1, const long long hrdata2, const long long hrdata3, const int irqvld);
void corehdl_ahbs_setup(const int ifn, const int data_width);
void corehdl_ahbs_transact(const int ifn, const int rst,
    const long long haddr, const int hwrite, const int hsize, const int hburst, const int hprot, const int htrans, const int hmastlock, const long long hwdata0, const long long hwdata1, const long long hwdata2, const long long hwdata3,
    int *hready, int *hresp, long long *hrdata0, long long *hrdata1, long long *hrdata2, long long *hrdata3);

#if IVERILOG_VPI
void corehdl_wbm_register(void);
void corehdl_wbs_register(void);
void corehdl_axim_register(void);
void corehdl_axis_register(void);
void corehdl_apbm_register(void);
void corehdl_ahbm_register(void);
void corehdl_ahbs_register(void);
#endif

#endif
