#pragma once

#include "Framework/Commands/Commands.h"
#include "Framework/Commands/UICommandInfo.h"

class FMyPluginCommands : public TCommands<FMyPluginCommands>
{
public:
    FMyPluginCommands()
        : TCommands<FMyPluginCommands>(
            TEXT("MySlatePlugin"),           // Context 名稱（唯一識別）
            NSLOCTEXT("Contexts", "MySlatePlugin", "My Slate Plugin"),
            NAME_None,                       // 父 Context（無）
            FAppStyle::GetAppStyleSetName()  // 使用 Editor 預設圖示集
        )
    {
    }

    // 每個動作宣告一個 TSharedPtr<FUICommandInfo>
    TSharedPtr<FUICommandInfo> MyAction;

    // TCommands 要求實作這個
    virtual void RegisterCommands() override;
};