// Copyright Epic Games, Inc. All Rights Reserved.

#include "MySlatePlugin.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "ToolMenus.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Input/SCheckBox.h"
#include "MyCommands.h"


static const FName MyTabName("MySlatePlugin");

#define LOCTEXT_NAMESPACE "FMySlatePluginModule"

void FMySlatePluginModule::StartupModule()
{
    FMyPluginCommands::Register();

    PluginCommands = MakeShareable(new FUICommandList);
    PluginCommands->MapAction(
        FMyPluginCommands::Get().MyAction,
        FExecuteAction::CreateLambda([]()
            {
                UE_LOG(LogTemp, Warning, TEXT("MyAction triggered via Ctrl+M"));
            }),
        FCanExecuteAction()
    );

    FLevelEditorModule& LevelEditorModule =
        FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
    LevelEditorModule.GetGlobalLevelEditorActions()->Append(PluginCommands.ToSharedRef());

    // StartupModule 的 Append 之後加：
    OnMapOpenedHandle = FEditorDelegates::OnMapOpened.AddLambda(
        [this](const FString&, bool)
        {
            FLevelEditorModule& LEModule =
                FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
            LEModule.GetGlobalLevelEditorActions()->Append(PluginCommands.ToSharedRef());
        }
    );

    FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
        MyTabName,
        FOnSpawnTab::CreateRaw(this, &FMySlatePluginModule::OnSpawnPluginTab)
    )
        .SetDisplayName(LOCTEXT("TabTitle", "My Slate Plugin"))
        .SetMenuType(ETabSpawnerMenuType::Hidden);

    UToolMenus::RegisterStartupCallback(
        FSimpleMulticastDelegate::FDelegate::CreateRaw(
            this, &FMySlatePluginModule::RegisterMenus
        )
    );
}


void FMySlatePluginModule::ShutdownModule()
{
    FMyPluginCommands::Unregister();
    // 一定要 Unregister，否則 Editor 重載 Plugin 會崩潰
    // ShutdownModule 加：
    FEditorDelegates::OnMapOpened.Remove(OnMapOpenedHandle);
    FMyPluginCommands::Unregister();
    UToolMenus::UnRegisterStartupCallback(this);
    UToolMenus::UnregisterOwner(this);

    FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(MyTabName);
}

TSharedRef<SDockTab> FMySlatePluginModule::OnSpawnPluginTab(
    const FSpawnTabArgs& SpawnTabArgs)
{
    return SNew(SDockTab)
        .TabRole(ETabRole::NomadTab)
        [
            SNew(SVerticalBox)
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(10.f)
                [
                    SNew(SEditableTextBox)
                        .HintText(LOCTEXT("Hint", "Type here..."))
                        .OnTextChanged_Lambda([this](const FText& NewText)
                            {
                                InputText = NewText;
                            })
                ]
                + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(10.f)
                    [
                        SNew(SButton)
                            .Text(LOCTEXT("ButtonLabel", "Print Text"))
                            .OnClicked(FOnClicked::CreateLambda([this]() -> FReply
                                {
                                    UE_LOG(LogTemp, Warning, TEXT("Input: %s"), *InputText.ToString());

                                    return FReply::Handled();
                                }))
                    ]
                + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(10.f)
                    [
                        SNew(SSearchBox)
                            .OnTextChanged_Lambda([this](const FText& NewText)
                                {
                                    UE_LOG(LogTemp, Warning, TEXT("Searching %s"), *NewText.ToString());
                            })
                    ]
                + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(10.f)
                    [
                        SNew(SCheckBox)
                            .OnCheckStateChanged(FOnCheckStateChanged::CreateLambda([this](ECheckBoxState NewState)
                                {
                                    if (NewState == ECheckBoxState::Checked)
                                    {
                                        UE_LOG(LogTemp, Warning, TEXT("Is Checked"))
                                    }
                                    else
                                    {
                                        UE_LOG(LogTemp, Warning, TEXT("Is UnChecked"))
                                    }
                                }))
                    ]
        ];
}

void FMySlatePluginModule::RegisterMenus()
{
    FToolMenuOwnerScoped OwnerScoped(this);

    if (UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("MainFrame.MainMenu.Window"))
    {
        FToolMenuSection& Section = Menu->FindOrAddSection("WindowLayout");

        Section.AddMenuEntry(
            "MySlatePluginEntry",   // Entry 名稱（唯一識別）
            LOCTEXT("MenuLabel", "Open My Slate Plugin"),
            LOCTEXT("MenuTooltip", "Opens the My Slate Plugin window"),
            FSlateIcon(),
            FUIAction(
                FExecuteAction::CreateLambda([]()
                    {
                        FGlobalTabmanager::Get()->TryInvokeTab(MyTabName);
                    })
            )
        );
    }
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FMySlatePluginModule, MySlatePlugin)