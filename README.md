# 軽量付箋

FLTK（Fast Light Toolkit）で作った、軽量でシンプルな黄色い付箋メモ帳デスクトップアプリです。上部バーのドラッグ移動、最前面固定（ピン留め）、閉じる際の `.txt` 保存に対応しています。

## 特徴

- **軽量・高速起動**：Webランタイムや巨大フレームワークを使わず、C++ / FLTKベースで軽快に動作
- **枠なし付箋UI**：ウィンドウ枠のない黄色のシンプルな画面と、上部バーのドラッグによる自由な移動
- **最前面固定（ピン留め）**：上部バーの `TOP` ボタンで、他のウィンドウの上に常に表示するかを切り替え可能
- **柔軟なテキスト保存**：閉じる（`×` またはウィンドウクローズ）ときに保存ダイアログを表示し、`.txt` 形式で保存
  - タイトル欄に文字がある場合：`タイトル.txt` を初期ファイル名として提案（ファイル名に使えない文字は自動整形）
  - タイトル欄が空欄の場合：`YYYYMMDD_HHMMSS.txt`（タイムスタンプ）を初期ファイル名として提案

## ダウンロード

[Releases](../../releases) から、お使いのOS向けの zip をダウンロードして展開してください。

| OS                    | ファイル                      |
| --------------------- | ----------------------------- |
| Windows (64bit)       | `sticky_note-windows-x64.zip` |
| macOS (Apple Silicon) | `sticky_note-macos-arm64.zip` |

> **注意**：実行ファイルは署名されていません。
>
> - Windows：SmartScreen の警告が出た場合は「詳細情報」→「実行」を選択してください。
> - macOS：初回は右クリック →「開く」で起動してください。起動できない場合は `xattr -d com.apple.quarantine ./sticky_note` を実行してください。

## 使い方

- **タイトルと本文を入力する**
  - 上部の入力欄にタイトル（任意）、下部のエリアにメモを入力します。
- **位置を移動する**
  - タイトル入力欄やボタン以外の黄色い上部バーをドラッグすると、ウィンドウを移動できます。
- **最前面に表示する**
  - 上部バーの `TOP` ボタンをクリックすると、最前面固定の ON / OFF を切り替えられます（固定中は `TOP*` と表示されます）。
  - 上部バーの右クリック、または `Ctrl+T`（macOS では `Cmd+T`）でも切り替えられます。
- **保存して閉じる**
  - 右上の `×` ボタン（またはウィンドウを閉じる操作）を押すと保存ダイアログが開きます。ファイル名を確認して保存するとアプリを終了します（キャンセルした場合は保存せずに終了します）。

## ソースからビルドする

### 必要環境

- **C++17** 対応コンパイラ（Clang / GCC / MSVC）
- **CMake**（3.16 以上）
- **FLTK**（1.4 系を推奨）

### Windows（MSYS2 UCRT64）

MSYS2 の **UCRT64** シェルで実行します。

```bash
pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-fltk

git clone {this repoditory}
cd <your-repo-name>

cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build

./build/sticky_note.exe
```

### macOS（Homebrew）

```bash
brew install fltk cmake

git clone {this repoditory}
cd <your-repo-name>

cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

./build/sticky_note
```

### Ubuntu / Debian 系

```bash
sudo apt update
sudo apt install build-essential cmake libfltk1.3-dev

git clone {this repoditory}
cd <your-repo-name>

cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

./build/sticky_note
```

> 実行ファイルをスリムにしたい場合は、ビルド後に `strip ./build/sticky_note`（Windows は `strip ./build/sticky_note.exe`）を実行できます。
