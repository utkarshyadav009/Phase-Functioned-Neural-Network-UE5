

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "PFNNAnimInstance.generated.h"

/**
 * 
 */
UCLASS(transient, Blueprintable, BlueprintType)
class PFNN_API UPFNNAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
	
public:

	virtual void NativeInitializeAnimation() override;

	virtual void NativeUpdateAnimation(float arg_DeltaTimeX) override;

	class UTrajectoryComponent* GetOwningTrajectoryComponent();
	
	void SetLeftLegJointPositions(TArray<FVector>& Arg_JointPoistions);
//	void SetRightLegJointTransform(FTransform);

	/*
	Blueprint Getter Functions for location and rotation of the joint 
	*/
	UFUNCTION(BlueprintCallable, Category = "PFNN")
		void GetPositionForLeftLegJoints(FVector& LHipJoint, FVector& LeftUpLeg, FVector& LeftLeg, FVector& LeftFoot, FVector& LeftToeBase);
	

	UFUNCTION(BlueprintCallable, Category = "PFNN")
		void GetPositionArrayLeftLegJoints(TArray<FVector>& LeftLegBonePositionArray);

	//UFUNCTION(BlueprintCallable, Category = "PFNN")
	//	void GetTransformForRightLegJoints(FTransform& RHipJoint, FTransform& RightUpLeg, FTransform& RightLeg, FTransform& RightFoot, FTransform& RightToeBase);

private:

	//Amount of joints
	enum
	{
		JOINT_NUM = 31,
		LeftLEG_JOINT_NUM = 5
	};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "True"))
	class UTrajectoryComponent* OwningTrajectoryComponent;

	TArray<FVector> LeftLegJointPosition;
	//TArray<FTransform> RightLegJointTransform;
	
};
