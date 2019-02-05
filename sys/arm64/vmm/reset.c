/*
 * Copyright (C) 2018 Alexandru Elisei <alexandru.elisei@gmail.com>
 * All rights reserved.
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

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/lock.h>

#include <machine/armreg.h>

#include "arm64.h"
#include "reset.h"

void
reset_vm_el1_sysregs(void *vcpu)
{
	struct hypctx *el2ctx;

	el2ctx = vcpu;

	el2ctx->sctlr_el1 = SCTLR_RES1;
	el2ctx->sctlr_el1 &= ~SCTLR_M;
	/* Use the same memory attributes as the host */
	el2ctx->mair_el1 = READ_SPECIALREG(mair_el1);
	/* Don't trap accesses to SVE, Advanced SIMD and FP to EL1 */
	el2ctx->cpacr_el1 = CPACR_FPEN_TRAP_NONE;
}

void
reset_vm_el2_sysregs(void *vcpu)
{
	struct hypctx *el2ctx;

	el2ctx = vcpu;

	/*
	 * Set the Hypervisor Configuration Register:
	 *
	 * HCR_RW: use AArch64 for EL1
	 * HCR_BSU_IS: barrier instructions apply to the inner shareable
	 * domain
	 * HCR_SWIO: turn set/way invalidate into set/way clean and
	 * invalidate
	 * HCR_FB: broadcast maintenance operations
	 * HCR_AMO: route physical SError interrupts to EL2
	 * HCR_IMO: route physical IRQ interrupts to EL2
	 * HCR_FMO: route physical FIQ interrupts to EL2
	 * HCR_VM: use stage 2 translation
	 */
	el2ctx->hcr_el2 = HCR_RW | HCR_BSU_IS | HCR_SWIO | HCR_FB | \
			  HCR_VM | HCR_AMO | HCR_IMO | HCR_FMO;
	/* The guest will detect a single-core, single-threaded CPU */
	el2ctx->vmpidr_el2 = get_mpidr();
	el2ctx->vmpidr_el2 |= VMPIDR_EL2_U;
	el2ctx->vmpidr_el2 &= ~VMPIDR_EL2_MT;
	/* Use the same CPU identification information as the host */
	el2ctx->vpidr_el2 = READ_SPECIALREG(midr_el1);
	/*
	 * Don't trap accesses to CPACR_EL1, trace, SVE, Advanced SIMD
	 * and floating point functionality to EL2.
	 */
	el2ctx->cptr_el2 = CPTR_RES1;
	/*
	 * Disable interrupts in the guest. The guest OS will re-enable
	 * them.
	 */
	el2ctx->spsr_el2 = PSR_D | PSR_A | PSR_I | PSR_F;
	/* Use the EL1 stack when taking exceptions to EL1 */
	el2ctx->spsr_el2 |= PSR_M_EL1h;
}
