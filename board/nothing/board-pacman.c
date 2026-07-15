/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2026, Umer Uddin <umer.uddin@mentallysanemainliners.org>
 */
#include <board.h>
#include <util.h>
#include <drivers/framework.h>
#include <lib/simplefb.h>
#include <string.h>

#define WDT_BASE		0x1c007000
#define WDT_MODE_KEY		0x22000000
#define WDT_MODE_EN		(1 << 0)

/* ============================================================
 * Platform Abstraction - Implement these for your environment
 * ============================================================ */
#define MHz(x)	(x * 1000000)

#define CNTFRQ_EL0 MHz(26)

void udelay(uint32_t us)
{
	uint64_t start, now;

	__asm__ volatile("mrs %0, cntpct_el0" : "=r" (start));

	uint64_t ticks = (CNTFRQ_EL0 / 1000000) * us;

	do {
		__asm__ volatile("mrs %0, cntpct_el0" : "=r" (now));
	} while ((now - start) < ticks);
}/* ============================================================
 * Base Addresses
 * ============================================================ */
#define MMSYS_BASE        0x14000000
#define MUTEX_BASE        0x14001000
#define OVL0_2L_BASE      0x14003000
#define RDMA0_BASE        0x14006000
#define DSI0_BASE         0x14017000
/* ============================================================
 * Helper Macros
 * ============================================================ */
#define BIT(n)            (1U << (n))
#define REG_FLD(msb, lsb, val) (((val) & ((1U << ((msb) - (lsb) + 1)) - 1)) << (lsb))
#define mmio_setbits(addr, bits)  writel(readl(addr) | (bits), addr)
#define mmio_clrbits(addr, bits)  writel(readl(addr) & ~(bits), addr)
/* ============================================================
 * MMSYS Clock Gate Registers
 * ============================================================ */
#define MM0_CG_SET        (void *)(MMSYS_BASE + 0x104)
#define MM0_CG_CLR        (void *)(MMSYS_BASE + 0x108)
#define MM0_CG_STA        (void *)(MMSYS_BASE + 0x100)
/* Clock bits */
#define CLK_MM_DISP_MUTEX0    BIT(0)
#define CLK_MM_APB_BUS        BIT(1)
#define CLK_MM_DISP_OVL0      BIT(2)
#define CLK_MM_DISP_RDMA0     BIT(3)
#define CLK_MM_DISP_OVL0_2L   BIT(4)
#define CLK_MM_DISP_WDMA0     BIT(5)
#define CLK_MM_DISP_RSZ0      BIT(7)
#define CLK_MM_DISP_COLOR0    BIT(10)
#define CLK_MM_SMI_INFRA      BIT(11)
#define CLK_MM_DISP_GAMMA0    BIT(13)
#define CLK_MM_DISP_SPR0      BIT(15)
#define CLK_MM_DISP_DITHER0   BIT(16)
#define CLK_MM_SMI_COMMON     BIT(17)
#define CLK_MM_DISP_CM0       BIT(18)
#define CLK_MM_DSI0           BIT(19)
#define CLK_MM_DISP_DSC_WRAP  BIT(23)
#define CLK_MM_DISP_OVL1_2L   BIT(25)
/* All display clocks mask */
#define DISP_CLK_MASK ( \
    CLK_MM_DISP_MUTEX0 | CLK_MM_APB_BUS | CLK_MM_DISP_OVL0 | \
    CLK_MM_DISP_RDMA0 | CLK_MM_DISP_OVL0_2L | CLK_MM_DISP_WDMA0 | \
    CLK_MM_DISP_RSZ0 | CLK_MM_DISP_COLOR0 | CLK_MM_SMI_INFRA | \
    CLK_MM_DISP_GAMMA0 | CLK_MM_DISP_SPR0 | CLK_MM_DISP_DITHER0 | \
    CLK_MM_SMI_COMMON | CLK_MM_DISP_CM0 | CLK_MM_DSI0 | \
    CLK_MM_DISP_DSC_WRAP | CLK_MM_DISP_OVL1_2L \
)
/* ============================================================
 * MMSYS Routing Registers
 * ============================================================ */
#define MMSYS_OVL_CON                 (void *)(MMSYS_BASE + 0xF04)
#define MMSYS_RDMA0_RSZ0_SOUT_SEL     (void *)(MMSYS_BASE + 0xF0C)
#define MMSYS_OVL0_2L_MOUT_EN         (void *)(MMSYS_BASE + 0xF14)
#define MMSYS_OVL0_MOUT_EN            (void *)(MMSYS_BASE + 0xF18)
#define MMSYS_RSZ0_MOUT_EN            (void *)(MMSYS_BASE + 0xF1C)
#define MMSYS_DITHER0_MOUT_EN         (void *)(MMSYS_BASE + 0xF20)
#define MMSYS_RDMA0_SEL_IN            (void *)(MMSYS_BASE + 0xF28)
#define MMSYS_DSI0_SEL_IN             (void *)(MMSYS_BASE + 0xF30)
/* Routing values */
#define RDMA0_SOUT_TO_DSI0            0
#define RDMA0_SOUT_TO_COLOR0          1
#define OVL0_2L_MOUT_TO_RDMA0         BIT(0)
#define DSI0_SEL_FROM_RDMA0           0
#define RDMA0_SEL_FROM_OVL0_2L        2
/* ============================================================
 * Mutex Registers
 * ============================================================ */
#define DISP_MUTEX_EN(n)              (void *)(MUTEX_BASE + (0x20 + 0x20 * (n)))
#define DISP_MUTEX_MOD0               (void *)(MUTEX_BASE + 0x30)
#define DISP_MUTEX_SOF                (void *)(MUTEX_BASE + 0x2C)
#define DISP_MUTEX_RST(n)             (void *)(MUTEX_BASE + (0x28 + 0x20 * (n)))
/* Mutex module bits */
#define MUTEX_MOD_OVL0_2L             BIT(1)
#define MUTEX_MOD_RDMA0               BIT(2)
#define MUTEX_MOD_RSZ0                BIT(3)
#define MUTEX_MOD_DSI0                BIT(14)
/* Mutex SOF values */
#define MUTEX_SOF_SINGLE_MODE         0
#define MUTEX_SOF_DSI0                1
/* ============================================================
 * OVL0_2L Registers
 * ============================================================ */
#define OVL_EN                        (void *)(OVL0_2L_BASE + 0x000C)
#define OVL_TRIG                      (void *)(OVL0_2L_BASE + 0x0010)
#define OVL_RST                       (void *)(OVL0_2L_BASE + 0x0014)
#define OVL_ROI_SIZE                  (void *)(OVL0_2L_BASE + 0x0020)
#define OVL_DATAPATH_CON              (void *)(OVL0_2L_BASE + 0x0024)
#define OVL_ROI_BGCLR                 (void *)(OVL0_2L_BASE + 0x0028)
#define OVL_SRC_CON                   (void *)(OVL0_2L_BASE + 0x002C)
#define OVL_CON(n)                    (void *)(OVL0_2L_BASE + 0x0030 + 0x20 * (n))
#define OVL_SRC_SIZE(n)               (void *)(OVL0_2L_BASE + 0x0038 + 0x20 * (n))
#define OVL_OFFSET(n)                 (void *)(OVL0_2L_BASE + 0x003C + 0x20 * (n))
#define OVL_PITCH(n)                  (void *)(OVL0_2L_BASE + 0x0044 + 0x20 * (n))
#define OVL_ADDR(n)                   (void *)(OVL0_2L_BASE + 0x0048 + 0x20 * (n))
/* OVL layer format */
#define OVL_CFMT_RGB565               0x0
#define OVL_CFMT_RGB888               0x2
#define OVL_CFMT_RGBA8888             0x4
#define OVL_CFMT_ARGB8888             0x5
/* ============================================================
 * RDMA0 Registers
 * ============================================================ */
#define RDMA_INT_ENABLE               (void *)(RDMA0_BASE + 0x0000)
#define RDMA_INT_STATUS               (void *)(RDMA0_BASE + 0x0004)
#define RDMA_GLOBAL_CON               (void *)(RDMA0_BASE + 0x0010)
#define RDMA_SIZE_CON_0               (void *)(RDMA0_BASE + 0x0014)
#define RDMA_SIZE_CON_1               (void *)(RDMA0_BASE + 0x0018)
#define RDMA_TARGET_LINE              (void *)(RDMA0_BASE + 0x001C)
#define RDMA_MEM_CON                  (void *)(RDMA0_BASE + 0x0024)
#define RDMA_MEM_SRC_PITCH            (void *)(RDMA0_BASE + 0x002C)
#define RDMA_FIFO_CON                 (void *)(RDMA0_BASE + 0x0040)
#define RDMA_MEM_START_ADDR           (void *)(RDMA0_BASE + 0x0F00)
#define RDMA_MEM_START_ADDR_MSB       (void *)(RDMA0_BASE + 0x0F10)
/* RDMA control bits */
#define RDMA_ENGINE_EN                BIT(0)
#define RDMA_MODE_MEMORY              BIT(1)
#define RDMA_SOFT_RESET               BIT(4)
/* RDMA format */
#define RDMA_FMT_RGB565               0x0
#define RDMA_FMT_RGB888               0x1
#define RDMA_FMT_RGBA8888             0x2
#define RDMA_FMT_ARGB8888             0x3
/* ============================================================
 * DSI0 Registers
 * ============================================================ */
#define DSI_START                     (void *)(DSI0_BASE + 0x00)
#define DSI_INTEN                     (void *)(DSI0_BASE + 0x08)
#define DSI_INTSTA                    (void *)(DSI0_BASE + 0x0C)
#define DSI_CON_CTRL                  (void *)(DSI0_BASE + 0x30)
#define DSI_MODE_CTRL                 (void *)(DSI0_BASE + 0x34)
#define DSI_TXRX_CTRL                 (void *)(DSI0_BASE + 0x38)
#define DSI_PSCTRL                    (void *)(DSI0_BASE + 0x3C)
#define DSI_SIZE_CON                  (void *)(DSI0_BASE + 0x38)
#define DSI_VACT_NL                   (void *)(DSI0_BASE + 0x6C)
#define DSI_CMDQ0                     (void *)(DSI0_BASE + 0x200)
#define DSI_CMDQ1                     (void *)(DSI0_BASE + 0x204)
#define DSI_MEM_CONTI                 (void *)(DSI0_BASE + 0x90)
#define DSI_PHY_TIMECON0              (void *)(DSI0_BASE + 0x110)
#define DSI_PHY_TIMECON1              (void *)(DSI0_BASE + 0x114)
#define DSI_PHY_TIMECON2              (void *)(DSI0_BASE + 0x118)
#define DSI_PHY_TIMECON3              (void *)(DSI0_BASE + 0x11C)
#define DSI_PHY_LCCON                 (void *)(DSI0_BASE + 0x1D0)
#define DSI_PHY_LD0CON                (void *)(DSI0_BASE + 0x1D4)
/* DSI control bits */
#define DSI_RESET                     BIT(0)
#define DSI_EN                        BIT(1)
#define DSI_PHY_RESET                 BIT(2)
#define DSI_DUAL_EN                   BIT(4)
/* DSI mode */
#define DSI_CMD_MODE                  0
#define DSI_SYNC_PULSE_MODE           1
#define DSI_SYNC_EVENT_MODE           2
#define DSI_BURST_MODE                3
/* DSI interrupts */
#define DSI_LPRX_RD_RDY_INT_FLAG      BIT(0)
#define DSI_CMD_DONE_INT_FLAG         BIT(1)
#define DSI_TE_RDY_INT_FLAG           BIT(2)
#define DSI_FRAME_DONE_INT_FLAG       BIT(4)
#define DSI_BUSY                      BIT(31)
/* DSI PS_SEL */
#define DSI_PS_RGB565                 0x1
#define DSI_PS_RGB666_LOOSE           0x2
#define DSI_PS_RGB666                 0x3
#define DSI_PS_RGB888                 0x4
/* DSI memory continuous */
#define DSI_WMEM_CONTI                0x3C
/* ============================================================
 * Display Configuration Structure
 * ============================================================ */
struct disp_config {
    uint32_t width;
    uint32_t height;
    uint32_t bpp;           /* Bits per pixel: 16, 24, or 32 */
    uint32_t fb_addr;       /* Physical framebuffer address */
    uint32_t fb_addr_high;  /* High 32 bits for >4GB addresses */
    uint32_t lane_count;    /* DSI lanes: 1, 2, 3, or 4 */
    uint32_t hs_trail;      /* PHY timing parameters (in UI) */
    uint32_t lpx;
};
/* ============================================================
 * Global State
 * ============================================================ */
static struct disp_config cfg;
/* ============================================================
 * Internal Helper Functions
 * ============================================================ */
static uint32_t get_ovl_cfmt(uint32_t bpp)
{
    switch (bpp) {
    case 16: return OVL_CFMT_RGB565;
    case 24: return OVL_CFMT_RGB888;
    case 32: return OVL_CFMT_RGBA8888;
    default: return OVL_CFMT_RGB565;
    }
}
static uint32_t get_rdma_fmt(uint32_t bpp)
{
    switch (bpp) {
    case 16: return RDMA_FMT_RGB565;
    case 24: return RDMA_FMT_RGB888;
    case 32: return RDMA_FMT_RGBA8888;
    default: return RDMA_FMT_RGB565;
    }
}
static uint32_t get_dsi_ps(uint32_t bpp)
{
    switch (bpp) {
    case 16: return DSI_PS_RGB565;
    case 24: return DSI_PS_RGB666;
    case 32: return DSI_PS_RGB888;
    default: return DSI_PS_RGB565;
    }
}
static uint32_t get_pitch_bytes(void)
{
    return (cfg.width * cfg.bpp) / 8;
}
/* ============================================================
 * Step 1: Reset Display Subsystem
 * ============================================================ */
static void disp_reset_subsystem(void)
{
    /* Disable all display clocks first */
    writel(DISP_CLK_MASK, MM0_CG_CLR);
    udelay(10);
    /* Reset mutex */
    writel(0, DISP_MUTEX_EN(0));
    udelay(10);
    /* Reset OVL */
    writel(0, OVL_EN);
    writel(1, OVL_RST);
    udelay(10);
    writel(0, OVL_RST);
    /* Reset RDMA */
    mmio_clrbits(RDMA_GLOBAL_CON, RDMA_ENGINE_EN);
    mmio_setbits(RDMA_GLOBAL_CON, RDMA_SOFT_RESET);
    udelay(10);
    mmio_clrbits(RDMA_GLOBAL_CON, RDMA_SOFT_RESET);
    /* Reset DSI */
    mmio_clrbits(DSI_CON_CTRL, DSI_EN);
    mmio_setbits(DSI_CON_CTRL, DSI_RESET);
    udelay(10);
    mmio_clrbits(DSI_CON_CTRL, DSI_RESET);
}
/* ============================================================
 * Step 2: Enable Clocks
 * ============================================================ */
static void disp_enable_clocks(void)
{
    /* Enable required display clocks */
    writel(DISP_CLK_MASK, MM0_CG_SET);
    udelay(10);
}
/* ============================================================
 * Step 3: Configure MMSYS Routing
 * ============================================================ */
static void disp_configure_routing(void)
{
    /* OVL0_2L -> RDMA0 */
    writel(OVL0_2L_MOUT_TO_RDMA0, MMSYS_OVL0_2L_MOUT_EN);
    /* RDMA0 -> DSI0 (bypass color processing for minimal path) */
    writel(RDMA0_SOUT_TO_DSI0, MMSYS_RDMA0_RSZ0_SOUT_SEL);
    /* DSI0 input from RDMA0 */
    writel(DSI0_SEL_FROM_RDMA0, MMSYS_DSI0_SEL_IN);
}
/* ============================================================
 * Step 4: Configure Mutex
 * ============================================================ */
static void disp_configure_mutex(void)
{
    /* Disable mutex before configuration */
    writel(0, DISP_MUTEX_EN(0));
    udelay(10);
    /* Set modules: OVL0_2L + RDMA0 + DSI0 */
    writel(MUTEX_MOD_OVL0_2L | MUTEX_MOD_RDMA0 | MUTEX_MOD_DSI0,
           DISP_MUTEX_MOD0);
    /* Set SOF to DSI0 for command mode triggering */
    writel(MUTEX_SOF_DSI0, DISP_MUTEX_SOF);
    /* Enable mutex */
    writel(1, DISP_MUTEX_EN(0));
}
/* ============================================================
 * Step 5: Configure OVL0_2L
 * ============================================================ */
static void disp_configure_ovl(void)
{
    uint32_t cfmt = get_ovl_cfmt(cfg.bpp);
    uint32_t pitch = get_pitch_bytes();
    /* Reset OVL */
    writel(1, OVL_RST);
    udelay(10);
    writel(0, OVL_RST);
    /* Set ROI size */
    writel((cfg.width << 16) | cfg.height, OVL_ROI_SIZE);
    /* Clear background to black */
    writel(0x00000000, OVL_ROI_BGCLR);
    /* Configure layer 0 */
    writel(
        REG_FLD(15, 12, cfmt) |  /* Color format */
        BIT(8) |                  /* Alpha enable */
        BIT(23),                  /* Manual color format */
        OVL_CON(0)
    );
    /* Layer source size */
    writel((cfg.width << 16) | cfg.height, OVL_SRC_SIZE(0));
    /* Layer offset */
    writel(0, OVL_OFFSET(0));
    /* Layer pitch */
    writel(pitch, OVL_PITCH(0));
    /* Layer framebuffer address */
    writel(cfg.fb_addr, OVL_ADDR(0));
    /* Enable OVL engine */
    writel(1, OVL_EN);
    /* Trigger OVL */
    writel(1, OVL_TRIG);
}
/* ============================================================
 * Step 6: Configure RDMA0
 * ============================================================ */
static void disp_configure_rdma(void)
{
    uint32_t fmt = get_rdma_fmt(cfg.bpp);
    uint32_t pitch = get_pitch_bytes();
    /* Soft reset RDMA */
    mmio_clrbits(RDMA_GLOBAL_CON, RDMA_ENGINE_EN);
    mmio_setbits(RDMA_GLOBAL_CON, RDMA_SOFT_RESET);
    udelay(10);
    mmio_clrbits(RDMA_GLOBAL_CON, RDMA_SOFT_RESET);
    /* Set size */
    writel(cfg.width, RDMA_SIZE_CON_0);
    writel(cfg.height, RDMA_SIZE_CON_1);
    /* Set target line (half of height for safety) */
    writel(cfg.height / 2, RDMA_TARGET_LINE);
    /* Configure memory mode and format */
    writel(
        RDMA_MODE_MEMORY |
        REG_FLD(7, 4, fmt),
        RDMA_GLOBAL_CON
    );
    /* Set framebuffer address */
    writel(cfg.fb_addr, RDMA_MEM_START_ADDR);
    writel(cfg.fb_addr_high, RDMA_MEM_START_ADDR_MSB);
    /* Set pitch */
    writel(pitch, RDMA_MEM_SRC_PITCH);
    /* Configure FIFO */
    writel((512 << 16) | 0, RDMA_FIFO_CON);
    /* Enable RDMA engine */
    mmio_setbits(RDMA_GLOBAL_CON, RDMA_ENGINE_EN);
}
/* ============================================================
 * Step 7: Configure DSI0 for Command Mode
 * ============================================================ */
static void disp_configure_dsi(void)
{
    uint32_t ps = get_dsi_ps(cfg.bpp);
    uint32_t wc = (cfg.width * cfg.bpp) / 8;
    /* Reset DSI */
    mmio_setbits(DSI_CON_CTRL, DSI_RESET);
    udelay(10);
    mmio_clrbits(DSI_CON_CTRL, DSI_RESET);
    /* Reset PHY */
    mmio_clrbits(DSI_CON_CTRL, DSI_PHY_RESET);
    udelay(10);
    mmio_setbits(DSI_CON_CTRL, DSI_PHY_RESET);
    udelay(10);
    mmio_clrbits(DSI_CON_CTRL, DSI_PHY_RESET);
    /* Configure PHY timing (conservative values) */
    writel(0x08080808, DSI_PHY_TIMECON0);  /* LPX, HS_PREP, HS_ZERO, HS_TRAIL */
    writel(0x20181420, DSI_PHY_TIMECON1);  /* TA_GO, TA_SURE, TA_GET, DA_HS_EXIT */
    writel(0x08000800, DSI_PHY_TIMECON2);  /* CONT_DET, CLK_ZERO */
    writel(0x14080808, DSI_PHY_TIMECON3);  /* CLK_HS_PREP, CLK_HS_POST, CLK_HS_EXIT */
    /* Enable PHY lanes */
    writel(BIT(0), DSI_PHY_LCCON);   /* LC_HS_TX_EN */
    writel(BIT(0), DSI_PHY_LD0CON);  /* LD0_HS_TX_EN */
    /* Set lane count */
    writel(cfg.lane_count << 2, DSI_TXRX_CTRL);
    /* Set command mode */
    writel(DSI_CMD_MODE, DSI_MODE_CTRL);
    /* Configure pixel stream */
    writel(
        REG_FLD(14, 0, wc) |
        REG_FLD(19, 16, ps),
        DSI_PSCTRL
    );
    /* Set vertical active lines */
    writel(cfg.height, DSI_VACT_NL);
    /* Enable memory continuous for command mode */
    writel(DSI_WMEM_CONTI, DSI_MEM_CONTI);
    /* Enable TE interrupt for command mode */
    writel(DSI_TE_RDY_INT_FLAG | DSI_FRAME_DONE_INT_FLAG, DSI_INTEN);
    /* Enable DSI engine */
    mmio_setbits(DSI_CON_CTRL, DSI_EN);
}
/* ============================================================
 * Step 8: Send DCS Sleep Out Command
 * ============================================================ */
static void disp_send_sleep_out(void)
{
    /* DCS Sleep Out (0x11) - short packet */
    writel(
        REG_FLD(7, 0, 0x00) |    /* CONFIG: short packet */
        REG_FLD(15, 8, 0x05) |   /* Data_ID: DCS short write */
        REG_FLD(23, 16, 0x11),   /* Data0: Sleep Out command */
        DSI_CMDQ0
    );
    /* Start transmission */
    writel(1, DSI_START);
    /* Wait for command done */
    while (readl(DSI_INTSTA) & DSI_BUSY)
        ;
    /* Clear interrupt */
    writel(DSI_CMD_DONE_INT_FLAG, DSI_INTSTA);
    /* Wait for panel to wake up (typically 120ms) */
    udelay(120000);
}
/* ============================================================
 * Step 9: Send DCS Display On Command
 * ============================================================ */
static void disp_send_display_on(void)
{
    /* DCS Display On (0x29) - short packet */
    writel(
        REG_FLD(7, 0, 0x00) |    /* CONFIG: short packet */
        REG_FLD(15, 8, 0x05) |   /* Data_ID: DCS short write */
        REG_FLD(23, 16, 0x29),   /* Data0: Display On command */
        DSI_CMDQ0
    );
    /* Start transmission */
    writel(1, DSI_START);
    /* Wait for command done */
    while (readl(DSI_INTSTA) & DSI_BUSY)
        ;
    /* Clear interrupt */
    writel(DSI_CMD_DONE_INT_FLAG, DSI_INTSTA);
}
/* ============================================================
 * Step 10: Trigger Display
 * ============================================================ */
static void disp_trigger_frame(void)
{
    /* Trigger DSI for command mode frame update */
    writel(1, DSI_START);
}
/* ============================================================
 * Fill Framebuffer with Test Pattern
 * ============================================================ */
static void disp_fill_test_pattern(void)
{
    uint32_t *fb = (uint32_t *)(uintptr_t)cfg.fb_addr;
    uint32_t pixels = cfg.width * cfg.height;
    uint32_t i;
    /* Simple color gradient test pattern */
    for (i = 0; i < pixels; i++) {
        uint32_t x = i % cfg.width;
        uint32_t y = i / cfg.width;
        if (cfg.bpp == 32) {
            fb[i] = (x << 16) | (y << 8) | 0xFF;  /* ARGB */
        } else if (cfg.bpp == 24) {
            uint8_t *p = (uint8_t *)&fb[i];
            p[0] = x & 0xFF;
            p[1] = y & 0xFF;
            p[2] = 0xFF;
        } else {
            /* RGB565 */
            uint16_t r = (x >> 3) & 0x1F;
            uint16_t g = (y >> 2) & 0x3F;
            uint16_t b = (x >> 3) & 0x1F;
            ((uint16_t *)fb)[i] = (r << 11) | (g << 5) | b;
        }
    }
}
/* ============================================================
 * Public API
 * ============================================================ */
/**
 * disp_init - Initialize display subsystem and bring up framebuffer
 * @config: Display configuration structure
 *
 * Returns 0 on success, negative error code on failure.
 *
 * This function performs a complete reset and initialization of the
 * MT6877 display subsystem for command mode panel operation.
 */
int disp_init(struct disp_config *config)
{
    if (!config || !config->fb_addr)
        return -1;
    /* Save configuration */
    cfg = *config;
    /* Validate configuration */
    if (cfg.width == 0 || cfg.height == 0)
        return -2;
    if (cfg.bpp != 16 && cfg.bpp != 24 && cfg.bpp != 32)
        return -3;
    if (cfg.lane_count < 1 || cfg.lane_count > 4)
        return -4;
    /* Execute initialization sequence */
    disp_reset_subsystem();
    disp_enable_clocks();
    disp_configure_routing();
    disp_configure_mutex();
    disp_configure_ovl();
    disp_configure_rdma();
    disp_configure_dsi();
    /* Send panel initialization commands */
    disp_send_sleep_out();
    disp_send_display_on();
    /* Trigger initial frame */
    disp_trigger_frame();
    return 0;
}
/**
 * disp_update - Update framebuffer and trigger new frame
 *
 * Call this function after modifying the framebuffer contents
 * to trigger a new frame update in command mode.
 */
void disp_update(void)
{
    disp_trigger_frame();
}
/**
 * disp_fill_pattern - Fill framebuffer with test pattern
 *
 * Utility function for testing. Fills the framebuffer with
 * a simple color gradient pattern.
 */
void disp_fill_pattern(void)
{
    disp_fill_test_pattern();
    disp_update();
}

void pacman_disable_wdt(void)
{
	uint32_t reg;

	reg = readl((void*)WDT_BASE);
	if (reg & WDT_MODE_EN)
	{
		reg &= ~WDT_MODE_EN;
		reg |= WDT_MODE_KEY;
		writel(reg, (void*)WDT_BASE);
	}

	return;
}

int pacman_late_init(void)
{
    struct disp_config cfg2 = {
        .width = 1080,
        .height = 2412,
        .bpp = 32,
        .fb_addr = 0xfe06c000,
        .fb_addr_high = 0,
        .lane_count = 4,
        .hs_trail = 0,
        .lpx = 0,
    };
	pacman_disable_wdt();
	disp_init(&cfg2);
	disp_fill_pattern();
	return 0;
}

static struct video_info pacman_fb = {
        .format = FB_FORMAT_ARGB8888,
        .width = 1080,
        .height = 2412,
        .stride = 4,
        .address = (void *)0xfe06c000
};

static const struct device pacman_devices[] = {
        { "simplefb", &pacman_fb, "fb" },
};

struct board_data board_ops = {
	.name = "nothing-phone-2a",
	.ops = {
		.late_init = pacman_late_init,
	},
        .devices = pacman_devices,
        .num_devices = ARRAY_SIZE(pacman_devices),
	.quirks = 0
};
