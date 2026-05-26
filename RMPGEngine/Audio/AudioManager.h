#pragma once
#include <Audio.h>
#include <map>

using namespace DirectX;

namespace RMPG
{
	using SoundID = int;
	using EffectID = int;

	class Audio
	{
	public:
		bool Initialize();
		bool Update();
		
		SoundID LoadWav(std::wstring path);
		EffectID AddEffect(SoundID sound);

		void SetMasterVolume(float volume);

		void PlayA(SoundID sound);
		bool IsInUse(SoundID sound);

		void PlayAfx(EffectID effect);
		void Pause(EffectID effect);
		void Resume(EffectID effect);
		void Stop(EffectID effect, bool immediate = true);
		bool IsLooped(EffectID effect);
		void SetEffectVolume(EffectID effect, float volume);

	private:
		std::unique_ptr<AudioEngine> audEngine;

		std::map<SoundID, std::unique_ptr<SoundEffect>> sounds;
		std::map<EffectID, std::unique_ptr<SoundEffectInstance>> effects;
		SoundID nextSoundId = 0;
		EffectID nextEffectId = 0;
	};
};
