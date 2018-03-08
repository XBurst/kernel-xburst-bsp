/*
 *  Copyright (C) 2010, Lars-Peter Clausen <lars@metafoo.de>
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under  the terms of the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the License, or (at your
 *  option) any later version.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#ifndef __LINUX_POWER_LI_ION_CHARGER_H__
#define __LINUX_POWER_LI_ION_CHARGER_H__

#include <linux/power_supply.h>
#include <linux/types.h>

/**
 * struct li_ion_charger_platform_data - platform_data for li_ion_charger device
 * @gpio:		GPIO which is used to indicate the chargers status
 * @gpio_active_low:	Should be set to 1 if the GPIO is active low otherwise 0
 */
struct li_ion_charger_platform_data {
	int gpio_ac;
	int gpio_charge;
	int gpio_active_low;
};

#endif
