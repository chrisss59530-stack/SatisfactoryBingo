# Satisfactory Bingo — bêta 0.8.4

- Progression personnelle, terminal natif et récompenses.
- Nouvelles validations conditionnées par un terminal posé et alimenté.
- Correction des parcours de convoyeurs et des fonderies intermédiaires.
- Prise en charge des répartiteurs et groupeurs successifs.
- Exclusion du concepteur de plans dans les contrôles concernés.
- Chaîne complète : stockage Mk1/Mk2, hors coffre du HUB.
- Objectifs d'inventaire personnel en remplacement de dix objectifs de production.
- Retrait de la dépendance éditeur Figma2UMG.

## État des tests

Compilation et packaging Windows réussis sur le poste du mainteneur.
Les validations et le multijoueur restent à tester en jeu.

Le crash Linux/Proton dans
UFGReplicationGraph::InitGlobalActorClassSettings reste non résolu.
SML seul charge la partie ; Bingo reproduit le crash même avec
-BingoSkipNativeHooks.

## Sources et assets

Cette mise à jour synchronise Source, Config, Resources et le descripteur.
Les assets Content déjà présents sur GitHub sont conservés ; les derniers
assets du poste local ne sont pas inclus dans cette synchronisation.
Le SDK et Unreal Engine ne sont pas inclus.