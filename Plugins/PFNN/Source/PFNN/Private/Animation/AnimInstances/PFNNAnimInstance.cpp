

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
	LeftLegJointTransform.SetNum(5);
	RightLegJointTransform.SetNum(5);
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

void UPFNNAnimInstance::SetLeftLegJointTransform(FTransform Arg_JointTransform)
{
	for (int i = 0; i < LeftLegJointTransform.Num(); i++)
	{
		LeftLegJointTransform[i] = Arg_JointTransform;
	}
}

