// BINGO_BIOMASS_WORLD_V1
// BINGO_WORLD_CHALLENGES_MIGRATION_V1
#include "BingoSubsystem.h"
#include "FGPlayerState.h"
#include "FGStatisticsSubsystem.h"
#include "Buildables/FGBuildableFoundation.h"
#include "FGFoliageRemovalSubsystem.h"
#include "UObject/UObjectIterator.h"
#include "FGWorkBench.h"
#include "Buildables/FGBuildableResourceExtractorBase.h"
#include "FGFactoryConnectionComponent.h"
#include "Buildables/FGBuildableStorage.h"
#include "Buildables/FGBuildableResourceExtractor.h"
#include "FGRecipe.h"
#include "FGMapArea.h"
#include "Buildables/FGBuildableManufacturer.h"
#include "Buildables/FGBuildableFactory.h"
#include "EngineUtils.h"
#include "FGInventoryComponent.h"
#include "FGPowerInfoComponent.h"
#include "FGPowerCircuit.h"
#include "TimerManager.h"
#include "Buildables/FGBuildableGenerator.h"
#include "FGBuildableSubsystem.h"
#include "Buildables/FGBuildable.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "FGPortableMiner.h"
#include "FGCharacterPlayer.h"
#include "Equipment/FGChainsaw.h"
#include "FGVehicle.h"
#include "BingoRadarPlayerActor.h"
#include "FGGameState.h"
#include "FGResearchManager.h"
#include "FGSchematicManager.h"
#include "FGSchematic.h"
#include "Buildables/FGBuildableMAM.h"
#include "BingoGameData.h"
#include "Net/UnrealNetwork.h"

ABingoSubsystem::ABingoSubsystem()
{
    ReplicationPolicy =
        ESubsystemReplicationPolicy::SpawnOnServer_Replicate;

    bReplicates = true;
}

void ABingoSubsystem::EnsureGameData()
{
    if (!GameData)
    {
        GameData = NewObject<UBingoGameData>(this);
        GameData->InitializeGame();
    }
}


// BINGO_P1_C21_BIOMASS_COUNTER_V2

static bool BingoIsWoodBiomassRecipe(
    const UObject* Context,
    TSubclassOf<UFGRecipe> Recipe)
{
    if (!Recipe)
        return false;

    bool bWood = false;
    bool bBiomass = false;
    bool bOtherIngredient = false;

    for (const FItemAmount& Ingredient :
         UFGRecipe::GetIngredients(Context, Recipe))
    {
        if (!Ingredient.ItemClass || Ingredient.Amount <= 0)
            continue;

        const FString Name = GetNameSafe(Ingredient.ItemClass.Get());

        if (Name.Contains(TEXT("Wood"), ESearchCase::IgnoreCase))
            bWood = true;
        else
            bOtherIngredient = true;
    }

    for (const FItemAmount& Product : UFGRecipe::GetProducts(Recipe))
    {
        if (!Product.ItemClass || Product.Amount <= 0)
            continue;

        const FString Name = GetNameSafe(Product.ItemClass.Get());

        if (Name.Contains(TEXT("Biomass"), ESearchCase::IgnoreCase))
            bBiomass = true;
    }

    return bWood && bBiomass && !bOtherIngredient;
}

// BINGO_P1_C21_CYCLE_TRACKING_V4
void ABingoSubsystem::BingoCheckWoodBiomassCycles()
{
    // BINGO_P1_C21_PAUSE_SAFE_V6
    if (!HasAuthority() || !GetWorld() ||
        BingoWoodBiomassProduced >= 500)
    {
        return;
    }

    for (TActorIterator<AFGBuildableManufacturer> It(GetWorld()); It; ++It)
    {
        AFGBuildableManufacturer* Machine = *It;

        if (!IsValid(Machine))
            continue;

        const TWeakObjectPtr<AFGBuildableManufacturer> Key(Machine);
        const TSubclassOf<UFGRecipe> Recipe = Machine->GetCurrentRecipe();

        const bool bConstructor =
            Machine->GetClass()->GetName().Contains(
                TEXT("Constructor"), ESearchCase::IgnoreCase);

        if (!bConstructor ||
            !BingoIsWoodBiomassRecipe(Machine, Recipe))
        {
            BingoWoodCycleProgress.Remove(Key);
            BingoWoodCycleRecipes.Remove(Key);
            continue;
        }

        // Ne pas effacer l'historique pendant une pause.
        // Ne pas compter non plus une remise a zero a l'arret.
        if (!Machine->IsProducing())
        {
            continue;
        }

        const float Progress = Machine->GetProductionProgress();

        if (!FMath::IsFinite(Progress) ||
            Progress < 0.0f || Progress > 1.0f)
        {
            BingoWoodCycleProgress.Remove(Key);
            BingoWoodCycleRecipes.Remove(Key);
            continue;
        }

        const float* Previous = BingoWoodCycleProgress.Find(Key);
        UClass* const* PreviousRecipe = BingoWoodCycleRecipes.Find(Key);

        if (Previous &&
            PreviousRecipe &&
            *PreviousRecipe == Recipe.Get() &&
            *Previous >= 0.80f &&
            Progress <= 0.20f)
        {
            for (const FItemAmount& Product :
                 UFGRecipe::GetProducts(Recipe))
            {
                if (Product.ItemClass &&
                    GetNameSafe(Product.ItemClass.Get()).Contains(
                        TEXT("Biomass"), ESearchCase::IgnoreCase))
                {
                    BingoAddWoodBiomass(Product.Amount);
                }
            }
        }

        BingoWoodCycleProgress.Add(Key, Progress);
        BingoWoodCycleRecipes.Add(Key, Recipe.Get());
    }
}
void ABingoSubsystem::BingoAddWoodBiomass(int32 Quantity)
{
    if (!HasAuthority() || Quantity <= 0)
        return;

    if (BingoWoodBiomassProduced >= 500)
        return;

    BingoWoodBiomassProduced =
        FMath::Clamp(BingoWoodBiomassProduced + Quantity, 0, 500);

    UE_LOG(LogTemp, Display,
        TEXT("[Bingo][P1_C21] Biomasse bois : %d / 500"),
        BingoWoodBiomassProduced);

    if (BingoWoodBiomassProduced >= 500)
    {
        ValidateChallengeForAllPlayers(TEXT("P1_C21"));
    }

    ForceNetUpdate();
}



// BINGO_P1_C14_EQUIPMENT_CRAFT_V2
void ABingoSubsystem::BingoOnEquipmentCrafted(
    AFGPlayerState* PlayerState,
    const FItemAmount& Crafted)
{
    if (!HasAuthority() ||
        !IsValid(PlayerState) ||
        !Crafted.ItemClass ||
        Crafted.Amount <= 0)
    {
        return;
    }

    // BINGO_P1_C18_C19_CRAFT_V2
    // L'evenement du jeu confirme une fabrication manuelle.
    const FString CraftedClassName =
        GetNameSafe(Crafted.ItemClass.Get());

    if (CraftedClassName.Contains(
            TEXT("PortableMiner"),
            ESearchCase::IgnoreCase))
    {
        UE_LOG(LogTemp, Display,
            TEXT("[Bingo][P1_C18] Foreuse portable fabriquee : %s"),
            *CraftedClassName);

        ValidateChallengeForPlayer(TEXT("P1_C18"), PlayerState);
    }

    if (CraftedClassName.Contains(
            TEXT("Chainsaw"),
            ESearchCase::IgnoreCase))
    {
        UE_LOG(LogTemp, Display,
            TEXT("[Bingo][P1_C19] Tronconneuse fabriquee : %s"),
            *CraftedClassName);

        ValidateChallengeForPlayer(TEXT("P1_C19"), PlayerState);
    }

    UE_LOG(LogTemp, Verbose,
        TEXT("[Bingo][Craft] Objet fabrique : %s"),
        *CraftedClassName);

    for (TObjectIterator<UFGWorkBench> It; It; ++It)
    {
        UFGWorkBench* Bench = *It;

        if (!IsValid(Bench) ||
            Bench->GetWorld() != GetWorld())
        {
            continue;
        }

        AFGCharacterPlayer* Player =
            Bench->GetWorkBenchUser();

        if (!IsValid(Player) ||
            Player->GetPlayerState() != PlayerState)
        {
            continue;
        }

        // Ne compter que l'atelier d'equipement.
        const AActor* WorkshopOwner = Bench->GetOwner();

        if (!IsValid(WorkshopOwner))
            continue;

        const FString OwnerName =
            WorkshopOwner->GetClass()->GetName();

        // BINGO_P1_C14_WORKSHOP_DIAGNOSTIC_V3
        UE_LOG(LogTemp, Warning,
            TEXT("[Bingo][P1_C14] Fabrication=%s Atelier=%s Classe=%s Recette=%s"),
            *GetNameSafe(Crafted.ItemClass.Get()),
            *GetNameSafe(WorkshopOwner),
            *OwnerName,
            *GetNameSafe(Bench->GetCurrentRecipe().Get()));

        // BINGO_P1_C14_WORKSHOP_V4
        // Rechercher l'atelier dans la hierarchie des classes.
        // BINGO_P1_C14_EXACT_WORKSHOP_V6
        // Classe generee du Blueprint vanilla Build_Workshop.
        const FString WorkshopClassPath =
            TEXT("/Game/FactoryGame/Buildable/Factory/Workshop/Build_Workshop.Build_Workshop_C");

        const bool bEquipmentWorkshop =
            WorkshopOwner->GetClass()->GetPathName() == WorkshopClassPath;

        if (!bEquipmentWorkshop)
        {
            UE_LOG(LogTemp, Verbose,
                TEXT("[Bingo][P1_C14] Atelier non reconnu : %s"),
                *OwnerName);
            continue;
        }

        const TSubclassOf<UFGRecipe> Recipe =
            Bench->GetCurrentRecipe();

        if (!Recipe)
            continue;

        // Verifier que la recette fabrique bien
        // l'objet signale par le jeu.
        const TArray<FItemAmount> Products =
            UFGRecipe::GetProducts(Recipe);

        for (const FItemAmount& Product : Products)
        {
            if (Product.ItemClass == Crafted.ItemClass &&
                Product.Amount > 0)
            {
                UE_LOG(LogTemp, Display,
                    TEXT("[Bingo][P1_C14] Equipement fabrique : %s"),
                    *GetNameSafe(Crafted.ItemClass.Get()));

                ValidateChallengeForPlayer(TEXT("P1_C14"), PlayerState);
                return;
            }
        }
    }
}
// BINGO_WORLD_PICKUPS_V1
// Only the authoritative statistics pickup event is counted; inventory-add
// notifications are deliberately ignored.
void ABingoSubsystem::BingoOnWorldItemPickedUp(
    AFGPlayerState* PlayerState, const FItemAmount& PickedUp)
{
    if (!HasAuthority() || !IsValid(PlayerState) ||
        !PickedUp.ItemClass || PickedUp.Amount <= 0)
    {
        return;
    }

        // BINGO_PERSONAL_COLLECTIONS_V2
    const FString PlayerId = BingoGetPlayerId(PlayerState);

    if (PlayerId.IsEmpty())
    {
        return;
    }

    FBingoPlayerProgress* Progress =
        BingoFindOrCreatePlayerProgress(PlayerId);

    if (!Progress)
    {
        return;
    }

    const FString ItemName = GetNameSafe(PickedUp.ItemClass.Get());
    if (ItemName.Equals(TEXT("Desc_HardDrive_C"), ESearchCase::IgnoreCase))
    {
        BingoHardDrivesCollected = FMath::Clamp(
            BingoHardDrivesCollected + PickedUp.Amount, 0, 1000000);

        Progress->HardDrivesCollected = FMath::Clamp(
            Progress->HardDrivesCollected + PickedUp.Amount, 0, 1000000);
    }
    else if (ItemName.Equals(TEXT("Desc_WAT2_C"), ESearchCase::IgnoreCase))
    {
        BingoMercerSpheresCollected = FMath::Clamp(
            BingoMercerSpheresCollected + PickedUp.Amount, 0, 1000000);

        Progress->MercerSpheresCollected = FMath::Clamp(
            Progress->MercerSpheresCollected + PickedUp.Amount, 0, 1000000);
    }
    else
    {
        return;
    }

    UE_LOG(LogTemp, Display,
        TEXT("[Bingo] Collecte observee : %s x%d | Disques=%d Mercer=%d"),
        *ItemName, PickedUp.Amount,
        BingoHardDrivesCollected, BingoMercerSpheresCollected);
    if (Progress->HardDrivesCollected >= 10)
        ValidateChallengeForPlayer(TEXT("P3_C3"), PlayerState);

    if (Progress->HardDrivesCollected >= 20)
        ValidateChallengeForPlayer(TEXT("P4_C5"), PlayerState);

    if (Progress->HardDrivesCollected >= 30)
        ValidateChallengeForPlayer(TEXT("P5_C9"), PlayerState);

    if (Progress->MercerSpheresCollected >= 10)
        ValidateChallengeForPlayer(TEXT("P5_C22"), PlayerState);
    ForceNetUpdate();
}

void ABingoSubsystem::BingoValidateCollectionChallenges()
{
    if (!HasAuthority()) return;
    if (BingoHardDrivesCollected >= 10) ValidateChallenge(TEXT("P3_C3"));
    if (BingoHardDrivesCollected >= 20) ValidateChallenge(TEXT("P4_C5"));
    if (BingoHardDrivesCollected >= 30) ValidateChallenge(TEXT("P5_C9"));
    if (BingoMercerSpheresCollected >= 10) ValidateChallenge(TEXT("P5_C22"));
}

void ABingoSubsystem::BeginPlay()
{
    Super::BeginPlay();

    UE_LOG(LogTemp, Warning,
        TEXT("[BINGO_SUBSYSTEM_234] BeginPlay Actor=%s Authority=%d"),
        *GetNameSafe(this),
        HasAuthority() ? 1 : 0);

    if (HasAuthority())
    {
        if (AFGStatisticsSubsystem* Stats =
            AFGStatisticsSubsystem::Get(GetWorld()))
        {
            Stats->mOnItemManuallyCrafted.AddUObject(
                this, &ABingoSubsystem::BingoOnEquipmentCrafted);
            Stats->mOnItemPickedUp.AddUObject(
                this, &ABingoSubsystem::BingoOnWorldItemPickedUp);
        }
    }



    EnsureGameData();
    RestoreProgress();

    if (HasAuthority())
    {
        // Periodically recheck thresholds as phases unlock.
        FTimerHandle BingoCollectionTimer;
        GetWorldTimerManager().SetTimer(
            BingoCollectionTimer, this,
            &ABingoSubsystem::BingoValidateCollectionChallenges,
            5.0f, true, 5.0f);
        BingoEnsurePlayerNetworkActors();
    BingoBindPlayerInventories();
        BingoBindManufacturerOutputs();
        // BINGO_P1_C21_CYCLE_TIMER_V4
        FTimerHandle BingoWoodCycleTimer;
        GetWorldTimerManager().SetTimer(
            BingoWoodCycleTimer,
            this,
            &ABingoSubsystem::BingoCheckWoodBiomassCycles,
            0.05f,
            true,
            0.05f
        );
        FTimerHandle BingoInventoryBindTimer;
        GetWorldTimerManager().SetTimer(
            BingoInventoryBindTimer, this,
            &ABingoSubsystem::BingoRefreshInventoryBindings,
            10.0f, true, 10.0f);
    }

    if (HasAuthority())
    {
        FTimerHandle BingoElectricityTimer;

        GetWorldTimerManager().SetTimer(
            BingoElectricityTimer,
            this,
            &ABingoSubsystem::BingoCheckElectricity,
            5.0f,
            true,
            5.0f
        );
    }

    if (HasAuthority())
    {
        if (AFGResearchManager* ResearchManager = AFGResearchManager::Get(GetWorld()))
        {
            ResearchManager->ResearchCompletedDelegate.AddUniqueDynamic(
                this, &ABingoSubsystem::BingoOnResearchCompleted);
        }

        if (AFGSchematicManager* SchematicManager = AFGSchematicManager::Get(GetWorld()))
        {
            SchematicManager->PurchasedSchematicDelegate.AddUniqueDynamic(
                this, &ABingoSubsystem::BingoOnSchematicPurchased);
            SchematicManager->PurchasedSchematicInstigatorDelegate.AddUniqueDynamic(
                this, &ABingoSubsystem::BingoOnSchematicPurchasedByPlayer);
        }

        if (AFGGameState* FGState = GetWorld()->GetGameState<AFGGameState>())
        {
            FGState->MapAreaVisistedDelegate.AddUniqueDynamic(
                this, &ABingoSubsystem::BingoOnMapAreaVisited);

            FGState->mOnActorsConstructedByPlayer.AddUniqueDynamic(
                this, &ABingoSubsystem::OnBingoActorsConstructed);

            UE_LOG(LogTemp, Display,
                TEXT("[Bingo] Detection constructions active"));
        }
    }
}

void ABingoSubsystem::GetLifetimeReplicatedProps(
    TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ABingoSubsystem, TotalBingoPoints);
    DOREPLIFETIME(ABingoSubsystem, SavedCurrentPhase);
    DOREPLIFETIME(ABingoSubsystem, CompletedChallengeIds);
    // BINGO_V3_REAL_BIOMASS_PROGRESS_01
    DOREPLIFETIME(ABingoSubsystem, BingoWoodBiomassProduced);
    DOREPLIFETIME(ABingoSubsystem, BingoHardDrivesCollected);
    DOREPLIFETIME(ABingoSubsystem, BingoMercerSpheresCollected);
}

UBingoGameData* ABingoSubsystem::GetBingoGameData() const
{
    return GameData;
}

FString ABingoSubsystem::BingoGetPlayerId(
    const AFGPlayerState* PlayerState) const
{
    // BINGO_PLAYER_ID_V1
    if (!IsValid(PlayerState))
    {
        return FString();
    }

    const FUniqueNetIdRepl UniqueId = PlayerState->GetUniqueNetId();

    if (!UniqueId.IsValid())
    {
        return FString();
    }

        const TSharedPtr<const FUniqueNetId> NetId =
        UniqueId.GetUniqueNetId();

    if (!NetId.IsValid())
    {
        return FString();
    }

    return NetId->ToString();
}
int32 ABingoSubsystem::GetTotalBingoPoints() const
{
    return TotalBingoPoints;
}

const FBingoPlayerProgress* ABingoSubsystem::BingoFindPlayerProgress(
    const FString& PlayerId) const
{
    // BINGO_PLAYER_PROGRESS_LOOKUP_V1
    if (PlayerId.IsEmpty())
    {
        return nullptr;
    }

    for (const FBingoPlayerProgress& Progress : PlayerProgressRecords)
    {
        if (Progress.PlayerId == PlayerId)
        {
            return &Progress;
        }
    }

    return nullptr;
}

FBingoPlayerProgress* ABingoSubsystem::BingoFindOrCreatePlayerProgress(
    const FString& PlayerId)
{
    if (!HasAuthority() || PlayerId.IsEmpty())
    {
        return nullptr;
    }

    for (FBingoPlayerProgress& Progress : PlayerProgressRecords)
    {
        if (Progress.PlayerId == PlayerId)
        {
            return &Progress;
        }
    }

    FBingoPlayerProgress NewProgress;
    NewProgress.PlayerId = PlayerId;

    const int32 Index = PlayerProgressRecords.Add(MoveTemp(NewProgress));

    return &PlayerProgressRecords[Index];
}
void ABingoSubsystem::BingoInitializeConnectedPlayers()
{
    // BINGO_INIT_PLAYERS_V1
    if (!HasAuthority() || !GetWorld())
    {
        return;
    }

    for (TActorIterator<AFGCharacterPlayer> It(GetWorld()); It; ++It)
    {
        AFGCharacterPlayer* Player = *It;

        if (!IsValid(Player))
        {
            continue;
        }

        AFGPlayerState* PlayerState =
            Cast<AFGPlayerState>(Player->GetPlayerState());

        if (!IsValid(PlayerState))
        {
            continue;
        }

        const FString PlayerId = BingoGetPlayerId(PlayerState);

        if (PlayerId.IsEmpty())
        {
            continue;
        }

        // BINGO_PLAYER_GRID_INIT_V1
        const bool bAlreadyRegistered =
            BingoFindPlayerProgress(PlayerId) != nullptr;

        UBingoGameData* PersonalGrid =
            BingoGetOrCreatePlayerGameData(PlayerId);

        if (PersonalGrid && !bAlreadyRegistered)
        {
            UE_LOG(
                LogTemp,
                Display,
                TEXT("[Bingo] Nouvelle grille personnelle initialisee")
            );
        }
    }
}
UBingoGameData* ABingoSubsystem::BingoGetOrCreatePlayerGameData(
    const FString& PlayerId)
{
    // BINGO_CREATE_PLAYER_GRID_V1
    if (!HasAuthority() || PlayerId.IsEmpty())
    {
        return nullptr;
    }

    if (TObjectPtr<UBingoGameData>* Existing =
        PlayerGameData.Find(PlayerId))
    {
        return Existing->Get();
    }

    FBingoPlayerProgress* Progress =
        BingoFindOrCreatePlayerProgress(PlayerId);

    if (!Progress)
    {
        return nullptr;
    }

    UBingoGameData* NewGrid = NewObject<UBingoGameData>(this);

    if (!NewGrid)
    {
        return nullptr;
    }

    NewGrid->InitializeGame();

    const int32 TargetPhase =
        FMath::Clamp(Progress->CurrentPhase, 1, 5);

    while (NewGrid->CurrentPhase < TargetPhase)
    {
        if (!NewGrid->UnlockNextPhase())
        {
            break;
        }
    }

    for (const FString& ChallengeId : Progress->CompletedChallenges)
    {
        NewGrid->CompleteChallenge(ChallengeId);
    }

    // Le solde sauvegarde prime sur les points recalcules.
    NewGrid->TotalScore = Progress->Points;

    PlayerGameData.Add(PlayerId, NewGrid);

    return NewGrid;
}
void ABingoSubsystem::RestoreProgress()
{
    EnsureGameData();

    // Reconstruction a partir des donnees sauvegardees ou repliquees.
    GameData->InitializeGame();

    const int32 TargetPhase = FMath::Clamp(SavedCurrentPhase, 1, 5);

    while (GameData->CurrentPhase < TargetPhase)
    {
        if (!GameData->UnlockNextPhase())
        {
            break;
        }
    }

    for (const FString& ChallengeId : CompletedChallengeIds)
    {
        GameData->CompleteChallenge(ChallengeId);
    }

    // Le score sauvegarde est le solde reel apres achats.
    GameData->TotalScore = TotalBingoPoints;
}

void ABingoSubsystem::OnRep_BingoProgress()
{
    RestoreProgress();

    UE_LOG(LogTemp, Display,
        TEXT("[Bingo] Progression client synchronisee : %d points"),
        TotalBingoPoints);
}

void ABingoSubsystem::BingoSyncPlayerProgress()
{
    // BINGO_SYNC_PLAYER_PROGRESS_V1
    if (!HasAuthority())
    {
        return;
    }

    for (const auto& Entry : PlayerGameData)
    {
        const FString& PlayerId = Entry.Key;
        UBingoGameData* Grid = Entry.Value.Get();

        if (!IsValid(Grid))
        {
            continue;
        }

        FBingoPlayerProgress* Progress =
            BingoFindOrCreatePlayerProgress(PlayerId);

        if (!Progress)
        {
            continue;
        }

        Progress->Points = Grid->TotalScore;
        Progress->CurrentPhase = Grid->CurrentPhase;
        Progress->CompletedChallenges.Empty();

        for (const FBingoPhase& Phase : Grid->Phases)
        {
            for (const FBingoChallenge& Challenge : Phase.Challenges)
            {
                if (Challenge.bCompleted)
                {
                    Progress->CompletedChallenges.AddUnique(
                        Challenge.ChallengeId);
                }
            }
        }
    }
}
void ABingoSubsystem::PreSaveGame_Implementation(
    int32 SaveVersion, int32 GameVersion)
{
        // BINGO_SAVE_PLAYERS_V1
    // Synchronisation des grilles personnelles avant sauvegarde.
    if (HasAuthority())
    {
        BingoSyncPlayerProgress();
    }

    if (!HasAuthority() || !GameData)
    {
        return;
    }

    CompletedChallengeIds.Empty();

    for (const FBingoPhase& Phase : GameData->Phases)
    {
        for (const FBingoChallenge& Challenge : Phase.Challenges)
        {
            if (Challenge.bCompleted)
            {
                CompletedChallengeIds.AddUnique(Challenge.ChallengeId);
            }
        }
    }

    TotalBingoPoints = GameData->TotalScore;
    SavedCurrentPhase = GameData->CurrentPhase;
}

void ABingoSubsystem::PostLoadGame_Implementation(
    int32 SaveVersion, int32 GameVersion)
{
    // BINGO_RESTORE_PLAYER_GRIDS_V1
    // Reconstruire les grilles depuis les donnees chargees.
    PlayerGameData.Empty();

    RestoreProgress();
}

bool ABingoSubsystem::ShouldSave_Implementation() const
{
    return true;
}

bool ABingoSubsystem::NeedTransform_Implementation()
{
    return false;
}

void ABingoSubsystem::GatherDependencies_Implementation(
    TArray<UObject*>& OutDependencies)
{
}

int32 ABingoSubsystem::ValidateChallengeForAllPlayers(
    const FString& ChallengeId)
{
    // BINGO_WORLD_VALIDATION_V1
    if (!HasAuthority() || !GetWorld())
    {
        return 0;
    }

    BingoInitializeConnectedPlayers();

    int32 ValidatedCount = 0;

    for (TActorIterator<AFGCharacterPlayer> It(GetWorld()); It; ++It)
    {
        // BINGO_WORLD_CONNECTED_ONLY_V1
        AFGCharacterPlayer* Player = *It;

        if (!IsValid(Player) || !IsValid(Player->GetController()))
            continue;

        AFGPlayerState* PlayerState =
            Cast<AFGPlayerState>(Player->GetPlayerState());

        if (!IsValid(PlayerState))
            continue;

        const FString PlayerId = BingoGetPlayerId(PlayerState);

        if (PlayerId.IsEmpty())
            continue;

        UBingoGameData* PersonalGrid =
            BingoGetOrCreatePlayerGameData(PlayerId);

        if (!IsValid(PersonalGrid))
        {
            continue;
        }

        if (PersonalGrid->CompleteChallenge(ChallengeId))
        {
            ++ValidatedCount;
        }
    }

    if (ValidatedCount > 0)
    {
        BingoSyncPlayerProgress();

        UE_LOG(
            LogTemp,
            Display,
            TEXT("[Bingo][Monde] Defi %s valide pour %d joueur(s)"),
            *ChallengeId,
            ValidatedCount
        );
    }

    return ValidatedCount;
}
bool ABingoSubsystem::ValidateChallengeForPlayer(
    const FString& ChallengeId,
    AFGPlayerState* PlayerState)
{
    // BINGO_PERSONAL_VALIDATION_V1
    if (!HasAuthority() || !IsValid(PlayerState))
    {
        return false;
    }

    const FString PlayerId = BingoGetPlayerId(PlayerState);

    if (PlayerId.IsEmpty())
    {
        return false;
    }

    UBingoGameData* PersonalGrid =
        BingoGetOrCreatePlayerGameData(PlayerId);

    if (!IsValid(PersonalGrid))
    {
        return false;
    }

    if (!PersonalGrid->CompleteChallenge(ChallengeId))
    {
        return false;
    }

    FBingoPlayerProgress* Progress =
        BingoFindOrCreatePlayerProgress(PlayerId);

    if (Progress)
    {
        Progress->Points = PersonalGrid->TotalScore;
        Progress->CurrentPhase = PersonalGrid->CurrentPhase;
        Progress->CompletedChallenges.AddUnique(ChallengeId);
        // BINGO_PERSONAL_SYNC_V2
        BingoSyncPlayerProgress();
    }

    UE_LOG(
        LogTemp,
        Display,
        TEXT("[Bingo][Personnel] Defi %s valide | Joueur %s | Points %d"),
        *ChallengeId,
        *PlayerId,
        PersonalGrid->TotalScore
    );

    return true;
}
bool ABingoSubsystem::ValidateChallenge(const FString& ChallengeId)
{
    if (!HasAuthority())
    {
        return false;
    }

    EnsureGameData();

    if (!GameData->CompleteChallenge(ChallengeId))
    {
        return false;
    }

    CompletedChallengeIds.AddUnique(ChallengeId);
    TotalBingoPoints = GameData->TotalScore;
    SavedCurrentPhase = GameData->CurrentPhase;

    ForceNetUpdate();

    UE_LOG(LogTemp, Display,
        TEXT("[Bingo] Defi valide : %s | Score : %d"),
        *ChallengeId, TotalBingoPoints);

    return true;
}

bool ABingoSubsystem::BuyBingoReward(
    const FString& RewardId,
    AFGCharacterPlayer* Player)
{
    if (!HasAuthority() || !IsValid(Player))
    {
        return false;
    }

        // BINGO_PERSONAL_REWARDS_V2
    AFGPlayerState* PlayerState =
        Cast<AFGPlayerState>(Player->GetPlayerState());

    if (!IsValid(PlayerState))
        return false;

    const FString PlayerId = BingoGetPlayerId(PlayerState);

    if (PlayerId.IsEmpty())
        return false;

    UBingoGameData* PersonalGrid =
        BingoGetOrCreatePlayerGameData(PlayerId);

    if (!IsValid(PersonalGrid))
        return false;

    ABingoRadarPlayerActor* RadarActor = nullptr;

    if (RewardId == TEXT("ExplorationRadar"))
    {
        UWorld* World = GetWorld();
        AController* Controller = Player->GetController();

        if (!IsValid(World) || !IsValid(Controller))
        {
            return false;
        }

        for (ABingoRadarPlayerActor* Existing : RadarPlayerActors)
        {
            if (IsValid(Existing) &&
                Existing->GetOwner() == Controller)
            {
                RadarActor = Existing;
                break;
            }
        }

        if (!IsValid(RadarActor))
        {
            FActorSpawnParameters Params;
            Params.Owner = Controller;

            RadarActor = World->SpawnActor<ABingoRadarPlayerActor>(
                ABingoRadarPlayerActor::StaticClass(),
                Player->GetActorLocation(),
                FRotator::ZeroRotator,
                Params
            );

            if (!IsValid(RadarActor))
            {
                UE_LOG(LogTemp, Error,
                    TEXT("[BingoRadar] Achat refuse : creation acteur impossible"));
                return false;
            }

            RadarPlayerActors.Add(RadarActor);
        }
    }

    if (!PersonalGrid->PurchaseReward(RewardId, Player))
    {
        return false;
    }

    if (RewardId == TEXT("ExplorationRadar"))
    {
        RadarExpirationTimes.Add(
            Player,
            GetWorld()->GetTimeSeconds() + 600.0
        );

        RadarActor->Client_ActivateRadar(600.0f);
    }

    BingoSyncPlayerProgress();

    UE_LOG(LogTemp, Display,
        TEXT("[Bingo] Achat %s | Solde %d"),
        *RewardId, PersonalGrid->TotalScore);

    return true;
}
bool ABingoSubsystem::IsExplorationRadarActive(
    AFGCharacterPlayer* Player) const
{
    if (!IsValid(Player) || !GetWorld())
    {
        return false;
    }

    const double* Expiration =
        RadarExpirationTimes.Find(Player);

    return Expiration &&
        GetWorld()->GetTimeSeconds() < *Expiration;
}


#include "Buildables/FGBuildableJumppad.h"
#include "Buildables/FGBuildableDroneStation.h"

#include "Buildables/FGBuildableSpaceElevator.h"
#include "Buildables/FGBuildableResourceSink.h"
#include "Buildables/FGBuildableStorage.h"
#include "Buildables/FGBuildableResourceSinkShop.h"
#include "Buildables/FGBuildablePipeline.h"
#include "Buildables/FGBuildablePipelinePump.h"
#include "Buildables/FGBuildableSplitterSmart.h"
#include "Buildables/FGBuildableRailroadStation.h"

static bool BingoIsManualBuildable(const AFGBuildable* Buildable)
{
    return IsValid(Buildable) &&
        !Buildable->IsBuildableInsideBlueprintDesigner() &&
        Buildable->GetBlueprintProxy() == nullptr;
}
static int32 BingoCountExistingBuildables(
    const TArray<AFGBuildable*>& Buildables,
    const TCHAR* ClassName)
{
    int32 Count = 0;

    for (const AFGBuildable* Buildable : Buildables)
    {
        if (!BingoIsManualBuildable(Buildable))
        {
            continue;
        }

        const UClass* ActualClass = Buildable->GetClass();

        if (ActualClass &&
            ActualClass->GetName().Equals(
                ClassName, ESearchCase::IgnoreCase))
        {
            ++Count;
        }
    }

    return Count;
}
// BINGO_CONNEXIONS_AUDIT_V1
// Diagnostic des ports raccordes, sans validation des cases.
static void BingoAuditConnections(const AFGBuildable* Built)
{
    if (!IsValid(Built)) return;
    const FString ClassName = Built->GetClass()->GetName();
    if (!ClassName.Contains(TEXT("Splitter"), ESearchCase::IgnoreCase) &&
        !ClassName.Contains(TEXT("Merger"), ESearchCase::IgnoreCase) &&
        !ClassName.Contains(TEXT("ConveyorAttachment"), ESearchCase::IgnoreCase))
    {
        return;
    }
    TInlineComponentArray<UFGFactoryConnectionComponent*> Ports;
    Built->GetComponents(Ports);
    int32 Connected = 0;
    int32 Inputs = 0;
    int32 Outputs = 0;
    for (UFGFactoryConnectionComponent* Port : Ports)
    {
        if (!IsValid(Port) || !Port->IsConnected()) continue;
        UFGFactoryConnectionComponent* Other = Port->GetConnection();
        if (!IsValid(Other)) continue;
        ++Connected;
        if (Port->GetDirection() == EFactoryConnectionDirection::FCD_INPUT) ++Inputs;
        if (Port->GetDirection() == EFactoryConnectionDirection::FCD_OUTPUT) ++Outputs;
        UE_LOG(LogTemp, Display, TEXT("[Bingo][Connexions] %s port=%s direction=%d vers=%s"),
            *ClassName, *GetNameSafe(Port), static_cast<int32>(Port->GetDirection()),
            *GetNameSafe(Other->GetOwner()));
    }
    UE_LOG(LogTemp, Display, TEXT("[Bingo][Connexions] %s ports=%d connectes=%d entrees=%d sorties=%d (audit seul)"),
        *ClassName, Ports.Num(), Connected, Inputs, Outputs);
}
// BINGO_GLOBAL_CONNECTIONS_V1
// Parcours des liaisons de convoyeurs avec protection contre les boucles.
// Les cases ne sont validees que si trois machines distinctes sont atteintes.
static void BingoReachMachineEndpoints(
    UFGFactoryConnectionComponent* Start,
    TSet<const UFGFactoryConnectionComponent*>& SeenPorts,
    TSet<const AFGBuildable*>& Machines,
    int32 Depth)
{
    if (!IsValid(Start) || Depth > 128 || SeenPorts.Contains(Start)) return;
    SeenPorts.Add(Start);
    UFGFactoryConnectionComponent* Next = Start->GetConnection();
    if (!IsValid(Next)) return;
    const AFGBuildable* Owner = Cast<AFGBuildable>(Next->GetOwner());
    if (!IsValid(Owner)) return;
    const FString Name = Owner->GetClass()->GetName();
    const bool IsConveyor =
        Name.Contains(TEXT("ConveyorBelt"), ESearchCase::IgnoreCase) ||
        Name.Contains(TEXT("ConveyorLift"), ESearchCase::IgnoreCase);
    if (IsConveyor)
    {
        TInlineComponentArray<UFGFactoryConnectionComponent*> Ports;
        Owner->GetComponents(Ports);
        for (UFGFactoryConnectionComponent* Port : Ports)
        {
            if (IsValid(Port) && Port != Next)
                BingoReachMachineEndpoints(Port, SeenPorts, Machines, Depth + 1);
        }
        return;
    }
    // A connected conveyor attachment is another network junction, not a machine.
    if (Name.Contains(TEXT("Splitter"), ESearchCase::IgnoreCase) ||
        Name.Contains(TEXT("Merger"), ESearchCase::IgnoreCase) ||
        Name.Contains(TEXT("ConveyorAttachment"), ESearchCase::IgnoreCase))
        return;
    if (Owner->IsA(AFGBuildableFactory::StaticClass()))
        Machines.Add(Owner);
}

static void BingoCheckJunctionConnections(
    const AFGBuildable* Built, bool& Merger3, bool& Splitter3)
{
    if (!IsValid(Built)) return;
    const FString Name = Built->GetClass()->GetName();
    const bool IsMerger = Name.Contains(TEXT("Merger"), ESearchCase::IgnoreCase);
    const bool IsSplitter = Name.Contains(TEXT("Splitter"), ESearchCase::IgnoreCase);
    if (!IsMerger && !IsSplitter) return;
    TInlineComponentArray<UFGFactoryConnectionComponent*> Ports;
    Built->GetComponents(Ports);
    TSet<const AFGBuildable*> InputMachines;
    TSet<const AFGBuildable*> OutputMachines;
    int32 InputPorts = 0;
    int32 OutputPorts = 0;
    for (UFGFactoryConnectionComponent* Port : Ports)
    {
        if (!IsValid(Port) || !Port->IsConnected()) continue;
        const EFactoryConnectionDirection Direction = Port->GetDirection();
        TSet<const UFGFactoryConnectionComponent*> Seen;
        TSet<const AFGBuildable*> Found;
        BingoReachMachineEndpoints(Port, Seen, Found, 0);
        if (Direction == EFactoryConnectionDirection::FCD_INPUT)
        {
            ++InputPorts;
            for (const AFGBuildable* Machine : Found) InputMachines.Add(Machine);
        }
        else if (Direction == EFactoryConnectionDirection::FCD_OUTPUT)
        {
            ++OutputPorts;
            for (const AFGBuildable* Machine : Found) OutputMachines.Add(Machine);
        }
    }
    // Each of three separate junction ports must lead to a real machine.
    if (IsMerger && InputPorts >= 3 && InputMachines.Num() >= 3) Merger3 = true;
    if (IsSplitter && OutputPorts >= 3 && OutputMachines.Num() >= 3) Splitter3 = true;
}
// BINGO_P1_C20_ANY_FOUNDATIONS_V3
// Compter 10 fondations de n'importe quel type, sans connexion.
static bool BingoP1HasTenFoundations(
    const TArray<AFGBuildable*>& Buildables)
{
    int32 Count = 0;

    for (AFGBuildable* Buildable : Buildables)
    {
        if (IsValid(Buildable) &&
            Buildable->IsA(AFGBuildableFoundation::StaticClass()))
        {
            ++Count;

            if (Count >= 10)
                return true;
        }
    }

    return false;
}
void ABingoSubsystem::OnBingoActorsConstructed(
    AFGCharacterPlayer* Player,
    TArray<AActor*> ConstructedActors)
{
    // P1_C20: 10 foundations of any type, anywhere.
    if (HasAuthority() && GetWorld())
    {
        if (AFGBuildableSubsystem* FoundationSystem = AFGBuildableSubsystem::Get(GetWorld()))
        {
            if (BingoP1HasTenFoundations(FoundationSystem->GetAllBuildablesRef()))
                ValidateChallengeForAllPlayers(TEXT("P1_C20"));
        }
    }
    // BINGO_P1_REPRISE_CONSTRUCTIONS_V1
    // Reconnaissance des batiments deja places apres rechargement de sauvegarde.
    // N'ajoute aucun defi de fabrication manuelle ou d'exploration.
    if (HasAuthority() && GetWorld())
    {
        if (AFGBuildableSubsystem* P1Buildables = AFGBuildableSubsystem::Get(GetWorld()))
        {
            const TArray<AFGBuildable*>& P1Existing = P1Buildables->GetAllBuildablesRef();
            if (BingoCountExistingBuildables(P1Existing, TEXT("Build_LookoutTower_C")) > 0)
                ValidateChallengeForAllPlayers(TEXT("P1_C3"));
            if (BingoCountExistingBuildables(P1Existing, TEXT("Build_SpaceElevator_C")) > 0)
                ValidateChallengeForAllPlayers(TEXT("P1_C4"));
            if (BingoCountExistingBuildables(P1Existing, TEXT("Build_MAM_C")) > 0)
                ValidateChallengeForAllPlayers(TEXT("P1_C6"));
            if (BingoCountExistingBuildables(P1Existing, TEXT("Build_ResourceSink_C")) > 0)
                ValidateChallengeForAllPlayers(TEXT("P1_C7"));
            if (BingoCountExistingBuildables(P1Existing, TEXT("Build_GeneratorBiomass_C")) > 0)
                ValidateChallengeForAllPlayers(TEXT("P1_C8"));
            if (BingoCountExistingBuildables(P1Existing, TEXT("Build_StorageContainerMk1_C")) > 0 ||
                BingoCountExistingBuildables(P1Existing, TEXT("Build_StorageContainerMk2_C")) > 0)
                ValidateChallengeForAllPlayers(TEXT("P1_C9"));
            if (BingoCountExistingBuildables(P1Existing, TEXT("Build_ResourceSinkShop_C")) > 0)
                ValidateChallengeForAllPlayers(TEXT("P1_C16"));
        }
        for (TActorIterator<AFGVehicle> VehicleIt(GetWorld()); VehicleIt; ++VehicleIt)
        {
            if (IsValid(*VehicleIt))
            {
                ValidateChallengeForAllPlayers(TEXT("P1_C12"));
                break;
            }
        }
    }
    if (HasAuthority())
    {
        if (AFGBuildableSubsystem* BuildableSystem =
            AFGBuildableSubsystem::Get(GetWorld()))
        {
            const TArray<AFGBuildable*>& Buildables =
                BuildableSystem->GetAllBuildablesRef();

        {
            const int32 Existing = BingoCountExistingBuildables(
                Buildables, TEXT("Build_GeneratorCoal_C"));

            if (Existing >= 1)
            {
                ValidateChallengeForAllPlayers(TEXT("P2_C12"));
            }
        }
        {
            const int32 Existing = BingoCountExistingBuildables(
                Buildables, TEXT("Build_GeneratorCoal_C"));

            if (Existing >= 2)
            {
                ValidateChallengeForAllPlayers(TEXT("P2_C8"));
            }
        }
        {
            const int32 Existing = BingoCountExistingBuildables(
                Buildables, TEXT("Build_GeneratorCoal_C"));

            if (Existing >= 3)
            {
                ValidateChallengeForAllPlayers(TEXT("P2_C10"));
            }
        }
        {
            const int32 Existing = BingoCountExistingBuildables(
                Buildables, TEXT("Build_Foundry_C"));

            if (Existing >= 1)
            {
                ValidateChallengeForAllPlayers(TEXT("P2_C21"));
            }
        }
        {
            const int32 Existing = BingoCountExistingBuildables(
                Buildables, TEXT("Build_Foundry_C"));

            if (Existing >= 2)
            {
                ValidateChallengeForAllPlayers(TEXT("P2_C19"));
            }
        }
        {
            const int32 Existing = BingoCountExistingBuildables(
                Buildables, TEXT("Build_Assembler_C"));

            if (Existing >= 1)
            {
                ValidateChallengeForAllPlayers(TEXT("P2_C20"));
            }
        }
        {
            const int32 Existing = BingoCountExistingBuildables(
                Buildables, TEXT("Build_MinerMk2_C"));

            if (Existing >= 1)
            {
                ValidateChallengeForAllPlayers(TEXT("P2_C22"));
            }
        }
        {
            const int32 Existing = BingoCountExistingBuildables(
                Buildables, TEXT("Build_TrainStation_C"));

            if (Existing >= 1)
            {
                ValidateChallengeForAllPlayers(TEXT("P3_C19"));
            }
        }
        {
            const int32 Existing = BingoCountExistingBuildables(
                Buildables, TEXT("Build_TrainStation_C"));

            if (Existing >= 2)
            {
                ValidateChallengeForAllPlayers(TEXT("P3_C16"));
            }
        }
        {
            const int32 Existing = BingoCountExistingBuildables(
                Buildables, TEXT("Build_OilRefinery_C"));

            if (Existing >= 1)
            {
                ValidateChallengeForAllPlayers(TEXT("P3_C20"));
            }
        }
        {
            const int32 Existing = BingoCountExistingBuildables(
                Buildables, TEXT("Build_OilRefinery_C"));

            if (Existing >= 2)
            {
                ValidateChallengeForAllPlayers(TEXT("P3_C17"));
            }
        }
        {
            const int32 Existing = BingoCountExistingBuildables(
                Buildables, TEXT("Build_Manufacturer_C"));

            if (Existing >= 1)
            {
                ValidateChallengeForAllPlayers(TEXT("P3_C18"));
            }
        }
        {
            const int32 Existing = BingoCountExistingBuildables(
                Buildables, TEXT("Build_Manufacturer_C"));

            if (Existing >= 2)
            {
                ValidateChallengeForAllPlayers(TEXT("P3_C15"));
            }
        }
        {
            const int32 Existing = BingoCountExistingBuildables(
                Buildables, TEXT("Build_HadronCollider_C"));

            if (Existing >= 1)
            {
                ValidateChallengeForAllPlayers(TEXT("P4_C14"));
            }
        }
        {
            const int32 Existing = BingoCountExistingBuildables(
                Buildables, TEXT("Build_HadronCollider_C"));

            if (Existing >= 2)
            {
                ValidateChallengeForAllPlayers(TEXT("P4_C11"));
            }
        }
        {
            const int32 Existing = BingoCountExistingBuildables(
                Buildables, TEXT("Build_Blender_C"));

            if (Existing >= 1)
            {
                ValidateChallengeForAllPlayers(TEXT("P4_C16"));
            }
        }
        {
            const int32 Existing = BingoCountExistingBuildables(
                Buildables, TEXT("Build_Blender_C"));

            if (Existing >= 2)
            {
                ValidateChallengeForAllPlayers(TEXT("P4_C12"));
            }
        }
        {
            const int32 Existing = BingoCountExistingBuildables(
                Buildables, TEXT("Build_Converter_C"));

            if (Existing >= 1)
            {
                ValidateChallengeForAllPlayers(TEXT("P5_C17"));
            }
        }
        {
            const int32 Existing = BingoCountExistingBuildables(
                Buildables, TEXT("Build_Converter_C"));

            if (Existing >= 2)
            {
                ValidateChallengeForAllPlayers(TEXT("P5_C15"));
            }
        }
        {
            const int32 Existing = BingoCountExistingBuildables(
                Buildables, TEXT("Build_QuantumEncoder_C"));

            if (Existing >= 1)
            {
                ValidateChallengeForAllPlayers(TEXT("P5_C18"));
            }
        }
        {
            const int32 Existing = BingoCountExistingBuildables(
                Buildables, TEXT("Build_QuantumEncoder_C"));

            if (Existing >= 2)
            {
                ValidateChallengeForAllPlayers(TEXT("P5_C16"));
            }
        }
        }
    }

    // BINGO_VALIDATIONS_MASSIF_01
    // Only physical manually placed structures are counted.
    if (HasAuthority() && GetWorld())
    {
        if (AFGBuildableSubsystem* BuildableSystem = AFGBuildableSubsystem::Get(GetWorld()))
        {
            const TArray<AFGBuildable*>& Existing = BuildableSystem->GetAllBuildablesRef();
            // Fuel generator: an actual built generator, not a recipe or item.
            if (BingoCountExistingBuildables(Existing, TEXT("Build_GeneratorFuel_C")) >= 1)
                ValidateChallengeForAllPlayers(TEXT("P3_C8"));
            // Truck station: an actual built station, not a parked truck.
            if (BingoCountExistingBuildables(Existing, TEXT("Build_TruckStation_C")) >= 1)
                ValidateChallengeForAllPlayers(TEXT("P3_C21"));
        }
    }
    // BINGO_LOT_VALIDATION_02_V1
    // P3_C12: at least two physically placed oil/fluid storage tanks.
    // Counting only a tank class, not pipes or ordinary storage containers.
    if (HasAuthority() && GetWorld())
    {
        if (AFGBuildableSubsystem* StorageSystem = AFGBuildableSubsystem::Get(GetWorld()))
        {
            const TArray<AFGBuildable*>& StorageBuildables = StorageSystem->GetAllBuildablesRef();
            int32 FluidTanks = 0;
            for (AFGBuildable* Candidate : StorageBuildables)
            {
                if (!IsValid(Candidate) || Candidate->GetBlueprintProxy() || Candidate->IsBuildableInsideBlueprintDesigner())
                    continue;
                const FString ClassName = Candidate->GetClass()->GetName();
                if (ClassName.Contains(TEXT("PipeStorageTank")) || ClassName.Contains(TEXT("FluidBuffer")))
                    ++FluidTanks;
            }
            if (FluidTanks >= 2)
                ValidateChallengeForAllPlayers(TEXT("P3_C12"));
        }
    }
    if (!HasAuthority() || !IsValid(Player))
    {
        return;
    }

    for (AActor* Actor : ConstructedActors)
    {
        if (!IsValid(Actor))
        {
            continue;
        }
        const AFGBuildable* BingoBuilt = Cast<AFGBuildable>(Actor);
        if (BingoBuilt) BingoAuditConnections(BingoBuilt);
        if (BingoBuilt && !BingoIsManualBuildable(BingoBuilt))
        {
            continue;
        }

                if (Actor->IsA(AFGBuildableJumppad::StaticClass()))
        {
            ValidateChallengeForAllPlayers(TEXT("P2_C23"));
        }
        if (Actor->IsA(AFGBuildableDroneStation::StaticClass()))
        {
            ValidateChallengeForAllPlayers(TEXT("P4_C17"));
        }
                if (Actor->IsA(AFGBuildableSpaceElevator::StaticClass()))
        {
            ValidateChallengeForAllPlayers(TEXT("P1_C4"));
        }
        if (Actor->IsA(AFGBuildableResourceSink::StaticClass()))
        {
            ValidateChallengeForAllPlayers(TEXT("P1_C7"));
        }
        if (Actor->IsA(AFGBuildableStorage::StaticClass()))
        {
            ValidateChallengeForAllPlayers(TEXT("P1_C9"));
        }
        if (Actor->IsA(AFGBuildableResourceSinkShop::StaticClass()))
        {
            ValidateChallengeForAllPlayers(TEXT("P1_C16"));
        }
        if (Actor->IsA(AFGBuildablePipeline::StaticClass()))
        {
            ValidateChallengeForAllPlayers(TEXT("P2_C16"));
        }
        if (Actor->IsA(AFGBuildablePipelinePump::StaticClass()))
        {
            ValidateChallengeForAllPlayers(TEXT("P2_C24"));
        }
        if (Actor->IsA(AFGBuildableSplitterSmart::StaticClass()))
        {
            ValidateChallengeForAllPlayers(TEXT("P3_C11"));
        }
        if (Actor->IsA(AFGBuildableRailroadStation::StaticClass()))
        {
            ValidateChallengeForAllPlayers(TEXT("P3_C19"));
        }
        // BINGO_PHASE1_BUILDING_CHECKS_V1
        // Conditions strictes : construction manuelle uniquement.
        // Les classes sont controlees par leur nom de Blueprint reel.
        if (BingoBuilt)
        {
            const FString BingoClassName = BingoBuilt->GetClass()->GetName();
            if (BingoClassName.Equals(TEXT("Build_LookoutTower_C"), ESearchCase::IgnoreCase))
            {
                ValidateChallengeForAllPlayers(TEXT("P1_C3"));
            }
            if (BingoClassName.Equals(TEXT("Build_GeneratorBiomass_C"), ESearchCase::IgnoreCase))
            {
                ValidateChallengeForAllPlayers(TEXT("P1_C8"));
            }
        }
        // BINGO_LOT_05_VEHICLE_CONSTRUCTION_V1
        // Valide uniquement un vehicule reel recu dans l'evenement de construction.
        // Ne valide pas un hologramme ou un batiment de transport.
        if (Actor->IsA(AFGVehicle::StaticClass()))
        {
            ValidateChallengeForAllPlayers(TEXT("P1_C12"));
        }
        if (Actor->IsA(AFGBuildableMAM::StaticClass()))
        {
            ValidateChallengeForAllPlayers(TEXT("P1_C6"));
        }
    }
}

// BINGO_C17_DIRECTED_CHAIN_V1
// Only traverse actual connected conveyor ports, in the output direction.
static void BingoChainTargets(UFGFactoryConnectionComponent* Start,
    TSet<const UFGFactoryConnectionComponent*>& Seen,
    TSet<const AFGBuildable*>& Targets, int32 Depth)
{
    if (!IsValid(Start) || Depth > 128 || Seen.Contains(Start)) return;
    Seen.Add(Start);
    UFGFactoryConnectionComponent* Other = Start->GetConnection();
    if (!IsValid(Other)) return;
    const AFGBuildable* B = Cast<AFGBuildable>(Other->GetOwner());
    if (!IsValid(B)) return;
    const FString N = B->GetClass()->GetName();
    const bool Transit = N.Contains(TEXT("ConveyorBelt"), ESearchCase::IgnoreCase) ||
        N.Contains(TEXT("ConveyorLift"), ESearchCase::IgnoreCase) ||
        N.Contains(TEXT("Splitter"), ESearchCase::IgnoreCase) ||
        N.Contains(TEXT("Merger"), ESearchCase::IgnoreCase) ||
        N.Contains(TEXT("ConveyorAttachment"), ESearchCase::IgnoreCase);
    if (!Transit) { Targets.Add(B); return; }
    TInlineComponentArray<UFGFactoryConnectionComponent*> Ports;
    B->GetComponents(Ports);
    for (UFGFactoryConnectionComponent* P : Ports)
        if (IsValid(P) && P != Other && P->IsConnected() &&
            P->GetDirection() != EFactoryConnectionDirection::FCD_INPUT)
            BingoChainTargets(P, Seen, Targets, Depth + 1);
}
static bool BingoChainLinks(const AFGBuildable* From, const AFGBuildable* To)
{
    if (!IsValid(From) || !IsValid(To)) return false;
    TInlineComponentArray<UFGFactoryConnectionComponent*> Ports;
    From->GetComponents(Ports);
    for (UFGFactoryConnectionComponent* P : Ports)
    {
        if (!IsValid(P) || !P->IsConnected() ||
            P->GetDirection() == EFactoryConnectionDirection::FCD_INPUT) continue;
        TSet<const UFGFactoryConnectionComponent*> Seen;
        TSet<const AFGBuildable*> Targets;
        BingoChainTargets(P, Seen, Targets, 0);
        if (Targets.Contains(To)) return true;
    }
    return false;
}
// BINGO_SHARED_NODE_C5_C15_V1
// A single real miner, attached to a resource node, must supply two distinct machines.
// We require conveyor paths; proximity and inventory contents are insufficient.
static void BingoCheckSharedNodeMachines(
    const TArray<const AFGBuildable*>& Miners,
    const TArray<const AFGBuildable*>& Targets,
    bool& Found)
{
    if (Found) return;
    for (const AFGBuildable* Miner : Miners)
    {
        if (!IsValid(Miner)) continue;
        const AFGBuildableResourceExtractorBase* Extractor =
            Cast<AFGBuildableResourceExtractorBase>(Miner);
        if (!IsValid(Extractor) || !IsValid(Extractor->GetResourceNode())) continue;
        int32 Connected = 0;
        for (const AFGBuildable* Machine : Targets)
        {
            if (!IsValid(Machine)) continue;
            if (BingoChainLinks(Miner, Machine) && ++Connected >= 2)
            {
                Found = true;
                return;
            }
        }
    }
}
// BINGO_NODES_4_6_V1
// Count DISTINCT resource nodes actively exploited by valid miners.
// Does not count machines merely placed on a node or manual inventory transfers.
// BINGO_LOT_VALIDATIONS_REELLES_02
// P1_C22/P1_C23: gisements distincts avec foreuse en production.
// P4_C19/P5_C21: une seule foreuse active sur son gisement, cadence de machine.
// P3_C24: pompe petroliere active avec debit de fluide mesure (m3/s -> m3/min).
// La cadence miner n'est pas une mesure du debit reel de convoyeur.
void ABingoSubsystem::BingoCheckActiveResourceNodes()
{
    if (!HasAuthority() || !GetWorld()) return;
    AFGBuildableSubsystem* System = AFGBuildableSubsystem::Get(GetWorld());
    if (!IsValid(System)) return;

    TSet<const AActor*> ActiveNodes;
    for (AFGBuildable* Built : System->GetAllBuildablesRef())
    {
        AFGBuildableResourceExtractor* Extractor = Cast<AFGBuildableResourceExtractor>(Built);
        if (!IsValid(Extractor) || !Extractor->IsProducing() ||
            !IsValid(Extractor->GetResourceNode())) continue;

        const FString Name = Extractor->GetClass()->GetName();
        const bool IsMiner = Name.Contains(TEXT("Miner"), ESearchCase::IgnoreCase);
        const bool IsOilPump = Name.Contains(TEXT("OilPump"), ESearchCase::IgnoreCase) ||
                               Name.Contains(TEXT("OilExtractor"), ESearchCase::IgnoreCase);

        if (IsMiner)
        {
            ActiveNodes.Add(Extractor->GetResourceNode());
            const float Rate = Extractor->GetExtractionPerMinute();
            if (Rate >= 600.0f) ValidateChallengeForAllPlayers(TEXT("P4_C19"));
            if (Rate >= 780.0f) ValidateChallengeForAllPlayers(TEXT("P5_C21"));
        }
        if (IsOilPump)
        {
            // GetFlowRateSmoothed retourne un debit fluide en m3/s.
            const float ActualM3PerMinute = Extractor->GetFlowRateSmoothed() * 60.0f;
            if (ActualM3PerMinute >= 300.0f)
                ValidateChallengeForAllPlayers(TEXT("P3_C24"));
        }
    }
    if (ActiveNodes.Num() >= 4) ValidateChallengeForAllPlayers(TEXT("P1_C22"));
    if (ActiveNodes.Num() >= 6) ValidateChallengeForAllPlayers(TEXT("P1_C23"));
}
void ABingoSubsystem::BingoCheckSharedResourceNode()
{
    if (!HasAuthority() || !GetWorld()) return;
    AFGBuildableSubsystem* System = AFGBuildableSubsystem::Get(GetWorld());
    if (!IsValid(System)) return;
    TArray<const AFGBuildable*> Miners, Constructors, Smelters;
    for (AFGBuildable* B : System->GetAllBuildablesRef())
    {
        if (!IsValid(B)) continue;
        const FString Name = B->GetClass()->GetName();
        if (B->IsA(AFGBuildableResourceExtractor::StaticClass()) &&
            Name.Contains(TEXT("Miner"), ESearchCase::IgnoreCase)) Miners.Add(B);
        if (Name.Contains(TEXT("Constructor"), ESearchCase::IgnoreCase)) Constructors.Add(B);
        if (Name.Contains(TEXT("Smelter"), ESearchCase::IgnoreCase)) Smelters.Add(B);
    }
    bool TwoConstructors = false, TwoSmelters = false;
    BingoCheckSharedNodeMachines(Miners, Constructors, TwoConstructors);
    BingoCheckSharedNodeMachines(Miners, Smelters, TwoSmelters);
    if (TwoConstructors) ValidateChallengeForAllPlayers(TEXT("P1_C5"));
    if (TwoSmelters) ValidateChallengeForAllPlayers(TEXT("P1_C15"));
}
void ABingoSubsystem::BingoCheckProductionChains()
{
    if (!HasAuthority() || !GetWorld()) return;

    AFGBuildableSubsystem* System =
        AFGBuildableSubsystem::Get(GetWorld());

    if (!IsValid(System)) return;

    TArray<AFGBuildable*> Miners;
    TArray<AFGBuildable*> Smelters;
    TArray<AFGBuildable*> Constructors;
    TArray<AFGBuildable*> Storages;

    for (AFGBuildable* Buildable : System->GetAllBuildablesRef())
    {
        if (!IsValid(Buildable)) continue;

        const FString Name = Buildable->GetClass()->GetName();

        if (Buildable->IsA(AFGBuildableResourceExtractor::StaticClass()) &&
            Name.Contains(TEXT("Miner"), ESearchCase::IgnoreCase))
            Miners.Add(Buildable);

        if (Name.Contains(TEXT("Smelter"), ESearchCase::IgnoreCase))
            Smelters.Add(Buildable);

        if (Name.Contains(TEXT("Constructor"), ESearchCase::IgnoreCase))
            Constructors.Add(Buildable);

        if (Buildable->IsA(AFGBuildableStorage::StaticClass()))
            Storages.Add(Buildable);
    }

    for (AFGBuildable* Miner : Miners)
    {
        AFGBuildableFactory* MinerFactory =
            Cast<AFGBuildableFactory>(Miner);

        if (!IsValid(MinerFactory) || !MinerFactory->IsProducing())
            continue;

        for (AFGBuildable* Smelter : Smelters)
        {
            AFGBuildableFactory* SmelterFactory =
                Cast<AFGBuildableFactory>(Smelter);

            if (!IsValid(SmelterFactory) ||
                !SmelterFactory->IsProducing() ||
                !BingoChainLinks(Miner, Smelter))
                continue;

            for (AFGBuildable* Constructor : Constructors)
            {
                AFGBuildableFactory* ConstructorFactory =
                    Cast<AFGBuildableFactory>(Constructor);

                if (!IsValid(ConstructorFactory) ||
                    !ConstructorFactory->IsProducing() ||
                    !BingoChainLinks(Smelter, Constructor))
                    continue;

                for (AFGBuildable* Storage : Storages)
                {
                    if (BingoChainLinks(Constructor, Storage))
                    {
                        UE_LOG(LogTemp, Display,
                            TEXT("[Bingo] P1_C17 : chaine complete et machines actives"));

                        ValidateChallengeForAllPlayers(TEXT("P1_C17"));
                        return;
                    }
                }
            }
        }
    }
}
void ABingoSubsystem::BingoCheckGlobalConnections()
{
    if (!HasAuthority() || !GetWorld()) return;
    AFGBuildableSubsystem* System = AFGBuildableSubsystem::Get(GetWorld());
    if (!IsValid(System)) return;
    bool Merger3 = false;
    bool Splitter3 = false;
    for (AFGBuildable* Built : System->GetAllBuildablesRef())
    {
        BingoCheckJunctionConnections(Built, Merger3, Splitter3);
        if (Merger3 && Splitter3) break;
    }
    if (Merger3) ValidateChallengeForAllPlayers(TEXT("P1_C10"));
    if (Splitter3) ValidateChallengeForAllPlayers(TEXT("P1_C11"));
}
void ABingoSubsystem::BingoCheckElectricity()
{
    // BINGO_COLLECTIBLES_PENDING: ne plus valider les defis de ramassage
    // par la taille de l'inventaire. Un ramassage authentifie sera necessaire.
    BingoCheckGlobalConnections();
    BingoCheckProductionChains();
    BingoCheckSharedResourceNode();
    BingoCheckActiveResourceNodes();
    if (!HasAuthority() || !GetWorld())
    {
        return;
    }

    AFGBuildableSubsystem* BuildableSystem =
        AFGBuildableSubsystem::Get(GetWorld());
    if (!IsValid(BuildableSystem))
    {
        return;
    }

    // Additionne la production observee sur chaque circuit unique,
    // pas la capacite theorique des generateurs.
    TSet<UFGPowerCircuit*> SeenCircuits;
    float TotalProducedMW = 0.0f;

    for (AFGBuildable* Buildable : BuildableSystem->GetAllBuildablesRef())
    {
        if (!IsValid(Buildable))
        {
            continue;
        }

        UFGPowerInfoComponent* PowerInfo =
            Buildable->FindComponentByClass<UFGPowerInfoComponent>();
        if (!IsValid(PowerInfo))
        {
            continue;
        }

        UFGPowerCircuit* Circuit = PowerInfo->GetPowerCircuit();
        if (!IsValid(Circuit) || SeenCircuits.Contains(Circuit))
        {
            continue;
        }
        SeenCircuits.Add(Circuit);

        FPowerCircuitStats Stats;
        Circuit->GetStats(Stats);
        const int32 NumPoints = Stats.GetNumGraphPoints();
        if (NumPoints <= 0)
        {
            continue;
        }

        FPowerGraphPoint LatestPoint;
        if (Stats.GetGraphPointAtIndex(NumPoints - 1, LatestPoint))
        {
            TotalProducedMW += FMath::Max(0.0f, LatestPoint.Produced);
        }
    }

    struct FBingoPowerThreshold
    {
        const TCHAR* ChallengeId;
        float RequiredMW;
    };

    const FBingoPowerThreshold Thresholds[] =
    {
        { TEXT("P1_C1"),   100.0f },
        { TEXT("P1_C2"),   200.0f },
        { TEXT("P2_C5"),  1000.0f },
        { TEXT("P2_C6"),  2000.0f },
        { TEXT("P3_C6"),  3000.0f },
        { TEXT("P3_C7"),  4000.0f },
        { TEXT("P4_C9"),  5000.0f },
        { TEXT("P4_C10"), 6000.0f },
        { TEXT("P5_C12"),12000.0f },
        { TEXT("P5_C14"),15000.0f }
    };

    for (const FBingoPowerThreshold& Threshold : Thresholds)
    {
        if (TotalProducedMW > Threshold.RequiredMW)
        {
            ValidateChallenge(FString(Threshold.ChallengeId));
        }
    }
}

void ABingoSubsystem::BingoOnResearchCompleted(TSubclassOf<UFGSchematic> Schematic)
{
    if (!HasAuthority() || !Schematic)
    {
        return;
    }

    const FString SchematicKey = GetNameSafe(Schematic.Get());
    if (!BingoUniqueResearchSchematics.Contains(SchematicKey))
    {
        BingoUniqueResearchSchematics.Add(SchematicKey);
        // BINGO_LOT_06_RESEARCH_FIXED_V2
    BingoResearchCompletions = BingoUniqueResearchSchematics.Num();
    }

    UE_LOG(LogTemp, Display,
        TEXT("[Bingo][Research] Recherche terminée : %s | Total session : %d"),
        *GetNameSafe(Schematic.Get()), BingoResearchCompletions);

    // La validation automatique d'une case n'est volontairement pas faite ici :
    // il faut confirmer l'identifiant exact du défi et si la case exige une ou deux recherches.
}

void ABingoSubsystem::BingoOnSchematicPurchased(TSubclassOf<UFGSchematic> Schematic)
{
    if (!HasAuthority() || !Schematic)
    {
        return;
    }

    BingoUniquePurchasedSchematics.Add(GetNameSafe(Schematic.Get()));

    UE_LOG(LogTemp, Display,
        TEXT("[Bingo][Shop] Schéma acheté : %s"),
        *GetNameSafe(Schematic.Get()));

    // Un achat de schéma ne prouve pas à lui seul le nombre de bons dépensés.
}

void ABingoSubsystem::BingoOnSchematicPurchasedByPlayer(
    TSubclassOf<UFGSchematic> Schematic,
    AFGCharacterPlayer* Player)
{
    if (!HasAuthority() || !Schematic || !IsValid(Player))
    {
        return;
    }

    UE_LOG(LogTemp, Display,
        TEXT("[Bingo][Shop] Achat attribué au joueur : %s"),
        *GetNameSafe(Schematic.Get()));
}

void ABingoSubsystem::BingoSendPersonalProgress()
{
    // BINGO_PERSONAL_NETWORK_SEND_V1
    if (!HasAuthority() || !GetWorld())
    {
        return;
    }

    BingoEnsurePlayerNetworkActors();
    BingoInitializeConnectedPlayers();
    BingoSyncPlayerProgress();

    for (TActorIterator<AFGCharacterPlayer> It(GetWorld()); It; ++It)
    {
        AFGCharacterPlayer* Player = *It;

        if (!IsValid(Player))
            continue;

        AController* Controller = Player->GetController();

        if (!IsValid(Controller))
            continue;

        AFGPlayerState* PlayerState =
            Cast<AFGPlayerState>(Player->GetPlayerState());

        if (!IsValid(PlayerState))
            continue;

        const FString PlayerId = BingoGetPlayerId(PlayerState);

        if (PlayerId.IsEmpty())
            continue;

        const FBingoPlayerProgress* Progress =
            BingoFindPlayerProgress(PlayerId);

        if (!Progress)
            continue;

        for (ABingoRadarPlayerActor* NetworkActor : RadarPlayerActors)
        {
            if (!IsValid(NetworkActor) ||
                NetworkActor->GetOwner() != Controller)
            {
                continue;
            }

            NetworkActor->Client_ReceiveBingoProgress(
                Progress->CurrentPhase,
                Progress->Points,
                Progress->CompletedChallenges
            );

            break;
        }
    }
}
void ABingoSubsystem::BingoEnsurePlayerNetworkActors()
{
    // BINGO_PLAYER_NETWORK_ACTORS_V1
    if (!HasAuthority() || !GetWorld())
        return;

    for (TActorIterator<AFGCharacterPlayer> It(GetWorld()); It; ++It)
    {
        AFGCharacterPlayer* Player = *It;

        if (!IsValid(Player))
            continue;

        AController* Controller = Player->GetController();

        if (!IsValid(Controller))
            continue;

        bool bExists = false;

        for (ABingoRadarPlayerActor* Existing : RadarPlayerActors)
        {
            if (IsValid(Existing) && Existing->GetOwner() == Controller)
            {
                bExists = true;
                break;
            }
        }

        if (bExists)
            continue;

        FActorSpawnParameters Params;
        Params.Owner = Controller;

        ABingoRadarPlayerActor* Actor =
            GetWorld()->SpawnActor<ABingoRadarPlayerActor>(
                ABingoRadarPlayerActor::StaticClass(),
                Player->GetActorLocation(),
                FRotator::ZeroRotator,
                Params
            );

        if (IsValid(Actor))
        {
            RadarPlayerActors.Add(Actor);

            UE_LOG(LogTemp, Display,
                TEXT("[Bingo] Acteur reseau cree pour %s"),
                *GetNameSafe(Controller));
        }
    }
}

void ABingoSubsystem::BingoBindPlayerInventories()
{
    if (!HasAuthority() || !GetWorld()) return;

    for (TActorIterator<AFGCharacterPlayer> It(GetWorld()); It; ++It)
    {
        AFGCharacterPlayer* Player = *It;
        if (!IsValid(Player)) continue;

        UFGInventoryComponent* Inventory = Player->GetInventory();
        if (!IsValid(Inventory) || BingoBoundInventories.Contains(Inventory))
            continue;

        Inventory->OnItemAddedDelegate.AddUniqueDynamic(
            this, &ABingoSubsystem::BingoOnPlayerItemAdded);
        BingoBoundInventories.Add(Inventory);
    }
}

void ABingoSubsystem::BingoOnPlayerItemAdded(
    TSubclassOf<UFGItemDescriptor> ItemClass,
    int32 NumAdded,
    UFGInventoryComponent* SourceInventory)
{
    if (!HasAuthority() || !ItemClass || NumAdded <= 0) return;
    const FString Key = GetNameSafe(ItemClass.Get());
    int32& Total = BingoObservedInventoryAdditions.FindOrAdd(Key);
    Total = FMath::Max(0, Total) + NumAdded;
    UE_LOG(LogTemp, Verbose,
        TEXT("[Bingo][PlayerInventoryObserved] Item=%s Added=%d Total=%d Source=%s"),
        *Key, NumAdded, Total, *GetNameSafe(SourceInventory));
    // Attention : ramassage, transfert, recompense et fabrication manuelle
    // peuvent tous ajouter un objet. Aucune validation de defi ici.
}
void ABingoSubsystem::BingoBindExtractorOutputs()
{
    if (!HasAuthority() || !GetWorld()) return;
    for (TActorIterator<AFGBuildableResourceExtractor> It(GetWorld()); It; ++It)
    {
        AFGBuildableResourceExtractor* Extractor = *It;
        if (!IsValid(Extractor)) continue;
        UFGInventoryComponent* Output = Extractor->GetOutputInventory();
        if (!IsValid(Output) || BingoBoundExtractorOutputs.Contains(Output)) continue;
        Output->OnItemAddedDelegate.AddUniqueDynamic(
            this, &ABingoSubsystem::BingoOnExtractorOutputAdded);
        BingoBoundExtractorOutputs.Add(Output);
    }
}

void ABingoSubsystem::BingoOnExtractorOutputAdded(
    TSubclassOf<UFGItemDescriptor> ItemClass,
    int32 NumAdded,
    UFGInventoryComponent* SourceInventory)
{
    if (!HasAuthority() || !ItemClass || NumAdded <= 0 || !IsValid(SourceInventory)) return;
    const FString Key = GetNameSafe(ItemClass.Get());
    // BINGO_BATCH_TELEMETRY_V1 - identify source extractor.
    for (TActorIterator<AFGBuildableResourceExtractor> It(GetWorld()); It; ++It)
    {
        AFGBuildableResourceExtractor* E = *It;
        if (!IsValid(E) || E->GetOutputInventory() != SourceInventory) continue;
        const FString SourceKey = GetNameSafe(E) + TEXT("|") + Key;
        int32& SourceTotal = BingoObservedByExtractor.FindOrAdd(SourceKey);
        SourceTotal = FMath::Max(0, SourceTotal) + NumAdded;
        break;
    }    int32& Total = BingoExtractorOutputObserved.FindOrAdd(Key);
    Total = FMath::Max(0, Total) + NumAdded;
    UE_LOG(LogTemp, Log,
        TEXT("[Bingo][ExtractorOutputObserved] Item=%s Added=%d Total=%d"),
        *Key, NumAdded, Total);
    // Un ajout d'inventaire ne garantit pas une extraction effective.
}
void ABingoSubsystem::BingoBindManufacturerOutputs()
{
    if (!HasAuthority() || !GetWorld()) return;
    for (TActorIterator<AFGBuildableManufacturer> It(GetWorld()); It; ++It)
    {
        AFGBuildableManufacturer* Manufacturer = *It;
        if (!IsValid(Manufacturer)) continue;
        UFGInventoryComponent* Output = Manufacturer->GetOutputInventory();
        if (!IsValid(Output) || BingoBoundManufacturerOutputs.Contains(Output)) continue;
        Output->OnItemAddedDelegate.AddUniqueDynamic(
            this, &ABingoSubsystem::BingoOnManufacturerOutputAdded);
        BingoBoundManufacturerOutputs.Add(Output);
    }
}

void ABingoSubsystem::BingoOnManufacturerOutputAdded(
    TSubclassOf<UFGItemDescriptor> ItemClass,
    int32 NumAdded,
    UFGInventoryComponent* SourceInventory)
{
    if (!HasAuthority() || !ItemClass || NumAdded <= 0 || !IsValid(SourceInventory) || !GetWorld()) return;
    for (TActorIterator<AFGBuildableManufacturer> It(GetWorld()); It; ++It)
    {
        AFGBuildableManufacturer* Machine = *It;
        if (!IsValid(Machine) || Machine->GetOutputInventory() != SourceInventory) continue;
        const TSubclassOf<UFGRecipe> Recipe = Machine->GetCurrentRecipe();
        if (!Recipe) return;
        bool bMatchesRecipe = false;
        for (const FItemAmount& Product : UFGRecipe::GetProducts(Recipe))
        {
            if (Product.ItemClass == ItemClass && Product.Amount > 0)
            {
                bMatchesRecipe = true;
                break;
            }
        }
        if (!bMatchesRecipe) return;
        // BINGO_P1_C21_SAFE_OUTPUT_V3
        // Observer la recette Bois -> Biomasse sans attribuer
        // de points sur un simple ajout dans l'inventaire.
        // Un cycle de fabrication termine devra etre confirme
        // par une source distincte et fiable.
        if (Machine->IsProducing() &&
            BingoIsWoodBiomassRecipe(Machine, Recipe))
        {
            UE_LOG(LogTemp, Verbose,
                TEXT("[Bingo][P1_C21] Constructeur bois/biomasse observe : %s"),
                *GetNameSafe(Machine));
        }
        const FString Key = GetNameSafe(ItemClass.Get());
        const FString SourceKey = GetNameSafe(Machine) + TEXT("|") +
            GetNameSafe(Recipe.Get()) + TEXT("|") + Key;
        int32& SourceTotal = BingoObservedByMachineAndRecipe.FindOrAdd(SourceKey);
        SourceTotal = FMath::Max(0, SourceTotal) + NumAdded;        int32& Total = BingoRecipeOutputObserved.FindOrAdd(Key);
        Total = FMath::Max(0, Total) + NumAdded;
        UE_LOG(LogTemp, Log,
            TEXT("[Bingo][RecipeOutputCount] Machine=%s Recipe=%s Item=%s Added=%d TotalObserved=%d Producing=%d"),
            *GetNameSafe(Machine), *GetNameSafe(Recipe.Get()),
            *Key, NumAdded, Total, Machine->IsProducing() ? 1 : 0);
        // Ne pas valider de defi ici : un transfert manuel peut aussi remplir la sortie.
        return;
    }
}
// BINGO_BATCH_RELIABILITY_V1
// BINGO_LOT_03_PRODUCTION_ACTIVE_V1
// Une recette choisie ne suffit pas : la machine doit etre effectivement en production.
// Cette detection valide une automatisation active, pas un simple objet dans l'inventaire.
static bool BingoActiveRecipeProduces(const AFGBuildableManufacturer* Machine, const TCHAR* ItemFragment)
{
    if (!IsValid(Machine) || !Machine->IsProducing()) return false;
    const TSubclassOf<UFGRecipe> Recipe = Machine->GetCurrentRecipe();
    if (!Recipe) return false;
    for (const FItemAmount& Product : UFGRecipe::GetProducts(Recipe))
    {
        if (!Product.ItemClass || Product.Amount <= 0) continue;
        const FString ItemName = GetNameSafe(Product.ItemClass.Get());
        if (ItemName.Contains(ItemFragment, ESearchCase::IgnoreCase)) return true;
    }
    return false;
}
static bool BingoActiveRecipeUsesAndProduces(const AFGBuildableManufacturer* Machine,
    const TCHAR* IngredientFragment, const TCHAR* ProductFragment)
{
    if (!BingoActiveRecipeProduces(Machine, ProductFragment)) return false;
    const TSubclassOf<UFGRecipe> Recipe = Machine->GetCurrentRecipe();
    if (!Recipe) return false;
    for (const FItemAmount& Ingredient : UFGRecipe::GetIngredients(Machine, Recipe))
    {
        if (Ingredient.ItemClass && Ingredient.Amount > 0 &&
            GetNameSafe(Ingredient.ItemClass.Get()).Contains(IngredientFragment, ESearchCase::IgnoreCase))
            return true;
    }
    return false;
}
void ABingoSubsystem::BingoCheckActiveManufacturingRecipes()
{
    if (!HasAuthority() || !GetWorld()) return;
    for (TActorIterator<AFGBuildableManufacturer> It(GetWorld()); It; ++It)
    {
        AFGBuildableManufacturer* Machine = *It;
        if (!IsValid(Machine) || !Machine->IsProducing()) continue;
        const FString ClassName = Machine->GetClass()->GetName();
        if (ClassName.Contains(TEXT("Blender"), ESearchCase::IgnoreCase))
            ValidateChallengeForAllPlayers(TEXT("P4_C15"));
        if (ClassName.Contains(TEXT("Refinery"), ESearchCase::IgnoreCase) &&
            BingoActiveRecipeUsesAndProduces(Machine, TEXT("Bauxite"), TEXT("AluminaSolution")))
            ValidateChallengeForAllPlayers(TEXT("P4_C18"));
        if (BingoActiveRecipeProduces(Machine, TEXT("TimeCrystal")))
            ValidateChallengeForAllPlayers(TEXT("P5_C3"));
        if (BingoActiveRecipeProduces(Machine, TEXT("Diamond")))
            ValidateChallengeForAllPlayers(TEXT("P5_C4"));
        if (BingoActiveRecipeProduces(Machine, TEXT("FicsiteIngot")))
            ValidateChallengeForAllPlayers(TEXT("P5_C5"));
        if (BingoActiveRecipeProduces(Machine, TEXT("FicsiteTrigon")))
            ValidateChallengeForAllPlayers(TEXT("P5_C6"));
        if (BingoActiveRecipeProduces(Machine, TEXT("NuclearPasta")))
            ValidateChallengeForAllPlayers(TEXT("P5_C7"));
    }
}
void ABingoSubsystem::BingoRefreshInventoryBindings()
{
    
    // BINGO_INIT_PLAYERS_CALL_V1
    BingoInitializeConnectedPlayers();

    // BINGO_PERSONAL_AUTO_SYNC_V1
    BingoSendPersonalProgress();
if (!HasAuthority()) return;
    if (BingoWoodBiomassProduced >= 500)
        ValidateChallengeForAllPlayers(TEXT("P1_C21"));

    // BINGO_P1_C14_WORKBENCH_TRACE_V1
    // Diagnostic uniquement : ne valide pas le defi sur une simple production.
    for (TObjectIterator<UFGWorkBench> It; It; ++It)
    {
        UFGWorkBench* Bench = *It;

        if (!IsValid(Bench) ||
            Bench->GetWorld() != GetWorld() ||
            !IsValid(Bench->GetWorkBenchUser()) ||
            !Bench->IsProducing())
        {
            continue;
        }

        UE_LOG(LogTemp, Display,
            TEXT("[Bingo][P1_C14] Atelier actif - Recette=%s Progression=%.3f"),
            *GetNameSafe(Bench->GetCurrentRecipe().Get()),
            Bench->GetManufacturingProgress());
    }
BingoEnsurePlayerNetworkActors();
    BingoBindPlayerInventories();
    BingoBindManufacturerOutputs();
    BingoCheckActiveManufacturingRecipes();
    // BINGO_LOT12_EXTRACTOR_REFRESH
    BingoBindExtractorOutputs();
    // BINGO_RECHECK_BUILDS_V1
    // Recheck existing structures on save load and phase changes (10s poll).
    // Null player + empty actors is supported: the count pass precedes the player check.

OnBingoActorsConstructed(nullptr, TArray<AActor*>());
}
// BINGO_REAL_BATCH_OCT31_V1
void ABingoSubsystem::BingoOnMapAreaVisited(TSubclassOf<UFGMapArea> MapArea)
{
    if (!HasAuthority() || !MapArea) return;
    UE_LOG(LogTemp, Display, TEXT("[Bingo][Exploration] Zone visitee : %s (classe a classifier)"),
        *GetNameSafe(MapArea.Get()));
    // BINGO_P1_EXPLORATION_CLASSIFIED_V1
    // Only engine-reported map-area visits; never infer exploration from inventory or position.
    // Conservative class-name classification. Unrecognized areas remain unvalidated.
    const FString BingoP1AreaClass = GetNameSafe(MapArea.Get());
    if (BingoP1AreaClass.Contains(TEXT("Cave"), ESearchCase::IgnoreCase) ||
        BingoP1AreaClass.Contains(TEXT("Grotte"), ESearchCase::IgnoreCase))
    {
        ValidateChallenge(TEXT("P1_C24"));
    }
    if (BingoP1AreaClass.Contains(TEXT("Coast"), ESearchCase::IgnoreCase) ||
        BingoP1AreaClass.Contains(TEXT("Coastal"), ESearchCase::IgnoreCase) ||
        BingoP1AreaClass.Contains(TEXT("Beach"), ESearchCase::IgnoreCase) ||
        BingoP1AreaClass.Contains(TEXT("Plage"), ESearchCase::IgnoreCase))
    {
        ValidateChallenge(TEXT("P1_C25"));
    }
    const FString AreaKey = GetNameSafe(MapArea.Get());
    int32& Visited = BingoVisitedAreaEvents.FindOrAdd(AreaKey);
    Visited = FMath::Max(0, Visited) + 1;    // Unique class, not an inferred biome, cave or coast achievement.
    const bool bFirstVisitOfThisArea = !BingoDistinctVisitedMapAreas.Contains(AreaKey);
    BingoDistinctVisitedMapAreas.Add(AreaKey);
    // BINGO_GROUPED_REAL_MAP_V1
    // P5_C22 est maintenant un defi de collecte de spheres de Mercer.
    // Ne plus le valider lors d'une visite de zone.
    // The engine reports this actual area as visited. It was not observed before.
    // This does not classify the area as a cave or coast.
    // P5_C22 : pas de validation sur simple visite de zone non classifiee.
    UE_LOG(LogTemp, Display, TEXT("[Bingo][MapAreas] Zones uniques observees=%d"),
        BingoDistinctVisitedMapAreas.Num());
    // No automatic challenge validation without an exact area-to-challenge mapping.
}
// BINGO_GROS_LOT_V3

