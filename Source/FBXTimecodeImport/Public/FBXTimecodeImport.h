// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "FbxImporter.h"

#define TC_HOUR_DEFAULT "TCHour"
#define TC_MINUTE_DEFAULT "TCMinute"
#define TC_SECOND_DEFAULT "TCSecond"
#define TC_FRAME_DEFAULT "TCFrame"
#define TC_SUBFRAME_DEFAULT "TCSubframe"
#define TC_RATE_DEFAULT "TCRate"

DECLARE_LOG_CATEGORY_EXTERN(LogFBXTimecodeImport, Log, All);

class FToolBarBuilder;
class FMenuBuilder;

class FFBXTimecodeImportModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	bool HandleSettingsSaved();
	void BindAssetImportEvents();

	// Set up UI
	void FillToolbar(FToolBarBuilder& ToolbarBuilder);
	TSharedRef<SWidget> MakeTimecodeToolsMenu();

	// Handle asset imports
	void OnObjectImported(UFactory* ImportFactory, UObject* InObject);
	void OnObjectReimported(UObject* InObject);
	void HandleAssetImport(UObject* InObject);

	// Check if the given sequence contains all timecode attributes
	bool FBXTIMECODEIMPORT_API AnimSequenceContainsTimecodeAttrs(UAnimSequence* sequence);

	// Inject timecode attributes into a sequence if they are missing
	void FBXTIMECODEIMPORT_API InjectTimecodeIntoSequence(UAnimSequence* sequence, FTimecode StartTimecode, FTimecode EndTimecode, FFrameRate framerate);

private:
	FTimecode FBXTimeToFTimecode(const FbxTime& time, FbxTime::EMode mode = FbxTime::EMode::eDefaultMode);
	const TMap<FName, FName> GetTimecodeBoneAttrNames();
	int32 GetTimecodeValueFromBoneAttrName(FName TCFieldName, FTimecode Timecode);

private:
	TSharedPtr<class FUICommandList> PluginCommands;

	FDelegateHandle FBXReimportDelegateHandle;
};
