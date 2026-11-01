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

#include <obs-kinect/FxVfxHub/FxVfxHub.hpp>
#include <obs-kinect-core/Helper.hpp>

FxVfxHub::FxVfxHub(obs_source_t* source) :
m_source(source)
{
	ObsGraphics gfx;
	m_inputCapture = gs_texrender_create(GS_RGBA, GS_ZS_NONE);
}

FxVfxHub::~FxVfxHub()
{
	ObsGraphics gfx;
	gs_texrender_destroy(m_inputCapture);
}

void FxVfxHub::Render()
{
	obs_source_t* target = obs_filter_get_target(m_source);
	if (!target)
	{
		obs_source_skip_video_filter(m_source);
		return;
	}

	uint32_t width  = obs_source_get_base_width(target);
	uint32_t height = obs_source_get_base_height(target);
	if (width == 0 || height == 0)
	{
		obs_source_skip_video_filter(m_source);
		return;
	}

	const bool anyEnabled = m_colorGradingConfig.enabled
		|| m_edgeGlowConfig.enabled
		|| m_chromaticAberrationConfig.enabled
		|| m_pixelateConfig.enabled
		|| m_motionTrailConfig.enabled;

	if (!anyEnabled)
	{
		obs_source_skip_video_filter(m_source);
		return;
	}

	gs_texrender_reset(m_inputCapture);
	if (!gs_texrender_begin(m_inputCapture, width, height))
	{
		obs_source_skip_video_filter(m_source);
		return;
	}

	vec4 black = { 0.f, 0.f, 0.f, 0.f };
	gs_clear(GS_CLEAR_COLOR, &black, 0.f, 0);
	gs_ortho(0.f, float(width), 0.f, float(height), -100.f, 100.f);

	gs_blend_state_push();
	gs_blend_function(GS_BLEND_ONE, GS_BLEND_ZERO);
	obs_source_video_render(target);
	gs_blend_state_pop();

	gs_texrender_end(m_inputCapture);

	gs_texture_t* current = gs_texrender_get_texture(m_inputCapture);

	if (m_colorGradingConfig.enabled && current)
		current = m_colorGrading.Apply(m_colorGradingConfig, current);

	if (m_edgeGlowConfig.enabled && current)
		current = m_edgeGlow.Apply(m_edgeGlowConfig, current);

	if (m_chromaticAberrationConfig.enabled && current)
		current = m_chromaticAberration.Apply(m_chromaticAberrationConfig, current);

	if (m_pixelateConfig.enabled && current)
		current = m_pixelate.Apply(m_pixelateConfig, current);

	if (m_motionTrailConfig.enabled && current)
		current = m_motionTrail.Apply(m_motionTrailConfig, current);

	if (!current)
	{
		obs_source_skip_video_filter(m_source);
		return;
	}

	gs_effect_t*    defaultEffect = obs_get_base_effect(OBS_EFFECT_DEFAULT);
	gs_eparam_t*    imageParam    = gs_effect_get_param_by_name(defaultEffect, "image");
	gs_technique_t* drawTech      = gs_effect_get_technique(defaultEffect, "Draw");

	gs_effect_set_texture(imageParam, current);

	gs_technique_begin(drawTech);
	gs_technique_begin_pass(drawTech, 0);
	gs_draw_sprite(current, 0, width, height);
	gs_technique_end_pass(drawTech);
	gs_technique_end(drawTech);
}

void FxVfxHub::Update(obs_data_t* settings)
{
	m_colorGradingConfig        = ColorGradingEffect::ToConfig(settings);
	m_edgeGlowConfig            = EdgeGlowEffect::ToConfig(settings);
	m_chromaticAberrationConfig = ChromaticAberrationEffect::ToConfig(settings);
	m_pixelateConfig            = PixelateEffect::ToConfig(settings);
	m_motionTrailConfig         = MotionTrailEffect::ToConfig(settings);
}

obs_properties_t* FxVfxHub::GetProperties()
{
	obs_properties_t* props = obs_properties_create();

	{
		obs_properties_t* groupProps = obs_properties_create();
		obs_properties_add_bool(groupProps, "fxvfx_colorgrading_enabled", obs_module_text("ObsKinect.FxVfx.Enabled"));
		obs_properties_t* effectProps = ColorGradingEffect::BuildProperties();
		obs_properties_add_group(groupProps, "colorgrading_params", obs_module_text("ObsKinect.FxVfx.ColorGrading.Params"), OBS_GROUP_NORMAL, effectProps);
		obs_properties_add_group(props, "fxvfx_colorgrading", obs_module_text("ObsKinect.FxVfx.ColorGrading"), OBS_GROUP_NORMAL, groupProps);
	}

	{
		obs_properties_t* groupProps = obs_properties_create();
		obs_properties_add_bool(groupProps, "fxvfx_edgeglow_enabled", obs_module_text("ObsKinect.FxVfx.Enabled"));
		obs_properties_t* effectProps = EdgeGlowEffect::BuildProperties();
		obs_properties_add_group(groupProps, "edgeglow_params", obs_module_text("ObsKinect.FxVfx.EdgeGlow.Params"), OBS_GROUP_NORMAL, effectProps);
		obs_properties_add_group(props, "fxvfx_edgeglow", obs_module_text("ObsKinect.FxVfx.EdgeGlow"), OBS_GROUP_NORMAL, groupProps);
	}

	{
		obs_properties_t* groupProps = obs_properties_create();
		obs_properties_add_bool(groupProps, "fxvfx_chromaber_enabled", obs_module_text("ObsKinect.FxVfx.Enabled"));
		obs_properties_t* effectProps = ChromaticAberrationEffect::BuildProperties();
		obs_properties_add_group(groupProps, "chromaber_params", obs_module_text("ObsKinect.FxVfx.ChromaticAberration.Params"), OBS_GROUP_NORMAL, effectProps);
		obs_properties_add_group(props, "fxvfx_chromaber", obs_module_text("ObsKinect.FxVfx.ChromaticAberration"), OBS_GROUP_NORMAL, groupProps);
	}

	{
		obs_properties_t* groupProps = obs_properties_create();
		obs_properties_add_bool(groupProps, "fxvfx_pixelate_enabled", obs_module_text("ObsKinect.FxVfx.Enabled"));
		obs_properties_t* effectProps = PixelateEffect::BuildProperties();
		obs_properties_add_group(groupProps, "pixelate_params", obs_module_text("ObsKinect.FxVfx.Pixelate.Params"), OBS_GROUP_NORMAL, effectProps);
		obs_properties_add_group(props, "fxvfx_pixelate", obs_module_text("ObsKinect.FxVfx.Pixelate"), OBS_GROUP_NORMAL, groupProps);
	}

	{
		obs_properties_t* groupProps = obs_properties_create();
		obs_properties_add_bool(groupProps, "fxvfx_motiontrail_enabled", obs_module_text("ObsKinect.FxVfx.Enabled"));
		obs_properties_t* effectProps = MotionTrailEffect::BuildProperties();
		obs_properties_add_group(groupProps, "motiontrail_params", obs_module_text("ObsKinect.FxVfx.MotionTrail.Params"), OBS_GROUP_NORMAL, effectProps);
		obs_properties_add_group(props, "fxvfx_motiontrail", obs_module_text("ObsKinect.FxVfx.MotionTrail"), OBS_GROUP_NORMAL, groupProps);
	}

	return props;
}

void FxVfxHub::SetDefaults(obs_data_t* settings)
{
	obs_data_set_default_bool(settings, "fxvfx_colorgrading_enabled", false);
	obs_data_set_default_bool(settings, "fxvfx_edgeglow_enabled",     false);
	obs_data_set_default_bool(settings, "fxvfx_chromaber_enabled",    false);
	obs_data_set_default_bool(settings, "fxvfx_pixelate_enabled",     false);
	obs_data_set_default_bool(settings, "fxvfx_motiontrail_enabled",  false);

	ColorGradingEffect::SetDefaultValues(settings);
	EdgeGlowEffect::SetDefaultValues(settings);
	ChromaticAberrationEffect::SetDefaultValues(settings);
	PixelateEffect::SetDefaultValues(settings);
	MotionTrailEffect::SetDefaultValues(settings);
}
