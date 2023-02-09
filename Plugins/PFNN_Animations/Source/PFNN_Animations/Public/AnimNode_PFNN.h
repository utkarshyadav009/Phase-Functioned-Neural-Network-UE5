// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNodeBase.h"

#include "ThirdParty/glm/glm.hpp"

#include "AnimNode_PFNN.generated.h"

class UTrajectoryComponent;
class UPhaseFunctionNeuralNetwork;

/**
 * 
 */
USTRUCT(BlueprintInternalUseOnly)
struct PFNN_ANIMATIONS_API FAnimNode_PFNN: public FAnimNode_Base
{
	GENERATED_USTRUCT_BODY()

	// FAnimNode_Base interface
	/**
	 * Default AnimNode function to initialize bones
	 * @param[in] Context, Bone context
	 */
		virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
	/**
	 * Default AnimNode function to cache bones
	 * @param[in] Context, Bone context
	 */
	virtual void CacheBones_AnyThread(const FAnimationCacheBonesContext& arg_Context) override
	{}
	/**
	* Default AnimNode function to update bones
	* @param[in] Context, Animation context
	*/
	virtual void Update_AnyThread(const FAnimationUpdateContext& arg_Context) override;
	/**
	* Default AnimNode function to evaluate bones
	* @param[in] Context, Bone context
	*/
	virtual void Evaluate_AnyThread(FPoseContext& arg_Output) override;
	// End of FAnimNode_Base interface

	FAnimNode_PFNN();

	void LoadData(FAnimInstanceProxy* arg_Context);
	void LoadXForms(FAnimInstanceProxy* arg_Context);
	void LoadPFNN();

	void ApplyPFNN();
	glm::quat QuaternionExpression(const glm::vec3 arg_Length);

	class UPFNNAnimInstance* GetPFNNInstanceFromContext(const FAnimationInitializeContext& Context);
	class UPFNNAnimInstance* GetPFNNInstanceFromContext(const FAnimationUpdateContext& Context);

	void LogNetworkData(int arg_FrameCounter);

	void ForwardKinematics();

	void Reset();

private:
	void DrawDebugSkeleton(const FPoseContext& arg_Context);
	void DrawDebugBoneVelocity(const FPoseContext& arg_Context);

public:
	static UPhaseFunctionNeuralNetwork* PFNN;
	bool bIsPFNNLoaded;
	int32 FrameCounter;
	bool bNeedToReset;

	// Joints utils
	int32 JOINT_NUM;
	TArray<FName> JointNameByIndex;
	TArray<int32> JointRange;

	UPROPERTY()
	class UPFNNAnimInstance* PFNNAnimInstance;

	UPROPERTY()
	UTrajectoryComponent* Trajectory = nullptr;

	//LOG THESE VARIABLES
	TArray<glm::vec3> JointPosition;
	TArray<glm::vec3> JointVelocitys;
	TArray<glm::mat3> JointRotations;
	
	TArray<glm::mat4> JointAnimXform;
	TArray<glm::mat4> JointRestXform;
	TArray<glm::mat4> JointMeshXform;
	TArray<glm::mat4> JointGlobalRestXform;
	TArray<glm::mat4> JointGlobalAnimXform;
	TArray<int32> JointParents;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = PFNN)
	float Phase;

	TArray<FVector> FinalBoneLocations;
	TArray<FQuat> FinalBoneRotations;
	//END LOG THESE VARIABLES
};
