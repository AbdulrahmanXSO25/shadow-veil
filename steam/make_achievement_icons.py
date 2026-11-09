"""Renders 256x256 achievement icons for Steamworks from steam/achievements.csv.
Achieved: the red seal with the achievement name; locked: the same in grey.
Run: python3 steam/make_achievement_icons.py"""
import csv
import os
from PIL import Image, ImageDraw, ImageFont, ImageOps

HERE = os.path.dirname(__file__)
ROOT = os.path.join(HERE, "..")
FONT_JP = os.path.join(ROOT, "assets", "fonts", "NotoSerifCJKjp-Bold-subset.otf")
FONT = os.path.join(ROOT, "assets", "fonts", "NotoSerif-SemiBold.ttf")
OUT = os.path.join(HERE, "achievement_icons")


def icon(title):
    S = 256
    im = Image.new("RGB", (S, S), (12, 10, 14))
    d = ImageDraw.Draw(im)
    d.rounded_rectangle([28, 20, S - 28, S - 64], radius=14, fill=(184, 28, 36))
    f = ImageFont.truetype(FONT_JP, 112)
    b = d.textbbox((0, 0), "影", font=f)
    d.text(((S - (b[2] - b[0])) / 2 - b[0], 20 + (S - 84 - (b[3] - b[1])) / 2 - b[1]), "影", font=f, fill=(246, 236, 220))
    size = 22
    ft = ImageFont.truetype(FONT, size)
    while d.textlength(title, font=ft) > S - 20 and size > 12:
        size -= 1
        ft = ImageFont.truetype(FONT, size)
    d.text(((S - d.textlength(title, font=ft)) / 2, S - 50), title, font=ft, fill=(226, 184, 103))
    return im


def main():
    os.makedirs(OUT, exist_ok=True)
    with open(os.path.join(HERE, "achievements.csv"), newline="", encoding="utf-8") as fh:
        rows = list(csv.DictReader(fh))
    for r in rows:
        im = icon(r["display_name"])
        im.save(os.path.join(OUT, f"{r['api_name']}.jpg"), quality=92)
        ImageOps.grayscale(im).point(lambda v: v * 0.55).save(os.path.join(OUT, f"{r['api_name']}_locked.jpg"), quality=92)
    print(f"{len(rows)} icon pairs -> {OUT}")


if __name__ == "__main__":
    main()
