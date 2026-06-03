# UE Slate 系統學習筆記

> 從一個最小可運行的 Editor Plugin 出發，逐步搞懂 Unreal Engine 的 Slate UI 框架。

---

## 目錄

| 章節                                       | 主題                                         |
| ---------------------------------------- | ------------------------------------------ |
| [第一章](./chapters/ch01_module_and_tab.md) | Module 生命週期 × Tab 系統 × 基礎 Slate 語法         |
| [第二章](./chapters/ch02_tool_menus.md)                     | 選單系統：ToolMenus 與 RegisterMenus 的 delay 機制  |
| [第三章](./chapters/ch03_input_widgets.md)                   | Slate 元件和排版系統：HAlign、VAlign 與常用 Layout Widget |
| 第四章 *(coming soon)*                      | 狀態與互動：SButton、SEditableTextBox、TAttribute  |

---

## 範本專案

本筆記以下列檔案為基礎：

```
MySlatePlugin/
├── MySlatePlugin.uplugin
├── Source/MySlatePlugin/
│   ├── MySlatePlugin.Build.cs
│   ├── Public/MySlatePlugin.h
│   └── Private/MySlatePlugin.cpp
```

這是一個最小的 UE Editor Plugin，啟動後會在 `Window` 選單新增一個入口，
點擊後開啟一個顯示 `Hello from Slate!` 的浮動視窗。

---

## 環境

- Unreal Engine 5.x
- Plugin 類型：Editor Module（只在 Editor 中存在，不進 Runtime）
