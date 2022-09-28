// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "FBXTimecodeImportSettings.generated.h"

/**
 * 
 */
UCLASS(config = Engine)
class FBXTIMECODEIMPORT_API UFBXTimecodeImportSettings : public UObject
{
	GENERATED_BODY()

public:
	/** Returns true if we are to automatically inject timecode attributes into imported FBX animation sequences */
	bool IsAutoInjectTimecodeEnabled() const;
private:

	/** Automatically inject timecode attributes into the root bone of imported animated sequences. The timecode will be based upon from the local timespan present in the imported FBX file.*/
	UPROPERTY(config, EditAnywhere, Category = "FBX Timecode Import")
	bool AutoInjectTimecodeIntoAnimSequences;
};
