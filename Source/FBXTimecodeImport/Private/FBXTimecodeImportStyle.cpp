// Copyright Epic Games, Inc. All Rights Reserved.

#include "FBXTimecodeImportStyle.h"
#include "FBXTimecodeImport.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/SlateStyleRegistry.h"
#include "Slate/SlateGameResources.h"
#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyleMacros.h"

#define RootToContentDir Style->RootToContentDir

TSharedPtr<FSlateStyleSet> FFBXTimecodeImportStyle::StyleInstance = nullptr;

void FFBXTimecodeImportStyle::Initialize()
{
	if (!StyleInstance.IsValid())
	{
		StyleInstance = Create();
		FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
	}
}

void FFBXTimecodeImportStyle::Shutdown()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
	ensure(StyleInstance.IsUnique());
	StyleInstance.Reset();
}

FName FFBXTimecodeImportStyle::GetStyleSetName()
{
	static FName StyleSetName(TEXT("FBXTimecodeImportStyle"));
	return StyleSetName;
}


const FVector2D Icon16x16(16.0f, 16.0f);
const FVector2D Icon20x20(20.0f, 20.0f);

TSharedRef< FSlateStyleSet > FFBXTimecodeImportStyle::Create()
{
	TSharedRef< FSlateStyleSet > Style = MakeShareable(new FSlateStyleSet("FBXTimecodeImportStyle"));
	Style->SetContentRoot(IPluginManager::Get().FindPlugin("FBXTimecodeImport")->GetBaseDir() / TEXT("Resources"));

	Style->Set("FBXTimecodeImport.SnapSectionSrcTimecode", new IMAGE_BRUSH_SVG(TEXT("Timecode"), Icon20x20));
	return Style;
}

void FFBXTimecodeImportStyle::ReloadTextures()
{
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
	}
}

const ISlateStyle& FFBXTimecodeImportStyle::Get()
{
	return *StyleInstance;
}
