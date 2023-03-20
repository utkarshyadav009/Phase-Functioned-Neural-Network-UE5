// Copyright 2018 Sticks & Stones. All Rights Reserved.

#include "Animation/AnimInstances/PFNNAnimInstance.h"

#include "Core/Character/PFNNCharacter.h"

#include "DrawDebugHelpers.h"

void UPFNNAnimInstance::NativeInitializeAnimation() 
{
	Super::NativeInitializeAnimation();
	APFNNCharacter* OwningCharacter = Cast<APFNNCharacter>(TryGetPawnOwner());
	if (OwningCharacter)
	{
		OwningTrajectoryComponent = OwningCharacter->GetTrajectoryComponent();
	}

}

void UPFNNAnimInstance::NativeUpdateAnimation(float arg_DeltaTimeX)
{
	Super::NativeUpdateAnimation(arg_DeltaTimeX);

	APFNNCharacter* OwningCharacter = Cast<APFNNCharacter>(TryGetPawnOwner());
	if (OwningCharacter) 
	{
		OwningTrajectoryComponent = OwningCharacter->GetTrajectoryComponent();
	}
}

UTrajectoryComponent * UPFNNAnimInstance::GetOwningTrajectoryComponent()
{
	return OwningTrajectoryComponent;
}
