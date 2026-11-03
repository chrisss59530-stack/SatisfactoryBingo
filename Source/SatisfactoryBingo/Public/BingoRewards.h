#pragma once
#include "CoreMinimal.h"

struct FBingoRewardDefinition
{
    FString Id;
    FString Name;
    int32 Cost;
    int32 RequiredPhase;
};

class FBingoRewards
{
public:
    static TArray<FBingoRewardDefinition> GetAll()
    {
        return {
            {TEXT("IronPlates"), TEXT("Kit de plaques FICSIT"), 5, 1},
            {TEXT("Wires"), TEXT("Reserve de cables et fils"), 7, 1},
            {TEXT("Concrete"), TEXT("Kit de beton"), 9, 1},
            {TEXT("Biomass"), TEXT("Ravitaillement biomasse"), 10, 1},
            {TEXT("ConstructionKit"), TEXT("Kit de construction"), 12, 1},
            {TEXT("Rotors"), TEXT("Stock de rotors"), 15, 1},
            {TEXT("ModularFrames"), TEXT("Reserve de cadres modulaires"), 20, 1},
            {TEXT("EmergencySupply"), TEXT("Ravitaillement d'urgence FICSIT"), 25, 1},
            {TEXT("SteelBeams"), TEXT("Stock de poutres d'acier"), 15, 2},
            {TEXT("SteelPipes"), TEXT("Stock de tuyaux d'acier"), 18, 2},
            {TEXT("AdvancedRotors"), TEXT("Reserve de rotors renforcee"), 20, 2},
            {TEXT("Stators"), TEXT("Kit de stators"), 23, 2},
            {TEXT("Motors"), TEXT("Reserve de moteurs"), 28, 2},
            {TEXT("ExplorationRadar"), TEXT("Radar d'exploration temporaire"), 35, 2},
            {TEXT("BackupBattery"), TEXT("Cadeau de bienvenue FICSIT"), 45, 2},
            {TEXT("IndustrialInspection"), TEXT("Le stagiaire a encore frappe"), 45, 2},
            {TEXT("Plastic"), TEXT("Reserve de plastique"), 30, 3},
            {TEXT("Rubber"), TEXT("Reserve de caoutchouc"), 32, 3},
            {TEXT("CircuitBoards"), TEXT("Kit de circuits imprimes"), 38, 3},
            {TEXT("HeavyFrames"), TEXT("Stock de cadres modulaires lourds"), 45, 3},
            {TEXT("Computers"), TEXT("Reserve d'ordinateurs"), 55, 3},
            {TEXT("HighSpeedConnectors"), TEXT("Pack de connecteurs haute vitesse"), 60, 3},
            {TEXT("ExperimentalThruster"), TEXT("Houston, on a un probleme"), 75, 3},
            {TEXT("ConstructionAssistant"), TEXT("Construisez, on vous regarde"), 85, 3},
            {TEXT("Aluminum"), TEXT("Reserve d'aluminium"), 45, 4},
            {TEXT("AluminumSheets"), TEXT("Stock de feuilles d'aluminium"), 48, 4},
            {TEXT("AIControl"), TEXT("Kit de systemes de controle IA"), 55, 4},
            {TEXT("Supercomputers"), TEXT("Reserve de superordinateurs"), 65, 4},
            {TEXT("TurboMotors"), TEXT("Stock de moteurs turbo"), 75, 4},
            {TEXT("CoolingSystems"), TEXT("Kit de refroidissement industriel"), 80, 4},
            {TEXT("Batteries"), TEXT("Reserve de batteries"), 85, 4},
            {TEXT("FicsitInsurance"), TEXT("Assurance FICSIT (non remboursable)"), 100, 4},
            {TEXT("EliteConnectors"), TEXT("Reserve de connecteurs haute vitesse"), 60, 5},
            {TEXT("AdvancedControls"), TEXT("Kit de systemes de controle avances"), 70, 5},
            {TEXT("FusedFrames"), TEXT("Stock de cadres modulaires fusionnes"), 80, 5},
            {TEXT("EliteTurboMotors"), TEXT("Reserve de moteurs turbo"), 90, 5},
            {TEXT("EliteSupercomputers"), TEXT("Pack de superordinateurs"), 100, 5},
            {TEXT("PremiumSupply"), TEXT("Ravitaillement industriel premium"), 110, 5},
            {TEXT("FicsitVIP"), TEXT("Employe du mois... a vie"), 120, 5},
            {TEXT("PioneerUltimate"), TEXT("Coffre ultime du pionnier"), 150, 5},
        };
    }

    static bool Find(
        const FString& RewardId,
        FBingoRewardDefinition& OutReward)
    {
        for (const FBingoRewardDefinition& Reward : GetAll())
        {
            if (Reward.Id == RewardId)
            {
                OutReward = Reward;
                return true;
            }
        }
        return false;
    }
};
