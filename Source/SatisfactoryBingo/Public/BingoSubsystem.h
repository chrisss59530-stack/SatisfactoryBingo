#pragma once

#include "CoreMinimal.h"
#include "Subsystem/ModSubsystem.h"
#include "FGSaveInterface.h"
#include "BingoSubsystem.generated.h"

class AFGPlayerState;
class AFGPlayerController;
struct FItemAmount;
class UBingoGameData;
class AFGCharacterPlayer;
class UFGSchematic;
class UFGMapArea;
class UFGItemDescriptor;
class UFGInventoryComponent;
class ABingoRadarPlayerActor;

/**
 * Progression Bingo individuelle, sauvegardee sur le serveur.
 * Chaque joueur possede ses propres points, phases et defis.
 */
// BINGO_PERSONAL_PROGRESS_V1
USTRUCT(BlueprintType)
struct FBingoPlayerProgress
{
    GENERATED_BODY()

    UPROPERTY(SaveGame)
    FString PlayerId;

    UPROPERTY(SaveGame)
    int32 Points = 0;

    UPROPERTY(SaveGame)
    int32 CurrentPhase = 1;

    UPROPERTY(SaveGame)
    TArray<FString> CompletedChallenges;

    // BINGO_PERSONAL_COLLECTIONS_V2
    UPROPERTY(SaveGame)
    int32 HardDrivesCollected = 0;

    UPROPERTY(SaveGame)
    int32 MercerSpheresCollected = 0;

    UPROPERTY(SaveGame)
    int32 ShopCouponsSpent = 0;

    UPROPERTY(SaveGame)
    TSet<FString> RecordedShopSchematics;

    UPROPERTY(SaveGame)
    bool bBoughtFoundation = false;

    UPROPERTY(SaveGame)
    bool bBoughtWall = false;

    UPROPERTY(SaveGame)
    bool bBoughtCup = false;

    UPROPERTY(SaveGame)
    int32 HoverPacksCrafted = 0;

    UPROPERTY(SaveGame)
    bool bSavedBlueprintInDesigner = false;

    UPROPERTY(SaveGame)
    bool bSafeJetpackOrParachuteLanding = false;

    UPROPERTY(SaveGame)
    bool bSafeHoverpackLanding = false;
};
UCLASS()
class SATISFACTORYBINGO_API ABingoSubsystem
    : public AModSubsystem, public IFGSaveInterface
{
    GENERATED_BODY()

public:
    // BINGO_P1_C14_EQUIPMENT_CRAFT_V2
    // Collecte observee via les statistiques officielles du jeu (serveur).
    void BingoOnWorldItemPickedUp(AFGPlayerState* PlayerState, const FItemAmount& PickedUp);

    UPROPERTY(Replicated, SaveGame)
    int32 BingoHardDrivesCollected = 0;

    UPROPERTY(Replicated, SaveGame)
    int32 BingoMercerSpheresCollected = 0;

    void BingoValidateCollectionChallenges();

    void BingoOnEquipmentCrafted(
        AFGPlayerState* PlayerState,
        const FItemAmount& Crafted);


public:
    // BINGO_P1_C21_BIOMASS_WOOD_V1
    // Compteur reserve aux fabrications dont la recette Bois -> Biomasse
    // et l'achevement du cycle ont ete verifies.
    // BINGO_V3_REAL_BIOMASS_PROGRESS_01
    UPROPERTY(Replicated, SaveGame)
    int32 BingoWoodBiomassProduced = 0;


public:
    ABingoSubsystem();
    // BINGO_P1_C21_BIOMASS_COUNTER_V2

    void BingoAddWoodBiomass(int32 Quantity);
    // BINGO_P1_C21_CYCLE_TRACKING_V4
    void BingoCheckWoodBiomassCycles();

    TMap<TWeakObjectPtr<class AFGBuildableManufacturer>, float>
        BingoWoodCycleProgress;

    TMap<TWeakObjectPtr<class AFGBuildableManufacturer>, UClass*>
        BingoWoodCycleRecipes;


    virtual void BeginPlay() override;

    void BingoCheckElectricity();
    void BingoCheckActiveManufacturingRecipes();
    void BingoCheckGlobalConnections();
    void BingoCheckProductionChains();
    void BingoCheckSharedResourceNode();
    void BingoCheckActiveResourceNodes();

    // Observe les ajouts dans les inventaires des joueurs, sans les confondre
    // avec une preuve de fabrication ou de récolte.
    void BingoEnsurePlayerNetworkActors();
    // BINGO_PERSONAL_NETWORK_SEND_V1
    void BingoSendPersonalProgress();
    void BingoBindPlayerInventories();
    // Compteurs d'observation de sorties compatibles avec la recette (pas une preuve de cycle).
    UPROPERTY(SaveGame)
    TMap<FString, int32> BingoRecipeOutputObserved;

    // Ajouts observes dans les inventaires joueurs : source non certifiee.
    UPROPERTY(SaveGame)
    TMap<FString, int32> BingoObservedInventoryAdditions;

    // Inventaires des extracteurs : quantites observees, pas preuve de cycle.
    UPROPERTY(SaveGame)
    TMap<FString, int32> BingoExtractorOutputObserved;
    // BINGO_BATCH_TELEMETRY_V1
    // Observations sauvegardees pour diagnostic; PAS des preuves de production.
    UPROPERTY(SaveGame)
    TMap<FString, int32> BingoObservedByMachineAndRecipe;
    UPROPERTY(SaveGame)
    TMap<FString, int32> BingoObservedByExtractor;
    UPROPERTY(SaveGame)
    TMap<FString, int32> BingoVisitedAreaEvents;

    TSet<TWeakObjectPtr<UFGInventoryComponent>> BingoBoundExtractorOutputs;

    void BingoBindExtractorOutputs();

    UFUNCTION()
    void BingoOnExtractorOutputAdded(
        TSubclassOf<UFGItemDescriptor> ItemClass,
        int32 NumAdded,
        UFGInventoryComponent* SourceInventory);

    void BingoBindManufacturerOutputs();

    UFUNCTION()
    void BingoOnManufacturerOutputAdded(
        TSubclassOf<UFGItemDescriptor> ItemClass,
        int32 NumAdded,
        UFGInventoryComponent* SourceInventory);

    TSet<TWeakObjectPtr<UFGInventoryComponent>> BingoBoundManufacturerOutputs;

    UFUNCTION()
    void BingoOnPlayerItemAdded(
        TSubclassOf<UFGItemDescriptor> ItemClass,
        int32 NumAdded,
        UFGInventoryComponent* SourceInventory);

    TSet<TWeakObjectPtr<UFGInventoryComponent>> BingoBoundInventories;

    UFUNCTION()
void BingoOnResearchCompleted(TSubclassOf<UFGSchematic> Schematic);
    // Distinct observed map areas, saved for later mapping to exact challenges.
    UPROPERTY(SaveGame)
    TSet<FString> BingoDistinctVisitedMapAreas;

    void BingoRefreshInventoryBindings();

    UFUNCTION()
    void BingoOnMapAreaVisited(TSubclassOf<UFGMapArea> MapArea);

    UFUNCTION()
    void BingoOnSchematicPurchased(TSubclassOf<UFGSchematic> Schematic);

    UFUNCTION()
    void BingoOnSchematicPurchasedByPlayer(TSubclassOf<UFGSchematic> Schematic, AFGCharacterPlayer* Player);

    UPROPERTY(SaveGame)
    int32 BingoResearchCompletions = 0;

    UPROPERTY(SaveGame)
    int32 BingoHardDriveAnalyses = 0;
    // Distinct events, recorded by real Satisfactory delegates.
    UPROPERTY(SaveGame)
    TSet<FString> BingoUniqueResearchSchematics;

    UPROPERTY(SaveGame)
    TSet<FString> BingoUniquePurchasedSchematics;

    UFUNCTION()
    void OnBingoActorsConstructed(
        AFGCharacterPlayer* Player,
        TArray<AActor*> ConstructedActors);

    virtual void GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps
    ) const override;

    UFUNCTION(BlueprintPure, Category="Bingo|Progress")
    UBingoGameData* GetBingoGameData() const;

    UFUNCTION(BlueprintPure, Category="Bingo|Progress")
    int32 GetTotalBingoPoints() const;

    UFUNCTION(BlueprintCallable, Category="Bingo|Progress")
    bool ValidateChallenge(const FString& ChallengeId);
    // BINGO_PERSONAL_VALIDATION_V1
    // BINGO_WORLD_VALIDATION_V1
    int32 ValidateChallengeForAllPlayers(
        const FString& ChallengeId);
    bool ValidateChallengeForPlayer(
        const FString& ChallengeId,
        AFGPlayerState* PlayerState);



    UFUNCTION(BlueprintCallable, Category="Bingo|Rewards")
    bool BuyBingoReward(
        const FString& RewardId,
        AFGCharacterPlayer* Player);

    // Called on the game thread with the game's confirmed cumulative factory
    // totals, never from an inventory-add event.
    void BingoObserveFactoryProduction(
        const TMap<TSubclassOf<UFGItemDescriptor>, uint64>& Totals,
        const TMap<TSubclassOf<UFGItemDescriptor>, int64>& Rates);

    void BingoBeginBlueprintSave(AFGPlayerController* Controller);
    void BingoEndBlueprintSave();
    void BingoConfirmBlueprintWrite();
    void BingoRecordConveyorUpgrade();
    void BingoRecordSafeLanding(AFGCharacterPlayer* Player, bool bJetpackOrParachute, bool bHoverpack);
    virtual void PreSaveGame_Implementation(
        int32 SaveVersion, int32 GameVersion) override;

    virtual void PostLoadGame_Implementation(
        int32 SaveVersion, int32 GameVersion) override;

    virtual bool ShouldSave_Implementation() const override;

    virtual bool NeedTransform_Implementation() override;

    virtual void GatherDependencies_Implementation(
        TArray<UObject*>& OutDependencies) override;

private:
    void BingoRecheckPersonalCollections();
    void BingoCheckGlobalHardDriveResearch();
    void BingoCheckCountedBuilds();
    void BingoRecheckPersonalExploration();
    void BingoCheckEquipmentAndTrains();
    void BingoCheckConfirmedProduction();
    void BingoRecordShopPurchase(TSubclassOf<UFGSchematic> Schematic, AFGCharacterPlayer* Player);
    void BingoRecordHoverPackCraft(AFGPlayerState* PlayerState, const FItemAmount& Crafted);
    void BingoRecheckPersonalAchievements();
    void BingoEnsureObjectiveBindings();

    UPROPERTY(SaveGame)
    TMap<FString, int64> BingoConfirmedProductionTotals;

    TMap<FString, int64> BingoLastProductionRates;
    double BingoLastProductionRateTime = -1.0;

    UPROPERTY(SaveGame)
    bool bBingoMk3ToMk5Upgrade = false;

    TArray<TWeakObjectPtr<AFGPlayerController>> BingoBlueprintSaveContexts;

    UPROPERTY(Transient)
    TSubclassOf<UFGItemDescriptor> BingoHardDriveDescriptor;

    UPROPERTY(Transient)
    TSubclassOf<UFGItemDescriptor> BingoMercerDescriptor;

    UPROPERTY(ReplicatedUsing=OnRep_BingoProgress, SaveGame)
    int32 TotalBingoPoints = 0;

    UPROPERTY(ReplicatedUsing=OnRep_BingoProgress, SaveGame)
    int32 SavedCurrentPhase = 1;

    UPROPERTY(ReplicatedUsing=OnRep_BingoProgress, SaveGame)
    TArray<FString> CompletedChallengeIds;


    // BINGO_PLAYER_PROGRESS_STORAGE_V1
    // Progressions individuelles sauvegardees sur le serveur.
    UPROPERTY(SaveGame)
    TArray<FBingoPlayerProgress> PlayerProgressRecords;

    UPROPERTY(Transient)
    TObjectPtr<UBingoGameData> GameData;

    // BINGO_PLAYER_GAME_DATA_V1
    // Grilles individuelles conservees en memoire.
    // Les sauvegardes persistantes restent dans PlayerProgressRecords.
    UPROPERTY(Transient)
    TMap<FString, TObjectPtr<UBingoGameData>> PlayerGameData;
    UFUNCTION()
    void OnRep_BingoProgress();

    // BINGO_PLAYER_ID_V1
    FString BingoGetPlayerId(const AFGPlayerState* PlayerState) const;
    // BINGO_PLAYER_PROGRESS_LOOKUP_V1
    const FBingoPlayerProgress* BingoFindPlayerProgress(
        const FString& PlayerId) const;

    FBingoPlayerProgress* BingoFindOrCreatePlayerProgress(
        const FString& PlayerId);
    // BINGO_INIT_PLAYERS_V1
    void BingoInitializeConnectedPlayers();
    // BINGO_CREATE_PLAYER_GRID_V1
    UBingoGameData* BingoGetOrCreatePlayerGameData(
        const FString& PlayerId);
    // BINGO_SYNC_PLAYER_PROGRESS_V1
    void BingoSyncPlayerProgress();
    void RestoreProgress();
    void EnsureGameData();
    // Radar temporaire : temps d'expiration par joueur.
    TMap<TWeakObjectPtr<AFGCharacterPlayer>, double> RadarExpirationTimes;
    UPROPERTY(Transient)
    TArray<TObjectPtr<ABingoRadarPlayerActor>> RadarPlayerActors;

public:
    bool IsExplorationRadarActive(AFGCharacterPlayer* Player) const;
};



