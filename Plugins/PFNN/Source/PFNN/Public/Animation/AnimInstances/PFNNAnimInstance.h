

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
	
	void SetLeftLegJointTransform(FTransform);
	void SetRightLegJointTransform(FTransform);

	/*
	Blueprint Getter Functions for location and rotation of the joint 
	*/
	UFUNCTION(BlueprintCallable, Category = "PFNN")
		void GetTransformForLeftLegJoints(FTransform& LHipJoint, FTransform& LeftUpLeg, FTransform& LeftLeg, FTransform& LeftFoot, FTransform& LeftToeBase);
	
	UFUNCTION(BlueprintCallable, Category = "PFNN")
		void GetTransformForRightLegJoints(FTransform& RHipJoint, FTransform& RightUpLeg, FTransform& RightLeg, FTransform& RightFoot, FTransform& RightToeBase);

private:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "True"))
	class UTrajectoryComponent* OwningTrajectoryComponent;

	TArray<FTransform> LeftLegJointTransform;
	TArray<FTransform> RightLegJointTransform;
};
