# 第三章：常用輸入 Widget × Lambda 綁定

[← 回目錄](../README.md)

---

## 3-1 SVerticalBox：放多個 Widget

`SBox` 只能放一個子 Widget。要垂直排列多個，改用 `SVerticalBox`：

```cpp
#include "Widgets/Layout/SVerticalBox.h"

SNew(SVerticalBox)
+ SVerticalBox::Slot()
    .AutoHeight()
    .Padding(10.f)
    [ Widget A ]
+ SVerticalBox::Slot()
    .AutoHeight()
    .Padding(10.f)
    [ Widget B ]
```

Slot 是容器和子 Widget 之間的中間層，排版設定（padding、高度模式）掛在 Slot 上，不是掛在 Widget 上。

| Slot 設定 | 意思 |
|---|---|
| `.AutoHeight()` | 高度跟著內容走 |
| `.Padding(10.f)` | 四周留 10 px |
| `.FillHeight(1.f)` | 填滿剩餘空間 |

---

## 3-2 SButton

```cpp
#include "Widgets/Input/SButton.h"

SNew(SButton)
    .Text(LOCTEXT("Label", "Click Me"))
    .OnClicked(FOnClicked::CreateLambda([]() -> FReply
    {
        UE_LOG(LogTemp, Warning, TEXT("Clicked!"));
        return FReply::Handled();
    }))
```

| 項目 | 說明 |
|---|---|
| `.Text()` | 按鈕文字，內部自動建立 STextBlock |
| `.OnClicked()` | delegate 型別 `FOnClicked` |
| 回傳值 | **一定要回傳 `FReply`**，Slate 用它決定事件要不要繼續往上傳 |
| `FReply::Handled()` | 我處理了，事件停在這裡 |
| `FReply::Unhandled()` | 我沒處理，繼續往上傳 |

---

## 3-3 SEditableTextBox

```cpp
#include "Widgets/Input/SEditableTextBox.h"

// .h 需要宣告成員變數來存文字
FText InputText;
```

```cpp
SNew(SEditableTextBox)
    .HintText(LOCTEXT("Hint", "請輸入文字..."))
    .OnTextChanged_Lambda([this](const FText& NewText)
    {
        InputText = NewText;
    })
```

| 項目 | 說明 |
|---|---|
| `.HintText()` | 空白時的提示文字，類似 HTML placeholder |
| `OnTextChanged` | 每打一個字觸發 |
| `OnTextCommitted` | 按 Enter 或失去 focus 才觸發 |
| callback 參數 | `const FText& NewText`，當前完整文字 |
| 為何 capture `[this]` | lambda 內要寫入成員變數 `InputText`，需要 `this` 指標才能存取 |

---

## 3-4 SSearchBox

```cpp
#include "Widgets/Input/SSearchBox.h"

SNew(SSearchBox)
    .OnTextChanged_Lambda([this](const FText& NewText)
    {
        UE_LOG(LogTemp, Warning, TEXT("Searching: %s"), *NewText.ToString());
    })
```

外觀上比 `SEditableTextBox` 多了放大鏡圖示和清除按鈕，API 幾乎相同，事件綁法一樣。

---

## 3-5 SCheckBox

```cpp
#include "Widgets/Input/SCheckBox.h"

SNew(SCheckBox)
    .OnCheckStateChanged_Lambda([this](ECheckBoxState NewState)
    {
        if (NewState == ECheckBoxState::Checked)
        {
            UE_LOG(LogTemp, Warning, TEXT("Checked"));
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Unchecked"));
        }
    })
```

| 項目 | 說明 |
|---|---|
| callback 參數 | `ECheckBoxState NewState`，Slate 通知你變成什麼狀態 |
| `ECheckBoxState` | `Checked` / `Unchecked` / `Undetermined` |
| 狀態不用自己存 | Slate 內部記錄勾選狀態，callback 只是通知 |

---

## 3-6 Lambda Capture 快速對照

| 寫法 | 能存取成員變數 | 風險 |
|---|---|---|
| `[this]` | ✅ | Module 活著就安全 |
| `[]` | ❌ | 無，適合不需要外部狀態的情況 |
| `[=]` | ❌（副本） | 寫入無效 |
| `[&變數]` | ✅ | ❌ delegate 生命週期比函式長，reference 可能懸空 |

規則：需要存取成員變數就用 `[this]`，完全不需要外部狀態就用 `[]`。

---

## 3-7 小結

| Widget | Include | 主要事件 | callback 簽名 |
|---|---|---|---|
| `SButton` | `Widgets/Input/SButton.h` | `OnClicked` | `FReply()` |
| `SEditableTextBox` | `Widgets/Input/SEditableTextBox.h` | `OnTextChanged` / `OnTextCommitted` | `void(const FText&)` |
| `SSearchBox` | `Widgets/Input/SSearchBox.h` | `OnTextChanged` | `void(const FText&)` |
| `SCheckBox` | `Widgets/Input/SCheckBox.h` | `OnCheckStateChanged` | `void(ECheckBoxState)` |

---

[← 回目錄](../README.md) | 第四章 *(coming soon)*
