// Copyright 2018 Sticks & Stones. All Rights Reserved.

#include "Core/GameInstance/PFNNGameInstance.h"

#include "Animation/PFNN/PFNNDataContainer.h"

UPFNNGameInstance::UPFNNGameInstance(const FObjectInitializer& arg_ObjectInitializer) : Super(arg_ObjectInitializer) 
{
	PFNNDataContainer = NewObject<UPFNNDataContainer>(this, UPFNNDataContainer::StaticClass(), TEXT("PFNN DataContainer"));

	LoadPFNNDataAsync();

}

UPFNNDataContainer* UPFNNGameInstance::GetPFNNDataContainer() 
{
	return PFNNDataContainer;
}

void UPFNNGameInstance::LoadPFNNDataAsync()
{
	if (PFNNDataContainer)
	{
		(new FAutoDeleteAsyncTask<PFNNDataLoader>(PFNNDataContainer))->StartBackgroundTask();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Attempting to load PFNNData into uninitialized DataContainer Object"));
	}
}