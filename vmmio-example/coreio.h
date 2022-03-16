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

#ifndef _COREIO_H
#define _COREIO_H

#include <stdint.h>
#include <sys/select.h>

/* Connect to a VM.
 *  target      string like "10.10.0.3:23456"
 * Returns error flag.
 */
int coreio_connect(const char *target);

#define COREIO_FLAGS_IFETCH     2
#define COREIO_FLAGS_PRIV       4
#define COREIO_FLAGS_SECURE     8

/* Start a DMA read.
 *  mid         IOMMU ID (0 = direct)
 *  sid         IOMMU region ID
 *  addr        target address
 *  size        size of read to perform
 *  buf         memory buffer to fill with data
 *  flags       bus access flags
 *  cpl         completion function; if NULL, call is blocking
 *  cplp        completion function parameter
 * Returns error flag.
 */
int coreio_dma_read(unsigned mid, unsigned sid, uint64_t addr, size_t len, void *buf, unsigned flags, void (*cpl)(void *, int), void *cplp);

/* Start a DMA write.
 *  mid         IOMMU ID (0 = direct)
 *  sid         IOMMU region ID
 *  addr        target address
 *  size        size of write to perform
 *  buf         memory buffer with data
 *  flags       bus access flags
 *  cpl         completion function; if NULL, call is blocking
 *  cplp        completion function parameter
 * Returns error flag.
 */
int coreio_dma_write(unsigned mid, unsigned sid, uint64_t addr, size_t len, void *buf, unsigned flags, void (*cpl)(void *, int), void *cplp);

/* Send an IRQ update.
 *  rid         MMIO range ID associated with IRQ
 *  iid         IRQ index
 *  state       IRQ line state (1 - active, 0 - inactive)
 */
int coreio_irq_update(unsigned rid, unsigned iid, unsigned state);

typedef struct {
    /* MMIO operations. */
    int (*read)(void *priv, uint64_t addr, size_t len, void *buf, unsigned flags);
    int (*write)(void *priv, uint64_t addr, size_t len, void *buf, unsigned flags);
    /* MMIO pair-wise operations. If not supplied, read/write are used twice. */
    int (*readp)(void *priv, uint64_t addr, size_t len, void *buf1, void *buf2, unsigned flags);
    int (*writep)(void *priv, uint64_t addr, size_t len, void *buf1, void *buf2, unsigned flags);
} coreio_func_t;

/* Register a MMIO handler.
 *  rid         MMIO range ID to register
 *  func        io handler function table
 *  funcp       io handler function parameter
 * Returns error flag.
 */
int coreio_register(unsigned rid, const coreio_func_t *func, void *funcp);

/* Prepare fd_sets for select(2).
 *  nfds        current index of maximum fd in sets + 1
 *  readfds     readfds to update
 *  writefds    writefds to update
 * Returns new nfds.
 */
int coreio_preparefds(int nfds, fd_set *readfds, fd_set *writefds);

/* Process fd_sets after select(2).
 *  readfds     readfds to process
 *  writefds    writefds to process
 * Returns error flag.
 */
int coreio_processfds(fd_set *readfds, fd_set *writefds);

/* Simple implementation of a main loop.
 *  usec        time to spend in loop, in microseconds; negative means forever
 * Returns error flag.
 */
int coreio_mainloop(long long usec);

/* Close connection to a VM. */
void coreio_disconnect(void);

#endif
