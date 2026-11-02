#!/usr/bin/env python3
"""Generate original geometric SVG demo scenes. MIT, copyright 2026 lhfchym.
Optional PNG rendering: install cairosvg, then pass --png-dir DIRECTORY.
"""
import argparse
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('--png-dir',type=Path);args=p.parse_args()
scenes=[
 ('demo01','Amber horizon','#f5deac','#d9855b','#ddab70','#697d79','#294f57'),
 ('demo02','Blue hour','#a2bfcc','#6c81b6','#b7c5d5','#536b91','#293e69'),
 ('demo03','Forest light','#d9e6bb','#91b6a8','#a9c4ae','#558c83','#285e60'),
 ('demo04','Desert dusk','#efc7ae','#c98491','#dba48c','#aa747e','#674f70'),
 ('demo05','Winter lake','#e6edf3','#adc9d8','#c5dbe5','#80a8bc','#436b89'),
 ('demo06','Violet coast','#d9c7e5','#9993ba','#bcafce','#827caa','#514c7c'),
]
for n,(key,title,top,bottom,sun,mid,front) in enumerate(scenes):
 x=1120 if n%2==0 else 480
 svg=f'''<svg xmlns="http://www.w3.org/2000/svg" width="1600" height="1000" viewBox="0 0 1600 1000">
<title>{title} — original geometric demo artwork</title>
<defs><linearGradient id="sky" x2="0" y2="1"><stop stop-color="{top}"/><stop offset="1" stop-color="{bottom}"/></linearGradient></defs>
<path fill="url(#sky)" d="M0 0H1600V1000H0Z"/>
<circle cx="{x}" cy="310" r="126" fill="{sun}"/>
<path d="M0 610L230 470 440 560 740 350 1010 600 1320 450 1600 570V1000H0Z" fill="{mid}"/>
<path d="M0 750Q260 600 560 745T1120 735T1600 720V1000H0Z" fill="{front}"/>
<path d="M0 863Q350 770 780 870T1600 855" fill="none" stroke="{top}" stroke-width="3" opacity=".35"/>
<path d="M0 908Q430 820 900 920T1600 900" fill="none" stroke="{top}" stroke-width="2" opacity=".22"/>
</svg>'''
 path=Path(__file__).parent/(key+'.svg');path.write_text(svg)
 if args.png_dir:
  import cairosvg
  args.png_dir.mkdir(parents=True,exist_ok=True)
  cairosvg.svg2png(bytestring=svg.encode(),write_to=str(args.png_dir/f'wallhaven-{key}-1600x1000.png'))
