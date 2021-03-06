#include <linux/fb.h>
//#include <linux/earlysuspend.h>

//#ifdef CONFIG_ONE_FRAME_BUFFERS
#define NUM_FRAME_BUFFERS CONFIG_FB_INGENIC_NR_FRAMES
//#endif

//#ifdef CONFIG_TWO_FRAME_BUFFERS
//#define NUM_FRAME_BUFFERS 2
//#endif

//#ifdef CONFIG_THREE_FRAME_BUFFERS
//#define NUM_FRAME_BUFFERS 3
//#endif

#define PIXEL_ALIGN 4
#define MAX_DESC_NUM 4

/**
 * @next: physical address of next frame descriptor
 * @databuf: physical address of buffer
 * @id: frame ID
 * @cmd: DMA command and buffer length(in word)
 * @offsize: DMA off size, in word
 * @page_width: DMA page width, in word
 * @cpos: smart LCD mode is commands' number, other is bpp,
 * premulti and position of foreground 0, 1
 * @desc_size: alpha and size of foreground 0, 1
 */
struct ingenicfb_framedesc {
	uint32_t next;
	uint32_t databuf;
	uint32_t id;
	uint32_t cmd;
	uint32_t offsize;
	uint32_t page_width;
	uint32_t cpos;
	uint32_t desc_size;
};

struct ingenicfb_display_size {
	u32 fg0_line_size;
	u32 fg0_frm_size;
	u32 panel_line_size;
	u32 height_width;
};

enum ingenicfb_format_order {
	FORMAT_X8R8G8B8 = 1,
	FORMAT_X8B8G8R8,
};

/**
 * @fg: foreground 0 or foreground 1
 * @bpp: foreground bpp
 * @x: foreground start position x
 * @y: foreground start position y
 * @w: foreground width
 * @h: foreground height
 */
struct ingenicfb_fg_t {
	u32 fg;
	u32 bpp;
	u32 x;
	u32 y;
	u32 w;
	u32 h;
};

/**
 *@decompress: enable decompress function, used by FG0
 *@block: enable 16x16 block function
 *@fg0: fg0 info
 *@fg1: fg1 info
 */
struct ingenicfb_osd_t {
	int block;
	struct ingenicfb_fg_t fg0;
	struct ingenicfb_fg_t fg1;
};

struct ingenicfb {
	int fb_bpp;		/* bpp of frame buffer */
	int is_lcd_en;		/* 0, disable  1, enable */
	int is_clk_en;		/* 0, disable  1, enable */
	int is_pclk_en;		/* 0, disable  1, enable */
	int irq;		/* lcdc interrupt num */
	int open_cnt;
	int irq_cnt;
	int desc_num;
	char clk_name[16];
	char pclk_name[16];
	char pwcl_name[16];
	char irq_name[16];

	struct fb_info *fb;
	struct device *dev;
	struct ingenicfb_platform_data *pdata;
	void __iomem *base;
	struct resource *mem;

	size_t vidmem_size;
	void *vidmem;
	dma_addr_t vidmem_phys;
	void *desc_cmd_vidmem;
	dma_addr_t desc_cmd_phys;

	int frm_size;
	int current_buffer;
	/* dma 0 descriptor base address */
	struct ingenicfb_framedesc *framedesc[MAX_DESC_NUM];
	struct ingenicfb_framedesc *fg1_framedesc;	/* FG 1 dma descriptor */
	dma_addr_t framedesc_phys;

	struct completion vsync_wq;
	struct task_struct *vsync_thread;
	unsigned int vsync_skip_map;	/* 10 bits width */
	int vsync_skip_ratio;

	struct mutex lock;
	struct mutex suspend_lock;

	enum ingenicfb_format_order fmt_order;	/* frame buffer pixel format order */
	struct ingenicfb_osd_t osd;	/* osd's config information */

	struct clk *clk;
	struct clk *pclk;
	struct clk *pwcl;

	int is_suspend;
	unsigned int pan_display_count;

	char eventbuf[64];
	int is_vsync;
	int is_inited;

	/* long long timestamp_array[16]; */
	ktime_t timestamp_array[16];
	int timestamp_irq_pos;
	int timestamp_thread_pos;
	spinlock_t vsync_lock;
};

void ingenicfb_clk_enable(struct ingenicfb *ingenicfb);
void ingenicfb_clk_disable(struct ingenicfb *ingenicfb);
static inline unsigned long reg_read(struct ingenicfb *ingenicfb, int offset)
{
	return readl(ingenicfb->base + offset);
}

static inline void reg_write(struct ingenicfb *ingenicfb, int offset, unsigned long val)
{
	writel(val, ingenicfb->base + offset);
}

/* structures for frame buffer ioctl */
struct ingenicfb_fg_pos {
	__u32 fg;		/* 0:fg0, 1:fg1 */
	__u32 x;
	__u32 y;
};

struct ingenicfb_fg_size {
	__u32 fg;
	__u32 w;
	__u32 h;
};

struct ingenicfb_fg_alpha {
	__u32 fg;		/* 0:fg0, 1:fg1 */
	__u32 enable;
	__u32 mode;		/* 0:global alpha, 1:pixel alpha */
	__u32 value;		/* 0x00-0xFF */
};

struct ingenicfb_bg {
	__u32 fg;		/* 0:fg0, 1:fg1 */
	__u32 red;
	__u32 green;
	__u32 blue;
};

struct ingenicfb_color_key {
	__u32 fg;		/* 0:fg0, 1:fg1 */
	__u32 enable;
	__u32 mode;		/* 0:color key, 1:mask color key */
	__u32 red;
	__u32 green;
	__u32 blue;
};

struct ingenicfb_mode_res {
	__u32 index;		/* 1-64 */
	__u32 w;
	__u32 h;
};

/* ioctl commands base fb.h FBIO_XXX */
/* image_enh.h: 0x142 -- 0x162 */
#define INGENICFB_GET_MODENUM		_IOR('F', 0x100, int)
#define INGENICFB_GET_MODELIST		_IOR('F', 0x101, int)
#define INGENICFB_SET_VIDMEM			_IOW('F', 0x102, unsigned int *)
#define INGENICFB_SET_MODE			_IOW('F', 0x103, int)
#define INGENICFB_ENABLE			_IOW('F', 0x104, int)
#define INGENICFB_GET_RESOLUTION		_IOWR('F', 0x105, struct ingenicfb_mode_res)

/* Reserved for future extend */
#define INGENICFB_SET_FG_SIZE		_IOW('F', 0x116, struct ingenicfb_fg_size)
#define INGENICFB_GET_FG_SIZE		_IOWR('F', 0x117, struct ingenicfb_fg_size)
#define INGENICFB_SET_FG_POS			_IOW('F', 0x118, struct ingenicfb_fg_pos)
#define INGENICFB_GET_FG_POS			_IOWR('F', 0x119, struct ingenicfb_fg_pos)
#define INGENICFB_GET_BUFFER			_IOR('F', 0x120, int)
/* Reserved for future extend */
#define INGENICFB_SET_ALPHA			_IOW('F', 0x123, struct ingenicfb_fg_alpha)
#define INGENICFB_SET_BACKGROUND		_IOW('F', 0x124, struct ingenicfb_bg)
#define INGENICFB_SET_COLORKEY		_IOW('F', 0x125, struct ingenicfb_color_key)
#define INGENICFB_16X16_BLOCK_EN		_IOW('F', 0x127, int)
#define INGENICFB_ENABLE_LCDC_CLK		_IOW('F', 0x130, int)
/* Reserved for future extend */
#define INGENICFB_ENABLE_FG0			_IOW('F', 0x139, int)
#define INGENICFB_ENABLE_FG1			_IOW('F', 0x140, int)

/* Reserved for future extend */
#define INGENICFB_SET_VSYNCINT		_IOW('F', 0x210, int)
/* define in image_enh.c */
extern int ingenicfb_config_image_enh(struct fb_info *info);
extern int ingenicfb_image_enh_ioctl(struct fb_info *info, unsigned int cmd,
		unsigned long arg);
extern int update_slcd_frame_buffer(void);
extern int lcd_display_inited_by_uboot(void);
extern int ingenicfb_register_panel(struct ingenicfb_platform_data *panel);

#define FB_MODE_IS_VGA    (1 << 30)
enum ingenicfb_lcd_type {
	LCD_TYPE_GENERIC_16_BIT = 0,
	LCD_TYPE_GENERIC_18_BIT = 0 | (1 << 7),
	LCD_TYPE_GENERIC_24_BIT = 0 | (1 << 6),
	LCD_TYPE_SPECIAL_TFT_1 = 1,
	LCD_TYPE_SPECIAL_TFT_2 = 2,
	LCD_TYPE_SPECIAL_TFT_3 = 3,
	LCD_TYPE_NON_INTERLACED_TV = 4 | (1 << 26),
	LCD_TYPE_INTERLACED_TV = 6 | (1 << 26) | (1 << 30),
	LCD_TYPE_8BIT_SERIAL = 0xc,
	LCD_TYPE_SLCD = 0xd | (1 << 31),
};

/* smart lcd command width */
enum smart_lcd_cwidth {
	SMART_LCD_CWIDTH_16_BIT_ONCE = (0 << 8),
	SMART_LCD_CWIDTH_9_BIT_ONCE = SMART_LCD_CWIDTH_16_BIT_ONCE,
	SMART_LCD_CWIDTH_8_BIT_ONCE = (0x1 << 8),
	SMART_LCD_CWIDTH_18_BIT_ONCE = (0x2 << 8),
	SMART_LCD_CWIDTH_24_BIT_ONCE = (0x3 << 8),
};

/* smart lcd data width */
enum smart_lcd_dwidth {
	SMART_LCD_DWIDTH_18_BIT_ONCE_PARALLEL_SERIAL = (0 << 10),
	SMART_LCD_DWIDTH_16_BIT_ONCE_PARALLEL_SERIAL = (0x1 << 10),
	SMART_LCD_DWIDTH_8_BIT_THIRD_TIME_PARALLEL = (0x2 << 10),
	SMART_LCD_DWIDTH_8_BIT_TWICE_TIME_PARALLEL = (0x3 << 10),
	SMART_LCD_DWIDTH_8_BIT_ONCE_PARALLEL_SERIAL = (0x4 << 10),
	SMART_LCD_DWIDTH_24_BIT_ONCE_PARALLEL = (0x5 << 10),
	SMART_LCD_DWIDTH_9_BIT_TWICE_TIME_PARALLEL = (0x7 << 10),
	SMART_LCD_DWIDTH_MASK = (0x7 << 10)
};

/* smart lcd new data width */
enum smart_lcd_new_dwidth {
	SMART_LCD_NEW_DWIDTH_24_BIT = (4 << 13),
	SMART_LCD_NEW_DWIDTH_18_BIT = (3 << 13),
	SMART_LCD_NEW_DWIDTH_16_BIT = (2 << 13),
	SMART_LCD_NEW_DWIDTH_9_BIT = (1 << 13),
	SMART_LCD_NEW_DWIDTH_8_BIT = (0 << 13),
	SMART_LCD_NEW_DWIDTH_MASK = (0x7 << 13),
};

/* smart lcd data times */
enum smart_lcd_new_dtimes {
	SMART_LCD_NEW_DTIMES_ONCE = (0 << 8),
	SMART_LCD_NEW_DTIMES_TWICE = (1 << 8),
	SMART_LCD_NEW_DTIMES_THICE = (2 << 8),
	SMART_LCD_NEW_DTIMES_MASK = (3 << 8),
};

enum smart_config_type {
	SMART_CONFIG_CMD =  0,
	SMART_CONFIG_DATA =  1,
	SMART_CONFIG_UDELAY =  2,
};

struct smart_lcd_data_table {
	enum smart_config_type type;
	uint32_t value;
};

enum smart_lcd_type {
	SMART_LCD_TYPE_PARALLEL,
	SMART_LCD_TYPE_SERIAL,
};

struct ingenicfb_platform_data {
	size_t num_modes;
	struct fb_videomode *modes;
	struct ingenicdsi_data *dsi_pdata;

	enum ingenicfb_lcd_type lcd_type;
	unsigned int bpp;
	unsigned int width;
	unsigned int height;
	unsigned pinmd:1;

	unsigned pixclk_falling_edge:1;
	unsigned data_enable_active_low:1;

	struct {
		enum smart_lcd_type smart_type;
		unsigned clkply_active_rising:1;
		unsigned rsply_cmd_high:1;
		unsigned csply_active_high:1;

		unsigned newcfg_6800_md:1;
		unsigned newcfg_fmt_conv:1;
		unsigned datatx_type_serial:1;
		unsigned cmdtx_type_serial:1;
		unsigned newcfg_cmd_9bit:1;

		size_t length_cmd;
		unsigned long *write_gram_cmd;
		unsigned int bus_width;
		unsigned int data_table_width;
		/*
		 *
		 * data_table_width:
		 * The width of the (struct)smart_lcd_data_table.value,
		 * which provided by panel manufacturers.
		 *	e.g.
		 * truly_tft240240_2_e is 8bit,
		 * kfm701a21_1a is 16bit.
		 *
		 * */
		size_t length_data_table;
		struct smart_lcd_data_table *data_table;
		int (*init) (void);
		int (*gpio_for_slcd) (void);
		int te_gpio;
		int te_irq_level;

	} smart_config;

	unsigned dither_enable:1;
	struct {
		unsigned dither_red;
		unsigned dither_green;
		unsigned dither_blue;
	} dither;

	struct {
		uint32_t spl;
		uint32_t cls;
		uint32_t ps;
		uint32_t rev;
	} special_tft_config;

	struct {
		int (*lcd_initialize_begin)(void*);
		int (*lcd_initialize_end)(void*);

		int (*lcd_power_on_begin)(void*);
		int (*lcd_power_on_end)(void*);

		int (*lcd_power_off_begin)(void*);
		int (*lcd_power_off_end)(void*);

		int (*dma_transfer_begin)(void*);
		int (*dma_transfer_end)(void*);
	} lcd_callback_ops;

};
