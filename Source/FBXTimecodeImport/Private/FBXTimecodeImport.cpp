// Copyright Epic Games, Inc. All Rights Reserved.

#include "FBXTimecodeImport.h"
#include "FBXTimecodeImportStyle.h"
#include "FBXTimecodeImportCommands.h"

#if WITH_EDITOR
#include "ISettingsModule.h"
#include "ISettingsSection.h"
#endif

#include "Animation/BuiltInAttributeTypes.h"
#include "Animation/AnimationSettings.h"
#include "ISequencerModule.h"
#include "Misc/MessageDialog.h"
#include "ToolMenus.h"
#include "FBXTimecodeImportSettings.h"

static const FName FBXTimecodeImportTabName("FBXTimecodeImport");

#define LOCTEXT_NAMESPACE "FFBXTimecodeImportModule"

void FFBXTimecodeImportModule::StartupModule()
{
	FFBXTimecodeImportStyle::Initialize();
	FFBXTimecodeImportStyle::ReloadTextures();
	FFBXTimecodeImportCommands::Register();

	// Setup asset import events
	BindAssetImportEvents();

	// Create Toolbar extension for the sequencer
	TSharedPtr<FExtender> SequencerToolbarExtender = MakeShareable(new FExtender);
	SequencerToolbarExtender->AddToolBarExtension(
		"Curve Editor",
		EExtensionHook::After,
		FFBXTimecodeImportCommands::Get().CommandActionList,
		FToolBarExtensionDelegate::CreateRaw(this, &FFBXTimecodeImportModule::FillToolbar)
	);

	// Add our timecode buttons to the sequencer window
	ISequencerModule& SequencerModule = FModuleManager::LoadModuleChecked<ISequencerModule>("Sequencer");
	SequencerModule.GetToolBarExtensibilityManager()->AddExtender(SequencerToolbarExtender);

#if WITH_EDITOR
	// register settings
	ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings");

	if (SettingsModule != nullptr)
	{
		ISettingsSectionPtr SettingsSection = SettingsModule->RegisterSettings("Project", "Plugins", "FBXTimecode",
			LOCTEXT("FBXTimecodeSettingsName", "FBX Timecode importing"),
			LOCTEXT("FBXTimecodeSettingsDescription", "Configure the FBX Timecode plug-in."),
			GetMutableDefault<UFBXTimecodeImportSettings>()
		);

		if (SettingsSection.IsValid())
		{
			SettingsSection->OnModified().BindRaw(this, &FFBXTimecodeImportModule::HandleSettingsSaved);
		}
	}
#endif // WITH_EDITOR
}

bool FFBXTimecodeImportModule::HandleSettingsSaved()
{
	BindAssetImportEvents();
	return true;
}

void FFBXTimecodeImportModule::BindAssetImportEvents() 
{
	if (GetDefault<UFBXTimecodeImportSettings>()->IsAutoInjectTimecodeEnabled()) 
	{
		if (!FBXReimportDelegateHandle.IsValid())
			FBXReimportDelegateHandle = GEditor->GetEditorSubsystem<UImportSubsystem>()->OnAssetReimport.AddRaw(this, &FFBXTimecodeImportModule::OnObjectReimported);
		
		UE_LOG(LogFBXTimecodeImport, Log, TEXT("FBX timecode injector listening for incoming imported animation sequences"));
	}
	else {
		if (FBXReimportDelegateHandle.IsValid()) {
			GEditor->GetEditorSubsystem<UImportSubsystem>()->OnAssetReimport.Remove(FBXReimportDelegateHandle);
			FBXReimportDelegateHandle.Reset();
		}
		UE_LOG(LogFBXTimecodeImport, Log, TEXT("FBX timecode injector not listening for incoming imported animation sequences"));
	}
}

void FFBXTimecodeImportModule::FillToolbar(FToolBarBuilder& ToolbarBuilder)
{
	ToolbarBuilder.BeginSection("TimecodeTools");
	{
		ToolbarBuilder.AddComboButton(
			FUIAction(),
			FOnGetContent::CreateRaw(this, &FFBXTimecodeImportModule::MakeTimecodeToolsMenu),
			LOCTEXT("TimecodeTools", "Timecode Tools"),
			FText::FromString("Timecode tools"),
			FFBXTimecodeImportCommands::Get().SnapSectionSrcTimecode->GetIcon());
	}
	ToolbarBuilder.EndSection();
}

TSharedRef<SWidget> FFBXTimecodeImportModule::MakeTimecodeToolsMenu()
{
	TWeakPtr<ISequencer> WeakSequencer = FFBXTimecodeImportCommands::GetSequencer();
	auto SequencerPtr = WeakSequencer.Pin();

	const bool bShouldCloseWindowAfterMenuSelection = true;
	FMenuBuilder MenuBuilder(bShouldCloseWindowAfterMenuSelection, FFBXTimecodeImportCommands::Get().CommandActionList);

	MenuBuilder.AddMenuEntry(FFBXTimecodeImportCommands::Get().SnapSectionSrcTimecode);
	return MenuBuilder.MakeWidget();
}

void FFBXTimecodeImportModule::ShutdownModule()
{
	UToolMenus::UnRegisterStartupCallback(this);
	UToolMenus::UnregisterOwner(this);

	FFBXTimecodeImportStyle::Shutdown();
	FFBXTimecodeImportCommands::Unregister();
}

void FFBXTimecodeImportModule::OnObjectReimported(UObject* InObject)
{
	HandleAssetImport(InObject);
}

bool FFBXTimecodeImportModule::AnimSequenceContainsTimecodeAttrs(UAnimSequence* sequence)
{
	if (!sequence)
		return false;

	const int32 RootBoneTrackIndex = 0;
	const FBoneAnimationTrack* RootBoneTrack = sequence->GetDataModel()->FindBoneTrackByIndex(RootBoneTrackIndex);
	const FName& RootBoneName = RootBoneTrack->Name;

	TArray<const FAnimatedBoneAttribute*> BoneAttrs;
	sequence->GetDataModel()->GetAttributesForBone(RootBoneName, BoneAttrs);

	// Our sequence contains valid TC attributes only if EVERY TC attribute is present
	bool found_valid = true;
	auto ValidBoneAttrNames = GetTimecodeBoneAttrNames();
	for (auto Attr : ValidBoneAttrNames) {
		auto BoneAttr = BoneAttrs.FindByPredicate([Attr](const FAnimatedBoneAttribute* BoneAttr) { return BoneAttr->Identifier.GetName() == Attr.Value; });
		found_valid &= (BoneAttr != nullptr);
	}

	return found_valid;
}

void FFBXTimecodeImportModule::HandleAssetImport(UObject* InObject)
{
	const UFBXTimecodeImportSettings* Settings = GetDefault<UFBXTimecodeImportSettings>();
	if (!Settings->IsAutoInjectTimecodeEnabled())
		return;

	if (UAnimSequence* ImportedSeq = Cast<UAnimSequence>(InObject)) {
		// Don't need to inject timecode attrs if they're already present in the imported sequence
		if (AnimSequenceContainsTimecodeAttrs(ImportedSeq)) {
			UE_LOG(LogFBXTimecodeImport, Warning, TEXT("Sequence already contains timecode attributes. Overwriting"));
		}

		// Re-import the source FBX file to grab the timecodes
		auto source_path = ImportedSeq->AssetImportData->GetFirstFilename();
		UnFbx::FFbxImporter* FbxImporter = UnFbx::FFbxImporter::GetInstance();
		const FString FileExtension = FPaths::GetExtension(source_path);
		if (!FbxImporter->ImportFromFile(*source_path, FileExtension, true))
		{
			FbxImporter->ReleaseScene();
			return;
		}

		// Get timecode data from FBX
		FbxAnimStack* AnimStack = FbxImporter->Scene->GetMember<FbxAnimStack>(0);
		FbxTimeSpan FBXTimeSpan = AnimStack->GetLocalTimeSpan();
		auto FBXStart = FBXTimeSpan.GetStart();
		auto FBXEnd = FBXTimeSpan.GetStop();

		// Save timecode as attributes in anim sequence
		InjectTimecodeIntoSequence(ImportedSeq, FBXTimeToFTimecode(FBXStart), FBXTimeToFTimecode(FBXEnd), FFrameRate(ImportedSeq->ImportFileFramerate, 1));
		
		// Cleanup
		FbxImporter->ReleaseScene();
	}
}

FTimecode FFBXTimecodeImportModule::FBXTimeToFTimecode(const FbxTime& time, FbxTime::EMode mode)
{
	int Hour, Min, Sec, Frame, Field, Residual;
	time.GetTime(Hour, Min, Sec, Frame, Field, Residual, mode);
	return FTimecode(Hour, Min, Sec, Frame, time.IsDropFrame());
}

int32 FFBXTimecodeImportModule::GetTimecodeValueFromBoneAttrName(FName TCFieldName, FTimecode Timecode)
{
	auto TimecodeNames = GetTimecodeBoneAttrNames();
	if (TCFieldName == *TimecodeNames.Find(FName(TEXT(TC_HOUR_DEFAULT))))
		return Timecode.Hours;
	else if (TCFieldName == *TimecodeNames.Find(FName(TEXT(TC_MINUTE_DEFAULT))))
		return Timecode.Minutes;
	else if (TCFieldName == *TimecodeNames.Find(FName(TEXT(TC_SECOND_DEFAULT))))
		return Timecode.Seconds;
	else if (TCFieldName == *TimecodeNames.Find(FName(TEXT(TC_FRAME_DEFAULT))))
		return Timecode.Frames;
	else if (TCFieldName == *TimecodeNames.Find(FName(TEXT(TC_SUBFRAME_DEFAULT))))
		return 0;
	return 0;
}

void FFBXTimecodeImportModule::InjectTimecodeIntoSequence(UAnimSequence* Sequence, FTimecode StartTimecode, FTimecode EndTimecode, FFrameRate Framerate)
{
	auto& Controller = Sequence->GetController();
	const int32 RootBoneTrackIndex = 0;
	const FBoneAnimationTrack* RootBoneTrack = Sequence->GetDataModel()->FindBoneTrackByIndex(RootBoneTrackIndex);
	const FName& RootBoneName = RootBoneTrack->Name;
	auto TimecodeBoneAttributeNames = GetTimecodeBoneAttrNames();
	
	// We remove the rate attribute from the list since it will be manually added later as a float attribute instead of an integer
	FName RateAttrName;
	TimecodeBoneAttributeNames.RemoveAndCopyValue(FName(TEXT(TC_RATE_DEFAULT)), RateAttrName);

	TArray<float> TCKeyTimes;
	TArray<FIntegerAnimationAttribute> TCFramerateKeyValues;
	FTimecode CurrentTimecode;

	// Only add valid bone attribute names
	for (auto AttrName : TimecodeBoneAttributeNames) {
		TArray<FIntegerAnimationAttribute> TCKeyValues;

		// Create a new integer attribute identifier to hold our attribute ID and data
		FAnimationAttributeIdentifier Identifier = UAnimationAttributeIdentifierExtensions::CreateAttributeIdentifier(Sequence, AttrName.Value, RootBoneName, FIntegerAnimationAttribute::StaticStruct());
		Controller.AddAttribute(Identifier);

		FFrameNumber Duration = EndTimecode.ToFrameNumber(Framerate) - StartTimecode.ToFrameNumber(Framerate);
		
		for (int FrameNum = 0; FrameNum < Duration; ++FrameNum) {
			// Calculate incremented timecode using the framerate from the timecode source
			CurrentTimecode = FTimecode::FromFrameNumber(FrameNum, Framerate);
			auto OffsetTimecode = FTimecode::FromFrameNumber(StartTimecode.ToFrameNumber(Framerate) + FrameNum, Framerate);
			auto OffsetTimespan = CurrentTimecode.ToTimespan(Framerate);

			// Only add frame timings during the first attribute pass
			if (TCKeyTimes.Num() < Duration) {
				TCKeyTimes.Add(OffsetTimespan.GetTotalMilliseconds() / 1000.0f);
			}
			
			// Calculate frame-incremented timecode
			FIntegerAnimationAttribute Attr;
			FIntegerAnimationAttribute RateAttr;

			Attr.Value = GetTimecodeValueFromBoneAttrName(AttrName.Value, OffsetTimecode);
			RateAttr.Value = Framerate.AsDecimal();

			TCKeyValues.Add(Attr);
			TCFramerateKeyValues.Add(RateAttr);
		}
			
		// Timecode values are saved in a single key at the start of the sequence
		Controller.SetTypedAttributeKeys<FIntegerAnimationAttribute>(Identifier, TCKeyTimes, TCKeyValues);
	};

	// Add float attributes seperately
	FAnimationAttributeIdentifier Identifier = UAnimationAttributeIdentifierExtensions::CreateAttributeIdentifier(Sequence, RateAttrName, RootBoneName, FIntegerAnimationAttribute::StaticStruct());
	Controller.AddAttribute(Identifier);
	Controller.SetTypedAttributeKeys<FIntegerAnimationAttribute>(Identifier, TCKeyTimes, TCFramerateKeyValues);

	UE_LOG(LogFBXTimecodeImport, Log, TEXT("Injected timecode keys starting from %s from imported FBX animation into %s"), *StartTimecode.ToString(), *Sequence->GetFullName());
}

const TMap<FName, FName> FFBXTimecodeImportModule::GetTimecodeBoneAttrNames()
{
	// Get timecode custom attribute names
	FName TCHourAttrName(TEXT(TC_HOUR_DEFAULT));
	FName TCMinuteAttrName(TEXT(TC_MINUTE_DEFAULT));
	FName TCSecondAttrName(TEXT(TC_SECOND_DEFAULT));
	FName TCFrameAttrName(TEXT(TC_FRAME_DEFAULT));
	FName TCSubframeAttrName(TEXT(TC_SUBFRAME_DEFAULT));
	FName TCRateAttrName(TEXT(TC_RATE_DEFAULT));

	// Get custom timecode attribute names if set in the editor settings
	if (const UAnimationSettings* AnimationSettings = UAnimationSettings::Get())
	{
		TCHourAttrName = AnimationSettings->BoneTimecodeCustomAttributeNameSettings.HourAttributeName;
		TCMinuteAttrName = AnimationSettings->BoneTimecodeCustomAttributeNameSettings.MinuteAttributeName;
		TCSecondAttrName = AnimationSettings->BoneTimecodeCustomAttributeNameSettings.SecondAttributeName;
		TCFrameAttrName = AnimationSettings->BoneTimecodeCustomAttributeNameSettings.FrameAttributeName;
		TCSubframeAttrName = AnimationSettings->BoneTimecodeCustomAttributeNameSettings.SubframeAttributeName;
		TCRateAttrName = AnimationSettings->BoneTimecodeCustomAttributeNameSettings.RateAttributeName;
	}
	return TMap<FName, FName> {
		{ FName(TEXT(TC_HOUR_DEFAULT)), TCHourAttrName},
		{ FName(TEXT(TC_MINUTE_DEFAULT)), TCMinuteAttrName },
		{ FName(TEXT(TC_SECOND_DEFAULT)), TCSecondAttrName },
		{ FName(TEXT(TC_FRAME_DEFAULT)), TCFrameAttrName },
		{ FName(TEXT(TC_SUBFRAME_DEFAULT)), TCSubframeAttrName },
		{ FName(TEXT(TC_RATE_DEFAULT)), TCRateAttrName }
	};
}



#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FFBXTimecodeImportModule, FBXTimecodeImport)

DEFINE_LOG_CATEGORY(LogFBXTimecodeImport);
