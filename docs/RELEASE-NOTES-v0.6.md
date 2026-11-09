# v0.6 — NAM, français Unicode, amplis A/B
- Correctif WaveNet/LSTM/ConvNet/Linear : WHOLE_ARCHIVE au lien final pour inclure les parseurs.
- Test CTest automatique des enregistrements d'architectures, macOS et Windows.
- Conversion de toutes les chaînes françaises accentuées par juce::String::fromUTF8.
- Garde-fou de précompilation pour les futurs textes non ASCII.
- Habillage de deux têtes d'ampli distinctes, associé à NAM A et NAM B selon les noms des fichiers chargés.
- Illustration photo de la guitare préservée en arrière-plan.
- Les habillages sont artistiques et ne revendiquent aucune modélisation officielle de matériel.
- Vérification de collision géométrique des composants conservée.
- À tester : chargement de vrais NAM A1/A2 et lecture audio dans la version compilée.
