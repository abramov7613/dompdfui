# About DompdfUI

This is a portable application that implements a command line interface for the [Dompdf library](https://github.com/dompdf/dompdf/wiki) on Windows and Linux operating systems. There are no external dependencies since static linking is used, and the PHP interpreter and the Dompdf library itself are embedded inside the executable file. All you need to use it is to download and run it in the terminal. Dompdfui is not a full-featured HTML to PDF converter; it is only a wrapper around the Dompdf library and is intended for testing in various environments.

## Usage

You can build from source or download release from https://github.com/abramov7613/dompdfui/releases

```
dompdfui [OPTIONS] INPUT-FILE1 [INPUT-FILE2] [INPUT-FILE3] [...] OUTPUT-DIR
```

At least one input file and output directory must be specified. The output file is saved in the specified directory with the extension changed to pdf. Options are divided into two categories: for application and for Dompdf library.

### Application Options

| Short Option | Long Option | Default | Description |
| ----------- | ------------ | ------- | ----------- |
| `-v` | `--version` || print version |
| `-h` | `--help` || print help message |
| `-n` | `--no-clean` || don't clean temp files on exit |
| `-m` | `--php-memory-limit` | 268435456 | Limits the amount of memory (in bytes) a php-cli can use |
| `-f` | `--force-out` || replace output file if exists |

### Dompdf library Options

| Option | Default | Description |
| ------ | ------- | ----------- |
| `--isPhpEnabled` | false | Enable embedded PHP. If this setting is set to true then DOMPDF will automatically evaluate embedded PHP contained within `<script type="text/php"> ... </script>` tags. Enabling this for documents you do not trust (e.g. arbitrary remote html pages) is a security risk. Embedded scripts are run with the same level of system access available to dompdf. Set this option to false (recommended) if you wish to process untrusted documents. This setting may increase the risk of system exploit. Do not change this settings without understanding the consequences. |
| `--isRemoteEnabled` | false | Enable remote file access. If this setting is set to true, DOMPDF will access remote sites for  images and CSS files as required. This can be a security risk, in particular in combination with `isPhpEnabled` and  allowing remote html code to be passed to `$dompdf = new DOMPDF(); $dompdf->load_html(...);` This allows anonymous users to download legally doubtful internet content which on tracing back appears to being downloaded by your server, or allows malicious php code in remote html pages to be executed by your server with your account privileges. This setting may increase the risk of system exploit. Do not change this settings without understanding the consequences. |
| `--isPdfAEnabled` | false | Enable PDF/A-3 compliance mode. This feature is currently only supported with the CPDF backend and will have no effect if used with any other. Currently this mode only takes care of adding the necessary metadata, output intents, etc. It does not enforce font embedding, it's up to you to embed the fonts you plan on using. |
| `--isJavascriptEnabled` | true | Enable inline JavaScript. If this setting is set to true then DOMPDF will automatically insert JavaScript code contained within `<script type="text/javascript"> ... </script>` tags as written into the PDF. NOTE: This is PDF-based JavaScript to be executed by the PDF viewer, not browser-based JavaScript executed by Dompdf. |
| `--isHtml5ParserEnabled` | true | Use the HTML5 Lib parser. Deprecated |
| `--isFontSubsettingEnabled` | true | Whether to enable font subsetting or not. |
| `--debugPng` | false |  |
| `--debugKeepTemp` | false |  |
| `--debugCss` | false |  |
| `--debugLayout` | false |  |
| `--debugLayoutLines` | true |  |
| `--debugLayoutBlocks` | true |  |
| `--debugLayoutInline` | true |  |
| `--debugLayoutPaddingBox` | true |  |
| `--dpi` | 96 | Image DPI setting. This setting determines the default DPI setting for images and fonts. The DPI may be overridden for inline images by explicitly setting the image's width & height style attributes (i.e. if the image's native width is 600 pixels and you specify the image's width as 72 points, the image will have a DPI of 600 in the rendered PDF. The DPI of background images can not be overridden and is controlled entirely via this parameter. For the purposes of DOMPDF, pixels per inch (PPI) = dots per inch (DPI). If a size in html is given as px (or without unit as image size), this tells the corresponding size in pt at 72 DPI. This adjusts the relative sizes to be similar to the rendering of the html page in a reference browser. In pdf, always 1 pt = 1/72 inch. |
| `--fontHeightRatio` | 1.1 | A ratio applied to the fonts height to be more like browsers line height. |
| `--rootDir` || The root of your DOMPDF installation. |
| `--tempDir` || The location of a temporary directory. The directory specified must be writable by the executing process. The temporary directory is required to download remote images and when using the PFDLib back end. |
| `--fontDir` || The location of the DOMPDF font directory. The location of the directory where DOMPDF will store fonts and font metrics. Note: This directory must exist and be writable by the executing process. |
| `--fontCache` || The location of the DOMPDF font cache directory. This directory contains the cached font metrics for the fonts used by DOMPDF. This directory can be the same as $fontDir. Note: This directory must exist and be writable by the executing process. |
| `--logOutputFile` ||  |
| `--defaultMediaType` | screen | Styles targeted to this media type are applied to the document. This is on top of the media types that are always applied: all, static, visual, bitmap, paged, dompdf. |
| `--defaultPaperSize` | a4 | The default paper size. Available values: "4a0", "2a0", "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7", "a8", "a9", "a10", "b0", "b1", "b2", "b3", "b4", "b5", "b6", "b7", "b8", "b9", "b10", "c0", "c1", "c2", "c3", "c4", "c5", "c6", "c7", "c8", "c9", "c10", "ra0", "ra1", "ra2", "ra3", "ra4", "sra0", "sra1", "sra2", "sra3", "sra4", "letter", "half-letter", "legal", "ledger", "tabloid", "executive", "folio", "commercial #10 envelope", "catalog #10 1/2 envelope", "8.5x11", "8.5x14", "11x17". North America standard is "letter"; other countries generally "a4". |
| `--defaultPaperOrientation` | portrait | The orientation of the page (portrait or landscape). |
| `--defaultFont` | dejavu serif | Used if no suitable fonts can be found. This must exist in the font folder. |
| `--pdfBackend` | CPDF | The PDF rendering backend to use. Valid settings are 'PDFLib', 'CPDF', 'GD', and 'auto'. 'auto' will look for PDFLib and use it if found, or if not it will fall back on CPDF. 'GD' renders PDFs to graphic files. CanvasFactory object ultimately determines which rendering class to instantiate based on this setting. |
| `--pdflibLicense` || PDFlib license key. If you are using a licensed, commercial version of PDFlib, specify your license key here.  If you are using PDFlib-Lite or are evaluating the commercial version of PDFlib, comment out this setting. If pdflib present in web server and auto or selected explicitly above, a real license code must exist! |
| `--chroot` || It is Array of strings and can be specify multiple times. Utilized by Dompdf's default `file://` protocol URI validation rule. All local files opened by dompdf must be in a subdirectory of the directory or directories specified by this option. DO NOT set this value to '/' since this could allow an attacker to use dompdf to read any files on the server.  This should be an absolute path. IMPORTANT: This setting may increase the risk of system exploit. Do not change this settings without understanding the consequences. |
| `--allowedRemoteHosts` || It is Array of strings and can be specify multiple times. List of allowed remote hosts. Each value of the array must be a valid hostname. This will be used to filter which resources can be loaded in combination with isRemoteEnabled. If isRemoteEnabled is FALSE, then this will have no effect. Allow any remote host if not specified. |

Additional documentation is available on the dompdf wiki at: https://github.com/dompdf/dompdf/wiki

## Build

Regardless of your system, you need to install:
 - GCC 12.2 or higher (MinGW on Windows)
 - CMake 3.31 or higher
 - Boost 1.86 or higher
 - Git 2.40 or higher

You will also need access to https://github.com and https://static-php.dev during the CMake configuration stage.

### Linux

```
git clone https://github.com/abramov7613/dompdfui.git
cd dompdfui
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### Windows

```
git clone https://github.com/abramov7613/dompdfui.git
cd dompdfui
cmake -G "MinGW Makefiles" -S . -B build -DCMAKE_BUILD_TYPE=Release -DBoost_DIR="your/boost/installation/location" -DCMAKE_TLS_VERIFY=OFF
cmake --build build
```
After that you will find target executable in build directory.
