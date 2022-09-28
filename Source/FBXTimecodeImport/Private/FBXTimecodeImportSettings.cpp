// Fill out your copyright notice in the Description page of Project Settings.


#include "FBXTimecodeImportSettings.h"

bool UFBXTimecodeImportSettings::IsAutoInjectTimecodeEnabled() const
{
	return AutoInjectTimecodeIntoAnimSequences;
}
