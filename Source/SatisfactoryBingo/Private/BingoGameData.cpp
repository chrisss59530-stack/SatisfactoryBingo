#include "BingoGameData.h"
#include "BingoRewardItems.h"
#include "BingoRewards.h"

#include "FGCharacterPlayer.h"
#include "FGInventoryComponent.h"
#include "Resources/FGItemDescriptor.h"

void UBingoGameData::InitializeGame()
{
    Phases.Empty();
    CurrentPhase = 1;
    TotalScore = 0;

    static const TCHAR* PhaseDescriptions[5][25] = {
        {
            TEXT("Avoir une production de plus de 100MW d'énergie"),
            TEXT("Avoir une production de plus de 200MW d'énergie"),
            TEXT("Construire au moins une tour d'observation"),
            TEXT("Construire l'Ascenseur spatial"),
            TEXT("Relier au moins 2 constructeurs à une même foreuse"),
            TEXT("Construire une station de recherche MAM"),
            TEXT("Construire un broyeur AWESOME"),
            TEXT("Construire un brûleur de biomasse"),
            TEXT("Construire au moins un conteneur de stockage"),
            TEXT("Construire un groupeur pour 3 machines"),
            TEXT("Construire un répartiteur pour 3 machines"),
            TEXT("Récupérer 1 disque dur sur un site de crash"),
            TEXT("CASE LIBRE CENTRALE"),
            TEXT("Fabriquer manuellement un équipement dans l'atelier d'équipement"),
            TEXT("Relier au moins 2 fonderies à une même foreuse"),
            TEXT("Construire une boutique"),
            TEXT("Faire fonctionner une chaîne connectée : foreuse, fonderie, constructeur et stockage"),
            TEXT("Fabriquer manuellement une foreuse portable"),
            TEXT("Fabriquer manuellement une tronçonneuse"),
            TEXT("Poser 10 fondations de n'importe quel type"),
            TEXT("Produire 500 unités de biomasse à partir de bois dans un constructeur"),
            TEXT("Exploiter 4 gisements distincts avec des foreuses en production"),
            TEXT("Exploiter 6 gisements distincts avec des foreuses en production"),
            TEXT("Explorer 2 zones distinctes de la carte"),
            TEXT("Explorer la Côte des Flèches (Spire Coast)")
        },
        {
            TEXT("Acheter un nouveau type de fondation"),
            TEXT("Acheter un nouveau type de murs"),
            TEXT("Acheter votre tasse à café"),
            TEXT("Construire 20 fondations de tout type"),
            TEXT("Avoir une production de plus de 1000MW d'énergie"),
            TEXT("Avoir une production de plus de 2000MW d'énergie"),
            TEXT("Construire 2 boutiques AWESOME"),
            TEXT("Construire un 2e générateur à charbon"),
            TEXT("Construire 2 conteneurs de stockage Mk1"),
            TEXT("Construire un 3e générateur à charbon"),
            TEXT("Construire 4 conteneurs de stockage Mk1"),
            TEXT("Construire un générateur à charbon"),
            TEXT("CASE LIBRE CENTRALE"),
            TEXT("Construire un modeleur et sauvegarder votre 1er plan"),
            TEXT("Construire 20 murs"),
            TEXT("Construire un pipeline"),
            TEXT("Construire 10 tronçons de convoyeur"),
            TEXT("Construire 20 tronçons de pipeline"),
            TEXT("Construire une 2e fonderie avancée"),
            TEXT("Construire une assembleuse"),
            TEXT("Construire une fonderie avancée"),
            TEXT("Construire une foreuse mk2"),
            TEXT("Construire une plateforme de saut"),
            TEXT("Construire une pompe de pipeline"),
            TEXT("Construire 4 pompes de pipeline")
        },
        {
            TEXT("Atterrir en douceur avec le jetpack ou un parachute"),
            TEXT("Automatiser une chaîne de filactif simple"),
            TEXT("Récupérer 10 disques durs dans les épaves"),
            TEXT("Avoir dépensé 10 bons dans la boutique"),
            TEXT("Avoir dépensé 15 bons dans la boutique"),
            TEXT("Avoir une production de plus de 3000MW d'énergie"),
            TEXT("Avoir une production de plus de 4000MW d'énergie"),
            TEXT("Construire un générateur à carburant"),
            TEXT("Construire 50 fondations de tout type"),
            TEXT("Construire 10 tronçons de voie ferrée"),
            TEXT("Construire un répartiteur intelligent"),
            TEXT("Avoir au moins 2 réservoirs de stockage de fluides construits"),
            TEXT("CASE LIBRE CENTRALE"),
            TEXT("Construire un train à 2 wagons"),
            TEXT("Construire une 2e façonneuse"),
            TEXT("Construire une 2e gare de train"),
            TEXT("Construire une 2e raffinerie"),
            TEXT("Construire une façonneuse"),
            TEXT("Construire une gare de train"),
            TEXT("Construire une raffinerie"),
            TEXT("Construire une station de camions"),
            TEXT("Construire 2 stations de camions"),
            TEXT("Equiper une arme à distance et des munitions"),
            TEXT("Atteindre un débit réel d'extraction de 300 m³ de pétrole brut par minute"),
            TEXT("Posséder 20 moteurs dans son inventaire")
        },
        {
            TEXT("Améliorer un convoyeur Mk3 en Mk5"),
            TEXT("Terminer l'analyse de 5 disques durs dans le MAM"),
            TEXT("Terminer l'analyse de 6 disques durs dans le MAM"),
            TEXT("Atterrir en douceur avec l'hoverpack"),
            TEXT("Récupérer 20 disques durs dans les épaves"),
            TEXT("Avoir dépensé 20 bons dans la boutique"),
            TEXT("Avoir dépensé 25 bons dans la boutique"),
            TEXT("Construire 2 conteneurs de stockage industriels Mk2"),
            TEXT("Avoir une production de plus de 5000MW d'énergie"),
            TEXT("Avoir une production de plus de 6000MW d'énergie"),
            TEXT("Avoir au moins 2 accélérateurs de particules construits"),
            TEXT("Avoir au moins 2 mélangeurs construits"),
            TEXT("CASE LIBRE CENTRALE"),
            TEXT("Construire un accélérateur de particules"),
            TEXT("Faire fonctionner au moins un mélangeur en production"),
            TEXT("Construire un mélangeur"),
            TEXT("Construire un port de drone"),
            TEXT("Faire fonctionner une raffinerie produisant une solution d'alumine à partir de bauxite"),
            TEXT("Avoir une foreuse active avec une cadence d'extraction d'au moins 600 minerais/min"),
            TEXT("Posséder 200 minerais de bauxite dans son inventaire"),
            TEXT("Posséder 400 minerais de bauxite dans son inventaire"),
            TEXT("Posséder 10 oscillateurs à quartz dans son inventaire"),
            TEXT("Posséder 10 turbomoteurs dans son inventaire"),
            TEXT("Fabriquer un 2e hoverpack de secours"),
            TEXT("Fabriquer votre hoverpack")
        },
        {
            TEXT("Posséder 500 diamants dans son inventaire"),
            TEXT("Posséder 500 cubes de conversion dans son inventaire"),
            TEXT("Automatiser la chaine de cristal temporel"),
            TEXT("Automatiser la chaine de diamants"),
            TEXT("Automatiser la chaine de lingots de Ficsite"),
            TEXT("Automatiser la chaîne de trigones de Ficsite"),
            TEXT("Automatiser la chaîne de pâtes nucléaires"),
            TEXT("Automatiser une chaîne simple de diamants"),
            TEXT("Récupérer 30 disques durs dans les épaves"),
            TEXT("Avoir dépensé 30 bons dans la boutique"),
            TEXT("Avoir dépensé 35 bons dans la boutique"),
            TEXT("Avoir une production de plus de 12.000MW d'énergie"),
            TEXT("CASE LIBRE CENTRALE"),
            TEXT("Avoir une production de plus de 15.000MW d'énergie"),
            TEXT("Avoir au moins 2 convertisseurs construits"),
            TEXT("Avoir au moins 2 encodeurs quantiques construits"),
            TEXT("Construire un convertisseur"),
            TEXT("Construire un encodeur quantique"),
            TEXT("Faire fonctionner 2 convertisseurs produisant 2 minerais différents"),
            TEXT("Faire fonctionner un convertisseur produisant un minerai"),
            TEXT("Avoir une foreuse active avec une cadence d'extraction d'au moins 780 minerais/min"),
            TEXT("Récupérer 10 sphères de Mercer dans le monde"),
            TEXT("Posséder 1 diamant dans son inventaire"),
            TEXT("Posséder 1 lingot de Ficsite dans son inventaire"),
            TEXT("Posséder 1 serveur d'expansion d'I.A. dans son inventaire")
        }
    };

    for (int32 PhaseIndex = 1; PhaseIndex <= 5; ++PhaseIndex)
    {
        FBingoPhase Phase;
        Phase.PhaseNumber = PhaseIndex;
        Phase.bUnlocked = (PhaseIndex == 1);
        Phase.CompletedLines.Init(false, 12);

        for (int32 CellIndex = 0; CellIndex < 25; ++CellIndex)
        {
            FBingoChallenge Challenge;

            Challenge.ChallengeId = FString::Printf(
                TEXT("P%d_C%d"), PhaseIndex, CellIndex + 1);

            Challenge.Description = FText::FromString(PhaseDescriptions[PhaseIndex - 1][CellIndex]);
            Challenge.Points = (CellIndex == 12) ? 0 : 10;
            Challenge.bCompleted = (CellIndex == 12);
            Phase.Challenges.Add(Challenge);
        }

        Phases.Add(Phase);
    }
}

bool UBingoGameData::CompleteChallenge(const FString& ChallengeId)
{
    for (FBingoPhase& Phase : Phases)
    {
        if (!Phase.bUnlocked)
        {
            continue;
        }

        for (FBingoChallenge& Challenge : Phase.Challenges)
        {
            if (Challenge.ChallengeId != ChallengeId)
            {
                continue;
            }

            if (Challenge.bCompleted)
            {
                return false;
            }

            Challenge.bCompleted = true;
            TotalScore += Challenge.Points;

            EvaluateBingoLines(Phase.PhaseNumber);
            // A first Bingo advances only this personal grid.
            if (Phase.PhaseNumber == CurrentPhase && Phase.BingoCount > 0)
            {
                UnlockNextPhase();
            }
            return true;
        }
    }

    return false;
}

void UBingoGameData::EvaluateBingoLines(int32 PhaseNumber)
{
    for (FBingoPhase& Phase : Phases)
    {
        if (Phase.PhaseNumber != PhaseNumber ||
            Phase.Challenges.Num() != 25)
        {
            continue;
        }

        if (Phase.CompletedLines.Num() != 12)
        {
            Phase.CompletedLines.Init(false, 12);
        }

        for (int32 Line = 0; Line < 12; ++Line)
        {
            if (Phase.CompletedLines[Line])
            {
                continue;
            }

            bool bComplete = true;

            for (int32 Offset = 0; Offset < 5; ++Offset)
            {
                int32 Index;

                if (Line < 5)
                {
                    Index = Line * 5 + Offset;
                }
                else if (Line < 10)
                {
                    Index = Offset * 5 + (Line - 5);
                }
                else if (Line == 10)
                {
                    Index = Offset * 6;
                }
                else
                {
                    Index = 4 + Offset * 4;
                }

                if (!Phase.Challenges[Index].bCompleted)
                {
                    bComplete = false;
                    break;
                }
            }

            if (bComplete)
            {
                Phase.CompletedLines[Line] = true;
                ++Phase.BingoCount;

                constexpr int32 Bonus = 3;
                Phase.BingoBonus += Bonus;
                TotalScore += Bonus;

                UE_LOG(LogTemp, Display,
                    TEXT("[Bingo] PHASE %d : BINGO ligne %d ! +%d points"),
                    PhaseNumber, Line, Bonus);
            }
        }

        return;
    }
}

int32 UBingoGameData::GetBingoCount(int32 PhaseNumber) const
{
    for (const FBingoPhase& Phase : Phases)
    {
        if (Phase.PhaseNumber == PhaseNumber)
        {
            return Phase.BingoCount;
        }
    }

    return 0;
}
int32 UBingoGameData::GetCompletedCount(int32 PhaseNumber) const
{
    for (const FBingoPhase& Phase : Phases)
    {
        if (Phase.PhaseNumber != PhaseNumber)
        {
            continue;
        }

        int32 Count = 0;

        for (const FBingoChallenge& Challenge : Phase.Challenges)
        {
            if (Challenge.bCompleted)
            {
                ++Count;
            }
        }

        return Count;
    }

    return 0;
}

int32 UBingoGameData::GetTotalScore() const
{
    return TotalScore;
}

bool UBingoGameData::UnlockNextPhase()
{
    if (CurrentPhase >= 5)
    {
        return false;
    }

    ++CurrentPhase;

    for (FBingoPhase& Phase : Phases)
    {
        if (Phase.PhaseNumber == CurrentPhase)
        {
            Phase.bUnlocked = true;
            return true;
        }
    }

    return false;
}

bool UBingoGameData::PurchaseReward(
    const FString& RewardId,
    AFGCharacterPlayer* Player)
{
    if (!IsValid(Player) || !Player->HasAuthority())
    {
        return false;
    }

    FBingoRewardDefinition Reward;

    if (!FBingoRewards::Find(RewardId, Reward))
    {
        return false;
    }

    if (CurrentPhase < Reward.RequiredPhase ||
        TotalScore < Reward.Cost)
    {
        return false;
    }

    // RADAR BINGO : achat valide par le serveur.
    // L'activation temporaire sera raccordee au subsystem.
    if (RewardId == TEXT("ExplorationRadar"))
    {
        TotalScore -= Reward.Cost;
        return true;
    }
    // RECOMPENSE SPECIALE : EMERGENCY SUPPLY
    if (RewardId == TEXT("EmergencySupply"))
    {
        UFGInventoryComponent* Inventory = Player->GetInventory();

        if (!IsValid(Inventory) || !Inventory->HasAuthority())
        {
            return false;
        }

        struct FSupplyEntry
        {
            const TCHAR* DescriptorPath;
            int32 Quantity;
        };

        const FSupplyEntry Supplies[] =
        {
            { TEXT("/Game/FactoryGame/Resource/Parts/IronPlate/Desc_IronPlate.Desc_IronPlate_C"), 100 },
            { TEXT("/Game/FactoryGame/Resource/Parts/IronRod/Desc_IronRod.Desc_IronRod_C"), 100 },
            { TEXT("/Game/FactoryGame/Resource/Parts/Wire/Desc_Wire.Desc_Wire_C"), 200 },
            { TEXT("/Game/FactoryGame/Resource/Parts/Cement/Desc_Cement.Desc_Cement_C"), 100 }
        };

        TArray<FInventoryStack> Stacks;

        for (const FSupplyEntry& Supply : Supplies)
        {
            UClass* ItemClass = StaticLoadClass(
                UFGItemDescriptor::StaticClass(),
                nullptr,
                Supply.DescriptorPath
            );

            if (!ItemClass)
            {
                UE_LOG(LogTemp, Error,
                    TEXT("[Bingo] EmergencySupply : ressource introuvable : %s"),
                    Supply.DescriptorPath);
                return false;
            }

            Stacks.Add(FInventoryStack(Supply.Quantity, ItemClass));
        }

        if (!Inventory->HasEnoughSpaceForStacks(Stacks))
        {
            UE_LOG(LogTemp, Warning,
                TEXT("[Bingo] EmergencySupply : inventaire insuffisant"));
            return false;
        }

        Inventory->AddStacks(Stacks);
        TotalScore -= Reward.Cost;

        UE_LOG(LogTemp, Display,
            TEXT("[Bingo] EmergencySupply attribue. Solde : %d"),
            TotalScore);

        return true;
    }
    // BINGO_SIX_RESOURCE_PACKS_V1
    // Six packs multi-ressources, sans modifier les recompenses existantes.
    struct FBingoPackEntry
    {
        const TCHAR* RewardId;
        const TCHAR* DescriptorPath;
        int32 Quantity;
    };

    static const FBingoPackEntry PackEntries[] =
    {
        { TEXT("BackupBattery"), TEXT("/Game/FactoryGame/Resource/Parts/SteelPlate/Desc_SteelPlate.Desc_SteelPlate_C"), 100 },
        { TEXT("BackupBattery"), TEXT("/Game/FactoryGame/Resource/Parts/SteelPipe/Desc_SteelPipe.Desc_SteelPipe_C"), 100 },

        { TEXT("IndustrialInspection"), TEXT("/Game/FactoryGame/Resource/Parts/Rotor/Desc_Rotor.Desc_Rotor_C"), 30 },
        { TEXT("IndustrialInspection"), TEXT("/Game/FactoryGame/Resource/Parts/Stator/Desc_Stator.Desc_Stator_C"), 30 },

        { TEXT("ExperimentalThruster"), TEXT("/Game/FactoryGame/Resource/Parts/Motor/Desc_Motor.Desc_Motor_C"), 20 },
        { TEXT("ExperimentalThruster"), TEXT("/Game/FactoryGame/Resource/Parts/CircuitBoard/Desc_CircuitBoard.Desc_CircuitBoard_C"), 30 },

        { TEXT("ConstructionAssistant"), TEXT("/Game/FactoryGame/Resource/Parts/Cement/Desc_Cement.Desc_Cement_C"), 200 },
        { TEXT("ConstructionAssistant"), TEXT("/Game/FactoryGame/Resource/Parts/ModularFrame/Desc_ModularFrame.Desc_ModularFrame_C"), 100 },

        { TEXT("FicsitInsurance"), TEXT("/Game/FactoryGame/Resource/Parts/Computer/Desc_Computer.Desc_Computer_C"), 20 },
        { TEXT("FicsitInsurance"), TEXT("/Game/FactoryGame/Resource/Parts/ModularFrameHeavy/Desc_ModularFrameHeavy.Desc_ModularFrameHeavy_C"), 20 },

        { TEXT("FicsitVIP"), TEXT("/Game/FactoryGame/Resource/Parts/ComputerSuper/Desc_ComputerSuper.Desc_ComputerSuper_C"), 20 },
        { TEXT("FicsitVIP"), TEXT("/Game/FactoryGame/Resource/Parts/MotorLightweight/Desc_MotorLightweight.Desc_MotorLightweight_C"), 20 }
    };

    bool bIsResourcePack = false;
    TArray<FInventoryStack> PackStacks;

    for (const FBingoPackEntry& Entry : PackEntries)
    {
        if (RewardId != Entry.RewardId)
        {
            continue;
        }

        bIsResourcePack = true;

        UClass* ItemClass = StaticLoadClass(
            UFGItemDescriptor::StaticClass(),
            nullptr,
            Entry.DescriptorPath
        );

        if (!ItemClass)
        {
            UE_LOG(LogTemp, Error,
                TEXT("[Bingo] Pack : ressource introuvable : %s"),
                Entry.DescriptorPath);
            return false;
        }

        PackStacks.Add(FInventoryStack(Entry.Quantity, ItemClass));
    }

    if (bIsResourcePack)
    {
        UFGInventoryComponent* Inventory = Player->GetInventory();

        if (!IsValid(Inventory) || !Inventory->HasAuthority())
        {
            return false;
        }

        if (!Inventory->HasEnoughSpaceForStacks(PackStacks))
        {
            UE_LOG(LogTemp, Warning,
                TEXT("[Bingo] Pack : inventaire insuffisant : %s"),
                *RewardId);
            return false;
        }

        Inventory->AddStacks(PackStacks);
        TotalScore -= Reward.Cost;

        UE_LOG(LogTemp, Display,
            TEXT("[Bingo] Pack attribue : %s | Solde : %d"),
            *RewardId, TotalScore);

        return true;
    }
    FBingoRewardItem RewardItem;

    if (!FBingoRewardItems::Find(RewardId, RewardItem))
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[Bingo] Recompense speciale non implementee : %s"),
            *RewardId);
        return false;
    }

    static const TMap<FString, FString> DescriptorPaths = {
        {TEXT("AluminumCasing"), TEXT("/Game/FactoryGame/Resource/Parts/AluminumCasing/Desc_AluminumCasing.Desc_AluminumCasing_C")},
        {TEXT("AluminumPlate"), TEXT("/Game/FactoryGame/Resource/Parts/AluminumPlate/Desc_AluminumPlate.Desc_AluminumPlate_C")},
        {TEXT("Battery"), TEXT("/Game/FactoryGame/Resource/Parts/Battery/Desc_Battery.Desc_Battery_C")},
        {TEXT("Cement"), TEXT("/Game/FactoryGame/Resource/Parts/Cement/Desc_Cement.Desc_Cement_C")},
        {TEXT("CircuitBoard"), TEXT("/Game/FactoryGame/Resource/Parts/CircuitBoard/Desc_CircuitBoard.Desc_CircuitBoard_C")},
        {TEXT("Computer"), TEXT("/Game/FactoryGame/Resource/Parts/Computer/Desc_Computer.Desc_Computer_C")},
        {TEXT("ComputerQuantum"), TEXT("/Game/FactoryGame/Resource/Parts/ComputerQuantum/Desc_ComputerQuantum.Desc_ComputerQuantum_C")},
        {TEXT("ComputerSuper"), TEXT("/Game/FactoryGame/Resource/Parts/ComputerSuper/Desc_ComputerSuper.Desc_ComputerSuper_C")},
        {TEXT("CoolingSystem"), TEXT("/Game/FactoryGame/Resource/Parts/CoolingSystem/Desc_CoolingSystem.Desc_CoolingSystem_C")},
        {TEXT("GenericBiomass"), TEXT("/Game/FactoryGame/Resource/Parts/GenericBiomass/Desc_GenericBiomass.Desc_GenericBiomass_C")},
        {TEXT("HighSpeedConnector"), TEXT("/Game/FactoryGame/Resource/Parts/HighSpeedConnector/Desc_HighSpeedConnector.Desc_HighSpeedConnector_C")},
        {TEXT("IronPlate"), TEXT("/Game/FactoryGame/Resource/Parts/IronPlate/Desc_IronPlate.Desc_IronPlate_C")},
        {TEXT("IronRod"), TEXT("/Game/FactoryGame/Resource/Parts/IronRod/Desc_IronRod.Desc_IronRod_C")},
        {TEXT("ModularFrame"), TEXT("/Game/FactoryGame/Resource/Parts/ModularFrame/Desc_ModularFrame.Desc_ModularFrame_C")},
        {TEXT("ModularFrameFused"), TEXT("/Game/FactoryGame/Resource/Parts/ModularFrameFused/Desc_ModularFrameFused.Desc_ModularFrameFused_C")},
        {TEXT("ModularFrameHeavy"), TEXT("/Game/FactoryGame/Resource/Parts/ModularFrameHeavy/Desc_ModularFrameHeavy.Desc_ModularFrameHeavy_C")},
        {TEXT("Motor"), TEXT("/Game/FactoryGame/Resource/Parts/Motor/Desc_Motor.Desc_Motor_C")},
        {TEXT("MotorLightweight"), TEXT("/Game/FactoryGame/Resource/Parts/MotorLightweight/Desc_MotorLightweight.Desc_MotorLightweight_C")},
        {TEXT("Plastic"), TEXT("/Game/FactoryGame/Resource/Parts/Plastic/Desc_Plastic.Desc_Plastic_C")},
        {TEXT("Rotor"), TEXT("/Game/FactoryGame/Resource/Parts/Rotor/Desc_Rotor.Desc_Rotor_C")},
        {TEXT("Rubber"), TEXT("/Game/FactoryGame/Resource/Parts/Rubber/Desc_Rubber.Desc_Rubber_C")},
        {TEXT("Stator"), TEXT("/Game/FactoryGame/Resource/Parts/Stator/Desc_Stator.Desc_Stator_C")},
        {TEXT("SteelPipe"), TEXT("/Game/FactoryGame/Resource/Parts/SteelPipe/Desc_SteelPipe.Desc_SteelPipe_C")},
        {TEXT("SteelPlate"), TEXT("/Game/FactoryGame/Resource/Parts/SteelPlate/Desc_SteelPlate.Desc_SteelPlate_C")},
        {TEXT("Wire"), TEXT("/Game/FactoryGame/Resource/Parts/Wire/Desc_Wire.Desc_Wire_C")},
    };

    const FString* DescriptorPath =
        DescriptorPaths.Find(RewardItem.DescriptorName);

    if (!DescriptorPath)
    {
        return false;
    }

    UClass* ItemClass = StaticLoadClass(
        UFGItemDescriptor::StaticClass(),
        nullptr,
        **DescriptorPath
    );

    if (!ItemClass)
    {
        UE_LOG(LogTemp, Error,
            TEXT("[Bingo] Classe introuvable : %s"),
            **DescriptorPath);
        return false;
    }

    UFGInventoryComponent* Inventory =
        Player->GetInventory();

    if (!IsValid(Inventory) || !Inventory->HasAuthority())
    {
        return false;
    }

    // Protection contre les definitions invalides.
    if (RewardItem.Quantity <= 0 || Reward.Cost < 0)
    {
        UE_LOG(LogTemp, Error,
            TEXT("[Bingo] Recompense invalide : %s"),
            *RewardId);
        return false;
    }

    FInventoryStack Stack;
    Stack.Item = FInventoryItem(ItemClass);
    Stack.NumItems = RewardItem.Quantity;

    if (!Inventory->HasEnoughSpaceForStack(Stack))
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[Bingo] Inventaire plein : %s"),
            *RewardId);
        return false;
    }

    const int32 Added = Inventory->AddStack(Stack, false);

    if (Added != RewardItem.Quantity)
    {
        UE_LOG(LogTemp, Error,
            TEXT("[Bingo] Attribution incomplete : %s (%d/%d)"),
            *RewardId, Added, RewardItem.Quantity);
        return false;
    }

    TotalScore -= Reward.Cost;

    UE_LOG(LogTemp, Display,
        TEXT("[Bingo] Achat : %s | Quantite : %d | Solde : %d"),
        *RewardId, Added, TotalScore);

    return true;
}


