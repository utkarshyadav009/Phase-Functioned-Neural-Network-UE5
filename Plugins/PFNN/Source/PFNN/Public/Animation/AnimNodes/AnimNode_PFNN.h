

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNodeBase.h"

#include "ThirdParty/glm/glm.hpp"

#include "AnimNode_PFNN.generated.h"

	//Bone definitions
enum
{
	//Root
	JOINT_ROOT = 0,

	//Left leg
	JOINT_ROOT_L = 1,
	JOINT_HIP_L = 2,
	JOINT_KNEE_L = 3,
	JOINT_HEEL_L = 4,
	JOINT_TOE_L = 5,

	//Right leg
	JOINT_ROOT_R = 6,
	JOINT_HIP_R = 7,
	JOINT_KNEE_R = 8,
	JOINT_HEEL_R = 9,
	JOINT_TOE_R = 10,

	//Back to head
	JOINT_BACK = 11,
	JOINT_SPINE_1 = 12,
	JOINT_SPINE_2 = 13,
	JOINT_NECK = 14,
	JOINT_NECK1 = 15,
	JOINT_HEAD = 16,

	//Right arm
	JOINT_SHOULDER_L = 17,
	JOINT_ARM_L = 18,
	JOINT_FOREARM_L = 19,
	JOINT_HAND_L = 20,
	JOINT_FINGER_L = 21,
	JOINT_INDEX_L = 22,
	JOINT_THUMB_L = 23,

	//Left arm
	JOINT_SHOULDER_R = 24,
	JOINT_ARM_R = 25,
	JOINT_FOREARM_R = 26,
	JOINT_HAND_R = 27,
	JOINT_FINGER_R = 28,
	JOINT_INDEX_R = 29,
	JOINT_THUMB_R = 30,
};

class UTrajectoryComponent;
class UPhaseFunctionNeuralNetwork;

/**
 * 
 */
USTRUCT(BlueprintInternalUseOnly)
struct PFNN_API FAnimNode_PFNN : public FAnimNode_Base
{
	GENERATED_USTRUCT_BODY()

	FAnimNode_PFNN();

	void LoadData(FAnimInstanceProxy* arg_Context);
	void LoadXForms(FAnimInstanceProxy* arg_Context);
	void LoadPFNN();

	void ApplyPFNN();
	glm::quat QuaternionExpression(const glm::vec3 arg_Length);

	class UPFNNAnimInstance* GetPFNNInstanceFromContext(const FAnimationInitializeContext& Context);
	class UPFNNAnimInstance* GetPFNNInstanceFromContext(const FAnimationUpdateContext& Context);

	// FAnimNode_Base interface
	/**
	 * Default AnimNode function to initialize bones
	 * @param[in] Context, Bone context
	 */
	virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) 	override;
	/**
	 * Default AnimNode function to cache bones
	 * @param[in] Context, Bone context
	 */
	virtual void CacheBones_AnyThread(const FAnimationCacheBonesContext & arg_Context)	override {}
	/**
	* Default AnimNode function to update bones
	* @param[in] Context, Animation context
	*/
	virtual void Update_AnyThread(const FAnimationUpdateContext & arg_Context) 			override;
	/**
	* Default AnimNode function to evaluate bones
	* @param[in] Context, Bone context
	*/
	virtual void Evaluate_AnyThread(FPoseContext& arg_Output) 							override;
	// End of FAnimNode_Base interface

	void LogNetworkData(int arg_FrameCounter);


	//Amount of joints
	enum
	{
		JOINT_NUM = 31
	};

	// Joints utils
	TArray<FName> JointNameByIndex;
	TArray<int32> JointRange;

	UPROPERTY()
	class UPFNNAnimInstance* PFNNAnimInstance;

	UPROPERTY()
	UTrajectoryComponent* Trajectory = nullptr;

	//LOG THESE VARIABLES

	glm::vec3 JointPosition[JOINT_NUM];
	glm::vec3 JointVelocitys[JOINT_NUM];
	glm::mat3 JointRotations[JOINT_NUM];

	glm::mat4 JointAnimXform[JOINT_NUM];
	glm::mat4 JointRestXform[JOINT_NUM];
	glm::mat4 JointMeshXform[JOINT_NUM];
	glm::mat4 JointGlobalRestXform[JOINT_NUM];
	glm::mat4 JointGlobalAnimXform[JOINT_NUM];
	int JointParents[JOINT_NUM];

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = PFNN)
	float Phase;

	TArray<FVector> FinalBoneLocations;
	TArray<FQuat>	FinalBoneRotations;

	//END LOG THESE VARIABLES

	static UPhaseFunctionNeuralNetwork* PFNN;

	int FrameCounter;

	bool bIsPFNNLoaded;



private:
	void DrawDebugSkeleton(const FPoseContext& arg_Context);
	void DrawDebugBoneVelocity(const FPoseContext& arg_Context);
	void VisualizePhase();

};
