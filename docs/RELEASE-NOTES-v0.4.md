# Liberty Neural Matrix v0.4 — Guitar Edition

Four functional pages with industrial amp/cab styling. Parameters added:
bass, middle, treble, presence, ampOn, cabOn, irAGain, irBGain, irAPhase, irBPhase, cabHP, cabLP, gateOn, gateThreshold, boostOn, delayOn, delayMs, delayFeedback, delayMix.

Audio order: input > gate/boost > NAM A/B > tone EQ > IR A/B > cab filters > delay > output.

Known limits: no TONE3000 API yet; no auto IR alignment; no true NAM morphing; A2 models need playback testing; sample rate mismatch rejected; UI-thread model loading may momentarily block.
