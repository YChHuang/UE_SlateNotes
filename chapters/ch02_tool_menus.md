# 第二章：選單系統 × ToolMenus × RegisterMenus 的 delay 機制

[← 回目錄](../README.md)

---

## 2-1 整體流程

```
StartupModule()
  ├→ RegisterNomadTabSpawner   ← delegate 準備好，等著被觸發
  └→ RegisterStartupCallback   ← 把 RegisterMenus 排進 UToolMenus 等候清單

Editor 初始化完成，UToolMenus 廣播
  └→ RegisterMenus() 被呼叫
       └→ ExtendMenu → AddMenuEntry  ← 選單入口建立完成

使用者點選單
  └→ TryInvokeTab(MyTabName)
       └→ FGlobalTabmanager 找到 delegate
            └→ OnSpawnPluginTab() 被呼叫 → 視窗建立
```

delegate 本身不是入口，它是「建立入口的動作」。被觸發之後，入口才真正存在於 Editor 上。

---

## 2-2 為什麼選單要「延遲」登記？

`StartupModule()` 執行時，MainFrame（主視窗骨架）還不存在，`"MainFrame.MainMenu.Window"` 這個路徑也還不存在。若直接呼叫 `ExtendMenu` 會找不到目標。

```
UE 啟動流程（簡化）
  └→ 載入所有 Plugin（呼叫 StartupModule）
       └→ ... 很多其他初始化 ...
            └→ Editor MainFrame 才建立完成
                 └→ UToolMenus 廣播「我準備好了」
                      └→ RegisterMenus() 被呼叫 ← 這時才能安全 ExtendMenu
```

`RegisterStartupCallback` 就是把 `RegisterMenus` 放進等候清單，等 Editor 完全就緒後再呼叫。

---

## 2-3 UToolMenus 的兩個階段

`UToolMenus` 是管理整個 Editor 選單系統的單例，包括上方選單列、工具列、右鍵選單等。它的生命週期分兩段：

```
啟動期：等候清單
  └→ RegisterStartupCallback 把 delegates 排進來
       └→ Editor 就緒後廣播一次，全部呼叫，這個清單就結束了

執行期：帳本
  └→ 記錄所有 entry 屬於哪個 owner
       └→ 等 UnregisterOwner 來清理
```

自己廣播、通知大家，大家又回來找自己辦事——它是召集人，也是服務窗口。

---

## 2-4 RegisterStartupCallback 的 delegate 型別

```cpp
UToolMenus::RegisterStartupCallback(
    FSimpleMulticastDelegate::FDelegate::CreateRaw(
        this, &FMySlatePluginModule::RegisterMenus
    )
);
```

這裡的 delegate 型別是 `FSimpleMulticastDelegate::FDelegate`：

| 關鍵字 | 意思 |
| --- | --- |
| `Simple` | 沒有參數、沒有回傳值 |
| `Multicast` | 可同時綁定多個 delegate，廣播時全部呼叫 |

所有 Plugin 都可以把自己的 `RegisterMenus` 丟進來，Editor 就緒後統一廣播，不需要協調彼此。

和第一章的 `FOnSpawnTab` 比較：

| 型別 | 綁定數量 | 回傳值 |
| --- | --- | --- |
| `FOnSpawnTab` | 單一 | `TSharedRef<SDockTab>` |
| `FSimpleMulticastDelegate` | 多重 | 無 |

---

## 2-5 RegisterMenus 內部的流程

```cpp
void FMySlatePluginModule::RegisterMenus()
{
    FToolMenuOwnerScoped OwnerScoped(this);  // Step 1：設定 owner scope

    if (UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("MainFrame.MainMenu.Window"))  // Step 2：找目標選單
    {
        FToolMenuSection& Section = Menu->FindOrAddSection("WindowLayout");  // Step 3：找或建 Section

        Section.AddMenuEntry(...);  // Step 4：新增 Entry
    }
}
```

### 選單路徑

```
MainFrame           ← Editor 主視窗框架
  └→ MainMenu       ← 上方選單列（File / Edit / Window / Help...）
       └→ Window    ← Window 下拉選單
```

`ExtendMenu` 回傳 `UToolMenu*`（裸指標），找不到路徑時回傳 `nullptr`，所以要用 `if` 做 null check。

### FToolMenuOwnerScoped 與 owner stack

`FToolMenuOwnerScoped` 的建構/解構子會通知 `UToolMenus` 單例操作 owner stack：

```
建構子：把 this 推進 owner stack   → stack: [..., this]
解構子：把 this 從 stack 彈出      → stack: [...]
```

中間執行的所有 `AddMenuEntry` 都自動被貼上當前 stack 頂端的 owner 標籤。用 stack 設計也支援嵌套：

```cpp
FToolMenuOwnerScoped ScopeA(ownerA);  // stack: [A]
    FToolMenuOwnerScoped ScopeB(ownerB);  // stack: [A, B]
    // 這裡的 entry 屬於 B
// ScopeB 解構 → stack: [A]
// 這裡的 entry 屬於 A
```

### Section：選單的分組機制

選單項目先分組（Section），再在組裡加 Entry：

```
Window 選單
  ├── Section: "WindowLayout"   ← 我們的 entry 放在這裡
  │     └── Open My Slate Plugin
  └── Section: ...
```

`FindOrAddSection`：有就回傳，沒有就新建。用既有的 Section 名稱可以讓 entry 和 UE 內建項目放在同一組。

---

## 2-6 AddMenuEntry 的五個參數

```cpp
Section.AddMenuEntry(
    "MySlatePluginEntry",                                       // 1. FName：唯一識別名
    LOCTEXT("MenuLabel", "Open My Slate Plugin"),               // 2. 顯示文字
    LOCTEXT("MenuTooltip", "Opens the My Slate Plugin window"), // 3. Tooltip
    FSlateIcon(),                                               // 4. 圖示
    FUIAction(FExecuteAction::CreateLambda([](){ ... }))        // 5. 動作
);
```

**第 1 個（FName）**：`FName` 不是 `FString`，內部用 hash table 存，唯讀且輕量，專門用來當識別 key。同一個 Menu 裡不能重複。

**第 4 個（FSlateIcon）**：`FSlateIcon()` 空建構子代表無圖示。有圖示時：
```cpp
FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.SomeName")
```

**第 5 個（FUIAction）**：包裝三個 delegate 的容器：

```cpp
FUIAction(
    FExecuteAction::CreateLambda([](){ ... }),            // 點擊時做什麼（必要）
    FCanExecuteAction::CreateLambda([](){ return true; }), // 何時可以點（可選）
    FIsActionChecked::CreateLambda([](){ return false; })  // 有無勾選狀態（可選）
)
```

後兩個不傳時預設永遠可點、不勾選。

---

## 2-7 FUIAction 的三個 delegate

### FCanExecuteAction

回傳 `bool`，Slate 渲染選單時對每個 entry 詢問：「你現在可以點嗎？」

```
使用者打開選單
  └→ Slate 渲染每個 entry
       └→ 呼叫 FCanExecuteAction
            ├→ true  → 正常顯示
            └→ false → 灰色，不可點
```

每次打開選單都重新詢問，是動態的，所以邏輯要輕量。

### FIsActionChecked

也是回傳 `bool`，純粹 UI 層級的視覺回饋，`true` 時 entry 左邊出現勾號，告訴使用者「這個功能目前是開著的」：

```
View → Show FPS    ← 開啟時有勾
View → Grid Snap   ← 同上
```

實際的開關邏輯在 `FExecuteAction` 裡，`FIsActionChecked` 只是把當前狀態顯示出來。

`UToolMenus` 帳本存著整個 `FUIAction`，渲染系統打開選單時自己去帳本拿資料來用。`UToolMenus` 只是倉庫，不驅動渲染。

---

## 2-8 TryInvokeTab：連接兩個系統的橋梁

```cpp
FExecuteAction::CreateLambda([]()
{
    FGlobalTabmanager::Get()->TryInvokeTab(MyTabName);
})
```

`TryInvokeTab` 的語意：

- Tab 已開啟 → 帶到前景（focus）
- Tab 未開啟 → 呼叫 spawner callback 建立它
- 找不到 spawner → 靜默失敗

這就是選單系統和 Tab 系統的接合點：選單呼叫 `TryInvokeTab`，`FGlobalTabmanager` 找到 spawner delegate，spawner 建立 Tab。

---

## 2-9 小結

| 概念 | 重點 |
| --- | --- |
| delay 機制 | `RegisterStartupCallback` 把 `RegisterMenus` 排進等候清單，Editor 就緒後才執行 |
| `UToolMenus` 兩個階段 | 啟動期：等候清單廣播一次；執行期：帳本記錄 owner |
| `FSimpleMulticastDelegate` | 無參數無回傳，多重綁定，所有 Plugin 共用同一個廣播 |
| 選單路徑 | 點分隔字串，如 `"MainFrame.MainMenu.Window"` |
| `ExtendMenu` | 回傳 `UToolMenu*`，需要 null check |
| `FToolMenuOwnerScoped` | RAII 守衛，建構/解構子操作 UToolMenus 的 owner stack |
| Section | 選單的分組單位，`FindOrAddSection` 找不到就新建 |
| `FName` | hash table 存的唯讀識別 key，不是 `FString` |
| `FUIAction` | 包裝點擊、啟用、勾選三個 delegate 的容器 |
| `FCanExecuteAction` | 每次渲染選單動態詢問，回傳 false 則 entry 變灰 |
| `FIsActionChecked` | 純 UI 視覺，true 時顯示勾號 |
| `TryInvokeTab` | 連接選單系統和 Tab 系統的橋梁 |

---

[← 回目錄](../README.md) | [第三章 →](./ch03_layout.md) *(coming soon)*
