

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
    PFNNJointTransformControlRig.SetNum(18);
    JointNameByIndex.SetNum(18);
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

void UPFNNAnimInstance::SetJointTransformForControlRig(FTransform JointTransform, int index, FName JointName)
{
    switch (index)
    {
    case 3:
        PFNNJointTransformControlRig[0] = JointTransform;
        JointNameByIndex[0] = JointName;
        break;
    case 4:
        PFNNJointTransformControlRig[1] = JointTransform;
        JointNameByIndex[1] = JointName;
        break;
    case 5:
        PFNNJointTransformControlRig[2] = JointTransform;
        JointNameByIndex[2] = JointName;
        break;
    case 6:
        PFNNJointTransformControlRig[3] = JointTransform;
        JointNameByIndex[3] = JointName;
        break;
    case 8:
        PFNNJointTransformControlRig[4] = JointTransform;
        JointNameByIndex[4] = JointName;
        break;
    case 9:
        PFNNJointTransformControlRig[5] = JointTransform;
        JointNameByIndex[5] = JointName;
        break;
    case 10:
        PFNNJointTransformControlRig[6] = JointTransform;
        JointNameByIndex[6] = JointName;
        break;
    case 11:
        PFNNJointTransformControlRig[7] = JointTransform;
        JointNameByIndex[7] = JointName;
        break;
    case 13:
        PFNNJointTransformControlRig[8] = JointTransform;
        JointNameByIndex[8] = JointName;
        break;
    case 14:
        PFNNJointTransformControlRig[9] = JointTransform;
        JointNameByIndex[9] = JointName;
        break;
    case 15:
        PFNNJointTransformControlRig[10] = JointTransform;
        JointNameByIndex[10] = JointName;
        break;
    case 17:
        PFNNJointTransformControlRig[11] = JointTransform;
        JointNameByIndex[11] = JointName;
        break;
    case 19:
        PFNNJointTransformControlRig[12] = JointTransform;
        JointNameByIndex[12] = JointName;
        break;
    case 20:
        PFNNJointTransformControlRig[13] = JointTransform;
        JointNameByIndex[13] = JointName;
        break;
    case 21:
        PFNNJointTransformControlRig[14] = JointTransform;
        JointNameByIndex[14] = JointName;
        break;
    case 26:
        PFNNJointTransformControlRig[15] = JointTransform;
        JointNameByIndex[15] = JointName;
        break;
    case 27:
        PFNNJointTransformControlRig[16] = JointTransform;
        JointNameByIndex[16] = JointName;
        break;
    case 28:
        PFNNJointTransformControlRig[17] = JointTransform;
        JointNameByIndex[17] = JointName;
        break;
    default:
        // Do nothing for any other index value
        break;
    }
}
void UPFNNAnimInstance::LogJointTransform()
{
    for (int i = 0; i < PFNNJointTransformControlRig.Num(); i++)
    {
        FTransform& Transform = PFNNJointTransformControlRig[i];
        UE_LOG(LogTemp, Warning, TEXT("%s BoneTransform %d: Location=(%f, %f, %f), Rotation=(%f, %f, %f), Scale=(%f, %f, %f)"),*JointNameByIndex[i].ToString(),
            i, Transform.GetLocation().X, Transform.GetLocation().Y, Transform.GetLocation().Z,
            Transform.GetRotation().Euler().X, Transform.GetRotation().Euler().Y, Transform.GetRotation().Euler().Z,
            Transform.GetScale3D().X, Transform.GetScale3D().Y, Transform.GetScale3D().Z);
    }
}
FVector UPFNNAnimInstance::GetRootLocation()
{
	// Get the owning actor of the animation instance
	AActor* Owner = GetOwningActor();
	if (Owner)
	{
		// Cast the owning actor to a pawn
		APawn* OwnerPawn = Cast<APawn>(Owner);
		if (OwnerPawn)
		{
			// Get the pawn's root component and return its location
			USceneComponent* RootComponent = OwnerPawn->GetRootComponent();
			if (RootComponent)
			{
				return RootComponent->GetComponentLocation();
			}
		}
	}

	// If the root component or owning pawn is not valid, return the zero vector
	return FVector::ZeroVector;
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

void UPFNNAnimInstance::GetLeftLegJointTransform(FTransform& LeftUpLeg, FTransform& LeftLeg, FTransform& LeftFoot, FTransform& LeftToeBase)
{
    LeftUpLeg    = PFNNJointTransformControlRig[0];
    LeftLeg      = PFNNJointTransformControlRig[1];
    LeftFoot     = PFNNJointTransformControlRig[2];
    LeftToeBase  = PFNNJointTransformControlRig[3];
}

void UPFNNAnimInstance::GetRightLegJointTransform(FTransform& RightUpLeg, FTransform& RightLeg, FTransform& RightFoot, FTransform& RightToeBase)
{
    RightUpLeg   = PFNNJointTransformControlRig[4];
    RightLeg     = PFNNJointTransformControlRig[5];
    RightFoot    = PFNNJointTransformControlRig[6];
    RightToeBase = PFNNJointTransformControlRig[7];
}

void UPFNNAnimInstance::GetLeftArmJointTransform(FTransform& LeftArm, FTransform& LeftForeArm, FTransform& LeftHand)
{
    LeftArm     = PFNNJointTransformControlRig[15];
    LeftForeArm = PFNNJointTransformControlRig[16];
    LeftHand    = PFNNJointTransformControlRig[17];
}

void UPFNNAnimInstance::GetRightArmJointTransform(FTransform& RightArm, FTransform& RightForeArm, FTransform& RightHand)
{
    RightArm     = PFNNJointTransformControlRig[12];
    RightForeArm = PFNNJointTransformControlRig[13];
    RightHand    = PFNNJointTransformControlRig[14];
}

void UPFNNAnimInstance::GetSpineJointTransform(FTransform& Spine, FTransform& Spine1, FTransform& Neck, FTransform& Head)
{
    Spine   = PFNNJointTransformControlRig[8];
    Spine1  = PFNNJointTransformControlRig[9];
    Neck    = PFNNJointTransformControlRig[10];
    Head    = PFNNJointTransformControlRig[11];
}


