#pragma once
#include "CoreMinimal.h"

struct FBingoRewardItem
{
    FString RewardId;
    FString DescriptorName;
    int32 Quantity;
};

class FBingoRewardItems
{
public:
    static TArray<FBingoRewardItem> GetAll()
    {
        return {
            {TEXT("IronPlates"), TEXT("IronPlate"), 100},
            {TEXT("Wires"), TEXT("Wire"), 150},
            {TEXT("Concrete"), TEXT("Cement"), 100},
            {TEXT("Biomass"), TEXT("GenericBiomass"), 200},
            {TEXT("ConstructionKit"), TEXT("IronRod"), 100},
            {TEXT("Rotors"), TEXT("Rotor"), 20},
            {TEXT("ModularFrames"), TEXT("ModularFrame"), 15},
            {TEXT("SteelBeams"), TEXT("SteelPlate"), 100},
            {TEXT("SteelPipes"), TEXT("SteelPipe"), 100},
            {TEXT("AdvancedRotors"), TEXT("Rotor"), 50},
            {TEXT("Stators"), TEXT("Stator"), 30},
            {TEXT("Motors"), TEXT("Motor"), 20},
            {TEXT("Plastic"), TEXT("Plastic"), 100},
            {TEXT("Rubber"), TEXT("Rubber"), 100},
            {TEXT("CircuitBoards"), TEXT("CircuitBoard"), 40},
            {TEXT("HeavyFrames"), TEXT("ModularFrameHeavy"), 20},
            {TEXT("Computers"), TEXT("Computer"), 15},
            {TEXT("HighSpeedConnectors"), TEXT("HighSpeedConnector"), 30},
            {TEXT("Aluminum"), TEXT("AluminumCasing"), 100},
            {TEXT("AluminumSheets"), TEXT("AluminumPlate"), 100},
            {TEXT("AIControl"), TEXT("CircuitBoard"), 50},
            {TEXT("Supercomputers"), TEXT("ComputerSuper"), 10},
            {TEXT("TurboMotors"), TEXT("MotorLightweight"), 10},
            {TEXT("CoolingSystems"), TEXT("CoolingSystem"), 20},
            {TEXT("Batteries"), TEXT("Battery"), 50},
            {TEXT("EliteConnectors"), TEXT("HighSpeedConnector"), 100},
            {TEXT("AdvancedControls"), TEXT("ComputerSuper"), 20},
            {TEXT("FusedFrames"), TEXT("ModularFrameFused"), 30},
            {TEXT("EliteTurboMotors"), TEXT("MotorLightweight"), 20},
            {TEXT("EliteSupercomputers"), TEXT("ComputerSuper"), 25},
            {TEXT("PremiumSupply"), TEXT("ModularFrameHeavy"), 50},
            {TEXT("PioneerUltimate"), TEXT("ComputerQuantum"), 10},
        };
    }

    static bool Find(const FString& RewardId, FBingoRewardItem& OutItem)
    {
        for (const FBingoRewardItem& Item : GetAll())
        {
            if (Item.RewardId == RewardId)
            {
                OutItem = Item;
                return true;
            }
        }
        return false;
    }
};
