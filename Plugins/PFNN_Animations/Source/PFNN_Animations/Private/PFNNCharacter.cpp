// Fill out your copyright notice in the Description page of Project Settings.

#include "PFNNCharacter.h"

// Sets default values
APFNNCharacter::APFNNCharacter()
	: bIsSkeletonDebuggingEnabled(false)
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	TrajectoryComponent = CreateDefaultSubobject<UTrajectoryComponent>(TEXT("TrajectoryComponent"));
}

UTrajectoryComponent* APFNNCharacter::GetTrajectoryComponent()
{
	return TrajectoryComponent;
}

bool APFNNCharacter::HasDebuggingEnabled()
{
	return bIsSkeletonDebuggingEnabled;
}

// Called when the game starts or when spawned
void APFNNCharacter::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void APFNNCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Called to bind functionality to input
void APFNNCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

