# Windows kanata → macOS

この修正は、Windows版kanataの `imeoff` / `imeon` をDeskflow経由でMacの
「英数」/「かな」に届けるためのものです。Mod-Tapの判定はkanataが行います。

## 設定

kanataの該当部分は次の形です。既存設定のAlt部分に対応する例であり、
設定ファイル全体を置き換える必要はありません。

```lisp
(deflayermap (base)
  lalt (tap-hold-press 200 200 imeoff lalt)
  ralt (tap-hold-press 200 200 imeon  ralt)
)
```

- Windows側はこの修正版Deskflowを使用します。
- Mac側も修正版を使うと、右Alt（WindowsのAltGr）の右Optionへの対応と、
  左右のモディファイアを同時に押した場合の保持状態の修正が適用されます。
- Macには日本語入力ソースを追加しておきます。
- MacのDeskflow「Client Configuration」で
  **Use server's keyboard language on this computer** をオフにします。
  IMEのオン／オフとWindowsのキーボード言語は別の状態です。言語同期を有効に
  すると、その後の文字入力でWindowsの言語が再選択される場合があります。
- WindowsのLLHOOK版kanataは、Deskflowのサーバーを起動した後に起動／再起動します。
  後から登録されたフックが先に呼ばれるため、kanataが物理入力を変換してから
  Deskflowが結果を転送できる順序にします。Deskflowの再起動やフックの再登録後は
  この順序を再確認してください。Interception版は入力経路が異なるため別途検証が必要です。

## 修正内容

1. `VK_IME_OFF` を既存プロトコルの `Zenkaku`（MacではJIS英数）、
   `VK_IME_ON` を `Henkan`（MacではJISかな）に対応付けます。
2. IMEコマンドには独立した非ゼロのボタンIDを割り当てます。
   スキャンコードがなくても解放でき、他のキーと保持状態を共有しません。
3. スキャンコードなしの入力は、リピート／モディファイア状態の判定前に補完します。
   汎用 `VK_SHIFT` の右Shiftもスキャンコードから識別します。
4. Shiftなどのモディファイア単独で送信元の言語へ切り替えません。
5. Macで左右のShift・Control・Option・Commandを独立して保持します。
   WindowsのAltGrは右Optionとして受け取ります。

## 実機確認

Windowsの配布ZIPを試す場合は、既存のDeskflowで接続を停止して終了し、
ZIPを展開してその中の `deskflow.exe` を起動します。
ZIP内の `settings/Deskflow.conf` によってポータブルモードで起動します。
画面の接続設定は、この版のGUIで改めて行ってください。
この版では従来のサーバー設定の `section: screens` は読み込まれません。
既存GUIから書き出したファイルを指定するだけでは画面別設定を引き継げません。
画面名とモディファイア変換は `settings/Deskflow.conf` の
`[screen_画面名]` に保存する必要があります。
設定の **Use background service (daemon)** をオフにして試してください。
オンのままだと、インストール済みサービスが旧版のコアを起動する可能性があります。
この配布物は既存インストールのファイルやサービスを自動で置き換えません。

### AltとCommandを入れ替える場合

WindowsのWinキーは `Super`、右Altは `AltGr` として送信されます。
左右AltをMacのCommand、WinキーをOptionにするには、Macの画面設定を
`Alt → Super`、`AltGr → Super`、`Super → Alt` にします。
この設定を使う場合、下の表のAlt長押しの期待値はOptionではなくCommandです。
`Meta → Alt` だけではWindowsのWinキーを変換できません。

今回のIME対応の対象は `imeoff` / `imeon` です。`lang1` / `lang2` は
別のWindows仮想キーを送信するため、同じ対応としては扱えません。

Macのテキストエディターで、次を確認します。

| 操作 | 期待する結果 |
| --- | --- |
| 左Altを200ms未満で短押し | 英数へ切り替わる |
| 右Altを200ms未満で短押し | 日本語入力へ切り替わる |
| 左Alt／右Altを長押し | 左Option／右Optionとして保持される |
| 英数にした後、左右のShiftをそれぞれ押して離す | かなへ切り替わらない |
| Shiftを保持して文字を入力 | 大文字になる |
| 左右のShiftを押し、片方だけ離す | 残ったShiftが有効なまま |
| Altを短押しした後、通常の文字を入力 | Altが押しっぱなしにならない |
| キーを保持して画面を移動し、離す | キーが押しっぱなしにならない |

長押し時に左右を直接確認する場合はMacのキーイベント表示ツールを利用します。
症状が残る場合、両側のDeskflowをVerboseログにして、上記操作を一つずつ行います。
送信側の `onKeyDown` は英数が `id=61226`（0xEF2A）、かなが `id=61219`
（0xEF23）になり、別の `button` 値を持つことを確認します。
Mac側は `virtualKey=0x0066` が英数、`0x0068` がかなです。
Shiftの `virtualKey` は左が `0x0038`、右が `0x003c` です。
ログには入力内容が含まれる場合があるため、確認用の文字だけを入力してください。

## ビルドとテスト

基本手順は [公式ビルドガイド](https://github.com/deskflow/deskflow/wiki/Building) を参照してください。
追加したテストはWindowsの `MSWindowsKeyStateTests`、共通の `KeyMapTests`、
Macの `OSXKeyStateTests` です。

Windows上のテストだけでは、MacのIMEや二台間の実際のフック順序までは検証できません。
Mac用テストには既存の実入力を生成するテストも含まれるため、ビルド／実行時は
入力先に注意してください。

今回のWindows検証では、MSVC・Qt 6.8.3・OpenSSL 3.6.4でGUI、コア、デーモンを
ビルドし、`MSWindowsKeyStateTests` / `KeyMapTests` / `KeyStateTests` /
`IKeyStateTests` の4組がすべて成功しました。配布フォルダーからのコアの
`--version` 実行も成功しています。Macでのビルド、Mac用テスト、二台間の実機確認は未実施です。
