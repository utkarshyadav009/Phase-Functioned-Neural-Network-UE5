#include "PFNNModule.h"

DEFINE_LOG_CATEGORY(PFNNModuleLog);

#define LOCTEXT_NAMESPACE "PFNNModule"

void PFNNModule::StartupModule()
{
	UE_LOG(PFNNModuleLog, Log, TEXT("PFNNModule module has started!"));
}

void PFNNModule::ShutdownModule()
{
	UE_LOG(PFNNModuleLog, Log, TEXT("PFNNModule module has shut down"));
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(PFNNModule, PFNN)