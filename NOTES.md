Just some notes

I am targeting 25fps on PAL, 30fps on NTSC for now. Ideally this fits in 512k, but it's unlikely.

The first thing is that I don't think we can fit more than 16 colors, it's just too much memory.
Also, the engine needs to be fast enough to simulate and redraw the entire map screen in just 2 frames.
Simulation is ok to run at 15fps, this is also what the original Warcraft used iirc. So we can slice it
like

D = animate & draw 25/s
S = simulate 15/s
P = panel updates (resources, progress bars, minimap) 2/s
A = pathfinding rounds 8/s

Frames . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
Action D S D S D P D S D S D A D S D S D A D S D S D A D S D S D P D S D S D A D S D S D A D S D S D A D A

Fog of war with patterns looks ok, but overlap is a problem and then we'll have to calculate more. Using
EHB would be great but that seems impossible given the DMA time cost. We can do poor-man's EHB and have colors
8 - 15 be darker shades(-ish) of colors 0-7, so we can do 1bpp circular D=A|C blits.

With 32x32 tiles and a manually tweaked loop, I can redraw the entire visible mapscreen in ~70% of one frame,
so we don't need to undraw units which saves frametime. 32x32 is a bit coarse for maps, but maybe ok. An
alternative may be to keep the entire map in CHIP and blit a section to the screen. If we only use colors 0-7
for actual terrain, this takes 384k for a 64x64 map with 16x16 tiles which is the standard size in Warcraft 1.
We then blit 3bpp interleaved to 4bpp interleaved by setting bltdmod to skip over bitplane 4. But we then need a
full 1bpp blit to clear bitplane 4. The maximum speed for both seems to be ~8ms, so less than 50% frame.

