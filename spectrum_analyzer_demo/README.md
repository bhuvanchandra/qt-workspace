Spectrum Analyzer Demo
======================

This demo is a simple Qt based UI and uses iio utils helper functions
to get the data from the rpmsg iio driver. It uses QtCustom plot external
library(included in this project) for plotting the data.

_NOTE:_ This application only plots the data. ADC data acquisition and FFT transform is done at M4 side. FreeRTOS firmware for M4 side of spectrum analyzer demo project is available [here.](https://github.com/bhuvanchandra/freertos-toradex-vf6xx-m4/tree/colibri-vf61-m4-freertos-v8/examples/vf6xx_colibri_m4/demo_apps/spectrum-analyzer_sample)

![spectrum-analyzer-demo-screenshot](https://raw.githubusercontent.com/bhuvanchandra/images-repo/master/spectrum-analyzer.jpg)
