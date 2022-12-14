# checkhwenc

checkhwencはHWエンコーダ([QSVEnc](https://github.com/rigaya/QSVEnc)/[VCEEnc](https://github.com/rigaya/VCEEnc)/[VCEEnc](https://github.com/rigaya/VCEEnc))がお使いの環境で実行可能かを確認するためのアプリです。

[QSVEnc](https://github.com/rigaya/QSVEnc)/[QSVEnc](https://github.com/rigaya/NVEnc)/[VCEEnc](https://github.com/rigaya/VCEEnc)のチェックルーチンを切り出し、統合したものです。

## 想定動作環境

Windows 8.1/10/11 (x86/x64)  
Intel / NVIDIA / AMD のGPUドライバのインストールされた環境  

## checkhwenc 使用にあたっての注意事項

無保証です。自己責任で使用してください。clfiltersを使用したことによる、いかなる損害・トラブルについても責任を負いません。  

## 使用方法

```checkhwenc``` 実行で、その環境で実行可能なHWエンコードを表示します。

そのほか、```--encoder```オプションをつけることで、それぞれのHWエンコーダについて個別に確認できます。

戻り値は、デバイスが見つかった場合は0を、見つからなかった場合は1を返します。

## オプション

#### --encoder &lt;string&gt;

- qsv
Intel QSVが実行可能か確認します。

- nvenc
NVIDIA NVENCが実行可能か確認します。

- vce
- vcn
AMD VCE/VCNが実行可能か確認します。

### --opencl [&lt;string&gt;]
OpenCLをチェックするモードに切り替えます。

デフォルトではすべてのplatformのチェックを行いますが、引数で対象のplatformの選択できます。

- intel
- nvidia
- amd

### --opencl-name-only
OpenCLをチェックするモードで、デバイス名のみ表示します。

## checkhwenc ソースコード
- MIT license.




