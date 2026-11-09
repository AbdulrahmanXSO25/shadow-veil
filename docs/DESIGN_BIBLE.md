# SHADOW VEIL — 影ノ帳 (Kage no Tobari)
### Design Bible · v0.9

---

## 1. Pillars

1. **Ink & Steel.** Every frame should read like a sumi-e painting cut with a blade. Black ink silhouettes,
   painted skies, one accent colour per scene. Readability first: the player, enemies, and threats
   always separate cleanly from the backdrop.
2. **Deliberate violence.** Combat is about *posture*, not hit-point attrition. Read the tell, deflect,
   break the guard, execute. Every exchange should feel like a duel in a samurai film: stillness, then a cut.
3. **The Veil is a character.** The boundary between Kurosaki and the Hollow drives the story, the traversal
   powers (Veil-Step, Veil-Thread grapple), and the world's colour script.
4. **Built for the hands.** Designed on a DualShock: both index fingers time the duel (L1 deflect, □ strike);
   the thumbs jump and step (✕, ○); the right stick aims and looks. The light bar and haptics carry state.
   Keyboard is a complete fallback mapping, not the reference.
5. **Authored, not generated.** Handcrafted rooms, scripted set pieces, environmental storytelling.
   Procedural techniques are used to *render* art, never to *design* levels.

---

## 2. Story

### Premise
For six hundred years the **Kagenui clan** ("shadow-stitchers") of Kurosaki village have tended the
**Veil**: a woven barrier in the mountain pass that holds back the armies of the Hollow.
Its anchor is the **Veil Anchor**, kept at the Old Gate above the village.

On the night of the Lantern Fair, warlord **Lord Hakuro, the White Wolf**, sends his iron vanguard
to burn Kurosaki and take the Anchor. **Rei Kagenui**, the clan's youngest sworn blade, wakes to fire.

### Characters
| Name | Role | Silhouette / Accent | Arc |
|---|---|---|---|
| **Rei Kagenui** | Protagonist. 19. Quiet, exacting, distrusts her own gift (she can *step through* the Veil). | Lean, long crimson scarf, short katana | From obedience to authorship: she must decide what the Veil *should* be. |
| **Master Odaka Sen** | Clan elder; Rei's mentor and adoptive father. | Tall, straw cloak, staff-sword | Opened the Gate himself. Believes the Veil is a prison holding the clan's own exiles — including Rei's mother, Sayo. |
| **Fuki** | Camp keeper of Kurosaki; blunt, warm, half-blind. | Hunched, lantern | Awakens Rei's **Ember Art**. Keeper of the clan's records. |
| **Tetsu, the Iron Ronin** | Hakuro's vanguard captain. Chapter 1 boss. | Massive, crescent kabuto, nodachi | A disgraced swordsman who wants an honourable death. Dying, he reveals *who let them in*. |
| **Lord Hakuro** | Warlord. Wants the Anchor to bring down the Veil and march his army through. | Pale armour, wolf-pelt mantle | Not the true threat — a tool of Odaka's plan, and finally a victim of it. |
| **Sayo, the Weaver** | Odaka's sister; Rei's mother. | Gold threads, loom | Has held the Veil closed alone for twenty years. Speaks to Rei from Chapter V. |
| **Hana** | Fuki's granddaughter. | Pink ribbon | Taken to the castle in Chapter II with the Minase children; rescued in Chapter IV. |
| **Funaki** (II), **Karasu** (III), **Kagami** (V) | Champions | Tide chain · crow mask · mirror blade | Each is beaten and **spared**: the mercy theme. Each tells Rei part of the truth. |

### Chapter Plan (6 worlds, as shipped)
| # | World | Palette & Mood | Narrative Purpose | Champion |
|---|---|---|---|---|
| I | **The Ashen Grove** — Kurosaki & bamboo forest | Moon-indigo → ember → jade → crimson | Loss. Revelation: Odaka opened the Gate. | Tetsu, the Iron Ronin (killed) |
| II | **The Drowned Terrace** | Rain, teal, flooded rice terraces | Hakuro's captain drowns Minase; Hana is taken. | Funaki, the Tide Warden (spared) |
| III | **Crimson Maple Pass** | Autumn dusk, red-gold, vertical cliffs | The "crows" are exiles from the Hollow, not monsters. | Karasu, the Crow King (spared) |
| IV | **Castle Hakuro** | Snow, steel-blue, beacon light | Rescue the captives. Hakuro's pact with Odaka. | Lord Hakuro (spared) |
| V | **The Mirror Province** | Violet sky, the world doubled in glass | Sayo speaks. Rei learns she is Sayo's daughter. | Kagami of the Hollow (spared) |
| VI | **The Heart of the Veil** | Loom gold, unravelling amber | Odaka and Sayo. The choice. | Odaka Sen (spared) |

**Ending choice** (Chapter VI): *Reweave* the Veil (Sayo keeps it, but no longer alone), *Unravel* it gently
(the exiles come home; the borders are uneasy), or — only with all 25 ink scrolls — *Restitch* it into a weave
with doors, held by many hands, and bring Sayo home.

### Chapter 1 beat sheet (the vertical slice)
1. **Cold open** — Cliff camp under a white moon. Rei rests. Drums in the valley. The horizon catches fire.
2. **Descent** — Movement tutorial along the cliff path. The village burning below.
3. **Village edge** — A lone ashigaru, back turned: the stealth-kill lesson.
4. **Burning street** — First real fight. Parry taught through survival, not text walls.
5. **Fuki's camp** — Checkpoint. Fuki gives the Ember Emblem: *"Your master went up to the Gate. Alone."*
6. **Rooftops** — Archers; Veil-Thread grapple taught over a burning gap.
7. **The Bamboo Grove** — Colour shifts to jade. Stealth through tall grass; optional secrets.
8. **The Weight Gate** — Puzzle: hit the iron catch with a shuriken to drop the counterweight and raise the gate.
9. **The Ravine** — Cut the counterweight rope to drop the log bridge; mixed squad fight.
10. **Last camp** — Quiet before the storm. Final lore scroll hints at the twist.
11. **The Old Gate** — Tetsu duel, two deathblows, flame phase.
12. **Revelation** — *"Your master opened the gate for us, girl."* The Anchor is gone; the Veil tears.
    Odaka seen beyond the rift. Rei steps through. *End of Chapter I.*

---

## 3. Gameplay

### Movement
Run · variable jump · coyote time & input buffering · wall-slide & wall-jump · crouch-sneak ·
**Veil-Step** (dash with invulnerability frames, air-dash once) ·
**Veil-Thread** (grapple to anchors; pendulum swing; release to fling).

### Combat — Posture system
* Every combatant has **Vitality** and **Posture**. Attacks and deflections build posture damage.
* A **broken posture** opens an **Execution** (cinematic finisher, guaranteed kill or boss deathblow pip).
* **Deflect** (tap Guard just before impact): no damage, heavy posture damage to attacker, sparks, arrows
  are reflected. **Guard** (hold): reduced damage, posture chip.
* **Perilous attacks** are flagged with a red 危 sigil *and* a distinct diamond shape (colour-blind safe).
  They cannot be guarded; they must be Veil-Stepped through or jumped.
* **Stealth kill** from behind or above on unaware enemies. Tall grass hides; crouching silences footsteps.
* **Shuriken**: interrupt archers and wind-ups, strike catches and ropes. Limited ammo.
* **Ember Art** (Veil Art #1): flaming crescent projectile; costs **Sparks**, earned from hits and deflections.
* **Gourd**: heal; charges refill at camps.

### Enemy roster (Ch. 1)
| Enemy | Behaviour |
|---|---|
| Ashigaru Swordsman | Patrols, investigates noise, 2–3 hit strings, occasional perilous thrust. |
| Yari Spearman | Long reach, perilous sweep (jump it). Keeps spacing. |
| Archer | Holds distance, retreats; arrows can be deflected back. |
| Tetsu (boss) | Two deathblows. Phase 2 ignites his nodachi: flame waves, perilous grab. |

Attack tokens limit simultaneous attackers to two, so groups feel threatening but fair.

### Competition & mastery
* **Kage Rank** (D · C · B · A · S · SS): a style meter fed by varied actions. Repeats are worth less, getting hit
  costs dearly, and idling lets it decay.
* **Chapter score & grade** (C → S+): kills, executions, stealth kills, deflections, peak rank and speed, minus deaths.
  Personal bests and trial times are recorded.
* **Wind-beacon trials:** timed arena challenges that awaken **Stances**. Chapter I's *Wind Trial*
  grants the **Gale Stance**: swifter movement, wind blades on finishing cuts, and a wind ribbon around Rei.

### Progression
* **Equipment slots**: Blade · Garb · Mask · Token ×2. Blades have **Gem sockets**.
* **Gems**: elemental or stat modifiers (e.g. *Coal Ruby*: ember damage on every third strike).
* **Skill Tree** (*Way of the Shadow*): four branches: **Blade**, **Veil**, **Shadow**, and **Wind** (sealed until a
  Stance awakens: Rising Gale, Eye of the Storm, Tailwind perfect-dodge bullet time). Points come from
  **Veil Essence** earned from kills, executions, and secrets.
* **Crafting / Forge** at camps: materials (*Iron Sand*, *Moon Silk*, *Ember Stone*) → upgrades and items.
* **Currency**: *Mon* coins.
* **Collectibles**: Ink Scrolls (lore), Resin Beads (4 = +max Vitality), hidden equipment.
* **Achievements**, **Challenges** (no-damage boss, chapter speed run), **New Game+** (enemy scaling,
  keeps gear and skills, remixed placements planned).

---

## 4. Visual Language

* **Silhouette-first.** All gameplay-layer objects are ink-dark with a **rim light** in the local light colour
  (moon blue, fire orange, lantern gold). This keeps procedural geometry looking deliberate and readable.
* **One accent per scene.** Rei's crimson scarf is the only saturated red on screen unless danger is present.
  Red on screen means *Rei* or *threat*. Gold means *important / interactable*.
* **Atmospheric perspective.** 4–6 parallax layers fade into a per-room fog colour; foreground occluders
  (parallax > 1) frame shots.
* **Colour script per room**, blended across room boundaries:
  `Cliff (moon blue) → Village (ember orange) → Grove (jade/teal) → Ravine (smoke violet) → Gate (crimson moon)`.
* **Post-processing**: HDR scene, bloom, per-room colour grade (lift/gamma/gain), vignette, subtle grain,
  cinematic letterbox. Restraint: effects punctuate hits and moments; they never sit at full intensity.
* **Animation**: procedural skeleton with authored key poses, crossfades, anticipation and follow-through,
  verlet cloth for scarves and headband tails, hit-stop on impact.
* **Typography**: Noto Serif for text; Noto Serif CJK kanji used as seals and titles. Brush-stroke panels.

---

## 5. Audio Language
All synthesized at runtime, no samples. **No instruments**: the score is human voice, breath and wind.
* **Voice**: formant-synthesized hums and sung vowels for exploration, swelling with combat intensity.
* **Breath and wind**: filtered noise beds that follow the room's wind and the camera's height.
* **Ambience**: rain hiss and drops, running water, fire crackle, the rumble of the unravelling Veil.
* Camps: a warm breath of fire and wind. Chapter cards: silence, then voice.
* Steel: layered metallic partials for deflects; filtered noise for slashes; soft cloth for steps.

---

## 6. Accessibility
Screen-shake scale · flash reduction · deflect-window assist · game speed (60–100%) ·
hold/toggle guard · subtitle scale & background · high-contrast enemy outlines ·
colour-independent perilous telegraphs · full gamepad and keyboard support.

---

## 7. Technical Architecture (summary)
See `README.md` for building. Code is organised as:
`core/` (math, files, logging) · `render/` (batched GL renderer, post-FX, text) · `audio/` (synth & mixer) ·
`game/` (skeleton, actors, combat, level, cutscenes, progression, save) · `platform/` (store services) · `ui/` (glyphs, rich text).
Content lives in `data/`: room-based ASCII level files and a cutscene script language.
