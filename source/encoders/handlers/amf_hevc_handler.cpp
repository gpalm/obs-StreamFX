/*
 * Modern effects for a modern Streamer
 * Copyright (C) 2017-2018 Michael Fabian Dirks
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "amf_hevc_handler.hpp"
#include "strings.hpp"
#include "../codecs/hevc.hpp"
#include "../encoder-ffmpeg.hpp"
#include "amf_shared.hpp"
#include "ffmpeg/tools.hpp"
#include "plugin.hpp"

extern "C" {
#include <obs-module.h>
#pragma warning(push)
#pragma warning(disable : 4242 4244 4365)
#include <libavutil/opt.h>
#pragma warning(pop)
}

// Settings
#define ST_KEY_PROFILE "H265.Profile"
#define ST_KEY_TIER "H265.Tier"
#define ST_KEY_LEVEL "H265.Level"

using namespace streamfx::encoder::ffmpeg::handler;
using namespace streamfx::encoder::codec::hevc;

static std::map<profile, std::string> profiles{
	{profile::MAIN, "main"},
};

static std::map<tier, std::string> tiers{
	{tier::MAIN, "main"},
	{tier::HIGH, "high"},
};

static std::map<level, std::string> levels{
	{level::L1_0, "1.0"}, {level::L2_0, "2.0"}, {level::L2_1, "2.1"}, {level::L3_0, "3.0"}, {level::L3_1, "3.1"},
	{level::L4_0, "4.0"}, {level::L4_1, "4.1"}, {level::L5_0, "5.0"}, {level::L5_1, "5.1"}, {level::L5_2, "5.2"},
	{level::L6_0, "6.0"}, {level::L6_1, "6.1"}, {level::L6_2, "6.2"},
};

void amf_hevc_handler::adjust_info(ffmpeg_factory* factory, const AVCodec* codec, std::string& id, std::string& name,
								   std::string& codec_id)
{
	name = "AMD AMF H.265/HEVC (via FFmpeg)";
	if (!amf::is_available())
		factory->get_info()->caps |= OBS_ENCODER_CAP_DEPRECATED;
}

void amf_hevc_handler::get_defaults(obs_data_t* settings, const AVCodec* codec, AVCodecContext* context, bool)
{
	amf::get_defaults(settings, codec, context);

	obs_data_set_default_int(settings, ST_KEY_PROFILE, static_cast<int64_t>(profile::MAIN));
	obs_data_set_default_int(settings, ST_KEY_TIER, static_cast<int64_t>(profile::MAIN));
	obs_data_set_default_int(settings, ST_KEY_LEVEL, static_cast<int64_t>(level::UNKNOWN));
}

bool amf_hevc_handler::has_keyframe_support(ffmpeg_factory*)
{
	return true;
}

bool amf_hevc_handler::is_hardware_encoder(ffmpeg_factory* instance)
{
	return true;
}

bool amf_hevc_handler::has_threading_support(ffmpeg_factory* instance)
{
	return false;
}

bool amf_hevc_handler::has_pixel_format_support(ffmpeg_factory* instance)
{
	return false;
}

void amf_hevc_handler::get_properties(obs_properties_t* props, const AVCodec* codec, AVCodecContext* context, bool)
{
	if (!context) {
		this->get_encoder_properties(props, codec);
	} else {
		this->get_runtime_properties(props, codec, context);
	}
}

void amf_hevc_handler::update(obs_data_t* settings, const AVCodec* codec, AVCodecContext* context)
{
	amf::update(settings, codec, context);

	{ // HEVC Options
		auto found = profiles.find(static_cast<profile>(obs_data_get_int(settings, ST_KEY_PROFILE)));
		if (found != profiles.end()) {
			av_opt_set(context->priv_data, "profile", found->second.c_str(), 0);
		}
	}
	{
		auto found = tiers.find(static_cast<tier>(obs_data_get_int(settings, ST_KEY_TIER)));
		if (found != tiers.end()) {
			av_opt_set(context->priv_data, "tier", found->second.c_str(), 0);
		}
	}
	{
		auto found = levels.find(static_cast<level>(obs_data_get_int(settings, ST_KEY_LEVEL)));
		if (found != levels.end()) {
			av_opt_set(context->priv_data, "level", found->second.c_str(), 0);
		} else {
			av_opt_set(context->priv_data, "level", "auto", 0);
		}
	}
}

void amf_hevc_handler::override_update(ffmpeg_instance* instance, obs_data_t* settings)
{
	amf::override_update(instance, settings);
}

void amf_hevc_handler::log_options(obs_data_t* settings, const AVCodec* codec, AVCodecContext* context)
{
	amf::log_options(settings, codec, context);

	DLOG_INFO("[%s]     H.265/HEVC:", codec->name);
	::streamfx::ffmpeg::tools::print_av_option_string2(context, "profile", "      Profile",
													   [](int64_t v, std::string_view o) { return std::string(o); });
	::streamfx::ffmpeg::tools::print_av_option_string2(context, "level", "      Level",
													   [](int64_t v, std::string_view o) { return std::string(o); });
	::streamfx::ffmpeg::tools::print_av_option_string2(context, "tier", "      Tier",
													   [](int64_t v, std::string_view o) { return std::string(o); });
}

void amf_hevc_handler::get_encoder_properties(obs_properties_t* props, const AVCodec* codec)
{
	amf::get_properties_pre(props, codec);

	{
		obs_properties_t* grp = obs_properties_create();
		obs_properties_add_group(props, S_CODEC_HEVC, D_TRANSLATE(S_CODEC_HEVC), OBS_GROUP_NORMAL, grp);

		{
			auto p = obs_properties_add_list(grp, ST_KEY_PROFILE, D_TRANSLATE(S_CODEC_HEVC_PROFILE),
											 OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
			obs_property_list_add_int(p, D_TRANSLATE(S_STATE_DEFAULT), static_cast<int64_t>(profile::UNKNOWN));
			for (auto const kv : profiles) {
				std::string trans = std::string(S_CODEC_HEVC_PROFILE) + "." + kv.second;
				obs_property_list_add_int(p, D_TRANSLATE(trans.c_str()), static_cast<int64_t>(kv.first));
			}
		}
		{
			auto p = obs_properties_add_list(grp, ST_KEY_TIER, D_TRANSLATE(S_CODEC_HEVC_TIER), OBS_COMBO_TYPE_LIST,
											 OBS_COMBO_FORMAT_INT);
			obs_property_list_add_int(p, D_TRANSLATE(S_STATE_DEFAULT), static_cast<int64_t>(tier::UNKNOWN));
			for (auto const kv : tiers) {
				std::string trans = std::string(S_CODEC_HEVC_TIER) + "." + kv.second;
				obs_property_list_add_int(p, D_TRANSLATE(trans.c_str()), static_cast<int64_t>(kv.first));
			}
		}
		{
			auto p = obs_properties_add_list(grp, ST_KEY_LEVEL, D_TRANSLATE(S_CODEC_HEVC_LEVEL), OBS_COMBO_TYPE_LIST,
											 OBS_COMBO_FORMAT_INT);
			obs_property_list_add_int(p, D_TRANSLATE(S_STATE_AUTOMATIC), static_cast<int64_t>(level::UNKNOWN));
			for (auto const kv : levels) {
				obs_property_list_add_int(p, kv.second.c_str(), static_cast<int64_t>(kv.first));
			}
		}
	}

	amf::get_properties_post(props, codec);
}

void amf_hevc_handler::get_runtime_properties(obs_properties_t* props, const AVCodec* codec, AVCodecContext* context)
{
	amf::get_runtime_properties(props, codec, context);
}

void streamfx::encoder::ffmpeg::handler::amf_hevc_handler::migrate(obs_data_t* settings, std::uint64_t version,
																   const AVCodec* codec, AVCodecContext* context)
{
	amf::migrate(settings, version, codec, context);
}
