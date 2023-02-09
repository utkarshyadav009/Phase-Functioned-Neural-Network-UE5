// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "PFNNGameInstance.generated.h"

/**
 * 
 */
UCLASS()
class PFNN_ANIMATIONS_API UPFNNGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:

	UPFNNGameInstance(const FObjectInitializer& arg_ObjectInitializer);

	class UPFNNDataContainer* GetPFNNDataContainer();

	void LoadPFNNDataAsync();

private:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "True"))
	class UPFNNDataContainer* PFNNDataContainer;
};

