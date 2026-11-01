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

#include <obs-kinect/FxVfxHub/Effects/EdgeGlowEffect.hpp>
#include <obs-kinect-core/Helper.hpp>
#include <stdexcept>
#include <string>

EdgeGlowEffect::EdgeGlowEffect()
{
	ObsMemoryPtr<char> effectFilename(obs_module_file("fx_edge_glow.effect"));

	ObsGraphics gfx;

	char* errStr = nullptr;
	m_effect = gs_effect_create_from_file(effectFilename.get(), &errStr);
	ObsMemoryPtr<char> errStrOwner(errStr);

	if (!m_effect)
	{
		std::string err("FxVfxHub: failed to create edge glow effect: ");
		err.append(errStr ? errStr : "shader error");
		throw std::runtime_error(err);
	}

	m_param_Image         = gs_effect_get_param_by_name(m_effect, "Image");
	m_param_InvImageSize  = gs_effect_get_param_by_name(m_effect, "InvImageSize");
	m_param_EdgeThreshold = gs_effect_get_param_by_name(m_effect, "EdgeThreshold");
	m_param_GlowIntensity = gs_effect_get_param_by_name(m_effect, "GlowIntensity");
	m_param_GlowColor     = gs_effect_get_param_by_name(m_effect, "GlowColor");
	m_technique           = gs_effect_get_technique(m_effect, "EdgeGlow");

	m_workTexture = gs_texrender_create(GS_RGBA, GS_ZS_NONE);
}

EdgeGlowEffect::~EdgeGlowEffect()
{
	ObsGraphics gfx;
	gs_effect_destroy(m_effect);
	gs_texrender_destroy(m_workTexture);
}

gs_texture_t* EdgeGlowEffect::Apply(const Config& config, gs_texture_t* sourceTexture)
{
	uint32_t width  = gs_texture_get_width(sourceTexture);
	uint32_t height = gs_texture_get_height(sourceTexture);

	gs_texrender_reset(m_workTexture);
	if (!gs_texrender_begin(m_workTexture, width, height))
		return sourceTexture;

	gs_ortho(0.f, float(width), 0.f, float(height), -100.f, 100.f);

	vec2 invImageSize = { 1.f / float(width), 1.f / float(height) };
	vec4 glowColor    = { config.glowR, config.glowG, config.glowB, 1.f };

	gs_effect_set_texture(m_param_Image,          sourceTexture);
	gs_effect_set_vec2(m_param_InvImageSize,      &invImageSize);
	gs_effect_set_float(m_param_EdgeThreshold,    config.edgeThreshold);
	gs_effect_set_float(m_param_GlowIntensity,    config.glowIntensity);
	gs_effect_set_vec4(m_param_GlowColor,         &glowColor);

	gs_technique_begin(m_technique);
	gs_technique_begin_pass(m_technique, 0);
	gs_draw_sprite(nullptr, 0, width, height);
	gs_technique_end_pass(m_technique);
	gs_technique_end(m_technique);

	gs_texrender_end(m_workTexture);

	return gs_texrender_get_texture(m_workTexture);
}

obs_properties_t* EdgeGlowEffect::BuildProperties()
{
	obs_properties_t* props = obs_properties_create();

	obs_properties_add_float_slider(props, "edgeglow_threshold",  obs_module_text("ObsKinect.FxVfx.EdgeGlow.Threshold"),  0.0, 1.0, 0.01);
	obs_properties_add_float_slider(props, "edgeglow_intensity",  obs_module_text("ObsKinect.FxVfx.EdgeGlow.Intensity"),  0.0, 5.0, 0.1);
	obs_properties_add_color(props,        "edgeglow_color",      obs_module_text("ObsKinect.FxVfx.EdgeGlow.Color"));

	return props;
}

void EdgeGlowEffect::SetDefaultValues(obs_data_t* settings)
{
	Config defaults;
	obs_data_set_default_double(settings, "edgeglow_threshold", defaults.edgeThreshold);
	obs_data_set_default_double(settings, "edgeglow_intensity", defaults.glowIntensity);
	obs_data_set_default_int(settings, "edgeglow_color", 0xFF00FFFF);
}

EdgeGlowEffect::Config EdgeGlowEffect::ToConfig(obs_data_t* settings)
{
	Config config;
	config.enabled       = obs_data_get_bool(settings,   "fxvfx_edgeglow_enabled");
	config.edgeThreshold = float(obs_data_get_double(settings, "edgeglow_threshold"));
	config.glowIntensity = float(obs_data_get_double(settings, "edgeglow_intensity"));

	uint32_t colorBGRA = uint32_t(obs_data_get_int(settings, "edgeglow_color"));
	config.glowR = float((colorBGRA >> 16) & 0xFF) / 255.f;
	config.glowG = float((colorBGRA >>  8) & 0xFF) / 255.f;
	config.glowB = float((colorBGRA >>  0) & 0xFF) / 255.f;

	return config;
}
