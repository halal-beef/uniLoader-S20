/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2022, Ivaylo Ivanov <ivo.ivanov.ivanov1@gmail.com>
 */
.include "arch/aarch64/macros.h"

.section .text
ENTRY _start
	/* Preserve devices original FDT Address */
	mov x10, 0x95000000
	str x0, [x10]

	/* Set up the base address for the stack */
	adr x0, _stack_end

	/* Set up the stack pointer (SP) */
	mov sp, x0

	/* Fall through */
	adr	x0, dtb
	adr	x1, kernel
	b main

ENTRY load_kernel
	br	x4
ENDPROC load_kernel
