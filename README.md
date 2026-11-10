## v0.7 — corrective build
- Link NAM parsers with WHOLE_ARCHIVE, with a CTest registry smoke test.
- Use explicit UTF-8-to-JUCE conversions for ALL current accented UI labels and future strings (enforced by preflight).
- Two model-dependent Liberty amplifier faces: NAM A and NAM B can display distinct artwork.
- Existing microphone and IR DSP stays unchanged; visual amplifier heads do not change audio parameters.

# Liberty Neural Matrix - Guitar Edition v0.5

Liberty Music Software - AU/VST3/Standalone plugin, developed with JUCE 8 and NeuralAmpModelerCore.

## Key additions

- Two guitar-themed amplifier/cabinet images embedded directly in the plugin (no network required).
- Reworked section geometry at 1370x930, 1560x980 and 1860x1200, with strict no-overlap test of 515 component bounds.
- French UI text validated as UTF-8, with an automated mojibake/replacement-character preflight check.
- Actual WAV IR waveform previews from decoded file samples, not decorative synthesized curves.
- Two independent microphone profile selectors: neutral, dynamic 57-style, dynamic 421-style, condenser 87-style, tube 47-style, ribbon 121-style.
- Two interactive XY microphone positioning canvases. Dragging each control adjusts off-axis percentage and distance in centimeters, with DAW-automatable parameters.
- Position and model type drive real DSP filters **after the loaded IR**. This is a tonal approximation, not a physically accurate re-microphoning or endorsed microphone-model emulation.
- Existing NAM A/B, IR A/B, gain/phase, global HP/LP, post-NAM EQ, gate, boost and delay remain supported.

## What simulated microphone positioning means

A captured IR is a recording of a particular microphone and physical position. Moving a virtual microphone cannot reconstruct data that were never captured. Liberty v0.5 therefore uses frequency-colour filtering and proximity approximation, clearly marked as **simulation post-IR**. Authentic multi-mic positioning will require a measured grid of same-cabinet IRs at different placements, with phase-aligned interpolation. Manufacturer and microphone family names are descriptive inspirations; no official partnership or exact microphone emulation is claimed.

## Technical and commercial status

- NAM A1/A2 playback must still be validated with a test suite of actual models; the pinned NeuralAmpModelerCore library uses its A2 optimized path.
- The generated beta is not signed/notarized. GitHub Actions produces preserve-permissions macOS ZIP bundles.
- No TONE3000 OAuth/API, auto phase alignment, true multi-position IR interpolation, multi-band EQ or preset browser yet.
- NAM and IR loading still happen from the editor; a worker-thread loading system remains a future requirement.
- JUCE commercial/AGPL licensing and NAM Core MIT notices must be respected before distribution.
- Only release a build as tested once both GitHub CI jobs pass and runtime audio is manually validated.

## Reproduce checks and build

```bash
python3 scripts/preflight.py
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel 3
```

## UI testing

1. Change between AMPLI, BAFFLES / IR, EFFETS and RÉGLAGES at minimum and maximum window size.
2. Verify all accent-bearing labels (É, È, ê, à, ç) are correct with no corrupted symbols.
3. Load a real A1/A2 NAM and two known-good WAV IRs; verify both waveforms reflect the chosen recordings.
4. Change mic A and B families and drag each position. Listen to the actual tonal change; verify preset-state reload.
5. Confirm all knobs, popups and files remain readable and non-overlapping in both AU and VST3 hosts.


## v0.7 - Galerie visuelle des amplificateurs

- **18 familles visuelles** plus un mode neutre, avec des facades originales Liberty
  (Plexi, JCM, 5150, Rectifier, blackface, tweed, AC, SLO, BE, Orange,
  Hiwatt, ODS, SVT, Diezel, ENGL, Jazz Chorus, Trainwreck, Bogner).
- Reconnaissance **non acoustique**, a partir des noms de fichiers, puis du dossier
  et de chaines descriptives presentes dans les champs NAM. Elle ne prouve jamais
  l'origine materielle d'une capture. En cas d'indices contradictoires, le plugin
  choisit volontairement **Inconnu** au lieu d'inventer une identification.
- **NAM A et NAM B** conservent chacun leur image et leur choix manuel.
- Dans la page AMPLI, le menu **AUTO** ou la famille choisie manuellement change
  la facade. Le bouton **PHOTO...** permet de lier une photographie JPG/PNG
  locale (maximum 12 Mo) a votre fichier NAM ; **EFFACER** retire cette image.
- Les associations locales sont conservees dans le dossier utilisateur :
  `Liberty Music Software/NeuralMatrixVisuals.json`, avec cle de chemin local.
  Les profils manuels sont egalement sauvegardes dans l'etat du projet DAW.
- Un lien vers une photo locale ne la copie pas dans le projet : si le fichier
  est deplace ou partage sur une autre machine, l'illustration vectorielle
  prend le relais. Aucun fichier NAM ni photo n'est televerse.
- La fonctionnalite est **purement visuelle** : aucun DSP, sample rate, gain,
  poids NAM, format de fichier ou convolution n'est altere.
- Les dessins de facade sont originaux et ne constituent pas des logos,
  des representations officielles ou des partenariats avec les fabricants.

### Deux regles Liberty non negociables

1. Tous les textes accentues doivent passer par `libertyText::u8` pour les
   conversions UTF-8 JUCE. Le preflight rejette les chaines non converties.
2. Pas de superposition. `scripts/check_geometry.py` audite les commandes
   visibles dans 4 pages et 5 tailles, y compris les nouveaux selecteurs de NAM.
   La validation visuelle dans les builds reste indispensable.

### Tests

`python3 scripts/preflight.py`
`c++ -std=c++20 -ISource tests/amp_visual_detect_test.cpp -o /tmp/amp-test && /tmp/amp-test`
`cmake --build build --config Release && ctest --test-dir build -C Release --output-on-failure`


## v0.8 — Catalogue et bibliothèque (lot unique)

- Reconnaissance visuelle conservatrice : 18 familles + origine inconnue, indice de confiance, correction manuelle et photo personnelle (non officielles).
- Façades Liberty redessinées avec relief, textures et boutons métalliques. Elles **évoquent** une famille d'amplis ; il ne s'agit pas de reproductions de marques ni d'une preuve issue du son NAM.
- Baffles visuels séparés A/B avec configurations estimées 1×12, 2×12 ou 4×12 à partir du nom de l'IR. Le micro est représenté dans les canvases de déplacement ; les véritables positions enregistrées ne sont pas reconstituées.
- Cinquième onglet **BIBLIOTHÈQUE** : dossier local choisi volontairement, recherche, filtres NAM/IR/favoris, miniatures d'ampli, aperçu réel de l'onde IR, chargement vers A ou B. Favoris et dossier enregistré dans un petit JSON utilisateur.
- Protection de l'audio lors du changement de session DAW : anciens NAM et IR neutralisés avant rechargement, état IR atomique, type de snapshot validé avant restauration.
- Tests : 28 scénarios de reconnaissance, fichiers NAM officiels WaveNet/LSTM/A2, roundtrip du codec JUCE des sessions, 665 contrôles de placement et lint Unicode.

### Vérification dans un DAW réel (macOS AU/VST3 et Windows VST3)

1. Charger un NAM A1, un NAM A2, deux WAV IR et enregistrer le projet.
2. Régler le mix NAM A/B, les filtres et les deux positions de micro ; attribuer un profil visuel manuel et une photo.
3. Fermer le DAW, le rouvrir, vérifier les paramètres, les fichiers et le rendu audio.
4. Tester un projet vierge après un projet chargé : aucun ancien NAM/IR ne doit persister.
5. Renommer temporairement un modèle NAM et rouvrir : le plugin ne doit ni planter ni réutiliser silencieusement le modèle précédent.
6. Inspecter tous les accents, menus et fenêtres à 1370 × 930 et 1560 × 980, plus un petit écran externe.

Les tests CI ne remplacent pas ces manipulations réelles ; ils n'exécutent pas automatiquement les hôtes Ableton, Logic ou Reaper.
