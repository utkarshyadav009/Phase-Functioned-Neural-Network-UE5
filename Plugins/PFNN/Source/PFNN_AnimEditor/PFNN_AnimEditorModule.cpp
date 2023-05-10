#include "PFNN_AnimEditorModule.h"

DEFINE_LOG_CATEGORY(PFNN_AnimEditorModuleLog);

#define LOCTEXT_NAMESPACE "PFNN_AnimEditorModule"

void PFNN_AnimEditorModule::StartupModule()
{
	UE_LOG(PFNN_AnimEditorModuleLog, Log, TEXT("PFNN_AnimEditorModule module has started!"));
}

void PFNN_AnimEditorModule::ShutdownModule()
{
	UE_LOG(PFNN_AnimEditorModuleLog, Log, TEXT("PFNN_AnimEditorModule module has shut down"));
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(PFNN_AnimEditorModule, PFNN_AnimEditor)