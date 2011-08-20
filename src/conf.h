/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Gui Policy Editor for TOMOYO Linux
 *
 * conf.h
 * Copyright (C) Yoshihiro Kusuno 2011 <yocto@users.sourceforge.jp>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor Boston, MA 02110-1301,  USA
 */

#ifndef __CONF_H__
#define __CONF_H__

typedef struct _search_conf_t {
	gboolean	match;
	gboolean	wrap;
	gboolean	close;
} search_conf_t;


void get_conf_search(search_conf_t *conf);
void put_conf_search(search_conf_t *conf);


#endif	/* __CONF_H__ */
