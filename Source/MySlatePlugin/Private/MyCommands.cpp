#include "MyCommands.h"

#define LOCTEXT_NAMESPACE "FMyPluginCommands"

void FMyPluginCommands::RegisterCommands()
{
    UI_COMMAND(
        MyAction,                        // 對應 .h 裡的成員變數
        "My Action",                     // 顯示名稱
        "Does my custom action",         // Tooltip
        EUserInterfaceActionType::Button,// 按鈕類型（不是 toggle）
        FInputChord(EModifierKey::Control | EModifierKey::Shift, EKeys::J)  // Ctrl+Shift+J
        //          ↑ 主鍵     ↑ 修飾鍵，這裡是 Ctrl+Shift+M
    );
}

#undef LOCTEXT_NAMESPACE