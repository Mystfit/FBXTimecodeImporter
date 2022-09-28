// Copyright Epic Games, Inc. All Rights Reserved.

#include "FBXTimecodeImportCommands.h"
#include "LevelSequence.h"
#include "ILevelSequenceEditorToolkit.h"
#include "MovieSceneSection.h"

#define LOCTEXT_NAMESPACE "FFBXTimecodeImportModule"

void FFBXTimecodeImportCommands::RegisterCommands()
{
	//UI_COMMAND(CommandActionList, "SnapSectionSrcTimecode", "Snap section to source timecode", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(SnapSectionSrcTimecode, "Snap section to source timecode", "Move a section to the timecode specified in its SourceTimecode property", EUserInterfaceActionType::Button, FInputChord());

	// Action to open the TimecodeSynchronizerEditor
	CommandActionList = MakeShareable(new FUICommandList);

	CommandActionList->MapAction(
		SnapSectionSrcTimecode,
		FExecuteAction::CreateStatic(&FFBXTimecodeImportCommands::SnapSelectedSectionToSourceTimecode),
		FCanExecuteAction()
	);
}

void FFBXTimecodeImportCommands::SnapSelectedSectionToSourceTimecode(){

	TWeakPtr<ISequencer> WeakSequencer = FFBXTimecodeImportCommands::GetSequencer();
	if (!WeakSequencer.IsValid())
		return;

	if (auto Sequencer = WeakSequencer.Pin())
	{
		// Get all selected sections in this sequence
		TArray<UMovieSceneSection*> Sections;
		Sequencer->GetSelectedSections(Sections);
		for (auto Section : Sections) {
			// Flag section is being modified in the undo system
			Section->Modify();

			// Transform the timecode into a frame number based on the framerate of the section
			auto SectionFramerate = Section->GetTypedOuter<UMovieScene>()->GetTickResolution();
			auto StartFrame = Section->TimecodeSource.Timecode.ToFrameNumber(SectionFramerate);

			// Set the section's time range to the timecode's start frame + section length in frames
			TRange<FFrameNumber> FrameRange;
			FrameRange.SetLowerBound(FFrameNumber(StartFrame));
			FrameRange.SetUpperBound(FFrameNumber(StartFrame + (Section->GetAutoSizeRange()->GetUpperBoundValue() - Section->GetAutoSizeRange()->GetLowerBoundValue())));
			const FScopedTransaction Transaction(FText::FromString("Snapping section to source timecode"));
			Section->SetRange(FrameRange);
		}
	}
}

bool FFBXTimecodeImportCommands::CanSnapSelectedSectionToSourceTimecode() {
	TWeakPtr<ISequencer> WeakSequencer = FFBXTimecodeImportCommands::GetSequencer();
	if (!WeakSequencer.IsValid())
		return false;

	if (auto Sequencer = WeakSequencer.Pin())
	{
		// Get all selected sections in this sequence
		TArray<UMovieSceneSection*> Sections;
		Sequencer->GetSelectedSections(Sections);
		return Sections.Num() > 0;
	}

	return false;
}

TWeakPtr<ISequencer> FFBXTimecodeImportCommands::GetSequencer()
{
	auto AssetSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
	if (!AssetSubsystem)
		return nullptr;

	// Use all currently edited assets as a hint to find our sequence editor
	for (auto asset : AssetSubsystem->GetAllEditedAssets()) {
		if (asset->IsA<ULevelSequence>()) {
			// Get the sequence editor currently editing the asset - will also shift focus
			auto AssetEditor = AssetSubsystem->FindEditorForAsset(asset, true);
			if (!AssetEditor)
				continue;
			auto LevelSequenceEditor = static_cast<ILevelSequenceEditorToolkit*>(AssetEditor);
			return LevelSequenceEditor ? LevelSequenceEditor->GetSequencer() : nullptr;
		}
	}
	return nullptr;
}


#undef LOCTEXT_NAMESPACE
