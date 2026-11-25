#!/usr/bin/env python3
"""Draws docs/workflow.png (signal flow + DAW sync). Run: python3 tools/make_workflow_image.py"""
import os
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.patches import FancyBboxPatch, FancyArrowPatch

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BG, YEL = "#0b0b14", "#ffd84d"
fig, ax = plt.subplots(figsize=(16, 8.4), dpi=120)
fig.patch.set_facecolor(BG); ax.set_facecolor(BG)
ax.set_xlim(0, 164); ax.set_ylim(0, 84); ax.axis("off")

W, GAP, X0 = 22.5, 3.0, 5.0
def colx(c): return X0 + (c - 1) * (W + GAP)

def box(c, y, h, title, sub, col):
    x = colx(c)
    ax.add_patch(FancyBboxPatch((x, y), W, h, boxstyle="round,pad=0.4,rounding_size=1.8", fc="#12121f", ec=col, lw=2.2))
    ax.text(x + W / 2, y + h - 3.0, title, color=col, ha="center", va="center", fontsize=11.5, fontweight="bold")
    ax.text(x + W / 2, y + h / 2 - 2.0, sub, color="#cfcfe6", ha="center", va="center", fontsize=8.6, linespacing=1.5)

def arrow(p1, p2, col="#8a8aa8", ls="-", lw=2.0, head=True):
    ax.add_patch(FancyArrowPatch(p1, p2, arrowstyle="-|>" if head else "-", mutation_scale=15, color=col, lw=lw, ls=ls))

ax.text(82, 80, "PSYGOA SYNTH  -  VST3 for Psytrance & Goa Trance", color="#e8e8ff", ha="center", fontsize=22, fontweight="bold")
ax.text(82, 75.6, "signal flow  +  DAW sync", color="#8a8aa8", ha="center", fontsize=12)

# transport bar
ax.add_patch(FancyBboxPatch((X0, 60.5), colx(6) + W - X0, 9, boxstyle="round,pad=0.4,rounding_size=1.8", fc="#12121f", ec=YEL, lw=2.2))
ax.text(82, 67.0, "DAW HOST TRANSPORT", color=YEL, ha="center", fontsize=12, fontweight="bold")
ax.text(82, 63.2, "tempo (BPM) + song position (PPQ) + play state  ->  LFOs, gate steps and delay time follow the grid",
        color="#cfcfe6", ha="center", fontsize=9)

YB, HB, YC, HC = 36, 17, 10, 17
# row B (left -> right)
box(1, YB, HB, "MIDI IN", "notes, velocity, pitch bend\nvelocity = acid accent", "#9aa0ff")
box(2, YB, HB, "FM SYNTH x8", "FM + feedback, saw, sub\nscale quantizer\n(phrygian / arabic ...)", "#00e5ff")
box(3, YB, HB, "ACID FILTER", "303 ladder / SVF\nenv + synced LFO\nresonance, drive", "#ff9100")
box(4, YB, HB, "TRANCE GATE", "16 steps\n1/4 ... 1/32 + triplets\nsmoothed", "#ff2bd6")
box(5, YB, HB, "PHASER", "4 - 12 stages\nsynced sweep\nfeedback", "#9d6bff")
box(6, YB, HB, "FLANGER", "synced sweep\n+/- feedback\nstereo offset", "#00ffa8")
# row C (right -> left)
box(6, YC, HC, "PING-PONG DELAY", "1/8. default\nL <-> R, low/high cut\ntempo synced", "#ff5c8a")
box(5, YC, HC, "REVERB", "size, damping\nwidth, mix", "#4d8dff")
box(4, YC, HC, "MASTER", "gain + soft clip\n-> stereo out", "#e0e0ff")
box(3, YC, HC, "AUDIO IN (opt.)", "pads / vocals through\nfilter, gate and FX", "#b6ff00")

ymid = YB + HB / 2
for c, col in ((1, "#9aa0ff"), (2, "#00e5ff"), (3, "#ff9100"), (4, "#ff2bd6"), (5, "#9d6bff")):
    arrow((colx(c) + W + 0.6, ymid), (colx(c + 1) - 0.6, ymid), col)
arrow((colx(6) + W / 2, YB - 0.6), (colx(6) + W / 2, YC + HC + 0.6), "#00ffa8")                      # flanger -> delay
ymc = YC + HC / 2
arrow((colx(6) - 0.6, ymc), (colx(5) + W + 0.6, ymc), "#ff5c8a")                                      # delay -> reverb
arrow((colx(5) - 0.6, ymc), (colx(4) + W + 0.6, ymc), "#4d8dff")                                      # reverb -> master
arrow((colx(4) + W / 2, YC - 0.6), (colx(4) + W / 2, 3.6), "#e0e0ff")                                 # master -> out
ax.text(colx(4) + W / 2, 1.6, "STEREO OUT", color="#e0e0ff", ha="center", fontsize=10, fontweight="bold")
arrow((colx(3) + W / 2, YC + HC + 0.6), (colx(3) + W / 2, YB - 0.6), "#b6ff00")                       # audio in -> filter

# DAW sync (dashed yellow)
for c, lbl in ((3, "LFO sync"), (4, "step sync"), (5, "LFO sync"), (6, "LFO sync")):
    x = colx(c) + W / 2
    arrow((x, 60.0), (x, YB + HB + 0.6), YEL, ls="--", lw=1.6)
    ax.text(x + 1.2, 56.6, lbl, color=YEL, fontsize=8)
xr = colx(6) + W + 2.4                                                                                  # delay time sync, routed along the right edge
arrow((xr, 62.5), (xr, ymc), YEL, ls="--", lw=1.6, head=False)
arrow((xr, ymc), (colx(6) + W + 0.7, ymc), YEL, ls="--", lw=1.6)
ax.text(xr - 1.0, 28.2, "time\nsync", color=YEL, fontsize=8, ha="right")

# legend
ax.add_patch(FancyBboxPatch((colx(1), YC), 2 * W + GAP, HC, boxstyle="round,pad=0.4,rounding_size=1.8", fc="#0f0f19", ec="#3a3a55", lw=1.5))
ax.text(colx(1) + 2, YC + HC - 3.2, "How it syncs", color="#e8e8ff", fontsize=11, fontweight="bold")
ax.text(colx(1) + 2, YC + 7.6,
        "playing: locked to the DAW song position (PPQ)\nstopped: free-running clock at host BPM\nno host tempo: 145 BPM fallback\ndashed yellow = tempo-synced parameter",
        color="#cfcfe6", fontsize=8.6, linespacing=1.55, va="center")

out = os.path.join(ROOT, "docs", "workflow.png")
os.makedirs(os.path.dirname(out), exist_ok=True)
fig.savefig(out, facecolor=BG, bbox_inches="tight", pad_inches=0.3)
print("wrote", out)
