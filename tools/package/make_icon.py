"""Renders the Shadow Veil app icon (the red 影 seal from the HUD) to PNG sizes and a Windows .ico.
Run: python3 tools/package/make_icon.py"""
import os
from PIL import Image, ImageDraw, ImageFont, ImageFilter

ROOT = os.path.join(os.path.dirname(__file__), "..", "..")
OUT = os.path.join(ROOT, "assets", "icon")
FONT = os.path.join(ROOT, "assets", "fonts", "NotoSerifCJKjp-Bold-subset.otf")


def seal(size):
    S = 1024
    im = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    # ink-black rounded tile with a vermilion seal
    d.rounded_rectangle([0, 0, S - 1, S - 1], radius=S * 0.18, fill=(12, 10, 14, 255))
    m = S * 0.14
    d.rounded_rectangle([m, m, S - m, S - m], radius=S * 0.06, fill=(184, 28, 36, 255))
    f = ImageFont.truetype(FONT, int(S * 0.58))
    box = d.textbbox((0, 0), "影", font=f)
    w, h = box[2] - box[0], box[3] - box[1]
    d.text(((S - w) / 2 - box[0], (S - h) / 2 - box[1]), "影", font=f, fill=(246, 236, 220, 255))
    return im.resize((size, size), Image.LANCZOS)


def main():
    os.makedirs(OUT, exist_ok=True)
    for s in (16, 32, 48, 64, 128, 256, 512):
        seal(s).save(os.path.join(OUT, f"shadow-veil-{s}.png"))
    seal(256).save(os.path.join(OUT, "shadow-veil.ico"), sizes=[(16, 16), (24, 24), (32, 32), (48, 48), (64, 64), (128, 128), (256, 256)])
    print("icons written to", OUT)


if __name__ == "__main__":
    main()
