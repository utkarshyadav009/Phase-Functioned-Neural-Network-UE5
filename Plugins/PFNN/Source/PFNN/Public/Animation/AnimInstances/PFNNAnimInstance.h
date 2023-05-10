

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

	void SetJointTransformForControlRig(FTransform JointTransform, int index, FName JointName);

	FVector GetRootLocation();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PFNN AnimInstance")
	TArray<FTransform> PFNNJointTransformControlRig;
	
	
	TArray<FName> JointNameByIndex;

//	void SetRightLegJointTransform(FTransform);

	/*
	Blueprint Getter Functions for location and rotation of the joint 
	*/
	UFUNCTION(BlueprintCallable, Category = "PFNN")
		void GetPositionForLeftLegJoints(FVector& LHipJoint, FVector& LeftUpLeg, FVector& LeftLeg, FVector& LeftFoot, FVector& LeftToeBase);
	

	UFUNCTION(BlueprintCallable, Category = "PFNN")
		void GetPositionArrayLeftLegJoints(TArray<FVector>& LeftLegBonePositionArray);

	UFUNCTION(BlueprintCallable, Category = "PFNN")
		void GetLeftLegJointTransform(FTransform& LeftUpLeg, FTransform& LeftLeg, FTransform& LeftFoot, FTransform& LeftToeBase);

	UFUNCTION(BlueprintCallable, Category = "PFNN")
		void GetRightLegJointTransform(FTransform& RightUpLeg, FTransform& RightLeg, FTransform& RightFoot, FTransform& RightToeBase);

	UFUNCTION(BlueprintCallable, Category = "PFNN")
		void GetLeftArmJointTransform(FTransform& LeftArm, FTransform& LeftForeArm, FTransform& LeftHand);

	UFUNCTION(BlueprintCallable, Category = "PFNN")
		void GetRightArmJointTransform(FTransform& RightArm, FTransform& RightForeArm, FTransform& RightHand);
	
	UFUNCTION(BlueprintCallable, Category = "PFNN")
		void GetSpineJointTransform(FTransform& Spine, FTransform& Spine1, FTransform& Neck, FTransform& Head);

	//UFUNCTION(BlueprintCallable, Category = "PFNN")
	//	void GetTransformForRightLegJoints(FTransform& RHipJoint, FTransform& RightUpLeg, FTransform& RightLeg, FTransform& RightFoot, FTransform& RightToeBase);
	void LogJointTransform();
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
