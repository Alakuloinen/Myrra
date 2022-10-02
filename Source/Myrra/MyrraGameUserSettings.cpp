// Fill out your copyright notice in the Description page of Project Settings.


#include "MyrraGameUserSettings.h"

int32 UMyrraGameUserSettings::GetOption(const EMyrOptions O) const
{
	switch (O)
	{
		case EMyrOptions::VSync:		return (int32)IsVSyncEnabled();
		case EMyrOptions::Screen:		return (int32)GetFullscreenMode();
		case EMyrOptions::Antialiasing:	return (int32)GetAntiAliasingQuality();
		case EMyrOptions::Shading:		return (int32)GetShadingQuality();
		case EMyrOptions::Shadows:		return (int32)GetShadowQuality();
		case EMyrOptions::Textures:		return (int32)GetTextureQuality();
		case EMyrOptions::ViewDist:		return (int32)GetViewDistanceQuality();
		case EMyrOptions::VisualEffects:return (int32)GetVisualEffectQuality();
		case EMyrOptions::PostProc:		return (int32)GetPostProcessingQuality();
		case EMyrOptions::FrameRate:
			switch ((int)GetFrameRateLimit())
			{	case 30: return 0;
				case 60: return 1;
				case 120: return 2;
				case 0: return 3;
			}
		case EMyrOptions::SoundAmbient: return (int32)SoundAmbient;
		case EMyrOptions::SoundSubjective: return (int32)SoundSubjective;
		case EMyrOptions::SoundNoises:	return (int32)SoundNoises;
		case EMyrOptions::SoundVoice:	return (int32)SoundVoice;
		case EMyrOptions::SoundMusic:	return (int32)SoundMusic;
	}
	return 0;
}

void UMyrraGameUserSettings::SetOption(EMyrOptions O, int V)
{
	switch (O)
	{
		case EMyrOptions::VSync:		SetVSyncEnabled((bool)V);					break;
		case EMyrOptions::Screen:		SetFullscreenMode((EWindowMode::Type)V);	break;
		case EMyrOptions::Antialiasing:	SetAntiAliasingQuality(V);					break;
		case EMyrOptions::Shading:		SetShadingQuality(V);						break;
		case EMyrOptions::Shadows:		SetShadowQuality(V);						break;
		case EMyrOptions::Textures:		SetTextureQuality(V);						break;
		case EMyrOptions::ViewDist:		SetViewDistanceQuality(V);					break;
		case EMyrOptions::VisualEffects:SetVisualEffectQuality(V);					break;
		case EMyrOptions::PostProc:		SetPostProcessingQuality(V);				break;
		case EMyrOptions::FrameRate:
			switch (V)
			{	case 0: SetFrameRateLimit(30.0f); break;
				case 1: SetFrameRateLimit(60.0f); break;
				case 2: SetFrameRateLimit(120.0f); break;
				case 3: SetFrameRateLimit(0.0f); break;
			} break;
		case EMyrOptions::SoundAmbient: SoundAmbient = V; break;
		case EMyrOptions::SoundSubjective: SoundSubjective = V; break;
		case EMyrOptions::SoundNoises:	SoundNoises = V; break;
		case EMyrOptions::SoundVoice:	SoundVoice = V; break;
		case EMyrOptions::SoundMusic:	SoundMusic = V; break;
	}
}
