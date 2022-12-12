# checkclinfo

checkclinfoはOpenCLによるGPUの認識状況を確認するためのアプリです。

[QSVEnc](https://github.com/rigaya/QSVEnc)/[VCEEnc](https://github.com/rigaya/VCEEnc)/[clfilters](https://github.com/rigaya/clfilters)等でのOpenCLチェックルーチンを切り出したものです。

## 想定動作環境

Windows 8.1/10/11 (x86/x64)  
Intel / NVIDIA / AMD のGPUドライバのインストールされた環境  

## checkclinfo 使用にあたっての注意事項

無保証です。自己責任で使用してください。clfiltersを使用したことによる、いかなる損害・トラブルについても責任を負いません。  

## 使用方法

```checkclinfo``` 実行で、OpenCLの認識するGPUの詳細情報を表示します。

そのほか、OpenCLの認識するGPU名のみ表示することもできます。

checkclinfo -p intel --devname-only
```
device: Intel(R) Arc(TM) A380 Graphics
device: Intel(R) UHD Graphics 770
```

checkclinfo -p nvidia --devname-only
```
device: NVIDIA GeForce RTX 4080
device: NVIDIA GeForce GTX 1060 6GB
```

戻り値は、デバイスが見つかった場合は0を、見つからなかった場合は1を返します。

## オプション

### -p &lt;string&gt;
対象のplatformの選択。  
- intel
- nvidia
- amd

### --devname-only
デバイス名のみ表示する。

## checkclinfo ソースコード
- MIT license.




