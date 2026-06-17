# CrossPoint Reader — 日本語UI対応フォーク

[English](./README.md) · **日本語**

[![Fund contributors](https://img.shields.io/badge/%F0%9F%91%91_Fund_contributors-royalty.dev-BB953A?style=for-the-badge&labelColor=1a1a1a)](https://app.royalty.dev/crosspoint-reader/crosspoint-reader)

> ### 🇯🇵 このフォークについて
>
> これは [CrossPoint Reader](https://github.com/crosspoint-reader/crosspoint-reader) に**日本語UI対応**を追加したコミュニティフォークです。アップストリームの CrossPoint Reader v1.3.x をベースにしており、本家の全機能をそのまま継承しています。本家ファームウェアに加えて、以下を追加しています:
>
> - **日本語UI翻訳** — メニュー・設定UIをすべて日本語化（`lib/I18n/translations/japanese.yaml`）。
> - **ビルトインCJK UIグリフ** — flashに焼き込んだUI専用の20pxスパースビットマップフォント。SDカードへのフォント導入なしでメニューが日本語表示されます。CJK対応の縦メトリクスにより行間も崩れません。
>
> **制約:** これは*UI*の日本語化です。EPUB本文の日本語組版は追加**しません** — 本文表示は本に埋め込まれたフォント（または [SDカードフォント](#sdカード用カスタムフォント)）に依存します。
>
> **日本語ビルドの入手:** このフォークの [Releases](https://github.com/mtskf/crosspoint-reader-jp/releases) からダウンロードしてください。公式Webフラッシャーはアップストリームのビルドのみを配布しています。
>
> **書き込みはこちらから:** <https://crosspointreader.com/#flash-tools> — デバイスを USB-C で接続し、モデル（X3 または X4）を選択して **「Custom .bin」** をクリックし、Releases からダウンロードした `firmware.bin` をアップロードします。その他の方法は下記 [ファームウェアのインストール](#ファームウェアのインストール) を参照してください。

CrossPoint はオープンソースの電子書籍リーダーファームウェアです。コミュニティによって開発され、完全にハック可能で、永久に無料です。「デバイスはメーカーが決めたことではなく、あなたが望むことをすべきだ」と信じる開発者と読者のコミュニティによって維持されています。

**対応デバイス:** ESP32C3 ベースの Xteink [X4](https://www.xteink.com/products/xteink-x4) および [X3](https://www.xteink.com/products/xteink-x3)。

![Xteink デバイスで動作する CrossPoint Reader](./docs/images/cover.jpg)

## CrossPoint でできること

- **リーダーエンジン**: EPUB 2/3 レンダリング（埋め込みスタイル対応）、画像処理、ハイフネーション、カーニング、章ナビゲーション、脚注、しおり、パーセント指定ジャンプ、自動ページめくり、画面向き制御、フォーカスリーディング、KOReader 進捗同期 など。

- **多様なフォーマット**: `.epub`、`.xtc/.xtch`、`.txt`、`.bmp` をネイティブ処理。

- **スクリーンショット。**

- **カスタムフォント**: お気に入りのフォントを SD カードに導入可能。

- **傾けてページめくり（X3 のみ）。**

- **ライブラリ操作**: フォルダブラウザ、隠しファイル表示切替、長押し削除、最近の書籍、SD キャッシュ管理。

- **ワイヤレス機能**:

  - ファイル転送 Web UI
  - EPUB オプティマイザ
  - Web 設定 UI/API（多くのデバイス設定をブラウザから編集）
  - WebSocket 高速アップロード
  - WebDAV ハンドラ
  - AP モード（ホットスポット）と STA モード（既存 Wi-Fi に接続）、いずれも QR ヘルパー付き
  - Calibre ワイヤレス接続フロー
  - OPDS ブラウザ（サーバー保存 最大 8 件、検索、ページネーション、直接ダウンロード）
  - GitHub リリースからの OTA アップデート確認・インストール

- **カスタマイズ**: 複数テーマ（Classic、Lyra、Lyra Extended、RoundedRaff）、スリープ画面モード、フロント/サイドボタンのリマップ、ステータスバー制御、電源ボタン動作、リフレッシュ頻度 など。

- **多言語対応**: UI 言語は 24 以上で増加中。RTL（右から左）対応。

### 今後の予定

- 辞書引き — リーダーを離れずにインライン単語検索。

- さらなるテーマ。

- その他多数！お楽しみに。

---

## USB ロックされたデバイス（Xteink Unlocker）

サードパーティストア（例: AliExpress）で購入した一部の Xteink ユニットは、工場出荷時に USB 書き込みがロックされています。
デバイスがロックされている場合、CrossPoint を書き込む前に
<https://crosspointreader.com/#unlock-tool> で提供されている **Xteink Unlocker** ツールを使用する必要があります。

**xteink.com から直接購入した場合、このツールは不要です。** それらのユニットはロックされていません。

**ロックされているか不明な場合:** 電源を入れ、USB-C ケーブルを接続し、まず Web フラッシャー経由で書き込みを試してください（下記 [ファームウェアのインストール](#ファームウェアのインストール) 参照）。ブラウザのシリアルデバイス選択にデバイスが表示されない場合は、ロックされていると判断する前に別の USB ポートやブラウザを試してください。それでも表示されない場合のみ Unlocker を使用してください。

> ### ⚠️ 警告: UNLOCKER を使う前に必ず読むこと ⚠️
>
> **Unlock ツールで公式にサポートされているファームウェアは CrossPoint と CrossInk のみです。**
>
> USB ロックされたデバイスに他のファームウェアを書き込むと、**デバイスを永久に文鎮化**させたり、**そのファームウェアに永久に固定され復旧手段がなくなる**可能性があります。USB 書き込みが再ロックされると、唯一の復旧手段は OTA だけになり、書き込んだファームウェアが OTA に対応していなければ**脱出する方法がありません**。
>
> **Papyrix フォークはコードから OTA アップデート対応を削除しています。** USB ロックされたユニットに Papyrix を書き込むと、**アップデート・復旧手段が一切なくなり**、永久にそのまま固定されます。**ロックされたデバイスに Papyrix（およびその他の非サポートファームウェア）を書き込まないでください。**

## ファームウェアのインストール

### Web インストーラ（推奨）

日本語ビルドは Web フラッシャーのリリース選択肢には含まれません。カスタム `.bin` としてアップロードします。

1. このフォークの [Releases](https://github.com/mtskf/crosspoint-reader-jp/releases) から日本語ビルドの `firmware.bin` をダウンロードする。
2. デバイスを USB-C でコンピュータに接続し、デバイスをスリープ解除/アンロックする。
3. <https://crosspointreader.com/#flash-tools> にアクセスし、デバイス（X3 または X4）を選択し、**「Custom .bin」** をクリックして、ダウンロードした `firmware.bin` をアップロードする。

> フラッシャーの「公式 CrossPoint リリース」の選択肢には**アップストリームのビルドのみ**が並びます（日本語UIではありません）。必ずこのフォークの Releases からダウンロードした `firmware.bin` を「Custom .bin」でアップロードしてください。

### 公式ファームウェアへの復元

公式ファームウェアに戻すには、<https://crosspointreader.com/#flash-tools> を使って最新の公式ファームウェアを書き込むこともできます。

### コマンドライン

1. [`esptool`](https://github.com/espressif/esptool) をインストール:

```bash
pip install esptool
```

1. このフォークの [リリースページ](https://github.com/mtskf/crosspoint-reader-jp/releases) から `firmware.bin` をダウンロードする。
2. デバイスを USB-C で接続する。
3. デバイスのポートを確認する。Linux では接続後に `dmesg` を実行。macOS では:

```bash
log stream --predicate 'subsystem == "com.apple.iokit"' --info
```

1. 書き込み:

```bash
esptool.py --chip esp32c3 --port /dev/ttyACM0 --baud 921600 write_flash 0x10000 /path/to/firmware.bin
```

`/dev/ttyACM0` は環境に合わせて調整してください。

### 手動ビルド

下記 [開発クイックスタート](#開発クイックスタート) を参照。

---

## SDカード用カスタムフォント

手持ちの TTF/OTF ファイルを、SD カードから読み込める `.cpfont` ファイルに変換できます。ファームウェアの再書き込みは不要です。

1. <https://crosspointreader.com/fonts> にアクセスし、「SD-card font builder」フォームを開く。
2. 最大 4 スタイル（regular、bold、italic、bold-italic）をアップロードし、ファミリー名・ポイントサイズ・Unicode 範囲を設定する。
3. 生成された `.cpfont` ファイルをダウンロードする。
4. SD カードの `/fonts/YourFont/`（フォルダを隠す場合は `/.fonts/YourFont/`）にコピーする。
5. デバイスのフォント設定からフォントを選択する。

変換はファームウェアリポジトリの `lib/EpdFont/scripts/fontconvert_sdcard.py` スクリプトを未改変で実行するため、出力はローカルのホストビルドと一致します。

---

## ドキュメント

- [ユーザーガイド](./USER_GUIDE.md)（英語）
- [Web サーバーの使い方](./docs/webserver.md)（英語）
- [Web サーバーエンドポイント](./docs/webserver-endpoints.md)（英語）
- [プロジェクトスコープ](./SCOPE.md)（英語）
- [コントリビューションガイド](./docs/contributing/README.md)（英語）

---

## 開発クイックスタート

### 前提条件

- [pioarduino](https://github.com/pioarduino/pioarduino) または VS Code + pioarduino プラグイン
- Python 3.8+
- `clang-format` 21
- データ転送対応の USB-C ケーブル

### セットアップ

```bash
git clone --recursive https://github.com/mtskf/crosspoint-reader-jp
cd crosspoint-reader-jp

# --recursive なしで clone した場合:
git submodule update --init --recursive
```

### ビルド / 書き込み / モニタ

```bash
pio run --target upload
```

### コントリビュータ向け PR 前チェック

```bash
./bin/clang-format-fix
pio check -e default
pio run -e default
```

### デバッグ

新機能を書き込んだ後は、シリアルポートから詳細なログを取得することを推奨します。

まず、必要な Python パッケージがすべてインストールされていることを確認します:

```python
python3 -m pip install pyserial colorama matplotlib
```

その後、スクリプトを実行します:

```sh
# Linux 向け
# Debian でテスト済みで、ほとんどの Linux システムで動作するはずです。
python3 scripts/debugging_monitor.py

# macOS 向け
python3 scripts/debugging_monitor.py /dev/cu.usbmodem2101
```

Windows では多少の調整が必要な場合があります。

---

## 内部構造

CrossPoint Reader は RAM 使用量を最小化するため、データを積極的に SD カードにキャッシュします。ESP32-C3 の使用可能 RAM はわずか ~380KB しかないため、慎重に扱う必要があります。ファームウェア設計における多くの判断はこの制約に基づいています。

### データキャッシュ

書籍の章が初めて読み込まれるとき、SD カードにキャッシュされます。以降の読み込みはキャッシュから提供されます。このキャッシュディレクトリは SD カード上の `.crosspoint` に存在します。構造は以下の通りです:

```text
.crosspoint/
├── epub_<hash>/         # 書籍ごとに 1 ディレクトリ。コンテンツハッシュで命名
│   ├── progress.bin     # 読書位置（章、ページ など）
│   ├── cover.bmp        # 生成された表紙画像
│   ├── book.bin         # メタデータ: タイトル、著者、spine、目次
│   ├── css_rules.cache  # パース済み CSS ルールキャッシュ
│   ├── img_*            # レンダリング済み画像キャッシュファイル
│   └── sections/        # 章ごとのレイアウトキャッシュ
│       ├── 0.bin
│       ├── 1.bin
│       └── ...
├── settings.json        # デバイス設定
├── state.json           # 再開/ランタイム状態
└── recent.json          # 最近の書籍リスト
```

`/.crosspoint` を削除すると、キャッシュされた全メタデータがクリアされ、次回オープン時に完全に再生成されます。ファームウェアや Web UI 経由での書籍の削除・上書き・移動は、対応するキャッシュをクリアまたは再キー付けします。SD カードを手動で編集すると、古いキャッシュディレクトリが残る場合があります。

内部ファイル構造の詳細は [ファイルフォーマットドキュメント](./docs/file-formats.md)（英語）を参照してください。

---

## コントリビューション

コントリビューションを歓迎します。コードベースが初めての方は、[コントリビューションガイド](./docs/contributing/README.md)（英語）から始めてください。取り組む課題については [アイデア掲示板](https://github.com/crosspoint-reader/crosspoint-reader/discussions/categories/ideas) を確認し、作業が重複しないよう着手前にコメントを残してください。

ここにいる全員がボランティアです。敬意を持って、辛抱強く接してください。ガバナンスとコミュニティの期待については [GOVERNANCE.md](./GOVERNANCE.md)（英語）を参照してください。

---

## コミュニティフォーク

オープンソースの最良の点のひとつは、誰でもコードを別の方向に発展させられることです。CrossPoint の[スコープ](./SCOPE.md)外の機能が必要な場合は、コミュニティフォークをチェックしてください:

- [CrossInk](https://github.com/uxjulia/CrossInk) — タイポグラフィと読書トラッキング: Bionic Reading（単語の語幹を太字にして視線の固定点を作る）、単語間のガイドドット、段落インデントの改善、デフォルトフォントを ChareInk/Lexend/Bitter に置き換え。

- [papyrix-reader](https://github.com/bigbag/papyrix-reader) — FB2 と MD フォーマット対応を追加。アラビア文字対応で活発にメンテナンス中。SD カード経由のカスタムテーマ。

- [crosspet](https://github.com/trilwu/crosspet) — ベトナム語フォーク。読書のマイルストーン（読んだページ数、連続記録、世話）に応じて育つたまごっち風の仮想ニワトリを追加。さらに: フラッシュカード、天気、ポモドーロタイマー、ミニゲーム。

- [crosspoint-reader-cjk](https://github.com/aBER0724/crosspoint-reader-cjk) — 中国語・日本語・韓国語の読書専用に作られたフォーク。

- [inx](https://github.com/obijuankenobiii/inx) — タブナビゲーションでユーザーインターフェースを全面的に再構築。

- ~~[PlusPoint](https://github.com/ngxson/pluspoint-reader) — カスタム JS アプリ対応。~~（メンテナンス停止）

- [crosspoint-reader-papers3](https://github.com/juicecultus/crosspoint-reader-papers3) — M5Stack Paper S3 向け Crosspoint ポート。

- [t5s3-reader](https://github.com/ShallowGreen123/t5s3-reader) — LilyGo T5 ePaper S3 / T5S3 4.7インチ e-paper デバイス向け Crosspoint ポート。

**注:** これらの機能の多くは、時間をかけて CrossPoint 本家にも取り込まれていきます。盤石な安定性を確保し、バグがデバイスに届く前に潰すため、本家はゆっくりとしたペースを維持しています。

自分でデバイスを作りたいですか？ ぜひ [de-link](https://github.com/iandchasse/de-link) プロジェクトをチェックしてください。

---

## フォントとライセンス

このフォークは [Source Han Sans](https://github.com/adobe-fonts/source-han-sans)（© 2014–2021 Adobe）由来のビットマップグリフを、[SIL Open Font License 1.1](https://scripts.sil.org/OFL) のもとで埋め込んでいます。グリフは `lib/EpdFont/scripts/codepoints/` のコードポイントリストから `scripts/generate_cjk_ui_font.py` によって再生成されます。その他のコードはアップストリームプロジェクトのライセンスに従います（[LICENSE](./LICENSE) 参照）。

---

CrossPoint Reader は **Xteink およびいかなるデバイスメーカーとも提携していません**。

このプロジェクトに着想を与えてくれた [diy-esp32-epub-reader](https://github.com/atomic14/diy-esp32-epub-reader) に大きな感謝を。
