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

#ifndef OBS_KINECT_PLUGIN_FXVFXHUB
#define OBS_KINECT_PLUGIN_FXVFXHUB

#include <obs-kinect/FxVfxHub/Effects/ChromaticAberrationEffect.hpp>
#include <obs-kinect/FxVfxHub/Effects/ColorGradingEffect.hpp>
#include <obs-kinect/FxVfxHub/Effects/EdgeGlowEffect.hpp>
#include <obs-kinect/FxVfxHub/Effects/MotionTrailEffect.hpp>
#include <obs-kinect/FxVfxHub/Effects/PixelateEffect.hpp>
#include <obs-module.h>

class FxVfxHub
{
	public:
		FxVfxHub(obs_source_t* source);
		~FxVfxHub();

		void Render();
		void Update(obs_data_t* settings);

		static obs_properties_t* GetProperties();
		static void SetDefaults(obs_data_t* settings);

	private:
		obs_source_t* m_source;

		ColorGradingEffect        m_colorGrading;
		EdgeGlowEffect            m_edgeGlow;
		ChromaticAberrationEffect m_chromaticAberration;
		PixelateEffect            m_pixelate;
		MotionTrailEffect         m_motionTrail;

		ColorGradingEffect::Config        m_colorGradingConfig;
		EdgeGlowEffect::Config            m_edgeGlowConfig;
		ChromaticAberrationEffect::Config m_chromaticAberrationConfig;
		PixelateEffect::Config            m_pixelateConfig;
		MotionTrailEffect::Config         m_motionTrailConfig;

		gs_texrender_t* m_inputCapture = nullptr;
};

#endif
