# Vendor Libraries

This directory contains third-party libraries used by the UI Framework.

## uPlot - Fast Time Series Charts

**Required for chart functionality.**

### Download uPlot

**Option 1: Manual Download**
1. Visit: https://github.com/leeoniya/uPlot/releases
2. Download latest release (v1.6.30 or newer)
3. Extract the following files to this directory:
   - `dist/uPlot.iife.min.js` → `uplot.min.js`
   - `dist/uPlot.min.css` → `uplot.min.css`

**Option 2: CDN (for testing)**
Replace the file references in the code with:
```html
<link rel="stylesheet" href="https://unpkg.com/uplot@1.6.30/dist/uPlot.min.css">
<script src="https://unpkg.com/uplot@1.6.30/dist/uPlot.iife.min.js"></script>
```

**Option 3: NPM (then copy to this directory)**
```bash
npm install uplot
cp node_modules/uplot/dist/uPlot.iife.min.js web/vendor/uplot.min.js
cp node_modules/uplot/dist/uPlot.min.css web/vendor/uplot.min.css
```

### File Sizes

- `uplot.min.js`: ~45KB (minified)
- `uplot.min.css`: ~2KB

Total: ~47KB (lazy-loaded only when charts are used)

### License

uPlot is MIT licensed.
Copyright (c) 2023 Leon Sorokin

### Why uPlot?

- **Fast**: Canvas-based, handles millions of points
- **Small**: ~45KB total (many charts are 200KB+)
- **Efficient**: Low memory usage
- **Responsive**: Mobile-friendly
- **No dependencies**: Vanilla JavaScript

### Alternatives

If you prefer a different charting library:
1. Modify `web/js/charts.js`
2. Update the `loadUPlot()` function
3. Adjust the chart creation code

Popular alternatives:
- Chart.js (~200KB) - Simpler API, less performant
- Plotly (~3MB) - Feature-rich, heavy
- ApexCharts (~500KB) - Good balance, heavier

## Current Status

⚠️ **uPlot files not included in repository**

Reason: Reduces repository size and avoids bundling third-party code.

To enable charts:
1. Download uPlot as described above
2. Place files in this directory
3. Charts will lazy-load automatically when needed

## Verifying Installation

After adding the files, check:
```bash
ls -lh web/vendor/
# Should show:
# uplot.min.js   (~45KB)
# uplot.min.css  (~2KB)
```

Then open the web UI and click on a sensor card - the chart should load.
