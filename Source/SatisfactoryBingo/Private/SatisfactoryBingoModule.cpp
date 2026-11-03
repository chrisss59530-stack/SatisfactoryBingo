#include "SatisfactoryBingoModule.h"

#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogSatisfactoryBingo, Log, All);

void FSatisfactoryBingoModule::StartupModule()
{
    UE_LOG(LogSatisfactoryBingo, Log, TEXT("Satisfactory Bingo module started"));
}

void FSatisfactoryBingoModule::ShutdownModule()
{
    UE_LOG(LogSatisfactoryBingo, Log, TEXT("Satisfactory Bingo module stopped"));
}

IMPLEMENT_MODULE(FSatisfactoryBingoModule, SatisfactoryBingo);