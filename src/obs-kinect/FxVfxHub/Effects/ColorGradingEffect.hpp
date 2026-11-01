/******************************************************************************
	Copyright (C) 2021 by Jérôme Leclercq <lynix680@gmail.com>

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#pragma once

#ifndef OBS_KINECT_PLUGIN_FXVFX_COLORGRADINGEFFECT
#define OBS_KINECT_PLUGIN_FXVFX_COLORGRADINGEFFECT

#include <obs-module.h>

class ColorGradingEffect
{
	public:
		struct Config;

		ColorGradingEffect();
		~ColorGradingEffect();

		gs_texture_t* Apply(const Config& config, gs_texture_t* sourceTexture);

		static obs_properties_t* BuildProperties();
		static void SetDefaultValues(obs_data_t* settings);
		static Config ToConfig(obs_data_t* settings);

		struct Config
		{
			bool  enabled    = false;
			float hueShift   = 0.f;
			float saturation = 1.f;
			float brightness = 0.f;
			float contrast   = 1.f;
			float gamma      = 1.f;
		};

	private:
		gs_effect_t*    m_effect           = nullptr;
		gs_eparam_t*    m_param_Image      = nullptr;
		gs_eparam_t*    m_param_HueShift   = nullptr;
		gs_eparam_t*    m_param_Saturation = nullptr;
		gs_eparam_t*    m_param_Brightness = nullptr;
		gs_eparam_t*    m_param_Contrast   = nullptr;
		gs_eparam_t*    m_param_Gamma      = nullptr;
		gs_technique_t* m_technique        = nullptr;
		gs_texrender_t* m_workTexture      = nullptr;
};

#endif
