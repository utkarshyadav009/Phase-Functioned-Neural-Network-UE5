// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "TrajectoryComponent.h"
#include "PFNNCharacter.generated.h"

class UTrajectoryComponent;

UCLASS()
class PFNN_ANIMATIONS_API APFNNCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	APFNNCharacter();

	UTrajectoryComponent* GetTrajectoryComponent();

	bool HasDebuggingEnabled();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

private:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "True"))
	UTrajectoryComponent* TrajectoryComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = PFNNDebugging, meta = (AllowPrivateAccess = "True"))
	bool bIsSkeletonDebuggingEnabled;

};
