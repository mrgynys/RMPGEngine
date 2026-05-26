#include "AudioManager.h"
#include "../ErrorLogger.h"

bool RMPG::Audio::Initialize()
{
	AUDIO_ENGINE_FLAGS eflags = AudioEngine_Default;
#ifdef _DEBUG
	eflags |= AudioEngine_Debug;
#endif

	audEngine = std::make_unique<AudioEngine>(eflags);
	
	return true;
}

bool RMPG::Audio::Update()
{
	if (!audEngine->Update())
	{
		if (audEngine->IsCriticalError())
		{
			ErrorLogger::Log("Audio critical error");
			return false;
		}
	}
	return true;
}

RMPG::SoundID RMPG::Audio::LoadWav(std::wstring path)
{
	std::unique_ptr<SoundEffect> soundEffect;
	soundEffect = std::make_unique<SoundEffect>(audEngine.get(), path.data());

	SoundID newId = nextSoundId++;
	sounds[newId] = std::move(soundEffect);

	return newId;
}

RMPG::EffectID RMPG::Audio::AddEffect(SoundID sound)
{
	std::unique_ptr<SoundEffectInstance> effect = sounds[sound]->CreateInstance();
	EffectID newId = nextEffectId++;
	effects[newId] = std::move(effect);

	return newId;
}

void RMPG::Audio::SetMasterVolume(float volume)
{
	audEngine->SetMasterVolume(volume);
}

void RMPG::Audio::PlayA(SoundID sound)
{
	sounds[sound]->Play();
}

bool RMPG::Audio::IsInUse(SoundID sound)
{
	return sounds[sound]->IsInUse();
}

void RMPG::Audio::PlayAfx(EffectID effect)
{
	effects[effect]->Play(true);
}

void RMPG::Audio::Pause(EffectID effect)
{
	effects[effect]->Pause();
}

void RMPG::Audio::Resume(EffectID effect)
{
	effects[effect]->Resume();
}

void RMPG::Audio::Stop(EffectID effect, bool immediate)
{
	effects[effect]->Stop(immediate);
}

bool RMPG::Audio::IsLooped(EffectID effect)
{
	return effects[effect]->IsLooped();
}

void RMPG::Audio::SetEffectVolume(EffectID effect, float volume)
{
	effects[effect]->SetVolume(volume);
}
