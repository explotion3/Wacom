// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "WacomSettingsDeveloperSettings.generated.h"

class USoundClass;
class USoundMix;

/** Asset references used to apply local audio settings without coupling gameplay assets to settings code. */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Wacom Local Settings"))
class WACOMAPP_API UWacomSettingsDeveloperSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	virtual FName GetCategoryName() const override { return TEXT("Wacom"); }

	UPROPERTY(Config, EditAnywhere, Category = "Audio", meta = (ToolTip = "项目默认主音频 SoundClass。未单独分类的项目声音至少应通过该总线受主音量控制。"))
	TSoftObjectPtr<USoundClass> MasterSoundClass;

	UPROPERTY(Config, EditAnywhere, Category = "Audio", meta = (ToolTip = "音乐音频 SoundClass，必须作为主音频 SoundClass 的子类。"))
	TSoftObjectPtr<USoundClass> MusicSoundClass;

	UPROPERTY(Config, EditAnywhere, Category = "Audio", meta = (ToolTip = "战斗与世界音效 SoundClass，必须作为主音频 SoundClass 的子类。"))
	TSoftObjectPtr<USoundClass> SFXSoundClass;

	UPROPERTY(Config, EditAnywhere, Category = "Audio", meta = (ToolTip = "界面音效 SoundClass，必须作为主音频 SoundClass 的子类。"))
	TSoftObjectPtr<USoundClass> UISoundClass;

	UPROPERTY(Config, EditAnywhere, Category = "Audio", meta = (ToolTip = "本地音量覆盖使用的基础 SoundMix。"))
	TSoftObjectPtr<USoundMix> UserSettingsSoundMix;
};
