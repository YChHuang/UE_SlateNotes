# 第四章：快捷鍵綁定 × TCommands × FUICommandList

[← 回目錄](../README.md)

---

## 4-1 整體流程

```
新增 MyCommands.h / MyCommands.cpp
  └→ 繼承 TCommands<T>，宣告 TSharedPtr<FUICommandInfo> 成員
       └→ RegisterCommands() 裡用 UI_COMMAND 定義名稱與快捷鍵

StartupModule()
  ├→ FMyPluginCommands::Register()        ← 把所有 Command 註冊進系統
  ├→ FUICommandList::MapAction()          ← 把 Command 和 lambda 綁在一起
  ├→ GetGlobalLevelEditorActions()->Append ← 把 CommandList 掛進 Level Editor
  └→ FEditorDelegates::OnMapOpened        ← 每次開新 Level 重新 Append

ShutdownModule()
  ├→ FMyPluginCommands::Unregister()
  └→ FEditorDelegates::OnMapOpened.Remove(Handle)
```

---

## 4-2 為什麼要拆出 MyCommands 檔案？

UE 的快捷鍵系統要求每個「動作」都要先向系統登記（`RegisterCommands`），
登記後才能：

- 出現在 Editor Preferences → Keyboard Shortcuts 讓使用者自訂
- 被 `FUICommandList` 綁定到實際的執行函式

`TCommands<T>` 是 UE 提供的 CRTP 基底類別，負責管理這些 Command 的生命週期，
獨立成一個檔案是標準做法，避免主模組過於龐大。

---

## 4-3 MyCommands.h

```cpp
#pragma once

#include "Framework/Commands/Commands.h"
#include "Framework/Commands/UICommandInfo.h"

class FMyPluginCommands : public TCommands<FMyPluginCommands>
{
public:
    FMyPluginCommands()
        : TCommands<FMyPluginCommands>(
            TEXT("MySlatePlugin"),
            NSLOCTEXT("Contexts", "MySlatePlugin", "My Slate Plugin"),
            NAME_None,
            FAppStyle::GetAppStyleSetName()
        )
    {}

    // 每個動作宣告一個成員變數
    TSharedPtr<FUICommandInfo> MyAction;

    virtual void RegisterCommands() override;
};
```

---

## 4-4 MyCommands.cpp

```cpp
#include "MyCommands.h"

#define LOCTEXT_NAMESPACE "FMyPluginCommands"

void FMyPluginCommands::RegisterCommands()
{
    UI_COMMAND(
        MyAction,
        "My Action",
        "Prints a log message",
        EUserInterfaceActionType::Button,
        FInputChord(EModifierKey::Control | EModifierKey::Shift, EKeys::J)
    );
}

#undef LOCTEXT_NAMESPACE
```

### UI_COMMAND 的五個參數

| 位置 | 說明 |
|---|---|
| 1 | 要填充的 `TSharedPtr<FUICommandInfo>` 成員 |
| 2 | 顯示名稱（出現在 Keyboard Shortcuts 列表） |
| 3 | Tooltip |
| 4 | `EUserInterfaceActionType`：`Button` / `ToggleButton` / `RadioButton` |
| 5 | `FInputChord`：修飾鍵 + 主鍵組合 |

### FInputChord 的修飾鍵寫法

```cpp
FInputChord(EModifierKey::Control, EKeys::M)                          // Ctrl+M
FInputChord(EModifierKey::Control | EModifierKey::Shift, EKeys::J)    // Ctrl+Shift+J
FInputChord(EModifierKey::Alt, EKeys::F5)                             // Alt+F5
```

---

## 4-5 主模組整合

### MySlatePlugin.h 需要新增的成員變數

```cpp
#include "Framework/Commands/UICommandList.h"

private:
    TSharedPtr<FUICommandList> PluginCommands;
    FDelegateHandle OnMapOpenedHandle;
```

### StartupModule() 的快捷鍵部分

```cpp
#include "MyCommands.h"
#include "LevelEditor.h"
#include "Editor.h"

void FMySlatePluginModule::StartupModule()
{
    // 1. 註冊 Command
    FMyPluginCommands::Register();

    // 2. 建立 CommandList，把 Command 和 lambda 綁在一起
    PluginCommands = MakeShareable(new FUICommandList);
    PluginCommands->MapAction(
        FMyPluginCommands::Get().MyAction,
        FExecuteAction::CreateLambda([]()
        {
            UE_LOG(LogTemp, Warning, TEXT("MyAction triggered!"));
        }),
        FCanExecuteAction()
    );

    // 3. 掛進 Level Editor 全域快捷鍵
    FLevelEditorModule& LevelEditorModule =
        FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
    LevelEditorModule.GetGlobalLevelEditorActions()->Append(PluginCommands.ToSharedRef());

    // 4. 監聽 Level 切換，重新 Append（否則開新 Level 後快捷鍵失效）
    OnMapOpenedHandle = FEditorDelegates::OnMapOpened.AddLambda(
        [this](const FString&, bool)
        {
            FLevelEditorModule& LEModule =
                FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
            LEModule.GetGlobalLevelEditorActions()->Append(PluginCommands.ToSharedRef());
        }
    );

    // ... RegisterNomadTabSpawner、RegisterStartupCallback ...
}
```

### ShutdownModule()

```cpp
void FMySlatePluginModule::ShutdownModule()
{
    FMyPluginCommands::Unregister();
    FEditorDelegates::OnMapOpened.Remove(OnMapOpenedHandle);

    UToolMenus::UnRegisterStartupCallback(this);
    UToolMenus::UnregisterOwner(this);
    FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(MyTabName);
}
```

---

## 4-6 Build.cs 需要的模組

```csharp
PrivateDependencyModuleNames.AddRange(new string[]
{
    // ... 原有模組 ...
    "InputCore",   // EKeys 定義在這裡
});
```

`EKeys::M`、`EKeys::J` 等按鍵常數都來自 `InputCore`，忘記加會出現 LNK2019 連結錯誤。

---

## 4-7 為什麼開新 Level 後快捷鍵會失效？

`GetGlobalLevelEditorActions()` 回傳的 CommandList 在切換 Level 時會被重建，
原本 Append 進去的內容跟著消失。

解法是監聽 `FEditorDelegates::OnMapOpened`，每次開新 Level 後重新 Append：

```
StartupModule → Append PluginCommands → 快捷鍵生效
使用者開新 Level → GlobalLevelEditorActions 重建 → 快捷鍵消失
  └→ OnMapOpened 觸發 → 重新 Append → 快捷鍵恢復
```

監聽的 delegate 要在 ShutdownModule 用 `FDelegateHandle` 移除，否則 Plugin 卸載後 callback 仍可能被呼叫。

---

## 4-8 快捷鍵衝突排查

如果按下快捷鍵沒反應，先換一個組合確認是否衝突：

| 常見被佔用的組合 | 說明 |
|---|---|
| `Ctrl+M` | UE Editor 內建功能 |
| `Ctrl+Z` / `Ctrl+Y` | Undo / Redo |
| `Ctrl+S` | 儲存 |

建議使用 `Ctrl+Shift+` 系列，衝突機率較低。

確認快捷鍵有效後，再改成你想要的組合。

---

## 4-9 小結

| 概念 | 重點 |
|---|---|
| `TCommands<T>` | CRTP 基底，管理 Command 生命週期 |
| `TSharedPtr<FUICommandInfo>` | 每個動作一個成員變數 |
| `UI_COMMAND` | 定義名稱、Tooltip、按鍵組合的巨集 |
| `FUICommandList` | 把 Command 和執行函式綁在一起的容器 |
| `MapAction` | Command → lambda 的對應 |
| `GetGlobalLevelEditorActions()->Append` | 把 CommandList 掛進 Level Editor |
| `FEditorDelegates::OnMapOpened` | 監聽 Level 切換，重新 Append |
| `FDelegateHandle` | 存 delegate 綁定的 handle，Shutdown 時用來移除 |
| `InputCore` | `EKeys` 所在模組，Build.cs 要加 |
| 快捷鍵衝突 | 換 `Ctrl+Shift+` 系列，衝突機率低 |

---