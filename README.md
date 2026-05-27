# zmk-insight-display

`zmk-insight-display` is a ZMK module for SSD1306 I2C OLED status screens on split keyboards.

The initial target is:

- `ZMK v0.3.x`
- `BLE split`
- `SSD1306` monochrome OLED
- `128x32` and `128x64`
- `left only`, `right only`, and `both`

The main goal is to support split-wide status rendering even when the OLED exists only on the split peripheral side.

## What It Shows

The initial display model includes:

- current layer as `L0`, `L1`, ...
- output transport
- BLE state
- active BLE profile as `BT1`, `BT2`, ...
- left and right battery values

Display rules:

- `single` screen mode shows both batteries on one OLED
- `paired` screen mode shows only the local side battery on each OLED

## Current Constraints

- Initial support is `BLE split` only
- Initial support assumes exactly one split peripheral
- One `zmk,insight-display` node is required per firmware image
- The node always declares the local half `side`
- The `display` property is optional
- If `display` is omitted, that half still participates in sync but renders nothing locally
- The current UI uses a compact ASCII-first status row

## Module Setup

Add this repository to `west.yml`, then add `CONFIG_ZMK_INSIGHT_DISPLAY=y` to the firmware config that should use the module.
Also select the custom ZMK display status screen:

```conf
CONFIG_ZMK_INSIGHT_DISPLAY=y
CONFIG_ZMK_DISPLAY_STATUS_SCREEN_CUSTOM=y
```

Optional startup text:

```conf
CONFIG_ZMK_INSIGHT_DISPLAY_BOOT_TEXT="SYLVATWIN"
```

If unset, the module shows `INITIALIZING...` before representative state is ready.

Example `west.yml` entry:

```yaml
manifest:
  projects:
    - name: zmk-insight-display
      url: https://github.com/your-org/zmk-insight-display
      revision: main
```

Then add the module to the build with `ZMK_EXTRA_MODULES`, or import it through your config repository workflow.

## Required Overlay Node

Each half needs one enabled `zmk,insight-display` node.

Example shape:

```dts
/ {
    insight_display: insight_display {
        compatible = "zmk,insight-display";
        side = "left";
        display = <&oled>;
        layout = "128x32";
        screen-mode = "single";
    };
};
```

Properties:

- `side`
  Physical side for this firmware image. Required.
- `display`
  Local display phandle. Optional.
- `layout`
  Display layout template. Use `128x32` or `128x64`.
- `screen-mode`
  Use `single` for one-screen builds and `paired` for two-screen builds.

## Example Overlays

Examples are provided in [`examples/`](/D:/GitHub/zmk-insight-display/examples).

Patterns:

- `left only`
  Left half has `display = <&oled>;` and `screen-mode = "single"`.
  Right half omits `display`.
- `right only`
  Right half has `display = <&oled>;` and `screen-mode = "single"`.
  Left half omits `display`.
- `both`
  Both halves set `display = <&oled>;` and `screen-mode = "paired"`.

## Internal Design

This module intentionally does not patch ZMK core.

Instead:

1. The split input path stays on standard ZMK split BLE.
2. The central half collects canonical status.
3. The module exposes that canonical state through an auxiliary BLE GATT service.
4. The peripheral half subscribes to that service and renders the synced state locally.

That keeps installation to a module-only workflow while still allowing peripheral-side OLED rendering.

## Verification Status

This repository was implemented against ZMK `v0.3.x` source structure, but it was not built in a full ZMK workspace from this repository alone.

## 起動時メッセージ

起動時に OLED の中央 1 行へ表示する文字列は、設定ファイルから変更できます。

```conf
CONFIG_ZMK_INSIGHT_DISPLAY_BOOT_TEXT="SYLVATWIN"
```

未設定の場合は `INITIALIZING...` が表示されます。

---

# 日本語版

`zmk-insight-display` は、split キーボード向けの SSD1306 I2C OLED ステータス表示用 ZMK module です。

初期対応ターゲット:

- `ZMK v0.3.x`
- `BLE split`
- `SSD1306` モノクロ OLED
- `128x32` と `128x64`
- `left only` / `right only` / `both`

この module の主目的は、**OLED が split peripheral 側にしか無い場合でも、split 全体の代表状態を表示できるようにすること** です。

## 表示内容

初版で扱う表示状態:

- 現在レイヤー `L0`, `L1`, ...
- 出力先 transport
- BLE 状態
- 現在の BLE プロファイル `BT1`, `BT2`, ...
- 左右バッテリー値

表示ルール:

- `single` モードでは 1 枚の OLED に左右両方のバッテリーを表示
- `paired` モードでは各 OLED にその side のバッテリーだけを表示

## 現在の制約

- 初版は `BLE split` のみ対応
- 初版は split peripheral が 1 台の構成を前提
- 各 firmware image ごとに `zmk,insight-display` ノードが 1 つ必要
- ノードは常にその half の `side` を宣言する
- `display` プロパティは省略可能
- `display` を省略した half は同期には参加するが、ローカル描画はしない
- 現在の UI はコンパクトな ASCII ベースの表示

## Module の組み込み

このリポジトリを `west.yml` に追加し、使用する firmware config に `CONFIG_ZMK_INSIGHT_DISPLAY=y` を入れてください。

`west.yml` の例:

```yaml
manifest:
  projects:
    - name: zmk-insight-display
      url: https://github.com/your-org/zmk-insight-display
      revision: main
```

その上で、`ZMK_EXTRA_MODULES` 経由または config repository の workflow 経由で module を build に含めます。

あわせて、ZMK の display status screen を custom に切り替えてください。

```conf
CONFIG_ZMK_INSIGHT_DISPLAY=y
CONFIG_ZMK_DISPLAY_STATUS_SCREEN_CUSTOM=y
```

## 必須 overlay ノード

各 half には、有効な `zmk,insight-display` ノードが 1 つ必要です。

基本形:

```dts
/ {
    insight_display: insight_display {
        compatible = "zmk,insight-display";
        side = "left";
        display = <&oled>;
        layout = "128x32";
        screen-mode = "single";
    };
};
```

各プロパティ:

- `side`
  この firmware image が表す物理 side。必須。
- `display`
  ローカル display の phandle。省略可。
- `layout`
  表示テンプレート。`128x32` または `128x64`。
- `screen-mode`
  1 画面なら `single`、左右 2 画面なら `paired`。

## overlay 設定例

設定例は [`examples/`](/D:/GitHub/zmk-insight-display/examples) に入っています。

パターン:

- `left only`
  左 half に `display = <&oled>;` と `screen-mode = "single"` を設定。
  右 half は `display` を持たない。
- `right only`
  右 half に `display = <&oled>;` と `screen-mode = "single"` を設定。
  左 half は `display` を持たない。
- `both`
  左右両方で `display = <&oled>;` と `screen-mode = "paired"` を設定。

## 内部設計

この module は ZMK core を patch しません。

代わりに、次の構成を取ります。

1. split の入力経路自体は既存の ZMK split BLE をそのまま使う
2. central 側で canonical な代表状態を集約する
3. module が補助 BLE GATT service としてその状態を公開する
4. peripheral 側はその service を購読してローカル OLED に描画する

これにより、ZMK 本体を変更せずに、module 追加だけで peripheral 側 OLED 表示を成立させる狙いです。

## 検証状況

このリポジトリは ZMK `v0.3.x` のソース構造を前提に実装していますが、このリポジトリ単体ではフルの ZMK workspace build 検証までは行っていません。
