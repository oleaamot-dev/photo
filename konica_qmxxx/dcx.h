/*
 *	Konica-qm-sio-sample version 1.00
 *
 *	Copyright (C) 1999 Konica corporation .
 *
 *                                <qm200-support@konica.co.jp>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
typedef struct {
	byte	*alloc_buf;
	long	size;
} dcx_image_t;

typedef struct {
	long	total_pict;
	long	picture_count;
	byte	hour;
	byte	minute;
	byte	second;
	byte	day;
	byte	month;
	byte	year;
} dcx_summary_t;

typedef enum { UNKNOWN_MODEL, QM100, QM200 } dcx_camera_model_t;
typedef void (*dcx_percent_disp_func_t)(long percent);

ok_t	dcx_init(char *dev, dcx_camera_model_t model);
long	dcx_get_number_of_pictures(void);
ok_t	dcx_take_picture(void);
ok_t	dcx_alloc_and_get_thum(long no, dcx_image_t *ans);
ok_t	dcx_alloc_and_get_exif(long no, dcx_image_t *ans, dcx_percent_disp_func_t func);
ok_t	dcx_delete_picture(int no);
ok_t	dcx_format_cf_card(void);
ok_t	dcx_get_summary(dcx_summary_t *summary);

