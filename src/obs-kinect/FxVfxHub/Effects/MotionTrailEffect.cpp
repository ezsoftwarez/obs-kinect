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

#include <obs-kinect/FxVfxHub/Effects/MotionTrailEffect.hpp>
#include <obs-kinect-core/Helper.hpp>
#include <stdexcept>
#include <string>

MotionTrailEffect::MotionTrailEffect()
{
	ObsMemoryPtr<char> effectFilename(obs_module_file("fx_motion_trail.effect"));

	ObsGraphics gfx;

	char* errStr = nullptr;
	m_effect = gs_effect_create_from_file(effectFilename.get(), &errStr);
	ObsMemoryPtr<char> errStrOwner(errStr);

	if (!m_effect)
	{
		std::string err("FxVfxHub: failed to create motion trail effect: ");
		err.append(errStr ? errStr : "shader error");
		throw std::runtime_error(err);
	}

	m_param_CurrentFrame  = gs_effect_get_param_by_name(m_effect, "CurrentFrame");
	m_param_PreviousFrame = gs_effect_get_param_by_name(m_effect, "PreviousFrame");
	m_param_TrailDecay    = gs_effect_get_param_by_name(m_effect, "TrailDecay");
	m_technique           = gs_effect_get_technique(m_effect, "MotionTrail");

	m_accumulationTexture = gs_texrender_create(GS_RGBA, GS_ZS_NONE);
	m_workTexture         = gs_texrender_create(GS_RGBA, GS_ZS_NONE);
}

MotionTrailEffect::~MotionTrailEffect()
{
	ObsGraphics gfx;
	gs_effect_destroy(m_effect);
	gs_texrender_destroy(m_accumulationTexture);
	gs_texrender_destroy(m_workTexture);
}

gs_texture_t* MotionTrailEffect::Apply(const Config& config, gs_texture_t* sourceTexture)
{
	uint32_t width  = gs_texture_get_width(sourceTexture);
	uint32_t height = gs_texture_get_height(sourceTexture);

	gs_texrender_reset(m_workTexture);
	if (!gs_texrender_begin(m_workTexture, width, height))
		return sourceTexture;

	gs_ortho(0.f, float(width), 0.f, float(height), -100.f, 100.f);

	gs_texture_t* prevTexture = gs_texrender_get_texture(m_accumulationTexture);

	gs_effect_set_texture(m_param_CurrentFrame,  sourceTexture);
	gs_effect_set_texture(m_param_PreviousFrame, prevTexture ? prevTexture : sourceTexture);
	gs_effect_set_float(m_param_TrailDecay,      config.trailDecay);

	gs_technique_begin(m_technique);
	gs_technique_begin_pass(m_technique, 0);
	gs_draw_sprite(nullptr, 0, width, height);
	gs_technique_end_pass(m_technique);
	gs_technique_end(m_technique);

	gs_texrender_end(m_workTexture);

	gs_texrender_reset(m_accumulationTexture);
	if (gs_texrender_begin(m_accumulationTexture, width, height))
	{
		gs_ortho(0.f, float(width), 0.f, float(height), -100.f, 100.f);

		gs_effect_t*    defaultEffect = obs_get_base_effect(OBS_EFFECT_DEFAULT);
		gs_eparam_t*    imageParam    = gs_effect_get_param_by_name(defaultEffect, "image");
		gs_technique_t* drawTech      = gs_effect_get_technique(defaultEffect, "Draw");

		gs_effect_set_texture(imageParam, gs_texrender_get_texture(m_workTexture));

		gs_technique_begin(drawTech);
		gs_technique_begin_pass(drawTech, 0);
		gs_draw_sprite(nullptr, 0, width, height);
		gs_technique_end_pass(drawTech);
		gs_technique_end(drawTech);

		gs_texrender_end(m_accumulationTexture);
	}

	return gs_texrender_get_texture(m_workTexture);
}

obs_properties_t* MotionTrailEffect::BuildProperties()
{
	obs_properties_t* props = obs_properties_create();

	obs_properties_add_float_slider(props, "motiontrail_decay", obs_module_text("ObsKinect.FxVfx.MotionTrail.Decay"), 0.0, 0.99, 0.01);

	return props;
}

void MotionTrailEffect::SetDefaultValues(obs_data_t* settings)
{
	Config defaults;
	obs_data_set_default_double(settings, "motiontrail_decay", defaults.trailDecay);
}

MotionTrailEffect::Config MotionTrailEffect::ToConfig(obs_data_t* settings)
{
	Config config;
	config.enabled    = obs_data_get_bool(settings,   "fxvfx_motiontrail_enabled");
	config.trailDecay = float(obs_data_get_double(settings, "motiontrail_decay"));
	return config;
}
