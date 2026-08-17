# About

This is a small, portable command-line tool that combines [Static-PHP-cli](https://github.com/crazywhalecc/static-php-cli) and the [Dompdf library](https://github.com/dompdf/dompdf) to convert HTML files to PDF on Windows and Linux, without requiring a separate installation of PHP or Dompdf. It is designed for users who need a standalone, offline and single-executable HTML-to-PDF converter. There are no external dependencies, as it uses static linking, and both the PHP interpreter and the Dompdf library itself are built into the executable file. All you need to get started is to download it and run it in a terminal.

## Features

 * Handles most CSS 2.1 and a few CSS3 properties, including @import, @media &
   @page rules
 * Supports most presentational HTML 4.0 attributes
 * Supports external stylesheets, either local or through http/ftp (via
   fopen-wrappers)
 * Supports complex tables, including row & column spans, separate & collapsed
   border models, individual cell styling
 * Image support (gif, png (8, 24 and 32 bit with alpha channel), bmp & jpeg)
 * No dependencies on external PDF libraries, thanks to the R&OS PDF class
 * Basic SVG support (see "Limitations" below)

## Limitations (Known Issues)

 * Table cells are not pageable, meaning a table row must fit on a single page: See https://github.com/dompdf/dompdf/issues/98
 * Elements are rendered on the active page when they are parsed.
 * Embedding "raw" SVG's (`<svg><path...></svg>`) isn't working yet: See https://github.com/dompdf/dompdf/issues/320  
   Workaround: Either link to an external SVG file, or use a DataURI like this:
     ```php
     $html = '<img src="data:image/svg+xml;base64,' . base64_encode($svg) . '">';
     ```
 * Does not support CSS flexbox: See https://github.com/dompdf/dompdf/issues/971
 * Does not support CSS Grid: See https://github.com/dompdf/dompdf/issues/2988

## Usage

You can build from source or download release from [Releases](https://github.com/abramov7613/dompdfui/releases)

```
dompdfui [OPTIONS] INPUT-FILE1 [INPUT-FILE2] [INPUT-FILE3] [...] OUTPUT-DIR
```

At least one input file and output directory must be specified. The program extracts the PHP interpreter and Dompdf library into a temporary directory, and deletes it after completion. The output files are saved in the specified directory with the extension changed to pdf. Options are divided into two categories: for application and for Dompdf library:

### Application Options

| Short Option | Long Option | Default | Description |
| ----------- | ------------ | ------- | ----------- |
| `-v` | `--version` || print version |
| `-h` | `--help` || print help message |
| `-m` | `--php-memory-limit` | 268435456 | Limits the amount of memory (in bytes) a php-cli can use |
| `-f` | `--force-out` || replace output file if exists | |

### Dompdf library Options

| Option | Default | Description |
| ------ | ------- | ----------- |
| `--isRemoteEnabled` | false | Enable remote file access. If this setting is set to true, DOMPDF will access remote sites for images and CSS files as required. |
| `--isJavascriptEnabled` | true | Enable inline JavaScript. If this setting is set to true then DOMPDF will automatically insert JavaScript code contained within `<script type="text/javascript"> ... </script>` tags as written into the PDF. NOTE: This is PDF-based JavaScript to be executed by the PDF viewer, not browser-based JavaScript executed by Dompdf. |
| `--isFontSubsettingEnabled` | true | Whether to enable font subsetting or not. |
| `--sslAllowSelfSigned` | false | Enable downloading fonts or images that are hosted on a server with a self-signed security certificate or other certificate problems; ignore if `--isRemoteEnabled=false` |
| `--dpi` | 96 | Image DPI setting. This setting determines the default DPI setting for images and fonts. The DPI may be overridden for inline images by explicitly setting the image's width & height style attributes (i.e. if the image's native width is 600 pixels and you specify the image's width as 72 points, the image will have a DPI of 600 in the rendered PDF. The DPI of background images can not be overridden and is controlled entirely via this parameter. For the purposes of DOMPDF, pixels per inch (PPI) = dots per inch (DPI). If a size in html is given as px (or without unit as image size), this tells the corresponding size in pt at 72 DPI. This adjusts the relative sizes to be similar to the rendering of the html page in a reference browser. In pdf, always 1 pt = 1/72 inch. |
| `--fontHeightRatio` | 1.1 | A ratio applied to the fonts height to be more like browsers line height. |
| `--defaultMediaType` | screen | Styles targeted to this media type are applied to the document. This is on top of the media types that are always applied: all, static, visual, bitmap, paged, dompdf. |
| `--defaultPaperSize` | a4 | The default paper size. Available values: "4a0", "2a0", "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7", "a8", "a9", "a10", "b0", "b1", "b2", "b3", "b4", "b5", "b6", "b7", "b8", "b9", "b10", "c0", "c1", "c2", "c3", "c4", "c5", "c6", "c7", "c8", "c9", "c10", "ra0", "ra1", "ra2", "ra3", "ra4", "sra0", "sra1", "sra2", "sra3", "sra4", "letter", "half-letter", "legal", "ledger", "tabloid", "executive", "folio", "commercial #10 envelope", "catalog #10 1/2 envelope", "8.5x11", "8.5x14", "11x17". North America standard is "letter"; other countries generally "a4". |
| `--defaultPaperOrientation` | portrait | The orientation of the page ('portrait' or 'landscape'). |
| `--defaultFont` | dejavu serif | Used if no suitable fonts can be found. This must exist in the font folder. |
| `--allowedRemoteHosts` || It is Array of strings and can be specify multiple times. List of allowed remote hosts. Each value of the array must be a valid hostname. This will be used to filter which resources can be loaded in combination with isRemoteEnabled. If isRemoteEnabled is FALSE, then this will have no effect. Allow any remote host if not specified. |

Additional documentation is available at the [Dompdf Wiki](https://github.com/dompdf/dompdf/wiki)

## Build from source

Regardless of your system, you need to install:
 - GCC 12.2 or higher (MinGW on Windows)
 - CMake 3.31 or higher
 - Boost 1.86 or higher
 - Git 2.40 or higher

You will also need access to https://github.com and https://static-php.dev during the CMake configuration stage.

```shell
git clone https://github.com/abramov7613/dompdfui.git
cd dompdfui
# for Linux
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
# for Windows
cmake -G "MinGW Makefiles" -S . -B build -DCMAKE_BUILD_TYPE=Release -DBoost_DIR="your/boost/installation/location"
cmake --build build
cd build
ctest -V
```
After that you will find target executable in build directory.
