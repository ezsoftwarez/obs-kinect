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

#include <obs-kinect/FxVfxHub/Effects/ColorGradingEffect.hpp>
#include <obs-kinect-core/Helper.hpp>
#include <stdexcept>
#include <string>

ColorGradingEffect::ColorGradingEffect()
{
	ObsMemoryPtr<char> effectFilename(obs_module_file("fx_color_grading.effect"));

	ObsGraphics gfx;

	char* errStr = nullptr;
	m_effect = gs_effect_create_from_file(effectFilename.get(), &errStr);
	ObsMemoryPtr<char> errStrOwner(errStr);

	if (!m_effect)
	{
		std::string err("FxVfxHub: failed to create color grading effect: ");
		err.append(errStr ? errStr : "shader error");
		throw std::runtime_error(err);
	}

	m_param_Image      = gs_effect_get_param_by_name(m_effect, "Image");
	m_param_HueShift   = gs_effect_get_param_by_name(m_effect, "HueShift");
	m_param_Saturation = gs_effect_get_param_by_name(m_effect, "Saturation");
	m_param_Brightness = gs_effect_get_param_by_name(m_effect, "Brightness");
	m_param_Contrast   = gs_effect_get_param_by_name(m_effect, "Contrast");
	m_param_Gamma      = gs_effect_get_param_by_name(m_effect, "Gamma");
	m_technique        = gs_effect_get_technique(m_effect, "ColorGrading");

	m_workTexture = gs_texrender_create(GS_RGBA, GS_ZS_NONE);
}

ColorGradingEffect::~ColorGradingEffect()
{
	ObsGraphics gfx;
	gs_effect_destroy(m_effect);
	gs_texrender_destroy(m_workTexture);
}

gs_texture_t* ColorGradingEffect::Apply(const Config& config, gs_texture_t* sourceTexture)
{
	uint32_t width  = gs_texture_get_width(sourceTexture);
	uint32_t height = gs_texture_get_height(sourceTexture);

	gs_texrender_reset(m_workTexture);
	if (!gs_texrender_begin(m_workTexture, width, height))
		return sourceTexture;

	gs_ortho(0.f, float(width), 0.f, float(height), -100.f, 100.f);

	gs_effect_set_texture(m_param_Image,      sourceTexture);
	gs_effect_set_float(m_param_HueShift,     config.hueShift);
	gs_effect_set_float(m_param_Saturation,   config.saturation);
	gs_effect_set_float(m_param_Brightness,   config.brightness);
	gs_effect_set_float(m_param_Contrast,     config.contrast);
	gs_effect_set_float(m_param_Gamma,        config.gamma);

	gs_technique_begin(m_technique);
	gs_technique_begin_pass(m_technique, 0);
	gs_draw_sprite(nullptr, 0, width, height);
	gs_technique_end_pass(m_technique);
	gs_technique_end(m_technique);

	gs_texrender_end(m_workTexture);

	return gs_texrender_get_texture(m_workTexture);
}

obs_properties_t* ColorGradingEffect::BuildProperties()
{
	obs_properties_t* props = obs_properties_create();

	obs_properties_add_float_slider(props, "colorgrading_hueshift",   obs_module_text("ObsKinect.FxVfx.ColorGrading.HueShift"),   -180.0, 180.0, 1.0);
	obs_properties_add_float_slider(props, "colorgrading_saturation", obs_module_text("ObsKinect.FxVfx.ColorGrading.Saturation"),     0.0,   3.0, 0.01);
	obs_properties_add_float_slider(props, "colorgrading_brightness", obs_module_text("ObsKinect.FxVfx.ColorGrading.Brightness"),    -1.0,   1.0, 0.01);
	obs_properties_add_float_slider(props, "colorgrading_contrast",   obs_module_text("ObsKinect.FxVfx.ColorGrading.Contrast"),       0.0,   3.0, 0.01);
	obs_properties_add_float_slider(props, "colorgrading_gamma",      obs_module_text("ObsKinect.FxVfx.ColorGrading.Gamma"),          0.1,   5.0, 0.01);

	return props;
}

void ColorGradingEffect::SetDefaultValues(obs_data_t* settings)
{
	Config defaults;
	obs_data_set_default_double(settings, "colorgrading_hueshift",   defaults.hueShift);
	obs_data_set_default_double(settings, "colorgrading_saturation", defaults.saturation);
	obs_data_set_default_double(settings, "colorgrading_brightness", defaults.brightness);
	obs_data_set_default_double(settings, "colorgrading_contrast",   defaults.contrast);
	obs_data_set_default_double(settings, "colorgrading_gamma",      defaults.gamma);
}

ColorGradingEffect::Config ColorGradingEffect::ToConfig(obs_data_t* settings)
{
	Config config;
	config.enabled    = obs_data_get_bool(settings,   "fxvfx_colorgrading_enabled");
	config.hueShift   = float(obs_data_get_double(settings, "colorgrading_hueshift"));
	config.saturation = float(obs_data_get_double(settings, "colorgrading_saturation"));
	config.brightness = float(obs_data_get_double(settings, "colorgrading_brightness"));
	config.contrast   = float(obs_data_get_double(settings, "colorgrading_contrast"));
	config.gamma      = float(obs_data_get_double(settings, "colorgrading_gamma"));
	return config;
}
