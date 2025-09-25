# About DompdfUI

This is a portable application that implements a command line interface for the Dompdf library on Windows and Linux operating systems. There are no external dependencies since static linking is used, and the PHP interpreter and the Dompdf library itself are embedded inside the executable file. All you need to use it is to download and run it in the terminal. Dompdfui is not a full featured html to pdf converter, but is only intended for testing the Dompdf library in different environments.

## Usage

```
dompdfui [OPTIONS] INPUT-FILE1 [INPUT-FILE2] [INPUT-FILE3] [...] OUTPUT-DIR
```

At least one input file and output directory must be specified. The output file is saved in the specified directory with the extension changed to pdf. Options are divided into two categories: for application and for Dompdf library.

### Application Options

| Long Option | Short Option | Default | Description |
| ----------- | ------------ | ------- | ----------- |
| -v | --version || print version |
| -h | --help || print help message |
| -n | --no-clean || don't clean temp files on exit |
| -m | --php-memory-limit | 268435456 | Limits the amount of memory (in bytes) a php-cli can use |
| -f | --force-out || replace output file if exists |

### Dompdf library Options

| Option | Default | Description |
| ------ | ------- | ----------- |
| --isPhpEnabled | 0 |  |
| --isRemoteEnabled | 0 |  |
| --isPdfAEnabled | 0 |  |
| --isJavascriptEnabled | 1 |  |
| --isHtml5ParserEnabled | 1 |  |
| --isFontSubsettingEnabled | 1 |  |
| --debugPng | 0 |  |
| --debugKeepTemp | 0 |  |
| --debugCss | 0 |  |
| --debugLayout | 0 |  |
| --debugLayoutLines | 1 |  |
| --debugLayoutBlocks | 1 |  |
| --debugLayoutInline | 1 |  |
| --debugLayoutPaddingBox | 1 |  |
| --dpi | 96 |  |
| --fontHeightRatio | 1.1 |  |
| --rootDir ||  |
| --tempDir ||  |
| --fontDir ||  |
| --fontCache ||  |
| --logOutputFile ||  |
| --defaultMediaType | screen |  |
| --defaultPaperSize | a4 |  |
| --defaultPaperOrientation | portrait |  |
| --defaultFont | dejavu serif |  |
| --pdfBackend | CPDF |  |
| --pdflibLicense ||  |
| --chroot ||  |
| --allowedRemoteHosts ||  |
