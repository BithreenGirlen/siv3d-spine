# siv3d-spine

[Siv3D](https://github.com/Siv3D/OpenSiv3D) を使った[Spine](http://esotericsoftware.com/)の描画。

### 実行例

https://github.com/user-attachments/assets/2e2859bf-8c11-4c78-898c-bea186773a0d

## ファイル説明

### Spineに関係するもの

| ファイル | 機能 |
| --- | --- |
| [siv3d_spine.cpp/h](https://github.com/BithreenGirlen/Siv3D-Spine/blob/main/siv3d-spine/siv3d_spine.h) | Siv3Dの機能を使ったSpineのテクスチャ生成・破棄、描画処理。 |
| [siv3d_spine_blendmode.h](https://github.com/BithreenGirlen/Siv3D-Spine/blob/main/siv3d-spine/siv3d_spine_blendmode.h) | Siv3Dの定数に基づくSpine混色法定義。 |
| [siv3d_spine_player.cpp/h](https://github.com/BithreenGirlen/Siv3D-Spine/blob/main/siv3d-spine/siv3d_spine_player.h) | Siv3Dの機能を使ったSpine描画時の視点・拡縮補正。 |
| [spine_loader.cpp/h](https://github.com/BithreenGirlen/Siv3D-Spine/blob/main/siv3d-spine/spine_loader.h) | Spine出力ファイルの取り込み処理。 |
| [spine_player.cpp/h](https://github.com/BithreenGirlen/Siv3D-Spine/blob/main/siv3d-spine/spine_player.h) | Spineの機能命令をまとめたもの。 |

- これらのファイルを公式の汎用ランタイム`spine-cpp`と共に使うことでSiv3DでのSpine描画が行えます。
- `spine_player.cpp/h`は描画ライブラリ側の機能に依らないよう設計しています。
  - 厳密には二次元座標の点`FPoint2`は描画ライブラリ側の定義なのですが、概ねどのライブラリも`(x, y)`の変数名なので、内部で使用しています。
- Spine描画に用いる混色法は全てカスタム定義しています。これは定義済みの各種`s3d::BlendState`は基本的に`outA = dstA`の計算式になっていて`srcA`を寄与させる表現ができないためです。

### Spineに関係しないもの

`example/src`階層下のファイルで、実行例プログラムに係るものです。

| ファイル | 機能 |
| --- | --- |
| [siv3d_main_window.cpp/h](https://github.com/BithreenGirlen/Siv3D-Spine/blob/main/example/src/siv3d_main_window.h) | Siv3Dのウィンドウ表示。 |
| [siv3d_recorder.cpp/h](https://github.com/BithreenGirlen/Siv3D-Spine/blob/main/example/src/siv3d_recorder.h) | 描画結果の動画出力。 |
| [siv3d_window_menu.cpp/h](https://github.com/BithreenGirlen/Siv3D-Spine/blob/main/example/src/siv3d_window_menu.h) | メニュー欄状態変更・選択通知。 |
| [imgui/siv3d_imgui.cpp/h](https://github.com/BithreenGirlen/Siv3D-Spine/blob/main/example/src/imgui/siv3d_imgui.h) | [Dear ImGui](https://github.com/ocornut/imgui)とSiv3Dの[連繋](https://github.com/BithreenGirlen/siv3d-imgui) |
| [wic/wic_gif_encoder.cpp/h](https://github.com/BithreenGirlen/Siv3D-Spine/blob/main/example/src/wic/wic_gif_encoder.h) | WICによるGIF書き込み。 |

- `siv3d_recorder.cpp`はごく短時間の録画を想定しています。これは低遅延を優先し、録画中はフレームを内部に溜め込み録画終了時にストリームを開いて一挙に書き出す、という設計になっているためです。
- `wic_gif_encoder.cpp/h`はSiv3Dの機能に依存していないため、Windows環境であれば描画ライブラリに関係なく使用できます。

## 実行例プロジェクト説明

### 構成
- 環境変数は使わず、Siv3DのSDKは`example/project/deps`階層下に置く構成になっています。
  - [example/project/deps/CMakeLists](https://github.com/BithreenGirlen/Siv3D-Spine/blob/main/example/project/deps/CMakeLists.txt)を実行することでSpine含め依存ライブラリの取得と配置が行えます。

### 構築方法

1. `example/project/deps`階層をエクスプローラーで開く。
2. 階層表示欄に`cmd`と入力。
    - コマンドプロンプトが起動します。
3. コマンドプロンプト上で`start devenv .`と入力。
    - Visual Studioが起動し、CMakeの設定を開始します。
      - コマンドプロンプト上でもCMake設定は行えますが、セキュリティの関係なのかダウンロードが遅くなってしまいます。
4. 設定が終了したら、`OpenSiv3DSpineTest.sln`を開いてビルドを実行。
    - ビルドの完了を確認したら、`deps`階層下の`out`フォルダと`.vs`フォルダは手動で削除して下さい。
    - 但し、手元に動作を確認できるSpine出力ファイルがない場合、`out`階層下にある公式素材`spine-runtimes-X.X/examples`は動作確認用に残しておいて下さい。

## 補足

- Spine `3.8`, `4.0`, `4.1`, `4.2`にて動作確認を行っています。`siv3d_spine.cpp`はマクロに応じて動作分岐しますので、以下のマクロを定義する必要があります。

| 版 | 事前定義すべきマクロ |
| --- | --- |
| `3.8` | 不要 |
| `4.0` | `SPINE_4_0` |
| `4.1` | `SPINE_4_1_OR_LATER` |
| `4.2` | `SPINE_4_2_OR_LATER` |

- `spine-c`はより軽量なのですが、同じ出力ファイルを用いても剪断変形で`spine-cpp`よりも早く歪み限界が生じる、という挙動が見られるため不採用となりました。
- `4.2`からは`spine::RenderCommand`クラスが新設されましたが、これは`spine::AtlasPage`にアクセスできない設計となっており、描画の正確性に支障をきたすため使用しておりません。
