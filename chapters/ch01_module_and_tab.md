# 第一章：Module 生命週期 × Tab 系統 × 基礎 Slate 語法

[← 回目錄](../README.md)

---

## 1-1 整體流程

```
UE 啟動
  └→ 讀 .uplugin，找到模組 "MySlatePlugin"
       └→ 呼叫 StartupModule()
            ├→ 向 TabManager 登記「收到這個名字，呼叫我的 callback」
            └→ 向 ToolMenus  登記「Editor ready 後，幫我插入選單項目」

使用者點 Window → Open My Slate Plugin
  └→ TryInvokeTab("MySlatePlugin")
       └→ TabManager 找到登記的 callback
            └→ OnSpawnPluginTab() 被呼叫
                 └→ 回傳 SDockTab（裡面有你的 UI）
```

---

## 1-2 智慧指標

`FGlobalTabmanager::Get()` 是單例呼叫，回傳的不是裸指標，而是 `TSharedRef<FGlobalTabmanager>`。

UE 的智慧指標體系：

| 型別              | 特性                              |
| --------------- | ------------------------------- |
| `TSharedRef<T>` | 永遠有效，不可為 null（類似 C++ reference） |
| `TSharedPtr<T>` | 可為 null（類似 optional pointer）    |
| `TWeakPtr<T>`   | 不擁有物件，可能隨時失效，需要 `.Pin()` 才能使用   |

`::Get()` 回傳 `TSharedRef`，所以直接用 `->` 呼叫方法是安全的。

---

## 1-3 Tab 系統

### 登記 Spawner

```cpp
FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
    MyTabName,   // FName，視窗的唯一識別名
    FOnSpawnTab::CreateRaw(this, &FMySlatePluginModule::OnSpawnPluginTab)
)
    .SetDisplayName(LOCTEXT("TabTitle", "My Slate Plugin"))
    .SetMenuType(ETabSpawnerMenuType::Hidden);
```

這裡做的事：

- 告訴全域 TabManager：「有人呼叫 `MyTabName` 時，用我的 callback 建立視窗」
- `CreateRaw(this, ...)` 把 `this`（Module 實例）和成員函式綁在一起
- `.SetMenuType(Hidden)` 讓這個 Tab 不自動出現在 Window 選單（因為我們要自己控制入口）

### `FOnSpawnTab` 的簽名限制

這個 delegate 要求 callback 嚴格符合以下簽名：

```cpp
TSharedRef<SDockTab> MyFunction(const FSpawnTabArgs& Args);
```

`FSpawnTabArgs` 裡有觸發這次 spawn 的 TabId 等資訊，簡單情境下不需要用到。

### NomadTab

`NomadTab`（游牧 Tab）的特性：

- 可以獨立浮動
- 可以停靠（dock）到 Editor 的任何面板
- 在整個 Editor 裡拖來拖去

這是製作 Editor 工具視窗最常用的 Tab 類型。

### 一定要在 ShutdownModule 裡 Unregister

```cpp
void FMySlatePluginModule::ShutdownModule()
{
    UToolMenus::UnRegisterStartupCallback(this);
    UToolMenus::UnregisterOwner(this);
    FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(MyTabName);
}
```

若忘記 Unregister，熱重載（Hot Reload）Plugin 時會崩潰，因為舊的 callback 指標仍然殘留。

---

## 1-4 FToolMenuOwnerScoped 與 RAII

### 問題：誰負責清理選單項目？

`RegisterMenus()` 裡新增的每個 entry，在 `ShutdownModule` 時都必須清掉。
與其一條一條手動刪，UE 提供了 owner 機制：把所有 entry 標記成「屬於某個 owner」，之後一次清乾淨：

```cpp
UToolMenus::UnregisterOwner(this);  // 刪掉所有屬於 this 的 entry
```

### FToolMenuOwnerScoped 是怎麼運作的

```cpp
void FMySlatePluginModule::RegisterMenus()
{
    FToolMenuOwnerScoped OwnerScoped(this);  // ← 建立在 stack 上
    // ... AddMenuEntry ...
}
```

這是標準的 **RAII** 模式，利用 stack 物件的生命週期來管理狀態：

```
RegisterMenus() 被呼叫
  └→ OwnerScoped 建立在 stack 上
       └→ 建構子：告訴 UToolMenus 單例「現在 owner = this」
            └→ AddMenuEntry、AddMenuEntry...（單例自動幫每個 entry 貼上 this 的標籤）
       └→ 函式結束，stack 清掉
            └→ 解構子：告訴單例「owner = this 這段結束了」
```

`OwnerScoped` 本身不做清理，它只是在告訴單例「這個 scope 內進來的東西，owner 都是 this」。
真正的清理發生在 `ShutdownModule` 呼叫 `UnregisterOwner(this)` 的時候。

### 為什麼不直接每次傳 this 進去？

理論上可以，每個 `AddMenuEntry` 加一個 owner 參數也能達到同樣效果。 `FToolMenuOwnerScoped` 只是把這件事提出來，用 scope 守衛統一處理，省得每個呼叫都重複傳同一個參數。

### 三個角色各司其職

| 角色                      | 負責的事                                |
| ----------------------- | ----------------------------------- |
| `FToolMenuOwnerScoped`  | RAII 守衛，在 scope 內告訴單例「owner = this」 |
| `UToolMenus` 單例         | 內部帳本，記錄每個 entry 屬於哪個 owner          |
| `UnregisterOwner(this)` | 真正執行清理，刪掉所有屬於 this 的 entry          |

---

## 1-5 基礎 Slate 語法

### SNew vs SAssignNew

```cpp
// SNew：建立並直接使用，不保留外部參考
SNew(STextBlock)
    .Text(LOCTEXT("Hello", "Hello!"))

// SAssignNew：建立的同時，把 TSharedPtr 指向它（之後還需要操作它）
TSharedPtr<STextBlock> MyText;
SAssignNew(MyText, STextBlock)
    .Text(LOCTEXT("Hello", "Hello!"));

// 之後可以更新內容
MyText->SetText(FText::FromString("Updated!"));
```

目前的模板全部用 `SNew`，因為建立完就不再需要操作那些 Widget。

### Slot 語法：`[ ]`

Slate 用 `[ ]` 來放子 Widget，這叫做 **slot（插槽）**。

```cpp
SNew(SDockTab)
    .TabRole(ETabRole::NomadTab)
    [
        // SDockTab 只有一個 slot，放在這裡的就是視窗內容
        SNew(SBox)
            .HAlign(HAlign_Center)
            .VAlign(VAlign_Center)
            [
                // SBox 也只有一個 slot
                SNew(STextBlock)
                    .Text(LOCTEXT("HelloSlate", "Hello from Slate!"))
            ]
    ];
```

### SBox：單子容器

`SBox` 就是一個只能裝一個子 Widget 的箱子，主要用途是強制尺寸或對齊：

```cpp
SNew(SBox)
    .WidthOverride(200.f)    // 強制寬度
    .HeightOverride(100.f)   // 強制高度
    .HAlign(HAlign_Center)   // 子元件水平對齊
    .VAlign(VAlign_Center)   // 子元件垂直對齊
    [
        SNew(STextBlock).Text(...)
    ]
```
### HAlign / VAlign 的值
 
`.HAlign()` 和 `.VAlign()` 傳入的是 UE 定義的 enum 值：
 
```cpp
enum EHorizontalAlignment
{
    HAlign_Fill,    // 撐滿父容器寬度
    HAlign_Left,
    HAlign_Center,
    HAlign_Right,
};
 
enum EVerticalAlignment
{
    VAlign_Fill,    // 撐滿父容器高度
    VAlign_Top,
    VAlign_Center,
    VAlign_Bottom,
};
```
---

## 1-6 小結

| 概念                        | 重點                                      |
| ------------------------- | --------------------------------------- |
| `TSharedRef`              | UE 的安全指標，`::Get()` 單例通常回傳這個             |
| `RegisterNomadTabSpawner` | 登記「名字 → callback」的對應關係                  |
| `FOnSpawnTab`             | callback 簽名固定：回傳 `TSharedRef<SDockTab>` |
| `NomadTab`                | 可自由停靠浮動的視窗，Editor 工具首選                  |
| `FToolMenuOwnerScoped`    | RAII 守衛，scope 內的 entry 自動標記 owner       |
| `UnregisterOwner`         | 真正執行清理，搭配 OwnerScoped 使用                |
| `SNew` / `SAssignNew`     | 前者直接用，後者保留指標供後續操作                       |
| `[ ]` slot                | Slate 放子 Widget 的插槽語法                   |
| `SBox`                    | 單子容器，用來控制尺寸與對齊                          |
| `ShutdownModule`          | 一定要 Unregister 所有登記，否則熱重載崩潰             |

--- 
