// Copyright Wacom. All Rights Reserved.

#include "ContentBuilders/SettingsAudioAssetBuilder.h"

#include "ContentBuilders/ContentBuilderHelpers.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundMix.h"

namespace Wacom::ContentBuilder
{
	namespace
	{
		constexpr const TCHAR* AudioSettingsRoot = TEXT("/Game/Wacom/Audio/Settings");

		template <typename T>
		T* CreateAudioAsset(const TCHAR* AssetName, UPackage*& OutPackage, FString& OutPackagePath)
		{
			OutPackagePath = MakePackagePath(AudioSettingsRoot, AssetName);
			OutPackage = FindOrCreatePackage(OutPackagePath);
			return OutPackage
				? CreateOrReplaceAsset<T>(OutPackage, FName(AssetName))
				: nullptr;
		}
	}

	bool BuildSettingsAudioAssets()
	{
		UPackage* MasterPackage = nullptr;
		UPackage* MusicPackage = nullptr;
		UPackage* SFXPackage = nullptr;
		UPackage* UIPackage = nullptr;
		UPackage* MixPackage = nullptr;
		FString MasterPath;
		FString MusicPath;
		FString SFXPath;
		FString UIPath;
		FString MixPath;

		USoundClass* Master = CreateAudioAsset<USoundClass>(
			TEXT("SC_Wacom_Master"), MasterPackage, MasterPath);
		USoundClass* Music = CreateAudioAsset<USoundClass>(
			TEXT("SC_Wacom_Music"), MusicPackage, MusicPath);
		USoundClass* SFX = CreateAudioAsset<USoundClass>(
			TEXT("SC_Wacom_SFX"), SFXPackage, SFXPath);
		USoundClass* UI = CreateAudioAsset<USoundClass>(
			TEXT("SC_Wacom_UI"), UIPackage, UIPath);
		USoundMix* Mix = CreateAudioAsset<USoundMix>(
			TEXT("SM_Wacom_UserSettings"), MixPackage, MixPath);
		if (!Master || !Music || !SFX || !UI || !Mix)
		{
			return false;
		}

		Master->Properties.Volume = 1.0f;
		Music->Properties.Volume = 1.0f;
		SFX->Properties.Volume = 1.0f;
		UI->Properties.Volume = 1.0f;
		Master->ChildClasses.Reset();
		Master->ChildClasses.Add(Music);
		Master->ChildClasses.Add(SFX);
		Master->ChildClasses.Add(UI);

		bool bSucceeded = true;
		bSucceeded &= SaveAssetPackage(MusicPackage, Music, MusicPath);
		bSucceeded &= SaveAssetPackage(SFXPackage, SFX, SFXPath);
		bSucceeded &= SaveAssetPackage(UIPackage, UI, UIPath);
		bSucceeded &= SaveAssetPackage(MasterPackage, Master, MasterPath);
		bSucceeded &= SaveAssetPackage(MixPackage, Mix, MixPath);
		return bSucceeded;
	}
}
