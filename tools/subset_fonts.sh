#!/usr/bin/env bash
# Rebuilds bundled fonts. Noto fonts are SIL OFL 1.1 (see assets/fonts/OFL.txt).
# Re-run whenever new kanji or symbols are used in-game.
set -euo pipefail
cd "$(dirname "$0")/.."
OUT=assets/fonts
SRC=/usr/share/fonts
cp "$SRC/noto/NotoSerif-Regular.ttf" "$SRC/noto/NotoSerif-SemiBold.ttf" "$SRC/noto/NotoSerif-Italic.ttf" "$OUT/"
KANJI="影ノ帳危一二三四五六七八九十章灰竹林鉄源祠炎刀面鎧宝技道心忍封印門暁終生死魂月火風雷氷墨巻刃守破決闘黒崎玲師翁婆城楓狐鈴縄橋谷森村里社鳥居神仏魔鬼天狗水沈紅白狼武者弓槍兵足軽首領錬鍛冶珠玉石絹砂銭紋数珠祈願記録設定続新始終了戻再開装備品技能収集業績音量画面振動補助速度字幕言語戦斬受流投擲跳走潜伏隠暗殺処刑体幹命気力癒瓢箪試練極意真秘伝序破急壱弐参肆伍空無有夜明朧霧煙雨雪春夏秋冬花桜葉草根枝山川海島国常世黄泉境界織糸縫針母父娘弟子主君敵味方友愛憎怒悲喜恐勇誇名誉恥罪罰赦"
# Every non-ASCII character used anywhere in code or content is included automatically.
USED=$(python3 -c "
import pathlib
chars=set()
for d in ('src','data'):
    for f in pathlib.Path(d).rglob('*'):
        if f.is_file() and f.suffix in ('.cpp','.h','.lvl','.cut','.txt'):
            chars.update(c for c in f.read_text(encoding='utf-8') if ord(c) > 0x2000)
print(''.join(sorted(chars)))")
KANA=$(python3 -c "print(''.join(chr(c) for c in list(range(0x3041,0x3097))+list(range(0x30A1,0x30FB))+[0x30FC,0x3001,0x3002,0x300C,0x300D,0x300E,0x300F,0x30FB]))")
pyftsubset "$SRC/noto-cjk/NotoSerifCJK-Bold.ttc" --font-number=0 --text="$KANJI$KANA$USED" \
  --output-file="$OUT/NotoSerifCJKjp-Bold-subset.otf" --layout-features='*' --no-hinting
cp /usr/share/licenses/noto-fonts/LICENSE "$OUT/OFL.txt"
ls -la "$OUT"
