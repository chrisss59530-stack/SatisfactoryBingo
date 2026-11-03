#pragma once

#include "CoreMinimal.h"
#include "Module/GameWorldModule.h"
#include "BingoGameWorldModule.generated.h"

UCLASS(Blueprintable)
class SATISFACTORYBINGO_API UBingoGameWorldModule : public UGameWorldModule
{
    GENERATED_BODY()

public:
    UBingoGameWorldModule();
    virtual void DispatchLifecycleEvent(ELifecyclePhase Phase) override;

private:
    void TryUnlockBingoTerminal();

    FTimerHandle UnlockTimerHandle;
    int32 UnlockAttempts = 0;
};
