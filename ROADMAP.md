# TrueShot — Roadmap exhaustive

> Document de référence listant **tout** ce qu'il reste à faire pour passer
> du prototype actuel (practice range solo) à un jeu compétitif 5v5 publié
> sur Steam, monétisable, modable, e-sport ready.
>
> Organisé en **phases** (par ordre logique de réalisation) puis par
> **domaines** (chaque chantier est indépendant et peut être assigné).
> Chaque case `[ ]` est une tâche atomique sur quelques heures à quelques jours.

## Règles transverses

### Plateformes cibles obligatoires

**Tout doit tourner sur Windows, macOS ET Linux à chaque étape** — distribution
prévue via **Steam** uniquement pour démarrer. Concrètement :

- Pas de code Windows-only (`#include <windows.h>` direct) ni macOS-only sans
  `#ifdef` propre.
- Toute dépendance doit avoir un port vcpkg fonctionnel sur les trois OS.
- Toute feature mergée doit compiler et tourner sur les trois avant d'être
  considérée comme finie.
- **Steam Deck** (qui est Linux) doit être verified au lancement.
- **Consoles (PS5/Xbox/Switch)** = poubelle pour le moment, peut-être très
  longtemps après le succès PC, voir Phase 20 (bonus).

### Mode d'exécution

Chaque section est étiquetée avec un mode d'exécution :

| Étiquette                                | Signification                                                                                                                                                                               |
| ---------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| 🟢 **Ensemble**                          | Toi + moi en pair-programming. C'est le mode par défaut.                                                                                                                                    |
| 🟡 **Ensemble + assets externes**        | On code la mécanique, on achète/acquiert les assets (sound libs, modèles 3D, polices)                                                                                                       |
| 🟠 **Ensemble + freelance ponctuel**     | On dirige, un freelance livre une partie (artiste, sound designer, traducteur)                                                                                                              |
| 🔴 **Ensemble + équipe / devs externes** | Tu recrutes / embauches / t'associes ; je reste en support technique mais des humains spécialistes sont indispensables (anti-cheat sécurité kernel, juridique, comptable, marketing payant) |
| ⚫ **Pas avant équipe + budget sérieux** | À reporter à après une levée de fonds, un publisher, ou une preuve de traction commerciale                                                                                                  |

### Service externes assumés dès le départ

| Service                      | Quand                                      | Pourquoi                                                                                                                                                |
| ---------------------------- | ------------------------------------------ | ------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **Steam (Steamworks)**       | Phase 7+                                   | distribution, comptes, friends, marketplace, workshop, achievements                                                                                     |
| **Twitch**                   | Phase 14+                                  | broadcasting, embed, drops                                                                                                                              |
| **GitHub Actions**           | Maintenant                                 | CI/CD multi-OS gratuit pour repos open-source ; payant si privé (faisable)                                                                              |
| **Discord**                  | Maintenant                                 | communauté, playtests                                                                                                                                   |
| **Avocat numérique**         | Phase 16+ (lancement)                      | CGU/EULA/Privacy, RGPD, marque, contrats freelance                                                                                                      |
| **Comptable**                | **dès la création de l'entité (Phase 18)** | TVA OSS UE, factures freelance, crédit impôt jeu vidéo (CIJV) si France, immatriculation, déclarations sociales — **jamais en solo, c'est trop risqué** |
| **Banque pro**               | dès l'entité                               | obligatoire pour SAS/SARL                                                                                                                               |
| **Assurance RC pro + cyber** | dès le premier playtest payant ou gratuit  | data breach, bug game-breaking, plainte joueur                                                                                                          |

---

## Table des phases

1. [Phase 0 — Fondations techniques (où on en est aujourd'hui)](#phase-0--fondations-techniques)
2. [Phase 1 — Netcode jouable (1v1 LAN)](#phase-1--netcode-jouable-1v1-lan)
3. [Phase 2 — Boucle de match complète (5v5 sur LAN)](#phase-2--boucle-de-match-complète-5v5-sur-lan)
4. [Phase 3 — Maps & contenu de base](#phase-3--maps--contenu-de-base)
5. [Phase 4 — Graphismes & direction artistique](#phase-4--graphismes--direction-artistique)
6. [Phase 5 — Audio professionnel](#phase-5--audio-professionnel)
7. [Phase 6 — UI / UX complète](#phase-6--ui--ux-complète)
8. [Phase 7 — Backend, comptes & matchmaking](#phase-7--backend-comptes--matchmaking)
9. [Phase 8 — Serveurs dédiés & infrastructure](#phase-8--serveurs-dédiés--infrastructure)
10. [Phase 9 — Anti-cheat (mission critique)](#phase-9--anti-cheat-mission-critique)
11. [Phase 10 — Économie & progression](#phase-10--économie--progression)
12. [Phase 11 — Social & communauté](#phase-11--social--communauté)
13. [Phase 12 — Mod support, serveurs privés, workshop](#phase-12--mod-support-serveurs-privés-workshop)
14. [Phase 13 — Bots & IA](#phase-13--bots--ia)
15. [Phase 14 — Compétitif & e-sport](#phase-14--compétitif--e-sport)
16. [Phase 15 — Qualité, QA, certification](#phase-15--qualité-qa-certification)
17. [Phase 16 — Publication Steam](#phase-16--publication-steam)
18. [Phase 17 — Post-launch & live ops](#phase-17--post-launch--live-ops)
19. [Phase 18 — Juridique, légal, business](#phase-18--juridique-légal-business)
20. [Phase 19 — Marketing & growth](#phase-19--marketing--growth)
21. [Phase 20 — Plateformes (rappel & bonus très lointain)](#phase-20--plateformes-rappel--bonus-très-lointain)

---

## Phase 0 — Fondations techniques

> **Mode :** 🟢 Déjà fait, ensemble. Windows + macOS validés ; Linux validable en CI.
>
> **État :** terminé. Sert de socle stable pour toutes les phases suivantes.

- [x] Architecture C++17 / CMake / vcpkg
- [x] Renderer OpenGL 3.3, classes Application/Renderer/GameWorld
- [x] Movement Source-style (strafe jump, bhop, crouch, fixed-timestep physics — bumped to 128 Hz in Phase 1.1)
- [x] 5 armes avec recoil patterns, ADS, reload
- [x] Hit detection réelle (ray vs AABB) + damage par zone
- [x] HUD ImGui (score, ammo, accuracy, FPS, speed) + hit markers
- [x] Module réseau ENet isolé (client + serveur sample)
- [x] Audio system OpenAL-ready (architecture, pas encore branché)
- [x] Build moderne (presets, warnings, Werror optionnel)

---

## Phase 1 — Netcode jouable (1v1 LAN)

> **Mode :** 🟢 **Ensemble**. C'est ta priorité absolue après lecture de
> cette doc. Pas besoin de personne d'extérieur.
>
> **Plateformes :** Windows + macOS + Linux validés à chaque PR.
>
> **Pourquoi maintenant :** sans réseau, rien des phases 2-14 n'a de sens.
> Si on bloque ici, le projet bloque entièrement.
>
> **Durée estimée :** 2-4 mois en solo full-time, plus en partiel.
>
> **Décisions cadre (cf. [ADR 0002](docs/adr/0002-netcode-architecture.md)) :**
>
> - **128 Hz** tick rate fixe.
> - **Server-authoritative** dès le départ — le client prédit, le serveur tranche.
> - **Listen-server** : un client peut héberger localement (mode "Host & Play"),
>   en plus du binaire `trueshot_server` standalone.
> - **Pas de modèle de personnage propre** en Phase 1 — placeholder cubes,
>   les vrais models arrivent en Phase 4.

But : deux joueurs sur le même LAN voient leurs mouvements et leurs tirs.

### 1.0 — Design doc netcode

- [x] ADR 0002 — architecture netcode (tick rate, protocole, listen-server,
      lag compensation, anti-cheat foundation)

### 1.1 — Tick clock 128 Hz

- [x] `TickClock` à pas fixe (accumulator capé à 0.25 s, viser 128 Hz)
- [x] Mesure de la stabilité du tick (variance < 1 ms en pratique)
- [x] Constantes `FIXED_TIMESTEP` partagées client + serveur

### 1.2 — Sérialisation Bitstream

- [x] `writeU8/16/32/64` + `readU8/16/32/64` little-endian explicite
- [x] `writeFloat` / `readFloat` via memcpy bit-cast
- [x] `writeQ16_16` — fixed-point 4 octets, ±32 km, précision 1/65536
- [x] `writeAngleQ15` — angle quantifié 2 octets (0.0055° de précision à 180°)
- [x] `writeVec3Q` — 12 octets
- [x] `writeVarU32` / `writeVarI32` — varint zigzag (1-5 octets)
- [x] `writeString` length-prefixed
- [x] Toutes les `read*` retournent `bool` (false sur overflow)

### 1.3 — Types de paquets & schémas

- [x] Enum `PacketType` (Handshake / HandshakeAck / Disconnect / Ping / Pong /
      ClientInput / Snapshot / Event / RPC)
- [x] `InputState` (tick, seq, moveForward/Right, yaw, pitch, buttons,
      clientPingMs)
- [x] `EntityState` (id, pos Q16.16, yaw Q15, pitch Q15, stateFlags)
- [x] `Snapshot` (tick, ackSeq, vector\<EntityState\>)
- [x] `Handshake` (protocole, versions client, playerName)
- [x] Versioning protocole : `kProtocolVersion = 1`, handshake bloquant si
      mismatch

### 1.4 — `NetworkClient` dans `Application`

- [x] Classe `NetworkClient` (ENet, 2 channels reliable + unreliable sequenced)
- [x] API `initialize` / `shutdown` / `connectTo` / `disconnect` / `tick` /
      `sendInput` / `popSnapshot`
- [x] Métriques : `state`, `roundTripMs`, `packetsSent/Recv`, `bytesSent/Recv`,
      `localId`
- [x] CLI `--offline`, `--server host[:port]`, `--help`
- [x] Boucle réseau intégrée à `Application` avec input accumulator 128 Hz
- [x] Refcount partagé du `enet_initialize` (préparation listen-server)

### 1.5 — Serveur autoritaire + listen-server foundation

- [x] Classe `Net::Server` (`start`, `stop`, `step`) — boucle 128 Hz à
      accumulator, capée à 0.25 s
- [x] Per-peer `PlayerState` avec `lastAckedSeq`
- [x] **Hard input clamp côté serveur** (foundation anti-cheat) : moveForward/
      moveRight ∈ [-1, 1], yaw ∈ ±180°, pitch ∈ ±89°
- [x] Broadcast Snapshot par peer avec `ackSeq` personnalisé
- [x] Binaire `trueshot_server` standalone (`netcode/src/main_server.cpp`)
- [x] Library `Net::Server` réutilisable pour le mode listen-server (Phase 2)
- [x] Cross-platform Windows linker (`ws2_32`, `winmm`)

### 1.6 — Remote player + interpolation

- [x] `RemotePlayer` avec ring buffer 64 samples (~500 ms d'historique à 128 Hz)
- [x] `RemotePlayerRegistry` — exclut le `localPlayerId` (rendu via prédiction)
- [x] Interpolation linéaire 100 ms (`kInterpDelaySeconds = 0.100`)
- [x] Fallback freeze si extrapolation \> 100 ms (pas d'extrapolation par
      vélocité en Phase 1 — voir ADR 0004)
- [x] `Renderer::drawRemotePlayers` — cubes verticaux placeholder
- [x] Pruning automatique du ring buffer (window = 4× interp delay)

### 1.7 — Client prediction + server reconciliation

- [x] **Shared `NetSim::stepSim`** — la formule de mouvement vit
      dans `netcode/net_sim.h` et est utilisée bit-pour-bit par client et
      serveur. `Server::applyInput` délègue à `stepSim`.
- [x] Buffer ring 256 inputs locaux (`ClientPrediction::predict`) — 2 s
      d'unacknowledged à 128 Hz.
- [x] Application immédiate des inputs sur la `SimState` locale (prédiction).
- [x] À réception d'un Snapshot avec `ackSeq` : drop les inputs ack'd, snap
      à l'état autoritaire, replay des inputs encore pending.
- [x] Seuil de correction étagé : ignore \< 2 cm, lerp doux 25 % \< 50 cm,
      snap \>= 50 cm.
- [x] Vrais inputs (WASD / Space / Ctrl / Mouse1 / Mouse2 / R) lus depuis
      GLFW à chaque tick et empaquetés dans `InputState`.
- [x] Debug log netcode dans `printDebugInfo` (rtt, pending count, predicted
      pos, lastCorrection).
- [ ] _(Reporté Phase 1.7b ou 1.9)_ Brancher la position prédite sur la
      `FPSCamera` en mode Client. Pour l'instant `PlayerController` continue
      à driver la caméra ; la prédiction tourne en parallèle et est
      observable via le debug log.

### 1.8 — Lag compensation pour le tir

- [x] **`Net::LagCompensation`** + `PlayerHistory` (ring 128 samples = 1 s à
      128 Hz) — record automatique chaque tick après `applyInput`.
- [x] `computeViewTime` formula : `T_now - RTT/2 - kClientInterpDelay`.
- [x] Rewind interpolé entre deux samples enclosing du buffer
      (`PlayerHistory::sampleAt`).
- [x] Raycast slab AABB (`rayVsAabb`) contre les hitboxes rewindées
      (`makeHitbox`, taille = cube de rendu placeholder 0.8 × 1.8 × 0.4).
- [x] Cap à 200 ms (`kRewindCapSeconds = 0.200`) — au-delà, refuse
      l'opération.
- [x] `Server::handleFire` déclenché à chaque `InputState` portant
      `InputButton::Fire`, log `[Server] LAG-COMP HIT shooter=X victim=Y`.
- [x] `m_LagCompHits` exposé via `Server::lagCompHits()` pour le HUD
      réseau (Phase 1.9).
- [ ] _(Phase 2)_ Validation cross-check de `clientPingMs` rapporté vs RTT
      mesuré côté serveur — détecte un client qui ment sur son ping pour
      étendre la fenêtre de rewind.
- [ ] _(Phase 2)_ Event packet `PlayerHit` broadcast à tous les clients
      pour la kill feed + hit markers visuels.

### 1.9 — Network metrics + HUD

- [x] **`Net::NetMetrics`** POD agrégeant tout ce que le HUD affiche (state,
      RTT, ticks, packets, bytes, derived per-second, prediction health).
- [x] **`Net::NetMetricsSampler`** — EMA low-pass filter (α=0.20) pour
      bandwidth + snapshot rate, dérivés des compteurs cumulatifs de
      `NetworkClient`.
- [x] `Hud::drawNetPanel` — overlay top-right ImGui non-interactif avec
      coloration glance-able (RTT vert/jaune/rouge à 100/200 ms, corr
      vert/jaune/rouge à 5/50 cm).
- [x] Toggle indépendant via **F2** (edge-triggered, comme F1 pour le HUD).
      Le panneau ne s'affiche pas en offline.
- [x] Métriques affichées : state, RTT, local/server tick, player id, bytes/s
      up+down, packets cumulatifs, snapshots/s, last correction, pending
      inputs, remote player count.
- [x] `Application` agrège les métriques chaque frame depuis `NetworkClient`,
      `ClientPrediction`, `RemotePlayerRegistry` ; sampler notifié à chaque
      `popSnapshot`.
- [ ] _(Phase 2)_ Graphique scrollant du jitter tick par tick (style
      `cl_showperformance` Source) — ImGui PlotLines avec ring buffer 256.
- [ ] _(Phase 2)_ Packet loss mesuré côté client (séquences manquantes dans
      les Snapshots reçus).

### 1.10 — Test 1v1 LAN

**Automated (CI-enforced) :**

- [x] Suite GoogleTest `tests/` (gtest via vcpkg, opt-in via
      `-DTRUESHOT_BUILD_TESTS=ON`).
- [x] `test_netsim.cpp` — déterminisme `stepSim`, clamp inputs, buttons
      → flags.
- [x] `test_bitstream.cpp` — round-trip U8/U16/U32, float, Q16.16, Q15,
      varint U32/I32, overflow returns false.
- [x] `test_player_history.cpp` — ring buffer 128, interpolation mid-
      point, refus extrapolation backwards, ring overflow.
- [x] `test_lag_compensation.cpp` — formule `computeViewTime`, hitbox
      dimensions, cap 200 ms.
- [x] `test_client_prediction.cpp` — predict + reconcile match, replay
      pending inputs, big drift snap, overflow.
- [x] CI job `ctest --output-on-failure` ajouté à `build.yml` sur 3 OS ×
      2 build types = 6 runners.

**Network simulator (server-side, CLI) :**

- [x] `Server::NetSimSettings { lossProbability, baseDelayMs, jitterMs }`.
- [x] Flags `--simulate-loss <P>`, `--simulate-delay <ms>`,
      `--simulate-jitter <ms>` sur `trueshot_server`.
- [x] Drop probabiliste dans `sendTo` ; file différée drainée dans `step`
      avec timestamps de release.
- [x] Backwards-compat positional first arg = port.

**Manual cross-OS pass :**

- [x] Test plan documenté dans
      [docs/test/phase-1-lan-test-plan.md](docs/test/phase-1-lan-test-plan.md)
      (6 scénarios × 3 paires d'OS, 10 min chacun, acceptance criteria).
- [ ] _(À exécuter par Alex sur ses machines)_ Windows ↔ macOS S1+S3+S5
- [ ] _(À exécuter par Alex sur ses machines)_ Windows ↔ Linux S1+S3+S5
- [ ] _(À exécuter par Alex sur ses machines)_ macOS ↔ Linux S1+S3+S5
- [ ] _(À exécuter par Alex)_ Listen-server localhost S1

### 1.11 — Delta compression (à reporter)

> **Pas pour Phase 1.x.** Replanifié quand la bandwidth deviendra un problème
> mesuré (probablement Phase 2.x ou Phase 8 avec les serveurs dédiés).

- [ ] Snapshots envoyés en delta depuis le dernier ack
- [ ] Baseline snapshot (full state) tous les N ticks ou sur demande
- [ ] Bit-mask des champs modifiés par entité
- [ ] Compression LZ4 optionnelle sur la couche transport

---

## Phase 2 — Boucle de match complète (5v5 sur LAN)

> **Mode :** 🟢 **Ensemble**. C'est de la game logic + un peu de design.
>
> **Plateformes :** Windows + macOS + Linux.
>
> **Dépendances :** Phase 1 doit être stable. Pour la voice chat (Opus +
> capture micro multi-OS), on utilisera **libopus** + **miniaudio** (cross-OS
> propres, dispos via vcpkg).
>
> **Durée estimée :** 3-6 mois en solo full-time.

But : 10 joueurs sur un serveur jouent un match complet avec round structure.

### 2.0 — Dette technique à solder d'abord

Deux points relevés par clang-tidy et volontairement reportés ici plutôt
que corrigés à la va-vite. Ils touchent des zones que la Phase 2 réécrit de
toute façon.

- [ ] **Découper `Application::run()`.** Complexité cognitive 96 pour un
      seuil de 25 : la fonction porte l'horloge de frame, l'accumulateur
      réseau 128 Hz, l'échantillonnage des inputs, le drain des snapshots
      et la séquence render/HUD. Extraire au minimum `stepNetwork()` et
      `drainSnapshots()`. Actuellement suppressé par un
      `NOLINTNEXTLINE(readability-function-cognitive-complexity)` commenté
      dans `src/core/application.cpp` — retirer le NOLINT en même temps.
      La Phase 2 réécrit cette boucle pour le match flow, donc on la
      découpe à ce moment-là plutôt que de la churner deux fois.
- [ ] **Réparer `HeaderFilterRegex` dans `.clang-tidy`.** Le `^` ancre le
      motif sur un chemin relatif alors que clang-tidy compare un chemin
      absolu en CI : nos headers ne sont donc **jamais** analysés (le
      dernier run a signalé « suppressed 1056835 warnings in non-user
      code »). Désancrer expose potentiellement des centaines de
      diagnostics jamais vus, avec `WarningsAsErrors: "*"` — à faire comme
      une tâche à part, mesurée, pas en passant.

### 2.1 — Équipes

- [ ] Concept équipe (Attaquants / Défenseurs ou Red / Blue)
- [ ] Assignation équipe au connect (équilibrage automatique)
- [ ] Switch d'équipe à la mi-temps
- [ ] Friendly fire (option serveur : on / réduit / off)

### 2.2 — Round structure

- [ ] Phases : warmup → buy phase → action → end-of-round → next
- [ ] Timer par phase (buy 15 s, action 1:55, end 5 s)
- [ ] Conditions de victoire (élimination, objectif, time-out)
- [ ] Switch des camps à la mi-temps (12-12 → MR15)
- [ ] Overtime (MR3 ou similaire)

### 2.3 — Vie & dégâts

- [ ] HP / Armor system (100 / 100)
- [ ] Réduction des dégâts par armor
- [ ] Helmet (réduit headshot damage)
- [ ] Effets visuels : flash écran rouge, vignette, screen shake
- [ ] State `dead` + spectator mode

### 2.4 — Spectator mode

- [ ] Free camera
- [ ] Follow camera (cycle entre coéquipiers vivants)
- [ ] First-person spectate
- [ ] Death-cam (auto switch sur le tueur 2 sec)

### 2.5 — Achat d'armes

- [ ] Système d'argent par round (start, win, loss, kill bonus)
- [ ] Menu d'achat (touche B) avec catégories
- [ ] Drops d'arme à la mort
- [ ] Pick-up d'armes au sol

### 2.6 — Grenades

- [ ] Flashbang (avec écran blanc + sourdine audio)
- [ ] Smoke (volumes occlusion vis. + son)
- [ ] HE grenade (dégâts AOE + dropoff)
- [ ] Molotov / incendiaire (DOT au sol)
- [ ] Decoy / leurre
- [ ] Physique de grenade (bounce, throw force)

### 2.7 — Objectifs

- [ ] Mode Bomb : plant sites A/B, défuse kit
- [ ] Hostage rescue (alternative)
- [ ] Indicateurs HUD pour les objectifs

### 2.8 — Voice chat in-game

- [ ] Voice par équipe (push-to-talk + open mic)
- [ ] Codec Opus (32 kbps mono)
- [ ] Squelch / VAD
- [ ] Mute individuel par joueur
- [ ] Voice global pendant warmup uniquement

### 2.9 — Text chat

- [ ] Chat all (warmup uniquement) + chat équipe (toujours)
- [ ] Touche Y (all) / U (team)
- [ ] Filtrage profanity (option client)
- [ ] Cooldown anti-spam

---

## Phase 3 — Maps & contenu de base

> **Mode :** 🟢 **Ensemble** pour les maps techniques "blockout" (2 bombsites,
> un mid, géométrie en boîtes). Pas de prétention artistique : juste assez
> pour valider gameplay, callouts, équilibrage.
>
> **Plus tard :** 🟠 freelance level designer pour des maps polish ; couplé
> avec les artistes 3D (Phase 4) qui habilleront ces blockouts. Le level designer
> et l'artiste 3D peuvent souvent être la **même personne** chez un junior à
> mid-level.
>
> **Plateformes :** indifférent — les maps sont des assets, pas du code.
> Vérifier que Blender exporte bien sur les trois OS (oui, Blender est
> cross-platform).

### 3.1 — Format de map

- [ ] Choisir un format (glTF, BSP custom, source-style BSP)
- [ ] Loader de map dans `GameWorld`
- [ ] Collision mesh séparé du visual mesh
- [ ] Spawn points (team-tagged)
- [ ] Zones (bombsites, buyzones, danger zones)
- [ ] Triggers / brushes (eau, ladders, kill volumes)

### 3.2 — Outil de création de map

- [ ] Choisir : Blender + exporter custom, ou outil maison
- [ ] Documentation : grille, scale, conventions
- [ ] Plugin Blender pour spawn points et bombsites
- [ ] Visualiseur in-game de la nav-mesh

### 3.3 — Nav-mesh

- [ ] Génération auto depuis la géométrie de collision
- [ ] Détection chokepoints, callouts, hauteurs
- [ ] Utilisé par les bots IA (phase 13)

### 3.4 — Premières maps (blockout simple, 2 bombsites + mid)

> Au tout début : géométrie cubique grise, **zéro texture**. L'objectif est
> de valider la jouabilité et les distances, pas de faire joli. On habille
> tout en Phase 4 avec des artistes.

- [ ] **de_blockout_01** — première map test, 2 BP simples + 1 mid, callouts
      basiques
- [ ] **de_blockout_02** — variante (plus longue ligne de vue, style "dust")
- [ ] **de_blockout_03** — variante (couloirs serrés, style "inferno")
- [ ] **de_aim** — couloir d'entraînement 1v1
- [ ] **de_warmup** — petite arène pour pré-match
- [ ] Plus tard (🟠 avec freelance/artiste) : versions polish des blockouts qui
      ont prouvé leur valeur en playtest

### 3.5 — Callouts

- [ ] Données par map : noms des positions (long, banana, apps...)
- [ ] Affichage radar avec callouts
- [ ] Commande chat "I'm at <callout>"

### 3.6 — Radar / minimap

- [ ] Top-down view stylée par map
- [ ] Points équipiers visibles, ennemis sur ping
- [ ] Indicateur des bombsites
- [ ] Affichage projectiles (grenades vivantes)

---

## Phase 4 — Graphismes & direction artistique

> **Mode :** 🟡 **Ensemble + assets achetés** au début. Le code rendu (PBR,
> lights, shadows, post-processing) on le fait ensemble. Les **assets** —
> models, textures, animations, particules — on les achète sur Sketchfab,
> Quixel Megascans, CGTrader, Mixamo (animations gratuites), Kenney.nl
> (placeholder gratuit).
>
> **Plus tard :** 🔴 **équipe d'artistes 3D + AD + animateur** quand le
> jeu sera prouvé. Compter 3-5 artistes 3D minimum pour un FPS compétitif
> au visuel cohérent.
>
> **Plateformes :** **point dur** — macOS a déprécié OpenGL à 4.1. Voir 4.2.
>
> **Budget assets de démarrage :** 2-10 k€ pour un pack minimal acceptable.

### 4.1 — Décider la DA

- [ ] Style : photoréaliste / semi-stylisé / cel-shaded
- [ ] Mood-board, palette, références
- [ ] Briefer un AD freelance ou recruter

### 4.2 — Pipeline de rendu

> **Attention portabilité :** macOS a déprécié OpenGL et plafonne à 4.1.
> Vulkan ne tourne pas natif sur macOS (passe par MoltenVK). Deux options
> compatibles Windows+Mac : (a) rester en GL 4.1 — limite mais ça marche ;
> (b) passer en Vulkan via MoltenVK sur Mac ; (c) abstraire via une couche
> type bgfx / Sokol / Diligent qui cible GL+Vulkan+Metal.
> **Recommandation** : option (c) pour ne pas se peindre dans un coin.

- [ ] Choisir l'abstraction de rendu (rester GL 4.1 / Vulkan+MoltenVK / bgfx)
- [ ] Deferred rendering (G-buffer)
- [ ] PBR (metallic / roughness workflow)
- [ ] Cascade Shadow Maps (CSM) pour le soleil
- [ ] Local lights (point/spot) avec shadows
- [ ] SSAO
- [ ] Bloom + tone-mapping (ACES)
- [ ] FXAA / TAA / DLSS-FSR (au choix)
- [ ] Motion blur (camera-only, option utilisateur)

### 4.3 — Modèles

- [ ] **View-models** (animations 1st person) : 5 armes actuelles
- [ ] **World-models** (3rd person) : 5 armes
- [ ] Animations : draw, fire, reload (full + tactical), inspect, idle
- [ ] **Character models** : 4 attaquants + 4 défenseurs distinguables
- [ ] Animations character : idle, walk, run, crouch-walk, jump, climb, plant, defuse, death (multi-directionnelles)
- [ ] Skinning + rigging
- [ ] Hitboxes alignées avec la géométrie

### 4.4 — Particules & FX

- [ ] Muzzle flash par arme
- [ ] Smoke trails balles
- [ ] Impact decals + sparks par surface (béton, métal, bois, chair)
- [ ] Sang (option violence — désactivable)
- [ ] Smoke grenade volumetric
- [ ] Flash visuel
- [ ] Molotov flames

### 4.5 — Skins & customisation

- [ ] Pipeline pour skins d'armes (textures + finishes : wear, pattern, float)
- [ ] Pipeline pour skins de personnages
- [ ] Stickers, charms, music kits (inspiration CS)
- [ ] Système d'inspection in-game

### 4.6 — Animations / cinematic

- [ ] Intro de round (camera fly)
- [ ] Outro MVP highlight
- [ ] Animation de victoire / défaite

### 4.7 — Optimisation

- [ ] LOD système pour models / maps
- [ ] Frustum culling
- [ ] Occlusion culling (PVS pré-compilée par map)
- [ ] Instancing pour décor (caisses, plantes)
- [ ] Streaming des textures

---

## Phase 5 — Audio professionnel

> **Mode :** 🟡 **Ensemble + sound libraries achetées**. On code l'intégration
> OpenAL (3D positional, occlusion, reverb). On achète les samples bruts
> sur Soundsnap, Boom Library, Splice, A Sound Effect.
>
> **Plus tard :** 🟠 **sound designer freelance** pour layer/processer les
> samples bruts en sons signature ("ce flingue est reconnaissable à
> l'oreille"). 🔴 plus tard encore, un sound designer interne + un compositeur
> pour le main theme / saisons.
>
> **Plateformes :** OpenAL Soft tourne sur les 3 OS. Capture micro via
> miniaudio (cross-OS).
>
> **Budget samples de démarrage :** 500-3 k€ pour un pack arme + footsteps
> &nbsp;+ UI utilisable.

### 5.1 — Brancher OpenAL pour de vrai

- [ ] Loader WAV (déjà partiellement en place)
- [ ] Loader OGG (Vorbis via libvorbis)
- [ ] 3D positional audio
- [ ] Doppler + occlusion
- [ ] Reverb par zone (EAX-like)

### 5.2 — Sound design (toutes les armes)

- [ ] Tir, reload (sections : remove mag, insert, slide), draw, holster, inspect, dry-fire
- [ ] Versions distance (close, mid, far) — différents samples par distance
- [ ] Échantillons supresseur / sans
- [ ] Empreinte sonore unique par arme (reconnaissable à l'oreille)

### 5.3 — Footsteps

- [ ] Par matériau : concrete, metal, wood, sand, water, gravel, grass, ladder
- [ ] Par allure : walk / run / crouch / land
- [ ] Variations (4-6 samples par combo)
- [ ] Volume conditionné à la vitesse

### 5.4 — Voice / radio

- [ ] Callouts d'équipe ("enemy spotted", "go go go", "fire in the hole")
- [ ] Voix par character model (multilingue)
- [ ] Codec radio (filtre HF/LF, bruit)

### 5.5 — Ambient

- [ ] Wind, birds, distant traffic (par map)
- [ ] Stinger musical au round start
- [ ] Music pack changeable (skin)

### 5.6 — UI sons

- [ ] Hover, click, validation, erreur
- [ ] Menu music
- [ ] Buy menu confirm
- [ ] Hit confirm (tick)
- [ ] Headshot ding

### 5.7 — Mixage

- [ ] Bus master / SFX / weapons / voice / music / UI
- [ ] Side-chain ducking (voice abaisse SFX)
- [ ] Normalisation -14 LUFS pour music

---

## Phase 6 — UI / UX complète

> **Mode :** 🟢 **Ensemble**. ImGui pour la version fonctionnelle, suffisant
> pour playtests. Le visuel "AAA pro" ne s'impose qu'au lancement Steam.
>
> **Plus tard :** 🟠 **UI/UX designer freelance** pour redessiner toute la
> UI au moment du lancement public — un menu beau et lisible vaut
> littéralement des % de conversion / rétention.
>
> **Plateformes :** ImGui est natif cross-OS. Pour la version "skin custom"
> finale, soit on reste ImGui, soit on passe à RmlUI (HTML/CSS-like) ou à
> une UI maison. Choix à trancher avant le polish.

### 6.1 — Menu principal

- [ ] Background animé ou model 3D rotatif
- [ ] Play / Settings / Inventory / Friends / Quit
- [ ] News panel (RSS / API news live)
- [ ] Battle pass progress

### 6.2 — Pre-match lobby

- [ ] Liste des joueurs avec ping, level, K/D
- [ ] Map vote / pick & ban
- [ ] Ready button
- [ ] Captain mode (1 désigné, ou tournament mode)

### 6.3 — Settings — Video

- [ ] Résolution + display mode (fullscreen / borderless / windowed)
- [ ] Refresh rate
- [ ] VSync / G-Sync / FreeSync
- [ ] FPS cap
- [ ] Field of view (90-120)
- [ ] Qualité globale + sliders par poste (textures, shadows, AA, post-FX)
- [ ] HDR support
- [ ] Color blind modes (3 types)
- [ ] Brightness / gamma calibration screen

### 6.4 — Settings — Audio

- [ ] Master / SFX / Music / Voice / UI sliders
- [ ] Output device picker
- [ ] HRTF on / off
- [ ] Voice chat mode (PTT / open mic / off)
- [ ] Input device picker
- [ ] Mic test + level meter

### 6.5 — Settings — Controls (keybinds)

- [ ] Rebind toutes les touches
- [ ] Profils multiples (per game mode)
- [ ] Sensitivity (incl. ADS multipliers par scope level)
- [ ] Raw input + mouse acceleration off
- [ ] Crouch toggle / hold
- [ ] Walk toggle / hold
- [ ] Auto-jump / auto-bhop (option compétitive on/off)
- [ ] Quickswitch keys (slot 1-5, last weapon, knife, nades hotkeys)

### 6.6 — Settings — Gameplay

- [ ] HUD scale / position
- [ ] Crosshair editor complet (style, size, gap, thickness, color)
- [ ] Viewmodel position (FOV, offset X/Y/Z, hand)
- [ ] Net graph (ping, fps, var, loss)
- [ ] Damage indicators direction
- [ ] Chat opacity / size

### 6.7 — Scoreboard

- [ ] Touche Tab
- [ ] Lignes : nom, score, K, A, D, ping, MVP count, $$
- [ ] Sort par score / kills / KDR
- [ ] Couleurs équipe
- [ ] Avatar Steam + flag pays
- [ ] Click sur joueur → profil / report / mute / friend

### 6.8 — End-of-match screen

- [ ] Résumé score
- [ ] MVP du match
- [ ] Stats personnelles (kills, accuracy, headshot %)
- [ ] Drop d'item (skin, charm, sticker)
- [ ] XP gagnée + niveau
- [ ] Bouton "play again" / "back to lobby"

### 6.9 — HUD en jeu (à étendre depuis ce qu'on a)

- [ ] HP + Armor
- [ ] Money
- [ ] Timer round
- [ ] Score équipes
- [ ] Indicateur bombe (planted / defusing / time left)
- [ ] Damage indicators directionnels
- [ ] Kill feed (top droite)
- [ ] Money awards en bas (k+ pour kill)

### 6.10 — Accessibilité

- [ ] Subtitles (callouts, kill feed, etc.)
- [ ] Sound visualizer (directionnel pour malentendants)
- [ ] Hold to remap → toggle pour tout
- [ ] Echelles de texte
- [ ] Color blind (déjà mentionné mais répété)
- [ ] Reduced motion (désactive bobbing, screen shake)
- [ ] Photosensitive mode (limite flash)

### 6.11 — Localisation (i18n)

- [ ] String tables externalisées (JSON/PO)
- [ ] Tooling extraction strings du code
- [ ] Langues priorité 1 : EN, FR, ES, DE, PT-BR, RU, ZH-CN, ZH-TW, JA, KR
- [ ] Langues priorité 2 : IT, PL, TR, AR, TH, VN
- [ ] Tests RTL (arabe)
- [ ] Tests longueurs (allemand cassé un layout EN)

---

## Phase 7 — Backend, comptes & matchmaking

> **Mode :** 🟢 **Ensemble**. C'est du dev backend classique :
> PostgreSQL + Redis + un service HTTP (C++ Drogon/Crow, ou Go/Rust si on
> veut une stack séparée). Login via Steam OpenID — pas de mot de passe
> chez nous, on délègue.
>
> **Plus tard :** 🔴 quand la base de joueurs grossit (> 10k DAU),
> un **dev backend dédié + DBA** deviennent nécessaires.
>
> **Plateformes :** indifférent, ça tourne sur des serveurs Linux. Le
> client se contente d'appeler des endpoints HTTPS, donc cross-OS gratuit.
>
> **Dépendances :** Phase 16 (Steam) pour avoir l'AppID et tester le login
> Steam ; on peut commencer avant en hardcodant un fake auth.

### 7.1 — Compte joueur

- [ ] Login via Steam (OpenID) — pas de password chez nous
- [ ] Profil joueur : username, avatar, level, badges, stats lifetime
- [ ] GDPR : export des données + suppression
- [ ] Banlist + appeal process

### 7.2 — Matchmaking

- [ ] MMR / Elo / Glicko-2 par mode
- [ ] Queue par mode (compet 5v5, casual, deathmatch, training)
- [ ] Estimation du temps d'attente
- [ ] Solo queue / duo / 5-stack
- [ ] Pénalité de file (quit, AFK, vote-kick)

### 7.3 — Ranks (compétitif)

- [ ] Tiers (Bronze → Silver → Gold → Platinum → Diamond → Champion → Top 500)
- [ ] Placement matches (10 games)
- [ ] Decay si inactif > 30 jours
- [ ] Reset saisonnier
- [ ] Rang affiché dans le scoreboard

### 7.4 — Stats & leaderboard

- [ ] DB (PostgreSQL) : match history, stats agrégées
- [ ] Endpoints API REST pour profil public
- [ ] Leaderboard global / par région / par ami
- [ ] Replays accessibles via match ID

### 7.5 — Backend services

- [ ] Auth service (Steam tickets)
- [ ] Matchmaking service (Redis + worker)
- [ ] Game session service (alloue serveurs)
- [ ] Stats ingest service (depuis serveurs de jeu)
- [ ] CDN pour assets / patchs
- [ ] Logging centralisé (ELK / Loki)
- [ ] Monitoring (Prometheus + Grafana)
- [ ] Alerting (PagerDuty / Opsgenie)

---

## Phase 8 — Serveurs dédiés & infrastructure

> **Mode :** 🟢 **Ensemble** au début (1-2 régions, OVH ou Hetzner, K8s
> géré type DOKS/EKS).
>
> **Plus tard :** 🔴 **SRE / DevOps freelance puis interne** pour gérer
> l'auto-scaling, le multi-région, l'orchestration Agones. Coût mensuel
> qui grimpe vite à 4 chiffres puis 5.
>
> **Budget infra de démarrage :** 100-500 €/mois pour 1-2 régions et
> petite charge ; 5-50 k€/mois à 100k DAU.
>
> **Plateformes :** serveurs **Linux uniquement** (headless). Client
> client toujours Win/Mac/Linux.

### 8.1 — Serveur de jeu (dédié)

- [ ] Mode `--dedicated` headless (pas de window/GL)
- [ ] CLI args : map, port, gamemode, max players
- [ ] Hot reload de la config sans restart
- [ ] Graceful shutdown (drain matchs en cours)
- [ ] RCON / HTTP admin API

### 8.2 — Régions

- [ ] Régions cibles : EU-W, EU-E, NA-E, NA-W, SA, OCE, ASIA, ME, AF
- [ ] Cloud providers : AWS / GCP / OVH / Hetzner / bare metal
- [ ] Mesure ping par client → région suggérée
- [ ] Routing géographique du matchmaking

### 8.3 — Orchestration

- [ ] Kubernetes ou Nomad pour scheduling
- [ ] Auto-scaling selon la demande
- [ ] Bin-packing : N serveurs par machine
- [ ] Game Server Manager (Agones recommandé sur K8s)
- [ ] Health checks + auto-reboot

### 8.4 — Servers privés / community

- [ ] Téléchargement gratuit du binaire dédié
- [ ] Doc d'auto-hébergement (port forwarding, firewalls)
- [ ] Listing public des serveurs communauté
- [ ] Tags : ranked / unranked / mod / vanilla
- [ ] Slot reservation, password, voting kick

### 8.5 — Distinction matchs officiels vs communauté

- [ ] **Matchs officiels** = nos serveurs uniquement, anti-cheat strict, comptent pour le rang
- [ ] **Matchs communauté** = serveurs privés, anti-cheat lecture seule, ne comptent pas pour le rang
- [ ] Badge visuel pour distinguer
- [ ] Achievements progressent seulement en officiel

### 8.6 — DDoS protection

- [ ] Provider avec mitigation (OVH, Cloudflare Magic Transit)
- [ ] Rate-limiting au handshake ENet
- [ ] Banlist IP automatique sur abus
- [ ] Captcha au login si pattern suspect

### 8.7 — Patching & déploiement

- [ ] CI/CD GitHub Actions ou GitLab CI
- [ ] Build matrix Windows/Mac/Linux
- [ ] Signature des binaires
- [ ] Canal beta / live
- [ ] Patcher delta (binary diff, BPS / xdelta)
- [ ] Hot-fix rapide (1h de QA → push)

### 8.8 — CI/CD multi-OS (mise en place — H0 ✅)

> **Mode :** 🟢 **Ensemble**. Installé avant Phase 1 pour ne jamais
> merger du code qui casse un OS.

État actuel :

- [x] Workflow `build.yml` — matrix `[ubuntu, macos, windows] × [Debug, Release]`
- [x] vcpkg en mode manifest (`vcpkg.json`) + binary cache GHA
- [x] `TRUESHOT_WARNINGS_AS_ERRORS=ON` activé en CI (équivaut à `cmake --preset strict`)
- [x] Smoke test : présence du binaire compilé sur chaque OS
- [x] Artifacts uploadés (`TrueShot-{OS}-{sha}`) en Release, retention 14 jours
- [x] Concurrency control : un seul build par branche, ancien annulé
- [x] Workflow `lint.yml` — clang-format, EditorConfig, markdownlint, yamllint
- [x] Workflow `clang-tidy.yml` — analyse statique hebdo + on-demand
- [x] Configs versionnées : `.clang-format`, `.clang-tidy`, `.editorconfig`,
      `.markdownlint.json`, `.yamllint.yml`
- [x] PR template + ISSUE templates (bug / feature)
- [x] Documentation CI dans `CONTRIBUTING.md`

À ajouter au fil des phases :

- [ ] **Phase 1** : job `network-test` (boucle serveur + N clients fake, assert
      convergence des positions)
- [ ] **Phase 2** : job `gameplay-smoke` (match scripté de bout en bout)
- [ ] **Phase 4** : job `shaders-validate` (glslang validator + spirv-cross)
- [ ] **Phase 7** : job `backend-test` (lance la DB en service, teste les
      endpoints REST)
- [ ] **Phase 9** : job `security` (CodeQL, dependency scanning, secret scan)
- [ ] **Phase 15** : job `perf-regression` (Tracy benchmarks vs baseline)
- [ ] **Phase 16** : job `package-steam` (depot signé prêt pour
      `steamcmd app_build_*`)
- [ ] **Phase 17** : workflow `release.yml` (tag → upload Steam beta branch
      → poste sur Discord)
- [ ] Notification Discord/Slack en cas de fail sur `main`
- [ ] Branche `main` protégée GitHub : requirement = `required-checks` job
      green (déjà câblé dans build.yml)

### 8.9 — Budget infrastructure

- [ ] Estimation coût mensuel (X joueurs × Y matches × Z $$)
- [ ] Plan de scaling (10k DAU → 100k → 1M)
- [ ] Tracking dépenses cloud
- [ ] FinOps : reserved instances, spot pour matchmaking

---

## Phase 9 — Anti-cheat (mission critique)

> **Mode :** 🔴 **Ensemble + devs sécurité que tu recrutes**.
> **Décision produit forte :** on **ne licencie pas** EAC / BattlEye /
> Vanguard. On construit **notre propre anti-cheat kernel-based**, avec
> l'ambition d'être **le meilleur AC de l'industrie**. Cela prendra
> **2-4 ans** et nécessite des spécialistes sécurité bas-niveau, mais
> c'est un investissement stratégique : on possède la techno, on peut la
> licencier à d'autres studios à terme, et c'est un argument marketing
> majeur ("AC propriétaire, zéro dépendance externe").
>
> **Plateformes :**
>
> - **Windows :** kernel driver signé (EV cert obligatoire, ~250-400 €/an,
>   plus signature WHQL Microsoft).
> - **macOS :** Apple interdit les kexts depuis Big Sur. Sur Mac on reste
>   en user-mode + Endpoint Security Framework. Conséquence : **matchs
>   compétitifs officiels Windows-only au lancement**, Mac reste autorisé
>   pour casual / community. À communiquer clairement dans la FAQ et le
>   store.
> - **Linux :** module noyau possible mais segment trop petit pour
>   justifier ; user-mode + eBPF pour le compétitif Linux casual.
>
> **Équipe minimale :** 2-3 ingénieurs sécurité bas-niveau (Windows kernel,
> reverse engineering, ML détection patterns). Si tu connais des devs
> compétents en sécurité Windows, ils sont l'asset principal du projet.
>
> **Durée :** version 1 utilisable = 12-18 mois. Version "meilleur de
> l'industrie" = 2-4 ans en itération continue.
>
> **Approche défense en profondeur** : on superpose les couches, aucune
> n'est suffisante seule.

### 9.1 — Côté serveur (autoritaire)

- [ ] Validation des inputs (clamp angles, vitesse, accélération)
- [ ] Validation des hits (raycast côté serveur uniquement, jamais trust client)
- [ ] Sanity check positions (téléport > N m/s → rollback)
- [ ] Sanity check tirs (cadence, distance vs précision, headshot ratio anormal)
- [ ] Détection patterns suspects (KDR + accuracy > X = flag)
- [ ] Heatmap des hits (silent aim → patterns détectables)

### 9.2 — Anti-cheat kernel-level Windows (notre driver propriétaire)

> **Décision :** construit en interne. Non négociable.
> Cette section nécessite des **devs sécurité Windows kernel** dédiés.

- [ ] **Étude de faisabilité** (1 mois) : tour d'horizon des protections
      existantes (EAC, BattlEye, Vanguard, FACEIT AC) pour comprendre la
      cible à dépasser.
- [ ] Obtenir un **EV Code Signing Certificate** + **attestation WHQL**
      Microsoft (obligatoire pour driver kernel signé sur Win 10/11).
- [ ] Squelette de driver kernel signé (KMDF) qui démarre et communique
      avec le client user-mode via IOCTL.
- [ ] Protection process : empêcher l'ouverture du process `TrueShot.exe`
      par d'autres process en lecture/écriture (ObRegisterCallbacks).
- [ ] Détection des modules chargés dans le process (PsSetLoadImageNotifyRoutine).
- [ ] Détection des drivers tiers suspects (PsSetCreateProcessNotifyRoutineEx).
- [ ] Anti-VM detection (hyperviseur, sandbox malveillant, vmexit timing).
- [ ] **Anti-DMA** : détection accès mémoire externe (DMA cheats matériels
      type Captain DMA). Vérification IOMMU, scans périodiques.
- [ ] **Anti-hyperviseur cheats** : détection bluepill / hyperviseur posé
      sous le système (PCID, RDTSC timing, CPUID branding).
- [ ] **Heartbeat chiffré** vers le serveur (client mort = client kické).
- [ ] **Code self-integrity** : hash périodique des sections .text du
      driver + du process, comparaison serveur.
- [ ] Logs des événements suspects → telemetry pour ML (section 9.4).

### 9.3 — Anti-cheat user-mode

- [ ] Détection injection DLL (modules signés uniquement)
- [ ] Détection hooks (IAT, inline, VTable)
- [ ] Détection debuggers attachés
- [ ] Détection mémoire scannée (canary values, encryption)
- [ ] Anti-tampering du binaire (signature checks)
- [ ] Obfuscation runtime des structures critiques (positions ennemis)

### 9.4 — Telemetry & ML

- [ ] Collecte des inputs raw (mouse trajectories) avec consent
- [ ] Modèle ML pour détecter aimbot (snap suspects)
- [ ] Modèle pour détecter wallhack (pré-aim systématique)
- [ ] Pipeline d'analyse offline + flag manuel
- [ ] Système de signalements (player reports)

### 9.5 — Ban system

- [ ] Hardware ID ban (HWID via plusieurs facteurs)
- [ ] Steam account ban
- [ ] IP ban temporaire
- [ ] Ban en vague (1x/semaine — démoralise les cheaters)
- [ ] Trust factor (visible côté joueur seulement en bas)

### 9.6 — Replays comme preuve

- [ ] Replay automatique de chaque match officiel
- [ ] Outil de review pour modération
- [ ] Démos téléchargeables côté joueur (ses propres matchs)

### 9.7 — Side-channel

- [ ] OBS / streaming detection (info leaks possibles)
- [ ] Detection multi-account (alt smurfs)
- [ ] Detection boost (premade contre throwers)

### 9.8 — Politique

- [ ] CoC (Code of Conduct) clair
- [ ] Process d'appel
- [ ] Communication transparente sur les ban waves
- [ ] Statistiques publiques (anti-cheat report mensuel)

---

## Phase 10 — Économie & progression

> **Mode :** 🟢 **Ensemble**. **Passage par Steam uniquement** pour les
> achats — pas de wallet maison, pas de site marchand parallèle. Steam
> Marketplace + Steam Inventory Service gèrent les trades et les
> ownership ; on se concentre sur la mécanique en jeu.
>
> **Plus tard :** 🔴 quand le volume justifie, un **game economy
> designer** pour balancer pricing, drop rates, battle pass curve, et
> un **comptable spécialisé revenus internationaux** (TVA OSS UE, sales
> tax US par état, withholding taxes).
>
> **Loot boxes : interdites en Belgique et Pays-Bas**, fortement encadrées
> UK / FR. On évite les loot boxes payantes ; on fait du
> **direct purchase + battle pass** uniquement.

### 10.1 — XP & niveaux

- [ ] XP par match (base + win bonus + perfs)
- [ ] Niveau du compte (1 → 100 → prestige)
- [ ] Récompenses par palier

### 10.2 — Battle pass

- [ ] Saisons de ~3 mois
- [ ] 100 paliers gratuits + 100 premium
- [ ] Récompenses : skins, charms, stickers, music kits, XP boost
- [ ] Achetable une fois par saison

### 10.3 — Boutique

- [ ] Direct purchase d'items
- [ ] Rotations quotidiennes / hebdo
- [ ] Bundles
- [ ] Cas / loot boxes (attention règlementation pays : Belgique, NL, FR PEGI)

### 10.4 — Marketplace player-to-player

- [ ] Steam Community Market integration
- [ ] Trades inter-joueurs (anti-fraud)
- [ ] Commission marketplace

### 10.5 — Inventaire

- [ ] DB items avec ownership
- [ ] Skin equipping per arme
- [ ] Stockage de charms, stickers
- [ ] Statrak (compteur de kills sur arme)
- [ ] Inspect 3D des skins

### 10.6 — Économie in-match (déjà mentionnée)

- [ ] Argent par round (kill, win, loss bonus progressif)
- [ ] Buy menu
- [ ] Drops d'armes
- [ ] Money cap (16k inspiré CS)

### 10.7 — Anti-fraude économique

- [ ] Logs de toutes les transactions
- [ ] Refund process
- [ ] Détection chargeback abusif
- [ ] Hold sur trades pour nouveaux comptes

---

## Phase 11 — Social & communauté

> **Mode :** 🟢 **Ensemble** + **Steam Friends API**. On délègue à Steam
> la friend list, l'invite via overlay, le voice chat lobby pre-match.
> En jeu, on garde notre propre voice chat (Opus + ENet) pour le contrôle
> qualité.
>
> **Plus tard :** 🔴 **community managers** (1 à 4 selon taille
> communauté) pour Discord, modération chat in-game, gestion incidents
> toxiques.

### 11.1 — Amis

- [ ] Friend list (via Steam ou maison)
- [ ] Status (online, in-match, away)
- [ ] Invite to lobby
- [ ] Join via Steam overlay

### 11.2 — Clans / équipes

- [ ] Création d'équipe (jusqu'à 10 membres)
- [ ] Tag clan visible
- [ ] Stats clan
- [ ] Tournament mode (équipes vs équipes)

### 11.3 — Communication

- [ ] Voice chat lobby (avant queue)
- [ ] Voice chat in-match (team only en compet)
- [ ] Text chat lobby + in-match
- [ ] Quick chat wheel (radial menu callouts)

### 11.4 — Modération

- [ ] Bouton report (cheating, harassment, AFK)
- [ ] Système de tickets côté support
- [ ] Bot anti-toxic chat (filtres + warnings)
- [ ] Communication ban (text/voice) escalade

### 11.5 — Profils publics

- [ ] Page profil web (truestats.com style)
- [ ] Lifetime stats, charts, replays
- [ ] Badges / achievements
- [ ] Showcase d'inventaire

---

## Phase 12 — Mod support, serveurs privés, workshop

> **Mode :** 🟢 **Ensemble** + **Steam Workshop API**. Pour le scripting
> mod, **Lua** (via sol2 ou LuaJIT) est le choix par défaut : léger,
> sandboxable, communauté FPS habituée (CS:GO/Source). Wren et Squirrel
> sont des alternatives ; on tranchera à l'implémentation.
>
> **Plateformes :** Lua tourne sur les 3 OS sans souci. Steam Workshop
> gère le téléchargement / mise à jour des mods automatiquement.
>
> **Important :** les mods utilisateurs **désactivent automatiquement
> l'anti-cheat strict** (on passe en mode lecture-seule). Les matchs
> avec mods ne comptent jamais pour le rang officiel.

### 12.1 — SDK / outils mod

- [ ] Documentation API mod (Lua / Wren / Python embarqué)
- [ ] Hooks pour scripter le gameplay
- [ ] API pour spawn entities, modifier règles
- [ ] Sandbox de sécurité (pas d'accès fichiers en dehors mod dir)

### 12.2 — Workshop Steam

- [ ] Intégration Steam Workshop
- [ ] Upload de maps
- [ ] Upload de modes
- [ ] Upload de skins (si validés)
- [ ] Système de votes / favoris

### 12.3 — Modes de jeu communautaires

- [ ] Surf
- [ ] Kreedz / climb
- [ ] Bhop
- [ ] Aim_botz style
- [ ] Retake / Execute
- [ ] Zombie escape
- [ ] Trouble in Terrorist Town

### 12.4 — Serveurs privés

- [ ] Console admin
- [ ] Plugins serveur (charger des .so / .dll de mods)
- [ ] Compatibilité Linux serveur dédié
- [ ] Ranking communautaire (Faceit / ESEA-style intégrable)

---

## Phase 13 — Bots & IA

> **Mode :** 🟢 **Ensemble**. Objectif : bots **utiles pour le tutoriel**
> et **agréables pour débutants/casual**. Pas l'ambition de battre des
> joueurs experts, pas de ML lourd. Style "bots CS:GO" : un peu naïfs,
> mais ils tirent, ils bougent, ils plantent la bombe, ils servent de
> sparring partner.
>
> **Plus tard :** 🟠 si on veut faire des bots plus crédibles pour le
> deathmatch entraînement, un freelance IA peut affiner les behavior
> trees. Pas une priorité.
>
> **Plateformes :** code pur C++, cross-OS d'office.

### 13.1 — Bot pour test

- [ ] Bot stand-still (cible vivante)
- [ ] Bot navigation simple (waypoint)
- [ ] Spawn / kill / respawn commands

### 13.2 — Bot pour casual / training

- [ ] Niveaux : easy / normal / hard / expert
- [ ] Behavior tree (BehaviorTree.CPP ou maison)
- [ ] Path-finding via nav-mesh
- [ ] Tir avec recul humanisé (pas frame-perfect)
- [ ] Réaction aux sons, vision FOV cone

### 13.3 — Coop vs bots

- [ ] Mode 5 joueurs vs bots (defense / attack)
- [ ] Vague d'IA progressive

### 13.4 — Tutoriel

- [ ] Onboarding step-by-step (mouvement, tir, achat, plant, defuse)
- [ ] Bots scripts pour aider
- [ ] Aim training maps (aim_botz like) jouable seul

---

## Phase 14 — Compétitif & e-sport

> **Mode :** 🟢 **Ensemble pour le dev** des outils techniques :
> spectator mode complet avec POV switching, **délai broadcast configurable**
> (typiquement 30-120 s pour empêcher le stream sniping), API
> broadcast (GameState Integration JSON push). Twitch est la cible
> évidente pour le broadcasting.
>
> **Plus tard :** 🔴 **équipe e-sport** dédiée pour organiser tournois,
> contacter pros, gérer les sponsors, organiser les LANs majeurs. Ça
> arrive **après** que le jeu ait prouvé une base de joueurs compétitifs
> stable (>50k DAU compétitifs).
>
> **Économie e-sport :** on note dès maintenant les mécaniques (cash
> prizes, Pick'em, viewership drops Twitch) mais on ne les développe
> qu'à partir de l'an 2-3 post-lancement.

### 14.1 — Mode tournament

- [ ] Pause technique
- [ ] Coach mode (spectator d'équipe)
- [ ] Veto système (pick & ban map)
- [ ] Score MR12 / MR15 / Bo3 configurable
- [ ] Overtime activable

### 14.2 — Spectator & broadcasting (priorité dev)

- [ ] Mode spectator complet : free cam, follow cam, first-person POV
- [ ] **POV switching** : passer instantanément d'un joueur à l'autre
      (touche, ou click sur scoreboard)
- [ ] **Délai broadcast configurable** (30 / 60 / 120 s) — obligatoire pour
      empêcher le stream sniping pendant les matchs diffusés
- [ ] Caméras fixed par map (placées par le level designer dans le .map)
- [ ] X-ray vision (voir à travers murs) — réservé spectator, jamais joueur
- [ ] Économie visible (loadout par joueur, money par équipe)
- [ ] Stats live (per round, per joueur, ADR, KAST, HS%)
- [ ] **Replay rewind** in-match : revoir la dernière action
- [ ] **Talent overlay API** : GameState Integration JSON push vers HTTP
      endpoint configurable — broadcasters branchent OBS / vMix / leur
      stack custom (scoreboard, kill cam, MVP)
- [ ] **Twitch Extension** : drops viewer, prédictions, stats embeddées

### 14.3 — Démo / replay

- [ ] Format de replay propriétaire
- [ ] Playback contrôlé (pause, slowmo, fast forward, seek)
- [ ] POV switch (any player, any team, free)
- [ ] Export clip (vidéo H.264) avec marqueurs

### 14.4 — API broadcast

- [ ] GameState Integration (HTTP push à un endpoint configurable)
- [ ] Format JSON stable et documenté
- [ ] OBS plugin pour scoreboard live
- [ ] Twitch extension (drops, predictions, stats live)

### 14.5 — Anti-cheat tournaments

- [ ] Mode LAN tournament (anti-cheat encore plus strict)
- [ ] Préparation machines admin
- [ ] Outils forensic en cas de soupçon

---

## Phase 15 — Qualité, QA, certification

> **Mode :** 🟢 **Ensemble** pour les tests automatisés (unit, smoke,
> intégration). 🟠 **playtesters externes** (Discord communauté, NDA)
> pour les phases alpha/beta. 🔴 **QA testers internes** dédiés au
> lancement (3-5 personnes minimum).
>
> **Plateformes :** matrice de tests OBLIGATOIRE Windows + macOS + Linux
> et Steam Deck à chaque release. CI doit refuser un merge si un OS
> casse.

### 15.1 — Tests automatisés

- [ ] Unit tests (Catch2 / GoogleTest)
- [ ] Tests réseau (clients simulés)
- [ ] Tests de charge serveur
- [ ] CI : tous les tests à chaque PR

### 15.2 — Smoke tests

- [ ] Launcher → menu → quit
- [ ] Login → matchmaking → match → quit
- [ ] Crash detection + Sentry/Crashpad

### 15.3 — Playtests

- [ ] Closed alpha (friends + family)
- [ ] Closed beta (invités, NDA)
- [ ] Open beta (Steam playtest)
- [ ] Itération sur feedback

### 15.4 — Performance

- [ ] Profiling continu (Tracy)
- [ ] Budget frame : 16.6 ms @ 60 FPS, 8.3 ms @ 120, 6.9 @ 144
- [ ] Mesure 1 % low FPS, pas juste l'avg
- [ ] Memory leaks (Valgrind / ASan / heap profiler)

### 15.5 — Localization QA

- [ ] Test layout par langue
- [ ] Validation native speakers
- [ ] Tests subtitles + voix

### 15.6 — Compatibilité

- [ ] Matrix GPU testé : NVIDIA RTX 20/30/40, AMD RX 5/6/7, Intel Arc, iGPU
- [ ] Drivers : minimum supporté + recommandé
- [ ] OS : Windows 10/11, macOS 12+, Ubuntu 22.04+, Steam Deck

### 15.7 — Steam Deck compat

- [ ] Verified status (UI lisible 800p, controller mapping)
- [ ] Performance @ 30 FPS minimum
- [ ] Power profile

### 15.8 — Conformité plateforme

- [ ] Steamworks SDK
- [ ] Achievements
- [ ] Cloud saves (settings)
- [ ] Trading cards
- [ ] Steam Input (manettes Steam)

---

## Phase 16 — Publication Steam

> **Mode :** 🟢 **Ensemble** pour le setup technique (Steamworks SDK,
> upload builds, store page, achievements). 🔴 **comptable spécialisé
> jeu vidéo OBLIGATOIRE** dès cette phase — Steam retient 30 %, tu vas
> avoir des revenus internationaux multi-devises, TVA OSS UE, withholding
> US, et possiblement le crédit impôt jeu vidéo (CIJV) en France. **Ne
> pas faire la compta seul, même un mois.**
>
> 🔴 **Avocat numérique** également indispensable ici (voir Phase 18) :
> CGU/EULA/Privacy à valider, conformité PEGI/ESRB.
>
> **Plateformes Steam :** Windows + macOS + Linux activés sur la même
> Steam app. Steam Deck verified à demander activement.
>
> **Coûts directs :**
>
> - Steam Direct : 100 $ par jeu publié (récupérables après 1000 $ de revenu).
> - Age rating (IARC gratuit pour PEGI/ESRB en général, USK Allemagne payant).
> - Comptable : 200-1000 €/mois selon volume.
> - Avocat pack légal initial : 2-5 k€.

### 16.1 — Steamworks setup

- [ ] Compte Steamworks ($100 frais)
- [ ] App ID
- [ ] Store page (capsules, screenshots, trailers)
- [ ] Description multilingue
- [ ] System requirements
- [ ] Age rating (PEGI, ESRB, USK, CERO)

### 16.2 — Steam Direct review

- [ ] Build envoyé pour review (≥ 2 semaines)
- [ ] Page validée

### 16.3 — Soumission age rating

- [ ] IARC questionnaire
- [ ] Validation locale (Allemagne USK requise pour vente DACH)

### 16.4 — Wishlist campaign

- [ ] Coming soon page (3-6 mois avant)
- [ ] Trailers d'annonce, gameplay, sortie
- [ ] Demo pendant Next Fest

### 16.5 — Pricing

- [ ] Decide F2P vs payant (TrueShot est sans doute F2P avec économie skins)
- [ ] Si payant : pricing tier par région
- [ ] Discount strategy (lancement -10%, soldes saisonniers)

### 16.6 — Launch day

- [ ] Press kit (presskit() format)
- [ ] Build day -7 : freeze, QA final
- [ ] Build day -1 : push sur Steam, branch test
- [ ] Day 0 : flip switch, monitor live
- [ ] Day 0+ : hotfix room en standby

---

## Phase 17 — Post-launch & live ops

> **Mode :** 🟢 **Ensemble** pour le dev des patches et events.
> 🟠 **personnes extérieures** pour : support joueur (helpdesk), community
> management Discord, modération chat, traducteurs pour les patch notes,
> data analyst pour les dashboards live ops.
>
> Le **live ops** est un mode de fonctionnement, pas une étape : il dure
> tant que le jeu vit.

### 17.1 — Cadence de patches

- [ ] Hotfixes : urgents, < 48 h
- [ ] Patches contenu : 2-4 semaines
- [ ] Saisons : 3 mois

### 17.2 — Live ops dashboard

- [ ] DAU / MAU
- [ ] Funnel rétention D1/D7/D30
- [ ] Revenue ARPU / LTV
- [ ] Match completion rate
- [ ] Crash rate par version
- [ ] Anti-cheat detections / bans

### 17.3 — Events temporaires

- [ ] Game modes limités (Halloween, Noël)
- [ ] Maps événement
- [ ] Récompenses exclusives

### 17.4 — Communication

- [ ] Patch notes en N langues
- [ ] Roadmap publique (cette doc, version dégrossie)
- [ ] Devblog mensuel
- [ ] Communautés : Discord officiel, subreddit, Twitter / X

### 17.5 — Support joueur

- [ ] Helpdesk (Zendesk / Freshdesk)
- [ ] Workflows : ticket → triage → résolution
- [ ] SLA réponse : P1 < 24h, P2 < 72h
- [ ] FAQ + base de connaissances

---

## Phase 18 — Juridique, légal, business

> **Mode :** 🟢 **Ensemble** pour la prise de décisions, le suivi et
> l'archivage. 🔴 **avocat numérique + comptable + assureur indispensables
> en parallèle dès la création de l'entité**. Aucune partie de cette
> phase ne se fait en solo. **Toute économie ici se paiera 10x au
> premier procès, audit fiscal ou data breach.**
>
> **Quand démarrer :** **avant** Phase 16 (Steam) idéalement. Dès qu'un
> playtest public ou un €1 de revenu est en jeu, l'entité doit exister.

### 18.1 — Structure légale

- [ ] Créer entité (SAS / SARL / LLC selon pays)
- [ ] Compte bancaire pro
- [ ] Comptable / expert-comptable

### 18.2 — Propriété intellectuelle

- [ ] Marque déposée "TrueShot" (INPI, EUIPO, USPTO)
- [ ] Copyright assets
- [ ] Licence claire pour mods communautaires

### 18.3 — Contrats

- [ ] Freelance contracts (artistes, dev contributeurs)
- [ ] NDA pour playtesters
- [ ] CGU / EULA joueurs
- [ ] Privacy policy (RGPD-compliant)
- [ ] Cookie policy (site web)
- [ ] Conditions de vente (skins)

### 18.4 — Conformité

- [ ] RGPD (UE)
- [ ] CCPA (Californie)
- [ ] LGPD (Brésil)
- [ ] COPPA (US, mineurs)
- [ ] Loot boxes : interdites en Belgique, NL, restrictions UK/FR
- [ ] Age gating

### 18.5 — Fiscalité

- [ ] TVA / sales tax par pays
- [ ] Withholding tax sur revenus tiers (streamers, influencers)
- [ ] Crédit impôt jeu vidéo (France : CIJV) si éligible

### 18.6 — Assurances

- [ ] Responsabilité civile pro
- [ ] Cyber-assurance (data breach)
- [ ] Assurance D&O si levée de fonds

---

## Phase 19 — Marketing & growth

> **Mode :** ~50 % 🟢 **ensemble** (brand, trailers maison, Discord, X,
> YouTube, Twitch, TikTok organique, press kit, page Steam, Reddit).
> ~50 % 🔴 **personnes payées** (acquisition payante Google/Meta/TikTok
> Ads, partenariats influenceurs majeurs, PR agency optionnelle,
> tournaments invitationnels, événements physiques type Gamescom).
>
> **Budget marketing minimal au lancement :** 20-100 k€ pour acquérir
> 10-50 k joueurs initiaux. Sans budget, le bouche-à-oreille seul peut
> suffire si le jeu est exceptionnel et qu'on travaille la communauté
> très en amont (12+ mois avant launch).

### 19.1 — Brand

- [ ] Logo final + variations
- [ ] Guidelines marque (typo, couleurs, ton)
- [ ] Site officiel (truestot.gg, etc.)

### 19.2 — Trailers

- [ ] Reveal trailer (1 min, accroche)
- [ ] Gameplay deep-dive (5-10 min)
- [ ] Launch trailer (45 sec)
- [ ] Trailers de saison

### 19.3 — Réseaux sociaux

- [ ] X / Twitter ✅ (déjà existant)
- [ ] YouTube ✅
- [ ] Twitch ✅
- [ ] TikTok / Shorts
- [ ] Discord serveur officiel
- [ ] Reddit officiel
- [ ] Instagram

### 19.4 — Influenceurs

- [ ] Liste cible (streamers FPS compétitifs)
- [ ] Early access codes
- [ ] Programme d'affiliation
- [ ] Tournaments invitationnels

### 19.5 — Presse

- [ ] Press kit
- [ ] Liste contacts journalistes / sites
- [ ] Embargos sur les previews
- [ ] Review codes

### 19.6 — Events

- [ ] Gamescom / PAX / E3-successor
- [ ] Showcases indépendants
- [ ] Demos publiques

### 19.7 — Acquisition payante

- [ ] Google Ads / Facebook Ads / TikTok Ads
- [ ] YouTube pre-rolls
- [ ] CAC vs LTV monitoring

### 19.8 — Métriques marketing

- [ ] Wishlist count
- [ ] Trailer view rate
- [ ] Social engagement
- [ ] Discord member growth
- [ ] Press mentions

---

## Phase 20 — Plateformes (rappel & bonus très lointain)

### 20.1 — Plateformes obligatoires (= Phase 0 + maintenue à chaque phase)

> **Toutes les phases ci-dessus doivent rester valides sur ces plateformes.**

- [x] **Windows** via Steam — plateforme principale, compétitif officiel
- [x] **macOS** via Steam — Apple Silicon + Intel, build universel, notarization Apple
- [ ] **Linux** via Steam — build natif (pas Proton) — obligatoire pour Steam Deck
- [ ] **Steam Deck Verified** — controller mapping, lisibilité 800p, perf cible 30+ FPS

### 20.2 — Plateformes bonus (très loin, optionnelles, "au cas où")

> Notées pour la complétude. **Poubelle pour l'instant.** À reconsidérer
> seulement si le PC est un succès commercial avéré et qu'on a une équipe
> capable d'absorber le coût de certification.

- [ ] PlayStation 5 — devkits Sony, certification, équipe console dédiée
- [ ] Xbox Series X/S — ID@Xbox, certification Microsoft
- [ ] Switch / Switch 2 — selon perfs (probablement non pour un FPS compétitif)
- [ ] Mobile (iOS / Android) — pas dans la même catégorie de jeu, à oublier
- [ ] Cloud gaming (GeForce NOW, Xbox Cloud) — gratuit si on coche une case Steam
- [ ] Cross-platform play / progression — nécessite tout ce qui précède

---

## Tableau de bord — Priorités par horizon

> Horizons en **temps partiel** (toi à côté d'autres engagements). Diviser
> par ~2 si tu passes full-time, multiplier par 2 si très partiel.

| Horizon                   | Phases                                                                                           | Mode dominant                            | Objectif livrable                           |
| ------------------------- | ------------------------------------------------------------------------------------------------ | ---------------------------------------- | ------------------------------------------- |
| **0 → 6 mois**            | 1 (réseau 1v1 LAN)                                                                               | 🟢 ensemble                              | Deux fenêtres se voient et se tirent dessus |
| **6 → 12 mois**           | 2 (5v5 LAN), 3 (blockouts), 6 (UI fonctionnelle), 13 (bots tuto)                                 | 🟢 ensemble                              | Match 5v5 vs bots jouable end-to-end        |
| **12 → 18 mois**          | 5 (audio), assets achetés 4 (graphismes premier passage), CI/CD multi-OS, premier playtest fermé | 🟡 + assets achetés                      | Vidéo gameplay propre à montrer             |
| **18 → 30 mois**          | 7 (backend), 8 (infra Steam), 11 (social via Steam), 15 (QA)                                     | 🟢 ensemble                              | Closed beta Discord                         |
| **24 → 48 mois**          | 9 (anti-cheat kernel) en parallèle de tout le reste                                              | 🔴 devs sécu à recruter                  | AC v1 utilisable sur Win                    |
| **30 → 42 mois**          | 10 (économie Steam), 14 (spectator broadcast), polish Phase 4 avec freelances                    | 🟠 freelances + 🟢 dev                   | Open beta Steam (Phase 16 setup)            |
| **36 → 48 mois**          | 16 (publication Steam), 18 (légal/entité), 19 (marketing partiel)                                | 🔴 avocat + comptable + marketing payant | **Launch Steam Win+Mac+Linux**              |
| **48+ mois**              | 12 (mods), 17 (live ops continu), 14 (tournois e-sport), continuation 9 (AC)                     | 🔴 équipe complète                       | Croissance, e-sport, saisons                |
| **Très lointain / bonus** | 20.2 (consoles)                                                                                  | ⚫ équipe console                        | Si succès PC avéré                          |

**Critères clés pour avancer d'un palier :**

- Pour passer à la closed beta : le jeu doit tourner stable 30 min sans
  crash, sur les 3 OS, avec 10 joueurs en LAN.
- Pour passer à l'open beta : compte Steam fonctionnel, matchmaking
  basique, anti-cheat v1 actif sur Windows.
- Pour passer au launch Steam : entité légale créée, comptable engagé,
  CGU/EULA validés par l'avocat, capacité serveur testée à 1000 joueurs
  simultanés.

---

## Estimations d'équipe par horizon

> Évolution réaliste pour TrueShot avec la stratégie "toi + moi puis
> recrutements progressifs".

### Horizon 0-12 mois (solo + IA)

| Rôle                                  | Qui                | Coût/mois                  |
| ------------------------------------- | ------------------ | -------------------------- |
| Direction + dev gameplay + dev réseau | **Toi**            | 0 € (ou ton salaire perdu) |
| Pair-programmer / mentor technique    | **Moi**            | 0 €                        |
| Comptable (déclaratif minimal)        | freelance ponctuel | 100-300 €                  |
| **Total opérationnel**                |                    | **~200 €/mois**            |

### Horizon 12-24 mois (premiers freelances)

| Rôle                                                          | Qui                  | Coût/mois              |
| ------------------------------------------------------------- | -------------------- | ---------------------- |
| Toi + moi                                                     | inchangé             | 0 €                    |
| 1 artiste 3D freelance (weapons, characters, premier passage) | mission 3-6 mois     | 3-8 k€/mois en mission |
| 1 sound designer freelance (intégration sound libs)           | mission 1-2 mois     | 3-6 k€/mois en mission |
| Comptable + avocat (création entité)                          | one-shot + récurrent | 300-1000 €/mois        |
| Hébergement infra (test)                                      | OVH/Hetzner          | 50-200 €/mois          |
| **Budget assets** (modèles, sons achetés)                     | one-shot             | 5-15 k€ total          |

### Horizon 24-48 mois (équipe core en place)

| Rôle                                                  | Headcount cible | Coût annuel total brut  |
| ----------------------------------------------------- | --------------- | ----------------------- |
| Dev gameplay/engine (toi + 1-2 devs)                  | 2-3             | 100-200 k€              |
| **Dev anti-cheat kernel (spécialistes Windows sécu)** | **2-3**         | **150-300 k€**          |
| Dev backend / DevOps                                  | 1-2             | 60-150 k€               |
| Artiste 3D (interne)                                  | 1-2             | 60-120 k€               |
| Level designer                                        | 1               | 40-70 k€                |
| Sound designer                                        | 1               | 40-70 k€                |
| UI/UX designer                                        | 1               | 40-70 k€                |
| Game designer (économie, balance)                     | 1               | 40-70 k€                |
| QA tester                                             | 1-2             | 50-100 k€               |
| Community manager                                     | 1               | 35-60 k€                |
| Marketing (interne ou agence)                         | 0-1             | 0-80 k€                 |
| Comptable / avocat (récurrent)                        | external        | 15-30 k€                |
| Infra Steam launch (montée en charge)                 | external        | 30-200 k€               |
| **Total équipe core post-launch**                     | **~12-18**      | **~700 k€ - 1,5 M€/an** |

### Horizon 48+ mois (e-sport scale-up)

À ce stade, équipe de **25-40 personnes** dans la fourchette industrie
standard que je donnais initialement (4-6 dev gameplay, etc.).
Budget annuel **2-5 M€**, intégralement financé par les revenus si le
jeu marche, ou par une levée de fonds Série A si le jeu est prometteur
mais pas encore rentable.

### Budget global de référence

- **Bootstrap solo (toi + moi) jusqu'au 1er playable :** ~30-60 k€ (assets,
  comptable, hébergement)
- **Pre-launch + lancement Steam :** 500 k€ - 2 M€ (équipe core 1-2 ans,
  marketing et infra inclus)
- **Post-launch année 1-2 :** 1-3 M€/an
- **Total cumul jusqu'au break-even :** 3-8 M€ dans un scénario
  raisonnable, 8-20 M€ dans un scénario ambitieux esport

> Ces chiffres restent inférieurs aux **30-80 M$** des FPS majeurs, mais
> bien au-dessus des **50-200 k$** d'un jeu indé pur. TrueShot est dans
> la catégorie **"indé ambitieux"** qui nécessite soit un publisher, soit
> une levée, soit beaucoup de patience en solo.

---

## Choses à ne pas oublier (que tu as toi-même oublié de mentionner)

> Compilation de points que j'ai vus en relisant la liste et qui sont
> critiques mais souvent zappés.

### Data, vie privée, RGPD

- [ ] **DPO** (Data Protection Officer) ou délégué externe — obligatoire
      au-dessus d'un certain seuil de traitements personnels (RGPD).
- [ ] Cookie banner sur le site web (RGPD + ePrivacy)
- [ ] Politique de rétention des replays / chat logs / voice logs
- [ ] Export GDPR ("download my data") en self-service
- [ ] Suppression de compte en self-service (right to be forgotten)
- [ ] Registre des traitements (RGPD Art. 30)

### Sécurité des comptes et des paiements

- [ ] 2FA obligatoire pour comptes premium / inventaire à valeur > X €
- [ ] Hold sur trades pour comptes neufs (anti-vol par compromission)
- [ ] Détection login suspect (nouveau pays, IP datacenter)
- [ ] Email confirmation pour changements sensibles
- [ ] Coffre-fort secrets backend (HashiCorp Vault ou KMS cloud)

### Outils de support / debug joueur

- [ ] Système de tickets intégré au jeu (touche F12)
- [ ] Upload automatique du crashdump avec consentement
- [ ] Lien direct vers un "match info" partageable pour debug
- [ ] Outil interne admin pour kick/ban/inspect un joueur
- [ ] Audit log de toutes les actions admin

### Infra développement

- [ ] Repo Git privé (GitHub/GitLab) — version actuelle est privée ?
      Vérifier
- [ ] Branches naming convention + PR template
- [ ] Code review obligatoire (au moins une autre personne) dès qu'on
      est 2+ devs
- [ ] Documentation interne (Notion, Confluence, ou markdown dans repo)
- [ ] Issue tracker (GitHub Issues / Linear / Jira)
- [ ] Backups quotidiens DB + assets

### Onboarding nouveaux contributeurs (quand tu recrutes)

- [ ] Guide d'onboarding dev (build local en < 30 min)
- [ ] CONTRIBUTING.md ✅ déjà fait
- [ ] Style guide C++ (déjà partiellement dans CONTRIBUTING)
- [ ] Roles & responsabilités matrix (RACI)
- [ ] NDA + contrat freelance/employé prêts à signer

### Décisions produit formalisées dans des ADR

> ADR = Architecture Decision Record. L'index canonique (statut, phase,
> résumé) est dans [docs/README.md](docs/README.md) ; le template est
> [docs/adr/0000-adr-template.md](docs/adr/0000-adr-template.md). Cette
> section-ci sert uniquement de **suivi roadmap** : ce qui est livré, et
> ce qui reste à formaliser quand on tranchera la décision.

**Déjà écrits (Phase 0 / Phase 1) :**

- [x] [ADR-001 — Render API (GL 3.3 Core, interim)](docs/adr/0001-render-api.md)
- [x] [ADR-002 — Netcode architecture (128 Hz, ENet, server-auth)](docs/adr/0002-netcode-architecture.md)
- [x] [ADR-003 — Listen-server & input clamping](docs/adr/0003-listen-server-and-input-clamping.md)
- [x] [ADR-004 — Snapshot interpolation (100 ms delay, freeze on starvation)](docs/adr/0004-snapshot-interpolation.md)
- [x] [ADR-005 — Shared `NetSim` + client prediction](docs/adr/0005-client-prediction-and-shared-netsim.md)
- [x] [ADR-006 — Lag compensation (200 ms cap, AABB rewind)](docs/adr/0006-lag-compensation.md)
- [x] [ADR-007 — Source layout (sous-dossiers par subsystem, `snake_case`)](docs/adr/0007-source-layout.md)

**À écrire quand on tranche** (numérotation séquentielle, pas de
recyclage avec les numéros ci-dessus) :

- [ ] ADR-008 — Choix abstraction rendu pour Phase 4 (GL 4.1 vs Vulkan+MoltenVK vs bgfx vs Sokol)
- [ ] ADR-009 — Langage de scripting mod (Lua / Wren / WASM)
- [ ] ADR-010 — Stack backend (PostgreSQL + Redis, ou autre)
- [ ] ADR-011 — Authentification (Steam OpenID, ou compte propriétaire ?)
- [ ] ADR-012 — Approche anti-cheat kernel (driver Windows propriétaire — détails Phase 9)
- [ ] ADR-013 — Format de map et pipeline d'import (Phase 3)
- [ ] ADR-014 — Système d'animation (skeletal vs morph + state machine, Phase 4)
- [ ] ADR-015 — Voice chat encoding + transport (Opus 32 kbps sur ENet ? canal séparé ?)
- [ ] ADR-016 — Économie et anti-fraude marketplace (Phase 10)

### Risques majeurs identifiés

| Risque                                                     | Probabilité | Impact   | Mitigation                                            |
| ---------------------------------------------------------- | ----------- | -------- | ----------------------------------------------------- |
| Toi épuisé / burn-out en solo                              | Haute       | Critique | Cadence durable, pauses, accepter que ça prenne 4 ans |
| Anti-cheat pas prêt au launch                              | Haute       | Critique | Démarrer recrutement spécialistes sécu dès Phase 7    |
| Concurrent (CS2, Valorant) sort une feature qui copie      | Moyenne     | Moyen    | Différencier sur l'AC maison + community-first        |
| Levée de fonds ratée si nécessaire                         | Moyenne     | Critique | Plan B = continuer en bootstrap plus longtemps        |
| Bug majeur en production                                   | Haute       | Élevé    | QA + canary releases + rollback rapide                |
| Procès marque (un autre jeu s'appelle TrueShot)            | Faible      | Élevé    | Vérifier INPI/EUIPO/USPTO **maintenant**              |
| Joueur mineur dépense $$$ sans accord parental             | Moyenne     | Moyen    | Spending limits + parental controls Steam             |
| Data breach                                                | Faible      | Critique | Architecture zero-trust, audits sécurité réguliers    |
| Anti-cheat false positive (bannit un innocent influenceur) | Moyenne     | Élevé    | Process d'appel, communication transparente           |

---

## Notes finales

Cette roadmap est **vivante**. Quand une tâche est terminée :

1. Coche-la (`- [x]`).
2. Si elle révèle des sous-tâches imprévues, ajoute-les en dessous.
3. Met à jour le tableau de bord d'horizon si la priorisation change.

Une refonte trimestrielle de ce document est conseillée pour ne pas se laisser
dépasser par les pivots produit.
