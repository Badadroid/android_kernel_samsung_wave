/*
** =========================================================================
** File:
**	 ImmVibeSPI.c
**
** Description:
**	 Device-dependent functions called by Immersion TSP API
**	 to control PWM duty cycle, amp enable/disable, save IVT file, etc...
**
** P1 Compat Author: Humberto Borba <kernel@humberos.com.br>
**
** Portions Copyright (c) 2010 Immersion Corporation. All Rights Reserved.
**
** This file contains Original Code and/or Modifications of Original Code
** as defined in and that are subject to the GNU Public License v2 -
** (the 'License'). You may not use this file except in compliance with the
** License. You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software Foundation, Inc.,
** 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA or contact
** TouchSenseSales@immersion.com.
**
** The Original Code and all software distributed under the License are
** distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
** EXPRESS OR IMPLIED, AND IMMERSION HEREBY DISCLAIMS ALL SUCH WARRANTIES,
** INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS
** FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT. Please see
** the License for the specific language governing rights and limitations
** under the License.
** =========================================================================
*/
#include <linux/pwm.h>
#include <plat/gpio-cfg.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include "tspdrv.h"
#include <linux/regulator/consumer.h>

#ifdef IMMVIBESPIAPI
#undef IMMVIBESPIAPI
#endif
#define IMMVIBESPIAPI static

/*
** This SPI supports only one actuator.
*/
#define NUM_ACTUATORS 1
#define PWM_DEVICE	1

extern unsigned int g_PWM_duty_max;
extern bool isRunning;

struct pwm_device	*Immvib_pwm;
static bool g_bAmpEnabled = false;

/*
** Called to disable amp (disable output force)
*/
IMMVIBESPIAPI VibeStatus ImmVibeSPI_ForceOut_AmpDisable(VibeUInt8 nActuatorIndex)
{
	if (g_bAmpEnabled)
	{
		g_bAmpEnabled = false;
		s3c_gpio_cfgpin(VIB_PWM, S3C_GPIO_OUTPUT);
		pwm_config(Immvib_pwm, 0, g_PWM_duty_max/2);
		pwm_disable(Immvib_pwm);
		if (gpio_get_value(VIB_EN)) {
			if (isRunning) {
				regulator_force_disable(regulator_motor);
				isRunning = false;
			}
		}
		gpio_direction_output(VIB_EN, 0);
	}

	return VIBE_S_SUCCESS;
}

/*
** Called to enable amp (enable output force)
*/
IMMVIBESPIAPI VibeStatus ImmVibeSPI_ForceOut_AmpEnable(VibeUInt8 nActuatorIndex)
{
	if (!g_bAmpEnabled) {
		g_bAmpEnabled = true;
		s3c_gpio_cfgpin(VIB_PWM, S3C_GPIO_SFN(2));
		if(!isRunning) {
			regulator_enable(regulator_motor);
			isRunning = true;
		}
		pwm_enable(Immvib_pwm);
		gpio_direction_output(VIB_EN, 1);
	}

	return VIBE_S_SUCCESS;
}

/*
** Called at initialization time to set PWM freq, disable amp, etc...
*/
IMMVIBESPIAPI VibeStatus ImmVibeSPI_ForceOut_Initialize(void)
{
	g_bAmpEnabled = true;

	Immvib_pwm = pwm_request(PWM_DEVICE, "Immvibtonz");
	pwm_config(Immvib_pwm, g_PWM_duty_max/2, g_PWM_duty_max);
	ImmVibeSPI_ForceOut_AmpDisable(0);

	return VIBE_S_SUCCESS;
}

/*
** Called at termination time to set PWM freq, disable amp, etc...
*/
IMMVIBESPIAPI VibeStatus ImmVibeSPI_ForceOut_Terminate(void)
{
	/*
	** Disable amp.
	** If multiple actuators are supported, please make sure to call ImmVibeSPI_ForceOut_AmpDisable
	** for each actuator (provide the actuator index as input argument).
	*/
	ImmVibeSPI_ForceOut_AmpDisable(0);

	return VIBE_S_SUCCESS;
}

/*
** Called by the real-time loop to set PWM duty cycle, and enable amp if required
*/
IMMVIBESPIAPI VibeStatus ImmVibeSPI_ForceOut_Set(VibeUInt8 nActuatorIndex, VibeInt8 nForce)
{
	int pwm_duty=g_PWM_duty_max/2 + ((g_PWM_duty_max/2 - 2) * nForce)/127;

	if (nForce == 0)
		ImmVibeSPI_ForceOut_AmpDisable(0);
	else {
		pwm_config(Immvib_pwm, pwm_duty, g_PWM_duty_max);
		ImmVibeSPI_ForceOut_AmpEnable(0);
	}

	return VIBE_S_SUCCESS;
}

/*
** Called by the real-time loop to set force output, and enable amp if required
*/
IMMVIBESPIAPI VibeStatus ImmVibeSPI_ForceOut_SetSamples(VibeUInt8 nActuatorIndex, VibeUInt16 nOutputSignalBitDepth, VibeUInt16 nBufferSizeInBytes, VibeInt8* pForceOutputBuffer)
{
	return VIBE_S_SUCCESS;
}

/*
** Called to get the device name (device name must be returned as ANSI char)
*/
IMMVIBESPIAPI VibeStatus ImmVibeSPI_Device_GetName(VibeUInt8 nActuatorIndex, char *szDevName, int nSize)
{
	return VIBE_S_SUCCESS;
}
