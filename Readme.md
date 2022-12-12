# checkclinfo

checkclinfo will show result of GPU detection via OpenCL.

This is equivalent to OpenCL check routines in [QSVEnc](https://github.com/rigaya/QSVEnc)/[VCEEnc](https://github.com/rigaya/VCEEnc)/[clfilters](https://github.com/rigaya/clfilters).

## System Requirements
### Windows
Windows 8.1/10/11 (x86/x64)  

## Precautions for using checkclinfo
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND.

## Usage

Running ```checkclinfo``` will show all GPU information provided by OpenCL.

It is possible to check only GPU device name of selected platform.

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

Return value of the ```checkclinfo``` will be 0 when device was found, otherwise 1 will be returned.

## Options

### -p &lt;string&gt;
Select OpenCL platform to check.  
- intel
- nvidia
- amd

### --devname-only
Show device name only.

## checkclinfo source code
- MIT license.



