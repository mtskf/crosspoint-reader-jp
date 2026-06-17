# CrossPoint Reader — 日本語UI対応フォーク

[English](./README.md) · **日本語**

[![Fund contributors](https://img.shields.io/badge/%F0%9F%91%91_Fund_contributors-royalty.dev-BB953A?style=for-the-badge&labelColor=1a1a1a)](https://app.royalty.dev/crosspoint-reader/crosspoint-reader)

> ### 🇯🇵 このフォークについて
>
> これは [CrossPoint Reader](https://github.com/crosspoint-reader/crosspoint-reader)（本家）に**日本語UI対応**を追加したコミュニティフォークです。本家 v1.3.x をベースにしており、その全機能をそのまま継承しています。本家に加えて、以下を追加しています:
>
> - **メニューの日本語化** — メニューや設定画面の文字をすべて日本語に翻訳。
> - **メニュー用の日本語フォントを内蔵** — メニューや設定画面の表示に使う日本語フォントだけをファームウェアに同梱済み。SDカードにフォントを入れなくても、メニューは日本語で正しく表示されます。⚠️ ただしこれは**メニュー専用**で、本の**本文**を日本語で表示するには別途 SD カードへフォントを追加する必要があります（→ [書籍用の日本語フォント](#書籍用の日本語フォント)）。
>
> **① 日本語版 firmware.bin を入手:** このフォークの [Releases](https://github.com/mtskf/crosspoint-reader-jp/releases) から最新版の `firmware.bin` をダウンロードします（公式の Web フラッシャーには本家＝英語版しか並ばないため、必ずこちらから入手してください）。
>
> **② 端末に書き込み:** <https://crosspointreader.com/#flash-tools> を **Chrome または Edge** で開き、機種（X3 / X4）を選んで **「Custom .bin」** から先ほどの `firmware.bin` をアップロードします。詳しい手順は下記 [ファームウェアのインストール](#ファームウェアのインストール) を参照。
>
> **③ 日本語の本を読むにはフォントを追加:** ①②でメニューは日本語になりますが、**日本語の本文を表示するには SD カードに日本語フォントを追加**する必要があります（内蔵フォントは英数字のみのため）。手順は [書籍用の日本語フォント](#書籍用の日本語フォント) を参照してください。

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

### 書き込み手順（推奨）

日本語版は公式 Web フラッシャーの一覧には並びません。`firmware.bin` を手動でアップロードして書き込みます。

1. **ダウンロード** — このフォークの [Releases](https://github.com/mtskf/crosspoint-reader-jp/releases) から最新版の `firmware.bin` を PC に保存します。
2. **接続** — 端末を USB-C ケーブルで PC につなぎ、画面をオンにします（スリープ中なら解除）。
3. **書き込み** — <https://crosspointreader.com/#flash-tools> を **Chrome または Edge** で開きます（Safari / Firefox は非対応）。機種（X3 / X4）を選び、**「Custom .bin」** をクリックして、保存した `firmware.bin` を選んで書き込みます。

> 一覧に並ぶ「公式 CrossPoint リリース」は**本家＝英語版だけ**で、日本語UIは含まれません。必ず上記のフォークの `firmware.bin` を「Custom .bin」からアップロードしてください。

### 公式ファームウェアに戻す

元の公式ファームウェアに戻したいときは、<https://crosspointreader.com/#flash-tools> から最新の公式ファームウェアを書き込めます。

### 自分でビルドする場合

開発者向けの手順は下記 [開発クイックスタート](#開発クイックスタート) を参照してください。

---

## 書籍用の日本語フォント

メニューは日本語化されていますが、**本の中身（本文）を日本語で表示するには、SD カードに日本語フォントを別途入れる必要があります**（内蔵フォントは英数字のみのため）。次の手順で追加します:

1. **フォントを入手** — [Noto Sans JP](https://fonts.google.com/noto/specimen/Noto+Sans+JP) のページで **Get font** を押してダウンロードします（ZIP がダウンロードされるので、解凍すると中に `.ttf` ファイルが入っています）。
2. **`.cpfont` に変換** — [CrossPoint Font Builder](https://crosspointreader.com/fonts) を開き、解凍した `.ttf` をアップロードします。文字の範囲で **日本語 / CJK** を必ず選び（かな・漢字を含めるため）、生成された `.cpfont` ファイルをダウンロードします。
   - Noto Sans JP にはイタリック体が無いため、italic / bold-italic は空のままで問題ありません（本文中のイタリック指定は通常スタイルで表示されるだけで、文字化けはしません）。
3. **SD カードに置く** — 端末から SD カードを取り出して PC で開き、一番上の階層に `Fonts` フォルダを作り、その中にフォント名のフォルダ（例 `NotoSansJP`）を作って `.cpfont` ファイルを入れます。
4. **端末で選ぶ** — SD カードを端末に戻して電源を入れ、**設定 > リーダー > フォントファミリー** で追加したフォントを選びます。

SD カードの構成はこんな感じです:

```text
/  ← SD カードの一番上
├── Fonts/
│   └── NotoSansJP/
│       ├── NotoSansJP_12.cpfont
│       ├── NotoSansJP_14.cpfont
│       ├── NotoSansJP_16.cpfont
│       └── NotoSansJP_18.cpfont
├── Books/      ← 本を入れるフォルダ（例）
└── ...
```

### その他のカスタムフォント

同じ `.cpfont` のワークフローで、任意の TTF/OTF フォントを導入できます（ファームウェアの再書き込みは不要）:

1. <https://crosspointreader.com/fonts> にアクセスし、「SD-card font builder」フォームを開く。
2. 最大 4 スタイル（regular、bold、italic、bold-italic）をアップロードし、ファミリー名・ポイントサイズ・Unicode 範囲を設定する。
3. 生成された `.cpfont` ファイルをダウンロードする。
4. SD カードの `/fonts/YourFont/`（フォルダを隠す場合は `/.fonts/YourFont/`）にコピーする。
5. デバイスのフォント設定からフォントを選択する。

変換はファームウェアリポジトリの `lib/EpdFont/scripts/fontconvert_sdcard.py` スクリプトを未改変で実行するため、出力はローカルのホストビルドと一致します。Unicode 範囲プリセットや CLI 変換の詳細は [docs/sd-card-fonts.md](./docs/sd-card-fonts.md)（英語）を参照してください。

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

`/.crosspoint` を削除すると、キャッシュされた全メタデータがクリアされ、次回オープン時に完全に再生成されます。ファームウェアや Web UI 経由で書籍を削除・上書き・移動すると、対応するキャッシュもクリア、またはハッシュに合わせて付け替えられます。SD カードを手動で編集すると、古いキャッシュディレクトリが残る場合があります。

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
