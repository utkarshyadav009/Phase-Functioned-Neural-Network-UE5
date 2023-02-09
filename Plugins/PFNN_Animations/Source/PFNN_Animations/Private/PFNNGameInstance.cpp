// Fill out your copyright notice in the Description page of Project Settings.


#include "PFNNGameInstance.h"
#include "PFNNDataContainer.h"

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
	if(PFNNDataContainer)
	{
		(new FAutoDeleteAsyncTask<FPFNNDataLoader>(PFNNDataContainer))->StartBackgroundTask();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Attempting to load PFNNData into uninitialized DataContainer Object"));
	}
}