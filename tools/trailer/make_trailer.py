#!/usr/bin/env python3
"""Edits the SHADOW VEIL trailer from recorded clips (tools/trailer/record.sh).

Cut: cold open -> the village burns -> feature montage -> the Iron Ronin -> the revelation -> systems -> logo.
The score is rendered by the game's own synthesizer (sv_score) from cues placed on this edit's timeline.
Output: dist/SHADOW_VEIL_Trailer.mp4 (1920x1080, 60 fps, H.264 + AAC).
"""
import os, subprocess, sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))
CLIPS = f'{ROOT}/build/trailer/clips'
WORK = f'{ROOT}/build/trailer/edit'
FONTS = f'{ROOT}/assets/fonts'
F_SB, F_IT, F_RG, F_K = (f'{FONTS}/NotoSerif-SemiBold.ttf', f'{FONTS}/NotoSerif-Italic.ttf',
                         f'{FONTS}/NotoSerif-Regular.ttf', f'{FONTS}/NotoSerifCJKjp-Bold-subset.otf')
FPS, W, H = 60, 1920, 1080
PAPER, DIM, CRIMSON, GOLD = '0xefe6d2', '0xa89e8a', '0xc0182a', '0xd8b45a'

def spaced(s): return '  '.join(' '.join(w) for w in s.split(' '))

# ------------------------------------------------------------------ the cut
SEGS = [
    dict(kind='card', dur=4.6, lines=[('For six hundred years,', 0.3), ('one clan has kept the Veil', 1.2), ('between Kurosaki and the Hollow.', 2.1)]),
    dict(kind='clip', clip='01_cliff', a=0.6, b=8.8, fin=1.2, fout=0.5),
    dict(kind='card', dur=2.8, lines=[('Tonight,', 0.2), ('the Veil tears.', 0.9)], big=True),
    dict(kind='clip', clip='02_street', a=0.5, b=6.3, fin=0.0, fout=0.0, place='K U R O S A K I   ·   T H E   A S H E N   G R O V E'),
    dict(kind='clip', clip='03_stealth', a=0.3, b=4.9, cap=('忍', 'Strike from the shadows', 'Hide in tall grass. Assassinate from behind or above.')),
    dict(kind='clip', clip='04_duel', a=0.4, b=5.6, cap=('流', 'Deflect every blow', 'Perfect timing turns their steel against them.')),
    dict(kind='clip', clip='05_execute', a=1.0, b=3.9, cap=('破', 'Break their will', 'Shatter posture. Execute.')),
    dict(kind='clip', clip='06_grapple', a=0.9, b=3.7, cap=('糸', 'Ride the Veil-Thread', 'Swing above the burning rooftops.')),
    dict(kind='clip', clip='11_dash', a=0.3, b=2.5, cap=('影', 'Step through the Veil', 'Untouchable, for one heartbeat.')),
    dict(kind='clip', clip='07_ember', a=0.6, b=4.4, cap=('炎', 'Wield the Ember Art', 'Sparks earned in battle, loosed as fire.')),
    dict(kind='clip', clip='09_bell', a=0.6, b=4.6, cap=('錘', 'Read the old ways', 'Puzzles, secrets and forgotten paths.')),
    dict(kind='clip', clip='10_bridge', a=0.4, b=3.2),
    dict(kind='clip', clip='08_trial', a=0.6, b=7.0, fout=0.25, cap=('風', 'Awaken the Gale', 'Trials, stances, and the Kage Rank.')),
    dict(kind='clip', clip='12_boss_intro', a=8.1, b=11.2, fin=0.25),
    dict(kind='clip', clip='13_boss_fight', a=0.6, b=3.7),
    dict(kind='clip', clip='13_boss_fight', a=8.4, b=13.4, cap=('鉄', 'Face the Iron Ronin', 'Two deathblows. One burning blade.')),
    dict(kind='clip', clip='14_boss_end', a=7.2, b=8.9),
    dict(kind='clip', clip='14_boss_end', a=17.4, b=20.3, fin=0.2),
    dict(kind='clip', clip='14_boss_end', a=24.8, b=27.0),
    dict(kind='clip', clip='14_boss_end', a=31.6, b=33.6),
    dict(kind='clip', clip='14_boss_end', a=34.6, b=37.2, fout=1.1),
    dict(kind='clip', clip='15_menus', a=1.6, b=3.9, fin=0.6),
    dict(kind='clip', clip='15_menus', a=4.6, b=6.0, tag='GEAR  ·  GEMS  ·  TOKENS  ·  FORGE'),
    dict(kind='clip', clip='15_menus', a=6.4, b=7.9, tag='12 ARTS  ·  4 PATHS'),
    dict(kind='clip', clip='15_menus', a=8.2, b=9.7, tag='BUILT FOR THE CONTROLLER'),
    dict(kind='clip', clip='15_menus', a=10.0, b=11.4, fout=0.4, tag='21 ACHIEVEMENTS  ·  NEW GAME+  ·  PERSONAL BESTS'),
    dict(kind='end', dur=7.5),
]

def dur(s): return s['dur'] if 'dur' in s else s['b'] - s['a']

def frames(d): return int(round(d * FPS))

# ------------------------------------------------------------------ helpers
def run(cmd):
    r = subprocess.run(cmd, capture_output=True, text=True)
    if r.returncode != 0:
        print(' '.join(cmd)); print(r.stderr[-3000:]); sys.exit(1)

def textfile(name, text):
    p = f'{WORK}/txt_{name}.txt'
    with open(p, 'w', encoding='utf-8') as f: f.write(text)
    return p

def alpha(t0, t1, fi=0.45, fo=0.4):
    return (f"'if(lt(t,{t0:.3f}),0,if(lt(t,{t0+fi:.3f}),(t-{t0:.3f})/{fi},"
            f"if(lt(t,{t1-fo:.3f}),1,if(lt(t,{t1:.3f}),({t1:.3f}-t)/{fo},0))))'")

def dt(name, text, font, size, color, x, y, t0, t1, extra=''):
    return (f"drawtext=fontfile='{font}':textfile='{textfile(name, text)}':fontsize={size}:fontcolor={color}"
            f":x={x}:y={y}:alpha={alpha(t0, t1)}{extra}")

# ------------------------------------------------------------------ segments
def render_segment(i, s):
    d = dur(s); n = frames(d)
    out = f'{WORK}/seg_{i:02d}.mp4'
    filters = []
    if s['kind'] == 'clip':
        inputs = ['-ss', f"{s['a']:.3f}", '-i', f"{CLIPS}/{s['clip']}.mp4"]
        base = '[0:v]setpts=PTS-STARTPTS'
    elif s['kind'] == 'card':
        inputs = ['-f', 'lavfi', '-i', f'color=c=0x050507:s={W}x{H}:r={FPS}:d={d}']
        base = '[0:v]null'
    else:  # end card over a blurred, slowly pushing moonlit vista
        inputs = ['-ss', '8.6', '-i', f'{CLIPS}/01_cliff.mp4']
        base = (f"[0:v]trim=end_frame=1,loop=loop={n}:size=1:start=0,setpts=N/{FPS}/TB,"
                f"scale=2400:-1,zoompan=z='1+0.0006*on':d=1:x='iw/2-iw/zoom/2':y='ih/2-ih/zoom/2':s={W}x{H}:fps={FPS},"
                f"gblur=sigma=7,eq=brightness=-0.16:saturation=0.8,setpts=N/{FPS}/TB")
    chain = [base]
    if s['kind'] == 'card':
        big = s.get('big')
        lines = s['lines']
        y0 = H / 2 - (len(lines) - 1) * (60 if not big else 80) / 2 - 30
        for k, (txt, t0) in enumerate(lines):
            size = 76 if big else 52
            chain.append(dt(f'{i}_{k}', txt, F_IT, size, PAPER, '(w-text_w)/2', f'{y0 + k * (80 if big else 64):.0f}', t0, d - 0.35))
        chain.append(f"drawbox=x={W/2-110:.0f}:y={y0 + len(lines) * (80 if big else 64) + 18:.0f}:w=220:h=2:color={CRIMSON}@0.9:t=fill:enable='between(t,{lines[-1][1]+0.3},{d-0.4})'")
    if s.get('cap'):
        k, title, sub = s['cap']
        t0, t1 = 0.25, d - 0.15
        chain.append(dt(f'{i}_k', k, F_K, 118, CRIMSON, 140, H - 318, t0, t1, ":shadowcolor=black@0.6:shadowx=0:shadowy=3"))
        chain.append(dt(f'{i}_t', spaced(title.upper()), F_SB, 40, PAPER, 290, H - 284, t0 + 0.12, t1, ":shadowcolor=black@0.7:shadowx=0:shadowy=2"))
        chain.append(f"drawbox=x=292:y={H-226}:w=560:h=2:color={CRIMSON}@0.85:t=fill:enable='between(t,{t0+0.3},{t1-0.2})'")
        chain.append(dt(f'{i}_s', sub, F_IT, 27, DIM, 292, H - 208, t0 + 0.3, t1, ":shadowcolor=black@0.7:shadowx=0:shadowy=2"))
    if s.get('place'):
        chain.append(dt(f'{i}_p', s['place'], F_SB, 24, PAPER, '(w-text_w)/2', H - 120, 0.6, d - 0.3, ":shadowcolor=black@0.8:shadowx=0:shadowy=2"))
    if s.get('tag'):
        chain.append(dt(f'{i}_g', s['tag'], F_SB, 30, GOLD, '(w-text_w)/2', H - 86, 0.1, d - 0.05, ":box=1:boxcolor=0x050507@0.85:boxborderw=18"))
    if s['kind'] == 'end':
        a0 = 0.6
        for j, kj in enumerate('影ノ帳'):
            chain.append(dt(f'{i}_kk{j}', kj, F_K, 132, PAPER, 300, 250 + j * 150, a0 + j * 0.25, d - 0.3))
        chain.append(f"drawbox=x=470:y=715:w=64:h=64:color={CRIMSON}@0.95:t=fill:enable='between(t,{a0+1.2},{d-0.3})'")
        chain.append(dt(f'{i}_seal', '影', F_K, 44, PAPER, 480, 722, a0 + 1.2, d - 0.3))
        chain.append(dt(f'{i}_t1', 'S H A D O W', F_SB, 96, PAPER, 470, 300, a0 + 0.5, d - 0.3))
        chain.append(dt(f'{i}_t2', 'V E I L', F_SB, 96, PAPER, 470, 420, a0 + 0.7, d - 0.3))
        chain.append(dt(f'{i}_t3', 'Chapter I  —  The Ashen Grove', F_IT, 34, DIM, 474, 556, a0 + 1.4, d - 0.3))
        chain.append(f"drawbox=x=474:y=610:w=420:h=2:color={CRIMSON}@0.9:t=fill:enable='between(t,{a0+1.6},{d-0.3})'")
        chain.append(dt(f'{i}_t4', 'A cinematic ninja action game  ·  native Linux  ·  built for the controller', F_RG, 26, PAPER, 474, 640, a0 + 2.0, d - 0.3))
        chain.append(dt(f'{i}_t5', 'Every frame and every note generated in real time.', F_IT, 24, DIM, 474, 680, a0 + 2.4, d - 0.3))
    fin = s.get('fin', 0.6 if s['kind'] in ('card', 'end') else 0.0)
    fout = s.get('fout', 0.6 if s['kind'] in ('card', 'end') else 0.0)
    if fin > 0: chain.append(f'fade=t=in:st=0:d={fin}')
    if fout > 0: chain.append(f'fade=t=out:st={d - fout:.3f}:d={fout}')
    chain.append('format=yuv420p[v]')
    graph = ','.join(chain)
    gfile = f'{WORK}/graph_{i:02d}.txt'
    with open(gfile, 'w', encoding='utf-8') as f: f.write(graph)
    run(['ffmpeg', '-v', 'error', '-y', *inputs, '-/filter_complex', gfile, '-map', '[v]', '-frames:v', str(n), '-r', str(FPS),
         '-c:v', 'libx264', '-preset', 'slow', '-crf', '16', '-g', '120', out])
    # matching game audio (clips) or silence (cards)
    aout = f'{WORK}/seg_{i:02d}.wav'
    if s['kind'] == 'clip':
        run(['ffmpeg', '-v', 'error', '-y', '-f', 'f32le', '-ar', '48000', '-ac', '2', '-i', f"{CLIPS}/{s['clip']}.mp4.f32",
             '-af', f"atrim=start={s['a']:.4f}:duration={n / FPS:.4f},asetpts=PTS-STARTPTS,afade=t=in:d=0.04,afade=t=out:st={n / FPS - 0.05:.4f}:d=0.05,apad=whole_dur={n / FPS:.4f}",
             '-ar', '48000', '-ac', '2', '-c:a', 'pcm_f32le', aout])
    else:
        run(['ffmpeg', '-v', 'error', '-y', '-f', 'lavfi', '-i', 'anullsrc=r=48000:cl=stereo', '-t', f'{n / FPS:.4f}', '-c:a', 'pcm_f32le', aout])
    return n / FPS

# ------------------------------------------------------------------ score cues on the edit's timeline
def score_cues(starts, total):
    c = []
    def cue(t, *a): c.append(f'{t:.3f} ' + ' '.join(str(x) for x in a))
    S = starts
    cue(0, 'amb', 0.35, 0, 0, 0); cue(0, 'music', 'silent'); cue(0.25, 'sfx', 'campbell', 0.9, 1.0)
    cue(1.2, 'music', 'title'); cue(1.2, 'intensity', 0)
    # "Tonight, the Veil tears."
    cue(S[2], 'music', 'silent'); cue(S[2] + 0.1, 'sfx', 'gate', 0.7, 0.8); cue(S[2] + 0.9, 'sfx', 'perilous', 0.9, 0.55)
    # the village burns
    cue(S[3], 'sfx', 'execute', 1.0, 0.75); cue(S[3], 'music', 'tension'); cue(S[3], 'amb', 0.3, 0.9, 0, 0.1)
    # montage: drive, with a taiko-like hit on each cut
    cue(S[4], 'music', 'combat'); cue(S[4], 'intensity', 0.6); cue(S[4], 'amb', 0.25, 0.2, 0.4, 0)
    for k in range(4, 13): cue(S[k], 'sfx', 'thud', 0.55, 0.75)
    cue(S[8], 'intensity', 1.0)
    cue(S[13] - 0.35, 'music', 'silent')
    # the Iron Ronin
    cue(S[13], 'sfx', 'perilous', 1.0, 0.5); cue(S[13] + 0.6, 'music', 'boss'); cue(S[13], 'intensity', 1.0); cue(S[13], 'amb', 0.4, 0, 0, 0.3)
    cue(S[15], 'sfx', 'thud', 0.6, 0.7)
    cue(S[16], 'sfx', 'execute', 1.0, 0.85)
    # the revelation
    cue(S[17], 'music', 'elegy'); cue(S[17], 'lowpass', 0.25); cue(S[17], 'amb', 0.3, 0, 0, 0)
    cue(S[18], 'sfx', 'dash', 1.0, 0.55); cue(S[18], 'sfx', 'gate', 0.8, 0.7)
    cue(S[20] + 1.0, 'sfx', 'death', 0.8, 0.9); cue(S[20] + 1.2, 'music', 'silent')
    # systems
    cue(S[21], 'lowpass', 0); cue(S[21] + 0.1, 'sfx', 'campbell', 0.7, 1.1); cue(S[21] + 0.4, 'music', 'title'); cue(S[21], 'intensity', 0.4)
    for k in range(22, 26): cue(S[k], 'sfx', 'confirm', 0.35, 0.9)
    # logo
    cue(S[26] + 0.5, 'sfx', 'campbell', 1.0, 0.85); cue(S[26] + 0.7, 'music', 'silent')
    c.append(f'end {total + 0.2:.3f}')
    return '\n'.join(c) + '\n'

def main():
    os.makedirs(WORK, exist_ok=True)
    os.makedirs(f'{ROOT}/dist', exist_ok=True)
    missing = {s['clip'] for s in SEGS if 'clip' in s if not os.path.exists(f"{CLIPS}/{s['clip']}.mp4")}
    if missing: sys.exit(f'missing clips {sorted(missing)}: run tools/trailer/record.sh first')
    starts, t = [], 0.0
    for i, s in enumerate(SEGS):
        starts.append(t)
        print(f'  [{i:02d}] {t:6.2f}s  {s.get("clip", s["kind"]):<14} {dur(s):.2f}s', flush=True)
        t += render_segment(i, s)
    total = t
    with open(f'{WORK}/score.txt', 'w') as f: f.write(score_cues(starts, total))
    run([f'{ROOT}/build/sv_score', f'{WORK}/score.txt', f'{WORK}/score.f32'])
    # Frame-exact assembly: decode every segment and join with the concat filter in one final encode
    # (stream-copy concatenation drops frames around x264's B-frame edit lists and drifts off the score).
    n = len(SEGS)
    ins = []
    for i in range(n): ins += ['-i', f'{WORK}/seg_{i:02d}.mp4']
    for i in range(n): ins += ['-i', f'{WORK}/seg_{i:02d}.wav']
    ins += ['-f', 'f32le', '-ar', '48000', '-ac', '2', '-i', f'{WORK}/score.f32']
    vcat = ''.join(f'[{i}:v]' for i in range(n)) + f'concat=n={n}:v=1:a=0[v]'
    acat = ''.join(f'[{n + i}:a]' for i in range(n)) + f'concat=n={n}:v=0:a=1,volume=0.55[g]'
    mix = (f"{vcat};{acat};[{2 * n}:a]volume=0.95[m];[g][m]amix=inputs=2:duration=first:normalize=0,"
           f"afade=t=in:d=0.3,afade=t=out:st={total - 2.2:.3f}:d=2.2,loudnorm=I=-15:LRA=11:TP=-1.5,aresample=48000[a]")
    gfile = f'{WORK}/final_graph.txt'
    with open(gfile, 'w') as f: f.write(mix)
    out = f'{ROOT}/dist/SHADOW_VEIL_Trailer.mp4'
    run(['ffmpeg', '-v', 'error', '-y', *ins, '-/filter_complex', gfile, '-map', '[v]', '-map', '[a]',
         '-c:v', 'libx264', '-preset', 'slow', '-crf', '17', '-pix_fmt', 'yuv420p', '-profile:v', 'high', '-g', '120',
         '-c:a', 'aac', '-b:a', '256k', '-t', f'{total:.3f}', '-movflags', '+faststart', out])
    print(f'\ntrailer: {out}  ({total:.1f} s)')

if __name__ == '__main__':
    main()
