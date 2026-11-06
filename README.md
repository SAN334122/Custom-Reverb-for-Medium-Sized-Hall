## Overview

This project presents an algorithmic reverberator VST3 plugin optimized for medium- to large-sized halls, built in C++ using the JUCE framework. 

### Key Specifications & Features
* **12×6 Coprime Topology:** Incorporates 12 parallel Low-Pass Comb Filters (LBCFs) and 6 series All-Pass Filters (APFs) with incommensurate delay lengths to maximize modal and echo density while avoiding structural resonance overlap.
* **FOH Low-End Management:** Features a 150 Hz series input High-Pass Filter (HPF) on the wet path to protect PA systems from low-frequency buildup and preserve subwoofer headroom.
* **Control Parameters:** Exposes smooth, real-time controls for **Dry/Wet** mix and **Room Size** scaling.

### Project Goals
Designed as an applied DSP research project to evaluate the electroacoustic trade-offs between modal density, tail diffusion, real-time memory access, and execution performance in digital reverberation engines.
