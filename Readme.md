# checkhwenc

checkhwenc will show HW encoder([QSVEnc](https://github.com/rigaya/QSVEnc)/[VCEEnc](https://github.com/rigaya/VCEEnc)/[VCEEnc](https://github.com/rigaya/VCEEnc)) availability on your system.

This is equivalent to check routines in [QSVEnc](https://github.com/rigaya/QSVEnc)/[VCEEnc](https://github.com/rigaya/VCEEnc)/[VCEEnc](https://github.com/rigaya/VCEEnc).

## System Requirements
### Windows
Windows 8.1/10/11 (x86/x64)  

## Precautions for using checkhwenc
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND.

## Usage

Running ```checkhwenc``` will show all supported HW encoder on your system.

It is possible to check each hw encoder by adding ```--encoder``` option.

```checkhwenc``` will return 0 is encoder is avaialble, and 1 if not.

## Options

### --encoder &lt;string&gt;

- qsv
Check whether Intel QSV is available.

- nvenc
Check whether NVIDIA NVENC is available.

- vce
- vcn
Check whether AMD VCE/VCN is available.

## checkhwenc source code
- MIT license.



