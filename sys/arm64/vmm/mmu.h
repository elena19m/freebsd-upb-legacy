/*
 * Copyright (C) 2017 Alexandru Elisei <alexandru.elisei@gmail.com>
 * All rights reserved.
 *
 * This software was developed by Alexandru Elisei under sponsorship
 * from the FreeBSD Foundation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _VMM_MMU_H_
#define	_VMM_MMU_H_

/*
 * The translation tables for the hypervisor mode will hold mappings for kernel
 * virtual addresses and an identity mapping (VA == PA) necessary when
 * enabling/disabling the MMU.
 *
 * When in EL2 exception level the translation table base register is TTBR0_EL2
 * and the virtual addresses generated by the CPU must be at the bottom of the
 * memory, with the first 16 bits all set to zero:
 *
 *                  0x0000ffffffffffff  End hyp address space
 *                  0x0000000000000000  Start of hyp address space
 *
 * To run code in hyp mode we need to convert kernel virtual addresses to
 * addreses that fit into this address space.
 *
 * The kernel virtual address range is:
 *
 *                  0xffff007fffffffff  End of KVA
 *                  0xffff000000000000  Kernel base address & start of KVA
 *
 * (see /sys/arm64/include/vmparam.h).
 *
 * We could convert the kernel virtual addresses to valid EL2 addresses by
 * setting the first 16 bits to zero and thus mapping the kernel addresses in
 * the bottom half of the EL2 address space, but then they might clash with the
 * identity mapping addresses. Instead we map the kernel addresses in the upper
 * half of the EL2 address space.
 *
 * The hypervisor address space will look like this:
 *
 *                  0x0000807fffffffff  End of KVA mapping
 *                  0x0000800000000000  Start of KVA mapping
 *
 *                  0x00007fffffffffff  End of identity mapping
 *                  0x0000000000000000  Start of identity mapping
 *
 * With the scheme we have 47 bits at our disposable for the identity map and
 * another 47 bits for the kernel virtual addresses. For a maximum physical
 * memory size of 128TB we are guaranteed to not have any clashes between
 * addresses.
 */
#define	HYP_VM_MIN_ADDRESS	0x0000000000000000
#define	HYP_VM_MAX_ADDRESS	0x0000ffffffffffff

#define	HYP_KVA_OFFSET		0x0000800000000000
#define	HYP_KVA_MASK		0x0000ffffffffffff
#define ktohyp(kva)		(((vm_offset_t)(kva) & HYP_KVA_MASK) | \
					HYP_KVA_OFFSET)

void 		hypmap_init(pmap_t map);
void 		hypmap_map(pmap_t map, vm_offset_t va, size_t len,
			vm_prot_t prot);
void 		hypmap_map_identity(pmap_t map, vm_offset_t va, size_t len,
			vm_prot_t prot);
void 		hypmap_set(pmap_t map, vm_offset_t va, vm_offset_t pa,
			size_t len, vm_prot_t prot);
vm_paddr_t 	hypmap_get(pmap_t map, vm_offset_t va);
void 		hypmap_cleanup(pmap_t map);

#endif
