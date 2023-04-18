

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
	LeftLegJointPosition.SetNum(5);
	//RightLegJointTransform.SetNum(5);
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


void UPFNNAnimInstance::SetLeftLegJointPositions(TArray<FVector>& Arg_JointPositions)
{
	LeftLegJointPosition = Arg_JointPositions;

	//for (int32 i = 0; i < LeftLEG_JOINT_NUM; i++)
	//{
	//	if (GEngine)
	//		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Red, FString::Printf(TEXT("AnimInst %d: (%f, %f, %f)"), i, LeftLegJointPosition[i].X, LeftLegJointPosition[i].Y, LeftLegJointPosition[i].Z));
	//}
}

void UPFNNAnimInstance::GetPositionForLeftLegJoints(FVector& LHipJoint, FVector& LeftUpLeg, FVector& LeftLeg, FVector& LeftFoot, FVector& LeftToeBase)
{
	LHipJoint = LeftLegJointPosition[0];
	LeftUpLeg = LeftLegJointPosition[1];
	LeftLeg = LeftLegJointPosition[2];
	LeftFoot = LeftLegJointPosition[3];
	LeftToeBase = LeftLegJointPosition[4];
}

void UPFNNAnimInstance::GetPositionArrayLeftLegJoints(TArray<FVector>& LeftLegBonePositionArray)
{
	LeftLegBonePositionArray = LeftLegJointPosition;
}


