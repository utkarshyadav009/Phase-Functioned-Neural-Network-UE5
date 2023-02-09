// Copyright Epic Games, Inc. All Rights Reserved.

#include "PFNN_AnimationsEditor.h"

DEFINE_LOG_CATEGORY(PFNN_AnimationModuleLog);

#define LOCTEXT_NAMESPACE "FPFNN_AnimationsEditorModule"

void FPFNN_AnimationsEditorModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	UE_LOG(PFNN_AnimationModuleLog, Log, TEXT("PFNN_AnimationModule module has started!"));
}

void FPFNN_AnimationsEditorModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	UE_LOG(PFNN_AnimationModuleLog, Log, TEXT("PFNN_AnimationModule module has shut down"));
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FPFNN_AnimationsEditorModule, PFNN_AnimationsEditor)