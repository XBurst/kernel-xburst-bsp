/*
 *
 * Copyright (c) 2012 Ingenic Semiconductor Co., Ltd.
 *              http://www.ingenic.com/
 *
 * Author: Gao Wei <wei.gao@ingenic.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef JPEG_CORE_H_
#define JPEG_CORE_H_

/*#include <linux/wakelock.h>*/
#include <linux/dma-mapping.h>
#include <media/v4l2-device.h>
#include <media/v4l2-fh.h>
#include <media/v4l2-ctrls.h>

#define INGENIC_JPEG_M2M_NAME		"jz-vpu"

/* a selection of JPEG markers */
#define TEM				0x01
#define SOF0				0xc0
#define RST				0xd0
#define SOI				0xd8
#define EOI				0xd9
#define DHP				0xde

/* Flags that indicate a format can be used for capture/output */
#define MEM2MEM_CAPTURE			(1 << 0)
#define MEM2MEM_OUTPUT			(1 << 1)

/**
 * struct ingenic_jpeg - JPEG IP abstraction
 * @lock:		the mutex protecting this structure
 * @slock:		spinlock protecting the device contexts
 * @v4l2_dev:		v4l2 device for mem2mem mode
 * @vfd_encoder:	video device node for encoder mem2mem mode
 * @vfd_decoder:	video device node for decoder mem2mem mode
 * @m2m_dev:		v4l2 mem2mem device data
 * @regs:		JPEG IP registers mapping
 * @irq:		JPEG IP irq
 * @clk:		JPEG IP clock
 * @dev:		JPEG IP struct device
 * @alloc_ctx:		videobuf2 memory allocator's context
 */
struct ingenic_jpeg {
	struct mutex		lock;
	/*struct wake_lock	wake_lock;*/
	spinlock_t		slock;

	struct v4l2_device	v4l2_dev;
	struct video_device	*vfd_encoder;
	struct v4l2_m2m_dev	*m2m_dev;

	void __iomem		*regs;
	unsigned int		irq;
	struct clk		*clk;
	struct device		*dev;
	void			*alloc_ctx;
};

/**
 * struct jpeg_fmt - driver's internal color format data
 * @name:	format descritpion
 * @fourcc:	the fourcc code, 0 if not applicable
 * @depth:	number of bits per pixel
 * @colplanes:	number of color planes (1 for packed formats)
 * @h_align:	horizontal alignment order (align to 2^h_align)
 * @v_align:	vertical alignment order (align to 2^v_align)
 * @types:	types of queue this format is applicable to
 */
struct ingenic_jpeg_fmt {
	char	*name;
	u32	fourcc;
	int	depth;
	int	colplanes;
	int	h_align;
	int	v_align;
	u32	types;
};

/**
 * ingenic_jpeg_q_data - parameters of one queue
 * @fmt:	driver-specific format of this queue
 * @w:		image width
 * @h:		image height
 * @size:	image buffer size in bytes
 */
struct ingenic_jpeg_q_data {
	struct ingenic_jpeg_fmt	*fmt;
	u32			w;
	u32			h;
	u32			size;
};

/**
 * ingenic_jpeg_ctx - the device context data
 * @jpeg:		JPEG IP device for this context
 * @mode:		compression (encode) operation or decompression (decode)
 * @compr_quality:	destination image quality in compression (encode) mode
 * @m2m_ctx:		mem2mem device context
 * @out_q:		source (output) queue information
 * @cap_fmt:		destination (capture) queue queue information
 * @hdr_parsed:		set if header has been parsed during decompression
 * @ctrl_handler:	controls handler
 */
struct ingenic_jpeg_ctx {
	struct ingenic_jpeg	*jpeg;
	unsigned int		mode;
	unsigned short		compr_quality;
	unsigned short		restart_interval;
	unsigned short		subsampling;
	struct v4l2_m2m_ctx	*m2m_ctx;
	struct ingenic_jpeg_q_data	out_q;
	struct ingenic_jpeg_q_data	cap_q;
	struct v4l2_fh		fh;
	bool			hdr_parsed;
	struct v4l2_ctrl_handler ctrl_handler;

	void *desc_vidmem;
	dma_addr_t desc_phys;
};

/**
 * ingenic_jpeg_buffer - description of memory containing input JPEG data
 * @size:	buffer size
 * @curr:	current position in the buffer
 * @data:	pointer to the data
 */
struct ingenic_jpeg_buffer {
	unsigned long size;
	unsigned long curr;
	unsigned long data;
};

/* JPEG encode quantization table select level */
typedef enum {
	LOW_QUALITY,
	MEDIUMS_QUALITY,
	HIGH_QUALITY,
} QUANT_QUALITY;

typedef struct _JPEGE_SliceInfo {
	unsigned int des_va;	/* descriptor virtual address */
	unsigned int des_pa;	/* descriptor physical address */
	uint8_t ncol;		/* number of color/components of a MCU minus one */
	uint8_t rsm;		/* Re-sync-marker enable */
	uint8_t *bsa;		/* bitstream buffer address  */
	uint8_t nrsm;		/* Re-Sync-Marker gap number */
	uint32_t nmcu;		/* number of MCU minus one */
	uint32_t raw[3];	/* {rawy, rawu, rawv} or {rawy, rawc, N/C} */
	uint32_t stride[2];    /* {stride_y, stride_c}, only used in raster raw */
	uint32_t mb_width;
	uint32_t mb_height;
	uint8_t raw_format;

	/* Quantization level select,0-2 level */
	QUANT_QUALITY ql_sel;
	uint8_t huffenc_sel;           /* Huffman ENC Table select */
}JPEGE_SliceInfo;

/*The actual size required is 5000-6000*/
#define DESC_SIZE_MAX 8192

#define JPGE_TILE_MODE  0
#define JPGE_NV12_MODE  8
#define JPGE_NV21_MODE  12


#endif /* JPEG_CORE_H */
