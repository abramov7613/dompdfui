# About DompdfUI

This is a portable application that implements a command line interface for the Dompdf library on Windows and Linux operating systems. There are no external dependencies since static linking is used, and the PHP interpreter and the Dompdf library itself are embedded inside the executable file. All you need to use it is to download and run it in the terminal. Dompdfui is not a full featured html to pdf converter, but is only intended for testing the Dompdf library in different environments.

## Usage

```
dompdfui [OPTIONS] INPUT-FILE1 [INPUT-FILE2] [INPUT-FILE3] [...] OUTPUT-DIR
```

At least one input file and output directory must be specified. The output file is saved in the specified directory with the extension changed to pdf. Options are divided into two categories: for application and for Dompdf library.

### Application Options

| Short Option | Long Option | Default | Description |
| ----------- | ------------ | ------- | ----------- |
| -v | --version || print version |
| -h | --help || print help message |
| -n | --no-clean || don't clean temp files on exit |
| -m | --php-memory-limit | 268435456 | Limits the amount of memory (in bytes) a php-cli can use |
| -f | --force-out || replace output file if exists |

### Dompdf library Options

| Option | Default | Description |
| ------ | ------- | ----------- |
| --isPhpEnabled | 0 | Enable embedded PHP. If this setting is set to true then DOMPDF will automatically evaluate embedded PHP contained within \<script type="text/php"\> ... \</script\> tags. Enabling this for documents you do not trust (e.g. arbitrary remote html pages) is a security risk. Embedded scripts are run with the same level of system access available to dompdf. Set this option to false (recommended) if you wish to process untrusted documents. This setting may increase the risk of system exploit. Do not change this settings without understanding the consequences. |
| --isRemoteEnabled | 0 | Enable remote file access  If this setting is set to true, DOMPDF will access remote sites for  images and CSS files as required.  This can be a security risk, in particular in combination with isPhpEnabled and  allowing remote html code to be passed to $dompdf = new DOMPDF(); $dompdf->load_html(...);  This allows anonymous users to download legally doubtful internet content which on  tracing back appears to being downloaded by your server, or allows malicious php code  in remote html pages to be executed by your server with your account privileges.  This setting may increase the risk of system exploit. Do not change  this settings without understanding the consequences. |
| --isPdfAEnabled | 0 | Enable PDF/A-3 compliance mode. This feature is currently only supported with the CPDF backend and will have no effect if used with any other. Currently this mode only takes care of adding the necessary metadata, output intents, etc. It does not enforce font embedding, it's up to you to embed the fonts you plan on using. |
| --isJavascriptEnabled | 1 | Enable inline JavaScript. If this setting is set to true then DOMPDF will automatically insert JavaScript code contained within \<script type="text/javascript"\> ... \</script\> tags as written into the PDF. NOTE: This is PDF-based JavaScript to be executed by the PDF viewer, not browser-based JavaScript executed by Dompdf. |
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

Additional documentation is available on the dompdf wiki at: https://github.com/dompdf/dompdf/wiki
