/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2025, halal-beef <umer.uddin@mentallysanemainliners.org>
 * Copyright (c) 2025, Ivaylo Ivanov <ivo.ivanov.ivanov1@gmail.com>
 */
#include <string.h>
#include <board.h>
#include <drivers/framework.h>
#include <lib/simplefb.h>

void init_board_funcs(void *board)
{
	/*
	 * Parsing the struct directly without restructing is
	 * broken as of Sep 29 2024
	 */
	struct {
		const char *name;
		int ops[BOARD_OP_EXIT];
	} *board_restruct = board;

	board_restruct->name = "WASP";
}

// Early initialization
int board_init(void)
{
	// Disengage watchdog
	writel(0x22000000, (void *)0x10007000);
	return 0;
}

// Late initialization
int board_late_init(void)
{
	return 0;
}

int board_driver_setup(void)
{
	struct {
		int width;
		int height;
		int stride;
		void *address;
	} simplefb_data = {
		.width = 736, // 720p + 16 pixel stride
		.height = 1520,
		.stride = 4,
		.address = (void *)0x07d860000
	};

	REGISTER_DRIVER("simplefb", simplefb_probe, &simplefb_data);
	return 0;
}
