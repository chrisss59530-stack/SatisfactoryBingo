#include "FGInventoryComponent.h"
#include "BingoSubsystem.h"

#include "BingoGameData.h"
#include "FGCharacterPlayer.h"
#include "FGPlayerState.h"
#include "FGResearchManager.h"
#include "FGSchematic.h"
#include "FGBuildableSubsystem.h"
#include "Buildables/FGBuildable.h"
#include "Buildables/FGBuildableFoundation.h"
#include "Buildables/FGBuildableWall.h"
#include "Buildables/FGBuildableConveyorBelt.h"
#include "Buildables/FGBuildablePipeline.h"
#include "Buildables/FGBuildablePipelinePump.h"
#include "Buildables/FGBuildableRailroadTrack.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "UObject/UnrealType.h"
#include "UObject/UObjectIterator.h"
#include "Resources/FGItemDescriptor.h"
#include "FGMapArea.h"
#include "Equipment/FGWeapon.h"
#include "FGRailroadVehicle.h"
#include "FGFreightWagon.h"
#include "FGLocomotive.h"
#include "FGTrain.h"
#include "FGStatisticsSubsystem.h"
#include "FGRecipe.h"
#include "Resources/FGBuildDescriptor.h"
#include "Unlocks/FGUnlockRecipe.h"
#include "Unlocks/FGUnlockGiveItem.h"
#include "FGPlayerController.h"
#include "FGGameState.h"
#include "FGSchematicManager.h"

void ABingoSubsystem::BingoEnsureObjectiveBindings()
{
    if (!HasAuthority() || !GetWorld()) return;
    // Retry subsystem delegates after startup ordering delays, without adding
    // duplicate listeners to counters or validations.
    if (AFGStatisticsSubsystem* Stats = AFGStatisticsSubsystem::Get(GetWorld()))
    {
        if (!Stats->mOnItemManuallyCrafted.IsBoundToObject(this))
            Stats->mOnItemManuallyCrafted.AddUObject(this, &ABingoSubsystem::BingoOnEquipmentCrafted);
        if (!Stats->mOnItemPickedUp.IsBoundToObject(this))
            Stats->mOnItemPickedUp.AddUObject(this, &ABingoSubsystem::BingoOnWorldItemPickedUp);
    }
    if (AFGResearchManager* Manager = AFGResearchManager::Get(GetWorld()))
        Manager->ResearchCompletedDelegate.AddUniqueDynamic(this, &ABingoSubsystem::BingoOnResearchCompleted);
    if (AFGSchematicManager* Manager = AFGSchematicManager::Get(GetWorld()))
    {
        Manager->PurchasedSchematicDelegate.AddUniqueDynamic(this, &ABingoSubsystem::BingoOnSchematicPurchased);
        Manager->PurchasedSchematicInstigatorDelegate.AddUniqueDynamic(this, &ABingoSubsystem::BingoOnSchematicPurchasedByPlayer);
    }
    if (AFGGameState* State = GetWorld()->GetGameState<AFGGameState>())
    {
        State->MapAreaVisistedDelegate.AddUniqueDynamic(this, &ABingoSubsystem::BingoOnMapAreaVisited);
        State->mOnActorsConstructedByPlayer.AddUniqueDynamic(this, &ABingoSubsystem::OnBingoActorsConstructed);
    }
}

void ABingoSubsystem::BingoBeginBlueprintSave(AFGPlayerController* Controller)
{
    if (HasAuthority()) BingoBlueprintSaveContexts.Add(Controller);
}

void ABingoSubsystem::BingoEndBlueprintSave()
{
    if (BingoBlueprintSaveContexts.Num() > 0) BingoBlueprintSaveContexts.Pop();
}

void ABingoSubsystem::BingoConfirmBlueprintWrite()
{
    if (!HasAuthority() || BingoBlueprintSaveContexts.Num() == 0) return;
    AFGPlayerController* Controller = BingoBlueprintSaveContexts.Last().Get();
    if (!IsValid(Controller)) return;
    AFGCharacterPlayer* Player = Cast<AFGCharacterPlayer>(Controller->GetPawn());
    AFGPlayerState* State = IsValid(Player) ? Cast<AFGPlayerState>(Player->GetPlayerState()) : nullptr;
    if (!IsValid(State)) return;
    const FString PlayerId = BingoGetPlayerId(State);
    if (PlayerId.IsEmpty()) return;
    if (FBingoPlayerProgress* Record = BingoFindOrCreatePlayerProgress(PlayerId)) Record->bSavedBlueprintInDesigner = true;
    BingoRecheckPersonalAchievements();
}

void ABingoSubsystem::BingoRecordConveyorUpgrade()
{
    if (!HasAuthority()) return;
    bBingoMk3ToMk5Upgrade = true;
    ValidateChallengeForAllPlayers(TEXT("P4_C1"));
}

void ABingoSubsystem::BingoRecordSafeLanding(AFGCharacterPlayer* Player, bool bJetpackOrParachute, bool bHoverpack)
{
    if (!HasAuthority() || !IsValid(Player)) return;
    AFGPlayerState* State = Cast<AFGPlayerState>(Player->GetPlayerState());
    if (!IsValid(State)) return;
    const FString PlayerId = BingoGetPlayerId(State);
    if (PlayerId.IsEmpty()) return;
    if (FBingoPlayerProgress* Record = BingoFindOrCreatePlayerProgress(PlayerId))
    {
        Record->bSafeJetpackOrParachuteLanding |= bJetpackOrParachute;
        Record->bSafeHoverpackLanding |= bHoverpack;
    }
    BingoRecheckPersonalAchievements();
}

void ABingoSubsystem::BingoRecheckPersonalExploration()
{
    if (!HasAuthority() || !GetWorld()) return;
    for (TActorIterator<AFGCharacterPlayer> It(GetWorld()); It; ++It)
    {
        AFGCharacterPlayer* Player = *It;
        if (!IsValid(Player) || !IsValid(Player->GetController())) continue;
        AFGPlayerState* State = Cast<AFGPlayerState>(Player->GetPlayerState());
        if (!IsValid(State)) continue;
        const TArray<TSubclassOf<UFGMapArea>>* Areas = State->GetPlayerVisitedAreas();
        if (!Areas) continue;
        TSet<FString> DistinctAreas;
        bool bVisitedSpireCoast = false;
        for (const TSubclassOf<UFGMapArea>& Area : *Areas)
        {
            if (!Area) continue;
            const FString Path = Area->GetPathName();
            if (Path.StartsWith(TEXT("/Game/FactoryGame/Interface/UI/Minimap/MapAreaPersistenLevel/")))
                DistinctAreas.Add(Path);
            if (Path == TEXT("/Game/FactoryGame/Interface/UI/Minimap/MapAreaPersistenLevel/Area_SpireCoast.Area_SpireCoast_C"))
                bVisitedSpireCoast = true;
        }
        if (DistinctAreas.Num() >= 2) ValidateChallengeForPlayer(TEXT("P1_C24"), State);
        if (bVisitedSpireCoast) ValidateChallengeForPlayer(TEXT("P1_C25"), State);
    }
}

void ABingoSubsystem::BingoCheckEquipmentAndTrains()
{
    if (!HasAuthority() || !GetWorld()) return;
    for (TActorIterator<AFGCharacterPlayer> It(GetWorld()); It; ++It)
    {
        AFGCharacterPlayer* Player = *It;
        if (!IsValid(Player) || !IsValid(Player->GetController())) continue;
        AFGPlayerState* State = Cast<AFGPlayerState>(Player->GetPlayerState());
        if (!IsValid(State)) continue;
        for (AFGEquipment* Equipment : Player->GetActiveEquipments())
        {
            AFGWeapon* Weapon = Cast<AFGWeapon>(Equipment);
            if (IsValid(Weapon) && Weapon->GetCurrentAmmo() > 0)
                ValidateChallengeForPlayer(TEXT("P3_C23"), State);
        }
    }

    TSet<AFGTrain*> PoweredTrains;
    TMap<AFGTrain*, int32> Wagons;
    for (TActorIterator<AFGLocomotive> It(GetWorld()); It; ++It)
        if (IsValid(*It) && IsValid(It->GetTrain())) PoweredTrains.Add(It->GetTrain());
    for (TActorIterator<AFGFreightWagon> It(GetWorld()); It; ++It)
        if (IsValid(*It) && IsValid(It->GetTrain())) ++Wagons.FindOrAdd(It->GetTrain());
    for (const auto& Entry : Wagons)
        if (Entry.Value >= 2 && PoweredTrains.Contains(Entry.Key))
        {
            ValidateChallengeForAllPlayers(TEXT("P3_C14"));
            break;
        }
}

void ABingoSubsystem::BingoObserveFactoryProduction(
    const TMap<TSubclassOf<UFGItemDescriptor>, uint64>& Totals,
    const TMap<TSubclassOf<UFGItemDescriptor>, int64>& Rates)
{
    if (!HasAuthority() || !IsInGameThread()) return;
    for (const auto& Entry : Totals)
    {
        if (!Entry.Key) continue;
        const int64 Total = static_cast<int64>(FMath::Min<uint64>(Entry.Value, MAX_int64));
        int64& Recorded = BingoConfirmedProductionTotals.FindOrAdd(Entry.Key->GetPathName());
        // Repeated snapshots, reconnections and restored game totals cannot
        // count the same production twice.
        Recorded = FMath::Max(Recorded, Total);
    }
    BingoLastProductionRates.Empty();
    for (const auto& Entry : Rates)
        if (Entry.Key) BingoLastProductionRates.Add(Entry.Key->GetPathName(), FMath::Max<int64>(0, Entry.Value));
    BingoLastProductionRateTime = GetWorld() ? GetWorld()->GetTimeSeconds() : -1.0;
    BingoCheckConfirmedProduction();
}

void ABingoSubsystem::BingoCheckConfirmedProduction()
{
    // BINGO_PERSONAL_INVENTORY_V1
    if (!HasAuthority() || !GetWorld()) return;

    struct FInventoryGoal
    {
        const TCHAR* ClassName;
        const TCHAR* Challenge;
        int32 Required;
    };

    const FInventoryGoal Goals[] = {
        {TEXT("Desc_Diamond_C"), TEXT("P5_C1"), 500},
        {TEXT("Desc_PressureConversionCube_C"), TEXT("P5_C2"), 500},
        {TEXT("Desc_CrystalOscillator_C"), TEXT("P4_C22"), 10},
        {TEXT("Desc_Motor_C"), TEXT("P3_C25"), 20},
        {TEXT("Desc_MotorLightweight_C"), TEXT("P4_C23"), 10},
        {TEXT("Desc_Diamond_C"), TEXT("P5_C23"), 1},
        {TEXT("Desc_FicsiteIngot_C"), TEXT("P5_C24"), 1},
        {TEXT("Desc_SpaceElevatorPart_12_C"), TEXT("P5_C25"), 1},
        {TEXT("Desc_OreBauxite_C"), TEXT("P4_C20"), 200},
        {TEXT("Desc_OreBauxite_C"), TEXT("P4_C21"), 400}
    };

    TMap<FName, TSubclassOf<UFGItemDescriptor>> Descriptors;
    for (TObjectIterator<UClass> It; It; ++It)
    {
        if (It->IsChildOf(UFGItemDescriptor::StaticClass()) &&
            It->GetPathName().StartsWith(TEXT("/Game/FactoryGame/")))
        {
            Descriptors.Add(It->GetFName(), *It);
        }
    }

    for (TActorIterator<AFGCharacterPlayer> It(GetWorld()); It; ++It)
    {
        AFGCharacterPlayer* Player = *It;
        if (!IsValid(Player) || !IsValid(Player->GetController())) continue;

        AFGPlayerState* State = Cast<AFGPlayerState>(Player->GetPlayerState());
        UFGInventoryComponent* Inventory = Player->GetInventory();
        if (!IsValid(State) || !IsValid(Inventory)) continue;

        for (const FInventoryGoal& Goal : Goals)
        {
            const TSubclassOf<UFGItemDescriptor> Descriptor =
                Descriptors.FindRef(FName(Goal.ClassName));

            if (Descriptor && Inventory->GetNumItems(Descriptor) >= Goal.Required)
                ValidateChallengeForPlayer(Goal.Challenge, State);
        }
    }

    if (bBingoMk3ToMk5Upgrade)
        ValidateChallengeForAllPlayers(TEXT("P4_C1"));
}
void ABingoSubsystem::BingoRecheckPersonalCollections()
{
    if (!HasAuthority() || !GetWorld()) return;
    AFGStatisticsSubsystem* Stats = AFGStatisticsSubsystem::Get(GetWorld());
    if (!BingoHardDriveDescriptor || !BingoMercerDescriptor)
        for (TObjectIterator<UClass> It; It; ++It)
        {
            UClass* Candidate = *It;
            if (!Candidate->IsChildOf(UFGItemDescriptor::StaticClass()) ||
                !Candidate->GetPathName().StartsWith(TEXT("/Game/FactoryGame/"))) continue;
            if (Candidate->GetFName() == TEXT("Desc_HardDrive_C")) BingoHardDriveDescriptor = Candidate;
            if (Candidate->GetFName() == TEXT("Desc_WAT2_C")) BingoMercerDescriptor = Candidate;
        }
    for (TActorIterator<AFGCharacterPlayer> It(GetWorld()); It; ++It)
    {
        AFGCharacterPlayer* Player = *It;
        if (!IsValid(Player) || !IsValid(Player->GetController())) continue;
        AFGPlayerState* State = Cast<AFGPlayerState>(Player->GetPlayerState());
        if (!IsValid(State)) continue;
        const FString PlayerId = BingoGetPlayerId(State);
        if (PlayerId.IsEmpty() || !BingoGetOrCreatePlayerGameData(PlayerId)) continue;
        FBingoPlayerProgress* Record = BingoFindOrCreatePlayerProgress(PlayerId);
        if (!Record) continue;
        // Reconcile already-saved official pickups and late delegate delivery.
        // Carry historical mod progress forward without replaying increments.
        if (IsValid(Stats))
        {
            if (BingoHardDriveDescriptor)
                Record->HardDrivesCollected = FMath::Max(Record->HardDrivesCollected,
                    Stats->Stat_GetNumItemsPickedUp(BingoHardDriveDescriptor, State));
            if (BingoMercerDescriptor)
                Record->MercerSpheresCollected = FMath::Max(Record->MercerSpheresCollected,
                    Stats->Stat_GetNumItemsPickedUp(BingoMercerDescriptor, State));
        }
        // Do not retain a pointer into the save array during validation/sync.
        const int32 Drives = Record->HardDrivesCollected;
        const int32 Spheres = Record->MercerSpheresCollected;
        if (Drives >= 1) ValidateChallengeForPlayer(TEXT("P1_C12"), State);
        if (Drives >= 10) ValidateChallengeForPlayer(TEXT("P3_C3"), State);
        if (Drives >= 20) ValidateChallengeForPlayer(TEXT("P4_C5"), State);
        if (Drives >= 30) ValidateChallengeForPlayer(TEXT("P5_C9"), State);
        if (Spheres >= 10) ValidateChallengeForPlayer(TEXT("P5_C22"), State);
    }
}

void ABingoSubsystem::BingoCheckGlobalHardDriveResearch()
{
    if (!HasAuthority() || !GetWorld()) return;
    if (AFGResearchManager* Manager = AFGResearchManager::Get(GetWorld()))
    {
        // The game's saved ID counter increments for every completed drive
        // stored in the library, independently of choosing or rerolling rewards.
        const FIntProperty* Counter = FindFProperty<FIntProperty>(
            Manager->GetClass(), TEXT("mLastUsedHardDriveID"));
        if (Counter)
        {
            int32 Completed = FMath::Max(0, Counter->GetPropertyValue_InContainer(Manager));
            TArray<TSubclassOf<UFGSchematic>> Pending;
            Manager->GetAllCompletedResearch(Pending);
            for (const TSubclassOf<UFGSchematic>& Research : Pending)
            {
                if (Research && UFGSchematic::GetType(Research) == ESchematicType::EST_HardDrive)
                    ++Completed;
            }
            BingoHardDriveAnalyses = FMath::Max(BingoHardDriveAnalyses, Completed);
        }
    }
    if (BingoHardDriveAnalyses >= 5) ValidateChallengeForAllPlayers(TEXT("P4_C2"));
    if (BingoHardDriveAnalyses >= 6) ValidateChallengeForAllPlayers(TEXT("P4_C3"));
}

void ABingoSubsystem::BingoCheckCountedBuilds()
{
    if (!HasAuthority() || !GetWorld()) return;
    AFGBuildableSubsystem* Buildings = AFGBuildableSubsystem::Get(GetWorld());
    if (!IsValid(Buildings)) return;
    int32 Foundations = 0, Walls = 0, Belts = 0, Pipes = 0, Pumps = 0, Rails = 0;
    int32 Mk1Containers = 0, Mk2Containers = 0, Shops = 0, TruckStations = 0;
    for (AFGBuildable* Built : Buildings->GetAllBuildablesRef())
    {
        if (!IsValid(Built) || Built->IsBuildableInsideBlueprintDesigner()) continue;
        const FString ClassName = Built->GetClass()->GetName();
        if (Built->IsA(AFGBuildableFoundation::StaticClass())) ++Foundations;
        if (Built->IsA(AFGBuildableWall::StaticClass())) ++Walls;
        if (Built->IsA(AFGBuildableConveyorBelt::StaticClass())) ++Belts;
        if (Built->IsA(AFGBuildablePipeline::StaticClass())) ++Pipes;
        if (Built->IsA(AFGBuildablePipelinePump::StaticClass())) ++Pumps;
        if (Built->IsA(AFGBuildableRailroadTrack::StaticClass())) ++Rails;
        if (ClassName.Equals(TEXT("Build_StorageContainerMk1_C"), ESearchCase::IgnoreCase)) ++Mk1Containers;
        if (ClassName.Equals(TEXT("Build_StorageContainerMk2_C"), ESearchCase::IgnoreCase)) ++Mk2Containers;
        if (ClassName.Equals(TEXT("Build_ResourceSinkShop_C"), ESearchCase::IgnoreCase)) ++Shops;
        if (ClassName.Equals(TEXT("Build_TruckStation_C"), ESearchCase::IgnoreCase)) ++TruckStations;
    }
    if (Foundations >= 20) ValidateChallengeForAllPlayers(TEXT("P2_C4"));
    if (Shops >= 2) ValidateChallengeForAllPlayers(TEXT("P2_C7"));
    if (Mk1Containers >= 2) ValidateChallengeForAllPlayers(TEXT("P2_C9"));
    if (Mk1Containers >= 4) ValidateChallengeForAllPlayers(TEXT("P2_C11"));
    if (Walls >= 20) ValidateChallengeForAllPlayers(TEXT("P2_C15"));
    if (Belts >= 10) ValidateChallengeForAllPlayers(TEXT("P2_C17"));
    if (Pipes >= 20) ValidateChallengeForAllPlayers(TEXT("P2_C18"));
    if (Pumps >= 4) ValidateChallengeForAllPlayers(TEXT("P2_C25"));
    if (Foundations >= 50) ValidateChallengeForAllPlayers(TEXT("P3_C9"));
    if (Rails >= 10) ValidateChallengeForAllPlayers(TEXT("P3_C10"));
    if (TruckStations >= 2) ValidateChallengeForAllPlayers(TEXT("P3_C22"));
    if (Mk2Containers >= 2) ValidateChallengeForAllPlayers(TEXT("P4_C8"));
}

void ABingoSubsystem::BingoRecordShopPurchase(
    TSubclassOf<UFGSchematic> Schematic, AFGCharacterPlayer* Player)
{
    if (!HasAuthority() || !Schematic || !IsValid(Player)) return;
    AFGPlayerState* State = Cast<AFGPlayerState>(Player->GetPlayerState());
    if (!IsValid(State)) return;
    const FString PlayerId = BingoGetPlayerId(State);
    if (PlayerId.IsEmpty()) return;
    int32 Coupons = 0;
    for (const FItemAmount& Cost : UFGSchematic::GetCost(Schematic))
        if (Cost.ItemClass && Cost.ItemClass->GetName().Equals(TEXT("Desc_ResourceSinkCoupon_C"), ESearchCase::IgnoreCase))
            Coupons += FMath::Max(0, Cost.Amount);
    if (Coupons == 0) return;
    FBingoPlayerProgress* Record = BingoFindOrCreatePlayerProgress(PlayerId);
    if (!Record) return;
    const FString SchematicId = Schematic->GetPathName();
    if (!UFGSchematic::IsRepeatPurchasesAllowed(Schematic) && Record->RecordedShopSchematics.Contains(SchematicId)) return;
    Record->RecordedShopSchematics.Add(SchematicId);
    Record->ShopCouponsSpent = static_cast<int32>(FMath::Min<int64>(
        static_cast<int64>(Record->ShopCouponsSpent) + Coupons, MAX_int32));
    for (UFGUnlock* Unlock : UFGSchematic::GetUnlocks(Schematic))
    {
        if (const UFGUnlockGiveItem* Gift = Cast<UFGUnlockGiveItem>(Unlock))
            for (const FItemAmount& Item : Gift->GetItemsToGive())
                if (Item.ItemClass && Item.Amount > 0 && Item.ItemClass->GetName().Contains(TEXT("Cup"), ESearchCase::IgnoreCase))
                    Record->bBoughtCup = true;
        const UFGUnlockRecipe* Recipes = Cast<UFGUnlockRecipe>(Unlock);
        if (!Recipes) continue;
        for (const TSubclassOf<UFGRecipe>& Recipe : Recipes->GetRecipesToUnlock())
        {
            if (!Recipe) continue;
            for (const FItemAmount& Product : UFGRecipe::GetProducts(Recipe))
            {
                if (!Product.ItemClass) continue;
                if (Product.ItemClass->GetName().Contains(TEXT("Cup"), ESearchCase::IgnoreCase)) Record->bBoughtCup = true;
                if (!Product.ItemClass->IsChildOf(UFGBuildDescriptor::StaticClass())) continue;
                const TSubclassOf<UFGBuildDescriptor> Descriptor = Product.ItemClass.Get();
                const UClass* BuildingClass = UFGBuildDescriptor::GetBuildClass(Descriptor).Get();
                if (!BuildingClass) continue;
                if (BuildingClass->IsChildOf(AFGBuildableFoundation::StaticClass())) Record->bBoughtFoundation = true;
                if (BuildingClass->IsChildOf(AFGBuildableWall::StaticClass())) Record->bBoughtWall = true;
            }
        }
    }
    BingoRecheckPersonalAchievements();
}

void ABingoSubsystem::BingoRecordHoverPackCraft(AFGPlayerState* State, const FItemAmount& Crafted)
{
    if (!HasAuthority() || !IsValid(State) || !Crafted.ItemClass ||
        !Crafted.ItemClass->GetName().Contains(TEXT("HoverPack"), ESearchCase::IgnoreCase)) return;
    AFGStatisticsSubsystem* Stats = AFGStatisticsSubsystem::Get(GetWorld());
    if (!IsValid(Stats)) return;
    const FString PlayerId = BingoGetPlayerId(State);
    if (PlayerId.IsEmpty()) return;
    const int32 Count = Stats->Stat_GetNumItemsManuallyCrafted(Crafted.ItemClass, State);
    if (FBingoPlayerProgress* Record = BingoFindOrCreatePlayerProgress(PlayerId))
        Record->HoverPacksCrafted = FMath::Max(Record->HoverPacksCrafted, Count);
    BingoRecheckPersonalAchievements();
}

void ABingoSubsystem::BingoRecheckPersonalAchievements()
{
    if (!HasAuthority() || !GetWorld()) return;
    for (TActorIterator<AFGCharacterPlayer> It(GetWorld()); It; ++It)
    {
        AFGCharacterPlayer* Player = *It;
        if (!IsValid(Player) || !IsValid(Player->GetController())) continue;
        AFGPlayerState* State = Cast<AFGPlayerState>(Player->GetPlayerState());
        if (!IsValid(State)) continue;
        const FString PlayerId = BingoGetPlayerId(State);
        if (PlayerId.IsEmpty() || !BingoGetOrCreatePlayerGameData(PlayerId)) continue;
        const FBingoPlayerProgress* Found = BingoFindPlayerProgress(PlayerId);
        if (!Found) continue;
        const int32 Spent = Found->ShopCouponsSpent, HoverPacks = Found->HoverPacksCrafted;
        const bool Foundation = Found->bBoughtFoundation, Wall = Found->bBoughtWall, Cup = Found->bBoughtCup;
        const bool Blueprint = Found->bSavedBlueprintInDesigner;
        const bool SafeJet = Found->bSafeJetpackOrParachuteLanding, SafeHover = Found->bSafeHoverpackLanding;
        if (Foundation) ValidateChallengeForPlayer(TEXT("P2_C1"), State);
        if (Wall) ValidateChallengeForPlayer(TEXT("P2_C2"), State);
        if (Cup) ValidateChallengeForPlayer(TEXT("P2_C3"), State);
        if (Blueprint) ValidateChallengeForPlayer(TEXT("P2_C14"), State);
        if (SafeJet) ValidateChallengeForPlayer(TEXT("P3_C1"), State);
        if (SafeHover) ValidateChallengeForPlayer(TEXT("P4_C4"), State);
        if (Spent >= 10) ValidateChallengeForPlayer(TEXT("P3_C4"), State);
        if (Spent >= 15) ValidateChallengeForPlayer(TEXT("P3_C5"), State);
        if (Spent >= 20) ValidateChallengeForPlayer(TEXT("P4_C6"), State);
        if (Spent >= 25) ValidateChallengeForPlayer(TEXT("P4_C7"), State);
        if (Spent >= 30) ValidateChallengeForPlayer(TEXT("P5_C10"), State);
        if (Spent >= 35) ValidateChallengeForPlayer(TEXT("P5_C11"), State);
        if (HoverPacks >= 1) ValidateChallengeForPlayer(TEXT("P4_C25"), State);
        if (HoverPacks >= 2) ValidateChallengeForPlayer(TEXT("P4_C24"), State);
    }
}
