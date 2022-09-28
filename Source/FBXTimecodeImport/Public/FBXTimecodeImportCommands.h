// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "ISequencer.h"
#include "FBXTimecodeImportStyle.h"

class FFBXTimecodeImportCommands : public TCommands<FFBXTimecodeImportCommands>
{
public:

	FFBXTimecodeImportCommands()
		: TCommands<FFBXTimecodeImportCommands>(TEXT("FBXTimecodeImport"), NSLOCTEXT("Contexts", "FBXTimecodeImport", "FBXTimecodeImport Plugin"), NAME_None, FFBXTimecodeImportStyle::GetStyleSetName())
	{
	}

	// TCommands<> interface
	virtual void RegisterCommands() override;

	// Command definitions
	static void SnapSelectedSectionToSourceTimecode();
	static bool CanSnapSelectedSectionToSourceTimecode();

	static TWeakPtr<ISequencer> GetSequencer();

public:
	TSharedPtr<FUICommandInfo> SnapSectionSrcTimecode;
	TSharedPtr<class FUICommandList> CommandActionList;
};
