/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2024, halal-beef <78730004+halal-beef@users.noreply.github.com>
 */

#include <soc/mt6765.h>

void soc_init(void) {
	*(volatile unsigned int*)WDT_BASE = 0x1281;
}
