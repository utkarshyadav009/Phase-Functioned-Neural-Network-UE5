#pragma once

#include "Modules/ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(PFNN_AnimEditorModuleLog, All, All);

class PFNN_AnimEditorModule : public IModuleInterface
{
public:

	/* This will get called when the editor loads the module */
	virtual void StartupModule() override;

	/* This will get called when the editor unloads the module */
	virtual void ShutdownModule() override;
};