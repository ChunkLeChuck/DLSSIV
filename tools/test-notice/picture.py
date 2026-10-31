"""THE NOTICE OVER A FRAME OF THE GAME, FOR A PERSON TO LOOK AT (2026-10-10). nt.exe --out <folder> writes the notice's
picture as the shipping code made it (notice-WxH-for-SWxSH.raw: b g r a, premultiplied). This lays it over a frame of
the game scaled to that screen size, with the blend the shipping draw uses (ONE, INVSRCALPHA) at the place the shipping
rule gives (centred, 3 percent from the top), and saves a PNG. It proves nothing: nt.exe does that.

    python tools\\test-notice\\picture.py <folder of nt.exe's --out> <a frame of the game.png> <out folder>
Needs Pillow."""
import glob, os, re, sys
from PIL import Image


def main():
    src, frame, out = sys.argv[1], sys.argv[2], sys.argv[3]
    os.makedirs(out, exist_ok=True)
    game = Image.open(frame).convert("RGB")
    for raw in sorted(glob.glob(os.path.join(src, "notice-*-for-*.raw"))):
        m = re.search(r"notice-(\d+)x(\d+)-for-(\d+)x(\d+)\.raw$", raw)
        bw, bh, sw, sh = (int(x) for x in m.groups())
        b = open(raw, "rb").read()
        assert len(b) == bw * bh * 4, raw
        pic = Image.frombuffer("RGBA", (bw, bh), b, "raw", "BGRA", 0, 1)
        r, g, bl, a = pic.split()
        base = game.resize((sw, sh), Image.LANCZOS)
        x0, y0 = (sw - bw) // 2, (sh * 30 + 500) // 1000
        box = base.crop((x0, y0, x0 + bw, y0 + bh))
        # premultiplied over: result = picture + frame x (1 - alpha)
        inv = a.point(lambda v: 255 - v)
        under = Image.composite(box, Image.new("RGB", (bw, bh)), inv)        # frame x (1 - alpha), to a step
        lay = Image.merge("RGB", (r, g, bl))
        res = Image.new("RGB", (bw, bh))
        res.putdata([tuple(min(255, u + l) for u, l in zip(pu, pl)) for pu, pl in zip(under.getdata(), lay.getdata())])
        base.paste(res, (x0, y0))
        p = os.path.join(out, "notice-on-%dx%d.png" % (sw, sh))
        base.save(p)
        print(p)


if __name__ == "__main__":
    main()
