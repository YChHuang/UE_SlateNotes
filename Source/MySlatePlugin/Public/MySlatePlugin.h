// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"
#include "Framework/Commands/Commands.h"
#include "MyCommands.h"
#include "LevelEditor.h"
#include "Framework/Commands/UICommandList.h"

class FMySlatePluginModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
private:
	// 用來生成 Tab 內容的 callback
	TSharedRef<SDockTab> OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs);
	void RegisterMenus();

	FText InputText;
	bool isCheck = false;
	FDelegateHandle OnMapOpenedHandle;
	TSharedPtr<FUICommandList> CommandList;
	TSharedPtr<FUICommandList> PluginCommands;
};
