// Fill out your copyright notice in the Description page of Project Settings.

#include "AnimNode_PFNN.h"

#include "PFNNAnimInstance.h"
#include "TrajectoryComponent.h"
#include "PFNNCharacter.h"
#include "PFNNHelperFunctions.h"
#include "PhaseFunctionNeuralNetwork.h"

#include "Animation/AnimInstanceProxy.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "HAL/PlatformFilemanager.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <ThirdParty/glm/glm.hpp>
#include <ThirdParty/glm/gtx/transform.hpp>
#include <ThirdParty/glm/gtx/euler_angles.hpp>
#include <ThirdParty/glm/gtx/quaternion.hpp>

#include <fstream>

UPhaseFunctionNeuralNetwork* FAnimNode_PFNN::PFNN = nullptr;

FAnimNode_PFNN::FAnimNode_PFNN()
	: bIsPFNNLoaded(false)
	, bNeedToReset(true)
	, FrameCounter(0)
	, PFNNAnimInstance(nullptr)
	, Trajectory(nullptr)
	, Phase(0)
{}

void FAnimNode_PFNN::LoadData(FAnimInstanceProxy* arg_Context)
{
	LoadXForms(arg_Context);
	LoadPFNN();
}

void FAnimNode_PFNN::LoadXForms(FAnimInstanceProxy* arg_Context)
{
#if 0
	FBoneContainer& bones = arg_Context->GetRequiredBones();

	constexpr int32 iStartJoint = 1;

	JOINT_NUM = bones.GetNumBones();
	JointRange.SetNum(nJoint);
	for(int32 i = 0; i < nJoint; i++)
		JointRange[i] = i + iStartJoint;

	JointPosition.SetNum(JOINT_NUM);
	JointVelocitys.SetNum(JOINT_NUM);
	JointRotations.SetNum(JOINT_NUM);

	JointAnimXform.SetNum(JOINT_NUM);
	JointRestXform.SetNum(JOINT_NUM);
	JointMeshXform.SetNum(JOINT_NUM);
	JointGlobalRestXform.SetNum(JOINT_NUM);
	JointGlobalAnimXform.SetNum(JOINT_NUM);
	JointParents.SetNum(JOINT_NUM);

	for(int32 i = 0; i < JOINT_NUM; i++)
	{
		JointParents[i] = bones.GetParentBoneIndex(i);

		const FTransform& transform = bones.GetRefPoseTransform(FCompactPoseBoneIndex(i));
		const auto rot = transform.GetRotation();
		const auto tran = transform.GetLocation();
		const auto scale = transform.GetScale3D();

		//JointRestXform[i] = glm::transpose(glm::mat4(
		//	rot[0][0], rot[1][0], rot[2][0], pos[0],
		//	rot[0][1], rot[1][1], rot[2][1], pos[1],
		//	rot[0][2], rot[1][2], rot[2][2], pos[2],
		//	0, 0, 0, 1));
	}
#else
	constexpr int32 nJoint = 31;
	constexpr int32 iStartJoint = 0;

	JOINT_NUM = nJoint;
	JointRange.SetNum(nJoint);
	for(int32 i = 0; i < nJoint; i++)
		JointRange[i] = i + iStartJoint;

	JointPosition.SetNum(JOINT_NUM);
	JointVelocitys.SetNum(JOINT_NUM);
	JointRotations.SetNum(JOINT_NUM);

	JointAnimXform.SetNum(JOINT_NUM);
	JointRestXform.SetNum(JOINT_NUM);
	JointMeshXform.SetNum(JOINT_NUM);
	JointGlobalRestXform.SetNum(JOINT_NUM);
	JointGlobalAnimXform.SetNum(JOINT_NUM);
	JointParents.SetNum(JOINT_NUM);

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

	const FString RelativePath = FPaths::ProjectDir();
	const FString FullPathParents = RelativePath + FString::Printf(TEXT("Plugins/PFNN_Animations/Content/MachineLearning/PhaseFunctionNeuralNetwork/character_parents.bin"));
	const FString FullPathXforms = RelativePath + FString::Printf(TEXT("Plugins/PFNN_Animations/Content/MachineLearning/PhaseFunctionNeuralNetwork/character_xforms.bin"));

	IFileHandle* FileHandle = PlatformFile.OpenRead(*FullPathParents);
	if(FileHandle == nullptr)
	{
		UE_LOG(PFNN_Logging, Error, TEXT("Fatal error, Failed to load charater parents"));
		return;
	}

	float JointParentsFloat[nJoint];
	FileHandle->Read(reinterpret_cast<uint8*>(JointParentsFloat), sizeof(JointParentsFloat));

	for(int32 i = 0; i < JOINT_NUM; i++)
		JointParents[i - iStartJoint] = static_cast<int>(JointParentsFloat[i - iStartJoint]);
	delete FileHandle;


	FileHandle = PlatformFile.OpenRead(*FullPathXforms);
	if(FileHandle == nullptr)
	{
		UE_LOG(PFNN_Logging, Fatal, TEXT("Fatal error, Failed to load character xforms"));
		return;
	}

	glm::mat4 JointRestXformTemp[31];
	FileHandle->Read(reinterpret_cast<uint8*>(JointRestXformTemp), sizeof(JointRestXformTemp));

	for(int32 i = 0; i < JOINT_NUM; i++)
		JointRestXform[i - iStartJoint] = glm::transpose(JointRestXformTemp[i - iStartJoint]);

	delete FileHandle;
#endif

	// collect bone names
	JointNameByIndex.SetNum(JOINT_NUM);
	const auto& Skeleton = arg_Context->GetSkeleton()->GetReferenceSkeleton();
	for(int32 i = 0; i < JOINT_NUM; i++)
		JointNameByIndex[i] = Skeleton.GetBoneName(i);
}

void FAnimNode_PFNN::LoadPFNN()
{
	if(!bIsPFNNLoaded && Trajectory)
	{
		if(PFNN == nullptr)
			PFNN = NewObject<UPhaseFunctionNeuralNetwork>();
		bIsPFNNLoaded = PFNN->LoadNetworkData(Trajectory->GetOwner());
	}
}

inline FQuat CalculateRotation(const glm::mat3& a)
{
	FQuat q;
	float trace = a[0][0] + a[1][1] + a[2][2]; // I removed + 1.0f; see discussion with Ethan
	if(trace > 0)
	{// I changed M_EPSILON to 0
		float s = 0.5f / sqrtf(trace + 1.0f);
		q.W = 0.25f / s;
		q.X = (a[2][1] - a[1][2]) * s;
		q.Y = (a[0][2] - a[2][0]) * s;
		q.Z = (a[1][0] - a[0][1]) * s;
	}
	else
	{
		if(a[0][0] > a[1][1] && a[0][0] > a[2][2])
		{
			float s = 2.0f * sqrtf(1.0f + a[0][0] - a[1][1] - a[2][2]);
			q.W = (a[2][1] - a[1][2]) / s;
			q.X = 0.25f * s;
			q.Y = (a[0][1] + a[1][0]) / s;
			q.Z = (a[0][2] + a[2][0]) / s;
		}
		else if(a[1][1] > a[2][2])
		{
			float s = 2.0f * sqrtf(1.0f + a[1][1] - a[0][0] - a[2][2]);
			q.W = (a[0][2] - a[2][0]) / s;
			q.X = (a[0][1] + a[1][0]) / s;
			q.Y = 0.25f * s;
			q.Z = (a[1][2] + a[2][1]) / s;
		}
		else
		{
			float s = 2.0f * sqrtf(1.0f + a[2][2] - a[0][0] - a[1][1]);
			q.W = (a[1][0] - a[0][1]) / s;
			q.X = (a[0][2] + a[2][0]) / s;
			q.Y = (a[1][2] + a[2][1]) / s;
			q.Z = 0.25f * s;
		}
	}
	return q;
}

void FAnimNode_PFNN::ApplyPFNN()
{
#pragma region pre_render
	Trajectory->TickTrajectory();

	glm::vec3 RootPosition = Trajectory->GetRootPosition();
	glm::mat3 RootRotation = Trajectory->GetRootRotation();

	//Input trajectiory positions and directions
	const int w = UTrajectoryComponent::LENGTH / 10;
	for(int i = 0; i < UTrajectoryComponent::LENGTH; i += 10)
	{
		const glm::vec3 Position = glm::inverse(RootRotation) * (Trajectory->Positions[i] - RootPosition);
		const glm::vec3 Direction = glm::inverse(RootRotation) * Trajectory->Directions[i];
		PFNN->Xp((w * 0) + i / 10) = Position.x;
		PFNN->Xp((w * 1) + i / 10) = Position.z;
		PFNN->Xp((w * 2) + i / 10) = Direction.x;
		PFNN->Xp((w * 3) + i / 10) = Direction.z;
	}

	// Input trajectory gaits
	for(int i = 0; i < UTrajectoryComponent::LENGTH; i += 10)
	{
		PFNN->Xp((w * 4) + i / 10) = Trajectory->GaitStand[i];
		PFNN->Xp((w * 5) + i / 10) = Trajectory->GaitWalk[i];
		PFNN->Xp((w * 6) + i / 10) = Trajectory->GaitJog[i];
		PFNN->Xp((w * 7) + i / 10) = Trajectory->GaitCrouch[i]; 
		PFNN->Xp((w * 8) + i / 10) = Trajectory->GaitJump[i];
		PFNN->Xp((w * 9) + i / 10) = 0; //Unused input
	}

	//Input previous join position / velocity / rotations
	const glm::vec3 PreviousRootPosition = Trajectory->GetPreviousRootPosition();
	const glm::mat3 PreviousRootRotation = Trajectory->GetPreviousRootRotation();
	auto InversedPreviousRootRotation = glm::inverse(PreviousRootRotation);
	const int oP = ((UTrajectoryComponent::LENGTH / 10) * 10);
	for(int32 i = 0; i < JOINT_NUM; i++)
	{
		const glm::vec3 Position = InversedPreviousRootRotation * (JointPosition[i] - PreviousRootPosition);
		const glm::vec3 Previous = InversedPreviousRootRotation * JointVelocitys[i];

		//Magical numbers are indexes for the PFNN
		PFNN->Xp(oP + (JOINT_NUM * 3 * 0) + i * 3 + 0) = Position.x;
		PFNN->Xp(oP + (JOINT_NUM * 3 * 0) + i * 3 + 1) = Position.y;
		PFNN->Xp(oP + (JOINT_NUM * 3 * 0) + i * 3 + 2) = Position.z;
		PFNN->Xp(oP + (JOINT_NUM * 3 * 1) + i * 3 + 0) = Previous.x;
		PFNN->Xp(oP + (JOINT_NUM * 3 * 1) + i * 3 + 1) = Previous.y;
		PFNN->Xp(oP + (JOINT_NUM * 3 * 1) + i * 3 + 2) = Previous.z;
	}

	//Input heights for the trajectory
	const float DistanceOffsetHeight = 100.f;
	const float DistanceOffsetFloor = 450.f;
	const int oJ = (((UTrajectoryComponent::LENGTH) / 10) * 10) + JOINT_NUM * 3 * 2;
	for(int i = 0; i < UTrajectoryComponent::LENGTH; i += 10)
	{
		FCollisionQueryParams CollisionParams = FCollisionQueryParams(FName(TEXT("GroundGeometryTrace")), true, Trajectory->GetOwner());
		CollisionParams.AddIgnoredActor(Trajectory->GetOwner());

		FHitResult LeftOutResult;
		FHitResult RightOutResult;

		const auto LeftStartPoint = Trajectory->Positions[i] * 100.f + (Trajectory->Rotations[i] * glm::vec3(Trajectory->Width));
		const auto RightStartPoint = Trajectory->Positions[i] * 100.f + (Trajectory->Rotations[i] * glm::vec3(-Trajectory->Width));

		FVector ULeftStartPoint = UPFNNHelperFunctions::XYZTranslationToXZY(LeftStartPoint);
		FVector ULeftEndPoint = UPFNNHelperFunctions::XYZTranslationToXZY(LeftStartPoint);
		FVector URightStartPoint = UPFNNHelperFunctions::XYZTranslationToXZY(RightStartPoint);
		FVector URightEndPoint = UPFNNHelperFunctions::XYZTranslationToXZY(RightStartPoint);

		ULeftStartPoint.Z = Trajectory->GetOwner()->GetActorLocation().Z + DistanceOffsetHeight;
		ULeftEndPoint.Z = Trajectory->GetOwner()->GetActorLocation().Z - DistanceOffsetFloor;
		URightStartPoint.Z = Trajectory->GetOwner()->GetActorLocation().Z + DistanceOffsetHeight;
		URightEndPoint.Z = Trajectory->GetOwner()->GetActorLocation().Z - DistanceOffsetFloor;

		Trajectory->GetOwner()->GetWorld()->LineTraceSingleByChannel(
			LeftOutResult,
			ULeftStartPoint,
			ULeftEndPoint,
			ECollisionChannel::ECC_WorldStatic,
			CollisionParams);

		Trajectory->GetOwner()->GetWorld()->LineTraceSingleByChannel(
			RightOutResult,
			URightStartPoint,
			URightEndPoint,
			ECollisionChannel::ECC_WorldStatic,
			CollisionParams);

		glm::vec3 LeftResultLocation = UPFNNHelperFunctions::XZYTranslationToXYZ(LeftOutResult.Location);
		glm::vec3 RightResultLocation = UPFNNHelperFunctions::XZYTranslationToXYZ(RightOutResult.Location);

		//DrawDebugLine(Trajectory->GetOwner()->GetWorld(), ULeftStartPoint, ULeftEndPoint, FColor::Red, false, 0.01f, 0, 1);
		//DrawDebugLine(Trajectory->GetOwner()->GetWorld(), URightStartPoint, URightEndPoint, FColor::Red, false, 0.01f, 0, 1);

		PFNN->Xp(oJ + (w * 0) + (i / 10)) = LeftResultLocation.y * 0.01f;
		PFNN->Xp(oJ + (w * 1) + (i / 10)) = Trajectory->Positions[i].y * 0.01f;
		PFNN->Xp(oJ + (w * 2) + (i / 10)) = RightResultLocation.y * 0.01f;
		/*
		int o = (((Trajectory::LENGTH) / 10) * 10) + Character::JOINT_NUM * 3 * 2;
		int w = (Trajectory::LENGTH) / 10;
		glm::vec3 position_r = trajectory->positions[i] + (trajectory->rotations[i] * glm::vec3(trajectory->width, 0, 0));
		glm::vec3 position_l = trajectory->positions[i] + (trajectory->rotations[i] * glm::vec3(-trajectory->width, 0, 0));

		pfnn->Xp(o + (w * 0) + (i / 10)) = heightmap->sample(glm::vec2(position_r.x, position_r.z)) - root_position.y;
		pfnn->Xp(o + (w * 1) + (i / 10)) = trajectory->positions[i].y - root_position.y;
		pfnn->Xp(o + (w * 2) + (i / 10)) = heightmap->sample(glm::vec2(position_l.x, position_l.z)) - root_position.y;
		*/
	}

	PFNN->Predict(Phase);

	//Build local transformation for the joints
	const int halfLength = UTrajectoryComponent::LENGTH / 2;
	const int32 OPosition = 8 + (((halfLength) / 10) * 4) + (JOINT_NUM * 3 * 0);
	const int32 OVelocity = 8 + (((halfLength) / 10) * 4) + (JOINT_NUM * 3 * 1);
	const int32 ORoation = 8 + (((halfLength) / 10) * 4) + (JOINT_NUM * 3 * 2);
	const auto PFNN_Yp = PFNN->Yp;
	for(int32 i = 0; i < JOINT_NUM; i++)
	{
		const int32 i3 = i * 3;
		glm::vec3 Position = RootRotation * glm::vec3(PFNN_Yp(OPosition + i3 + 0), PFNN_Yp(OPosition + i3 + 1), PFNN_Yp(OPosition + i3 + 2)) + RootPosition;
		glm::vec3 Velocity = RootRotation * glm::vec3(PFNN_Yp(OVelocity + i3 + 0), PFNN_Yp(OVelocity + i3 + 1), PFNN_Yp(OVelocity + i3 + 2));
		glm::mat3 Rotation = RootRotation * glm::toMat3(QuaternionExpression(glm::vec3(PFNN_Yp(ORoation + i3 + 0), PFNN_Yp(ORoation + i3 + 1), PFNN_Yp(ORoation + i3 + 2))));

		/*
		** Blending Between the predicted positions and
		** the previous positions plus the velocities
		** smooths out the motion a bit in the case
		** where the two disagree with each other.
		*/

		JointPosition[i] = glm::mix(JointPosition[i] + Velocity, Position, Trajectory->ExtraJointSmooth);
		JointVelocitys[i] = Velocity;
		JointRotations[i] = Rotation;

		JointGlobalAnimXform[i] = glm::transpose(glm::mat4(
			Rotation[0][0], Rotation[1][0], Rotation[2][0], Position[0],
			Rotation[0][1], Rotation[1][1], Rotation[2][1], Position[1],
			Rotation[0][2], Rotation[1][2], Rotation[2][2], Position[2],
			0, 0, 0, 1));
	}

	// Convert to local space ... yes I know this is inefficient.
	for(int i = 0; i < JOINT_NUM; i++)
	{
		if(i == 0)
			JointAnimXform[i] = JointGlobalAnimXform[i];
		else
			JointAnimXform[i] = glm::inverse(JointGlobalAnimXform[JointParents[i]]) * JointGlobalAnimXform[i];
	}

	//Forward kinematics
	ForwardKinematics();
#pragma endregion

#pragma region post_render
	Trajectory->UpdatePastTrajectory();

	Trajectory->Positions[halfLength] = RootPosition; // TODO: ????

	//Update current trajectory
	const float StandAmount = powf(1.0f - Trajectory->GaitStand[halfLength], 0.25f);
	Trajectory->UpdateCurrentTrajectory(StandAmount, PFNN->Yp);

	{
		//TODO: Add wall logic
		/* Collide with walls */
		//for(int j = 0; j < areas->num_walls(); j++)
		//{
		//	glm::vec2 trjpoint = glm::vec2(trajectory->positions[Trajectory::LENGTH / 2].x, trajectory->positions[Trajectory::LENGTH / 2].z);
		//	glm::vec2 segpoint = segment_nearest(areas->wall_start[j], areas->wall_stop[j], trjpoint);
		//	float segdist = glm::length(segpoint - trjpoint);
		//	if(segdist < areas->wall_width[j] + 100.0)
		//	{
		//		glm::vec2 prjpoint0 = (areas->wall_width[j] + 0.0f) * glm::normalize(trjpoint - segpoint) + segpoint;
		//		glm::vec2 prjpoint1 = (areas->wall_width[j] + 100.0f) * glm::normalize(trjpoint - segpoint) + segpoint;
		//		glm::vec2 prjpoint = glm::mix(prjpoint0, prjpoint1, glm::clamp((segdist - areas->wall_width[j]) / 100.0f, 0.0f, 1.0f));
		//		trajectory->positions[Trajectory::LENGTH / 2].x = prjpoint.x;
		//		trajectory->positions[Trajectory::LENGTH / 2].z = prjpoint.y;
		//	}
		//}
	}

	Trajectory->UpdateFutureTrajectory(PFNN->Yp);

	{
		// TODO: ???
		// Final Bone Transformation
		FinalBoneLocations.Empty();
		FinalBoneRotations.Empty();

		FinalBoneLocations.SetNum(JOINT_NUM);
		FinalBoneRotations.SetNum(JOINT_NUM);
		for(int32 i = 0; i < JOINT_NUM; i++)
		{
			FinalBoneLocations[i] = UPFNNHelperFunctions::XYZTranslationToXZY(JointPosition[i]);


			//FVector UnrealJointRotation = UPFNNHelperFunctions::XYZTranslationToXZY(JointRotations[i]);
			FQuat UnrealJointRotation = CalculateRotation(JointRotations[i]);
			FinalBoneRotations[i] = FQuat(FQuat::MakeFromEuler(FVector::RadiansToDegrees(UnrealJointRotation.Vector())));
		}
	}

	//Phase update
	Phase = fmod(Phase + (StandAmount * 0.9f + 0.1f) * 2.0f * PI * PFNN->Yp(3), 2.0f * PI);
#pragma endregion

	// VisualizePhase
	if(GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Yellow, FString::Printf(TEXT("Phase is %f"), Phase));
	GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Yellow, FString::Printf(TEXT("Phase = %f, Stand Amount =  %f, PFNN.Yp(3) = %f"), Phase, StandAmount, PFNN->Yp(3)));
	UE_LOG(LogTemp, Warning, TEXT("Phase = %f, Stand Amount =  %f, PFNN.Yp(3) = %f"), Phase, StandAmount, PFNN->Yp(3));

/*
	// Perform IK (enter this block at your own risk...) 
	if(options->enable_ik)
	{
		// Get Weights
		glm::vec4 ik_weight = glm::vec4(pfnn->Yp(4 + 0), pfnn->Yp(4 + 1), pfnn->Yp(4 + 2), pfnn->Yp(4 + 3));

		glm::vec3 key_hl = glm::vec3(character->joint_global_anim_xform[Character::JOINT_HEEL_L][3]);
		glm::vec3 key_tl = glm::vec3(character->joint_global_anim_xform[Character::JOINT_TOE_L][3]);
		glm::vec3 key_hr = glm::vec3(character->joint_global_anim_xform[Character::JOINT_HEEL_R][3]);
		glm::vec3 key_tr = glm::vec3(character->joint_global_anim_xform[Character::JOINT_TOE_R][3]);

		key_hl = glm::mix(key_hl, ik->position[IK::HL], ik->lock[IK::HL]);
		key_tl = glm::mix(key_tl, ik->position[IK::TL], ik->lock[IK::TL]);
		key_hr = glm::mix(key_hr, ik->position[IK::HR], ik->lock[IK::HR]);
		key_tr = glm::mix(key_tr, ik->position[IK::TR], ik->lock[IK::TR]);

		ik->height[IK::HL] = glm::mix(ik->height[IK::HL], heightmap->sample(glm::vec2(key_hl.x, key_hl.z)) + ik->heel_height, ik->smoothness);
		ik->height[IK::TL] = glm::mix(ik->height[IK::TL], heightmap->sample(glm::vec2(key_tl.x, key_tl.z)) + ik->toe_height, ik->smoothness);
		ik->height[IK::HR] = glm::mix(ik->height[IK::HR], heightmap->sample(glm::vec2(key_hr.x, key_hr.z)) + ik->heel_height, ik->smoothness);
		ik->height[IK::TR] = glm::mix(ik->height[IK::TR], heightmap->sample(glm::vec2(key_tr.x, key_tr.z)) + ik->toe_height, ik->smoothness);

		key_hl.y = glm::max(key_hl.y, ik->height[IK::HL]);
		key_tl.y = glm::max(key_tl.y, ik->height[IK::TL]);
		key_hr.y = glm::max(key_hr.y, ik->height[IK::HR]);
		key_tr.y = glm::max(key_tr.y, ik->height[IK::TR]);

		// Rotate Hip / Knee

		{
			glm::vec3 hip_l = glm::vec3(character->joint_global_anim_xform[Character::JOINT_HIP_L][3]);
			glm::vec3 knee_l = glm::vec3(character->joint_global_anim_xform[Character::JOINT_KNEE_L][3]);
			glm::vec3 heel_l = glm::vec3(character->joint_global_anim_xform[Character::JOINT_HEEL_L][3]);

			glm::vec3 hip_r = glm::vec3(character->joint_global_anim_xform[Character::JOINT_HIP_R][3]);
			glm::vec3 knee_r = glm::vec3(character->joint_global_anim_xform[Character::JOINT_KNEE_R][3]);
			glm::vec3 heel_r = glm::vec3(character->joint_global_anim_xform[Character::JOINT_HEEL_R][3]);

			ik->two_joint(hip_l, knee_l, heel_l, key_hl, 1.0,
						  character->joint_global_anim_xform[Character::JOINT_ROOT_L],
						  character->joint_global_anim_xform[Character::JOINT_HIP_L],
						  character->joint_global_anim_xform[Character::JOINT_HIP_L],
						  character->joint_global_anim_xform[Character::JOINT_KNEE_L],
						  character->joint_anim_xform[Character::JOINT_HIP_L],
						  character->joint_anim_xform[Character::JOINT_KNEE_L]);

			ik->two_joint(hip_r, knee_r, heel_r, key_hr, 1.0,
						  character->joint_global_anim_xform[Character::JOINT_ROOT_R],
						  character->joint_global_anim_xform[Character::JOINT_HIP_R],
						  character->joint_global_anim_xform[Character::JOINT_HIP_R],
						  character->joint_global_anim_xform[Character::JOINT_KNEE_R],
						  character->joint_anim_xform[Character::JOINT_HIP_R],
						  character->joint_anim_xform[Character::JOINT_KNEE_R]);

			character->forward_kinematics();
		}

		// Rotate Heel 
		{
			const float heel_max_bend_s = 4;
			const float heel_max_bend_u = 4;
			const float heel_max_bend_d = 4;

			glm::vec4 ik_toe_pos_blend = glm::clamp(ik_weight * 2.5f, 0.0f, 1.0f);

			glm::vec3 heel_l = glm::vec3(character->joint_global_anim_xform[Character::JOINT_HEEL_L][3]);
			glm::vec4 side_h0_l = character->joint_global_anim_xform[Character::JOINT_HEEL_L] * glm::vec4(10, 0, 0, 1);
			glm::vec4 side_h1_l = character->joint_global_anim_xform[Character::JOINT_HEEL_L] * glm::vec4(-10, 0, 0, 1);
			glm::vec3 side0_l = glm::vec3(side_h0_l) / side_h0_l.w;
			glm::vec3 side1_l = glm::vec3(side_h1_l) / side_h1_l.w;
			glm::vec3 floor_l = key_tl;

			side0_l.y = glm::clamp(heightmap->sample(glm::vec2(side0_l.x, side0_l.z)) + ik->toe_height, heel_l.y - heel_max_bend_s, heel_l.y + heel_max_bend_s);
			side1_l.y = glm::clamp(heightmap->sample(glm::vec2(side1_l.x, side1_l.z)) + ik->toe_height, heel_l.y - heel_max_bend_s, heel_l.y + heel_max_bend_s);
			floor_l.y = glm::clamp(floor_l.y, heel_l.y - heel_max_bend_d, heel_l.y + heel_max_bend_u);

			glm::vec3 targ_z_l = glm::normalize(floor_l - heel_l);
			glm::vec3 targ_x_l = glm::normalize(side0_l - side1_l);
			glm::vec3 targ_y_l = glm::normalize(glm::cross(targ_x_l, targ_z_l));
			targ_x_l = glm::cross(targ_z_l, targ_y_l);

			character->joint_anim_xform[Character::JOINT_HEEL_L] = mix_transforms(
				character->joint_anim_xform[Character::JOINT_HEEL_L],
				glm::inverse(character->joint_global_anim_xform[Character::JOINT_KNEE_L]) * glm::mat4(
					glm::vec4(targ_x_l, 0),
					glm::vec4(-targ_y_l, 0),
					glm::vec4(targ_z_l, 0),
					glm::vec4(heel_l, 1)), ik_toe_pos_blend.y);

			glm::vec3 heel_r = glm::vec3(character->joint_global_anim_xform[Character::JOINT_HEEL_R][3]);
			glm::vec4 side_h0_r = character->joint_global_anim_xform[Character::JOINT_HEEL_R] * glm::vec4(10, 0, 0, 1);
			glm::vec4 side_h1_r = character->joint_global_anim_xform[Character::JOINT_HEEL_R] * glm::vec4(-10, 0, 0, 1);
			glm::vec3 side0_r = glm::vec3(side_h0_r) / side_h0_r.w;
			glm::vec3 side1_r = glm::vec3(side_h1_r) / side_h1_r.w;
			glm::vec3 floor_r = key_tr;

			side0_r.y = glm::clamp(heightmap->sample(glm::vec2(side0_r.x, side0_r.z)) + ik->toe_height, heel_r.y - heel_max_bend_s, heel_r.y + heel_max_bend_s);
			side1_r.y = glm::clamp(heightmap->sample(glm::vec2(side1_r.x, side1_r.z)) + ik->toe_height, heel_r.y - heel_max_bend_s, heel_r.y + heel_max_bend_s);
			floor_r.y = glm::clamp(floor_r.y, heel_r.y - heel_max_bend_d, heel_r.y + heel_max_bend_u);

			glm::vec3 targ_z_r = glm::normalize(floor_r - heel_r);
			glm::vec3 targ_x_r = glm::normalize(side0_r - side1_r);
			glm::vec3 targ_y_r = glm::normalize(glm::cross(targ_z_r, targ_x_r));
			targ_x_r = glm::cross(targ_z_r, targ_y_r);

			character->joint_anim_xform[Character::JOINT_HEEL_R] = mix_transforms(
				character->joint_anim_xform[Character::JOINT_HEEL_R],
				glm::inverse(character->joint_global_anim_xform[Character::JOINT_KNEE_R]) * glm::mat4(
					glm::vec4(-targ_x_r, 0),
					glm::vec4(targ_y_r, 0),
					glm::vec4(targ_z_r, 0),
					glm::vec4(heel_r, 1)), ik_toe_pos_blend.w);

			character->forward_kinematics();
		}

		// Rotate Toe
		{
			const float toe_max_bend_d = 0;
			const float toe_max_bend_u = 10;

			glm::vec4 ik_toe_rot_blend = glm::clamp(ik_weight * 2.5f, 0.0f, 1.0f);

			glm::vec3 toe_l = glm::vec3(character->joint_global_anim_xform[Character::JOINT_TOE_L][3]);
			glm::vec4 fwrd_h_l = character->joint_global_anim_xform[Character::JOINT_TOE_L] * glm::vec4(0, 0, 10, 1);
			glm::vec4 side_h0_l = character->joint_global_anim_xform[Character::JOINT_TOE_L] * glm::vec4(10, 0, 0, 1);
			glm::vec4 side_h1_l = character->joint_global_anim_xform[Character::JOINT_TOE_L] * glm::vec4(-10, 0, 0, 1);
			glm::vec3 fwrd_l = glm::vec3(fwrd_h_l) / fwrd_h_l.w;
			glm::vec3 side0_l = glm::vec3(side_h0_l) / side_h0_l.w;
			glm::vec3 side1_l = glm::vec3(side_h1_l) / side_h1_l.w;

			fwrd_l.y = glm::clamp(heightmap->sample(glm::vec2(fwrd_l.x, fwrd_l.z)) + ik->toe_height, toe_l.y - toe_max_bend_d, toe_l.y + toe_max_bend_u);
			side0_l.y = glm::clamp(heightmap->sample(glm::vec2(side0_l.x, side0_l.z)) + ik->toe_height, toe_l.y - toe_max_bend_d, toe_l.y + toe_max_bend_u);
			side1_l.y = glm::clamp(heightmap->sample(glm::vec2(side0_l.x, side1_l.z)) + ik->toe_height, toe_l.y - toe_max_bend_d, toe_l.y + toe_max_bend_u);

			glm::vec3 side_l = glm::normalize(side0_l - side1_l);
			fwrd_l = glm::normalize(fwrd_l - toe_l);
			glm::vec3 upwr_l = glm::normalize(glm::cross(side_l, fwrd_l));
			side_l = glm::cross(fwrd_l, upwr_l);

			character->joint_anim_xform[Character::JOINT_TOE_L] = mix_transforms(
				character->joint_anim_xform[Character::JOINT_TOE_L],
				glm::inverse(character->joint_global_anim_xform[Character::JOINT_HEEL_L]) * glm::mat4(
					glm::vec4(side_l, 0),
					glm::vec4(-upwr_l, 0),
					glm::vec4(fwrd_l, 0),
					glm::vec4(toe_l, 1)), ik_toe_rot_blend.y);

			glm::vec3 toe_r = glm::vec3(character->joint_global_anim_xform[Character::JOINT_TOE_R][3]);
			glm::vec4 fwrd_h_r = character->joint_global_anim_xform[Character::JOINT_TOE_R] * glm::vec4(0, 0, 10, 1);
			glm::vec4 side_h0_r = character->joint_global_anim_xform[Character::JOINT_TOE_R] * glm::vec4(10, 0, 0, 1);
			glm::vec4 side_h1_r = character->joint_global_anim_xform[Character::JOINT_TOE_R] * glm::vec4(-10, 0, 0, 1);
			glm::vec3 fwrd_r = glm::vec3(fwrd_h_r) / fwrd_h_r.w;
			glm::vec3 side0_r = glm::vec3(side_h0_r) / side_h0_r.w;
			glm::vec3 side1_r = glm::vec3(side_h1_r) / side_h1_r.w;

			fwrd_r.y = glm::clamp(heightmap->sample(glm::vec2(fwrd_r.x, fwrd_r.z)) + ik->toe_height, toe_r.y - toe_max_bend_d, toe_r.y + toe_max_bend_u);
			side0_r.y = glm::clamp(heightmap->sample(glm::vec2(side0_r.x, side0_r.z)) + ik->toe_height, toe_r.y - toe_max_bend_d, toe_r.y + toe_max_bend_u);
			side1_r.y = glm::clamp(heightmap->sample(glm::vec2(side1_r.x, side1_r.z)) + ik->toe_height, toe_r.y - toe_max_bend_d, toe_r.y + toe_max_bend_u);

			glm::vec3 side_r = glm::normalize(side0_r - side1_r);
			fwrd_r = glm::normalize(fwrd_r - toe_r);
			glm::vec3 upwr_r = glm::normalize(glm::cross(side_r, fwrd_r));
			side_r = glm::cross(fwrd_r, upwr_r);

			character->joint_anim_xform[Character::JOINT_TOE_R] = mix_transforms(
				character->joint_anim_xform[Character::JOINT_TOE_R],
				glm::inverse(character->joint_global_anim_xform[Character::JOINT_HEEL_R]) * glm::mat4(
					glm::vec4(side_r, 0),
					glm::vec4(-upwr_r, 0),
					glm::vec4(fwrd_r, 0),
					glm::vec4(toe_r, 1)), ik_toe_rot_blend.w);

			character->forward_kinematics();
		}

		// Update Locks 
		if((ik->lock[IK::HL] == 0.0) && (ik_weight.y >= ik->threshold))
		{
			ik->lock[IK::HL] = 1.0; ik->position[IK::HL] = glm::vec3(character->joint_global_anim_xform[Character::JOINT_HEEL_L][3]);
			ik->lock[IK::TL] = 1.0; ik->position[IK::TL] = glm::vec3(character->joint_global_anim_xform[Character::JOINT_TOE_L][3]);
		}

		if((ik->lock[IK::HR] == 0.0) && (ik_weight.w >= ik->threshold))
		{
			ik->lock[IK::HR] = 1.0; ik->position[IK::HR] = glm::vec3(character->joint_global_anim_xform[Character::JOINT_HEEL_R][3]);
			ik->lock[IK::TR] = 1.0; ik->position[IK::TR] = glm::vec3(character->joint_global_anim_xform[Character::JOINT_TOE_R][3]);
		}

		if((ik->lock[IK::HL] > 0.0) && (ik_weight.y < ik->threshold))
		{
			ik->lock[IK::HL] = glm::clamp(ik->lock[IK::HL] - ik->fade, 0.0f, 1.0f);
			ik->lock[IK::TL] = glm::clamp(ik->lock[IK::TL] - ik->fade, 0.0f, 1.0f);
		}

		if((ik->lock[IK::HR] > 0.0) && (ik_weight.w < ik->threshold))
		{
			ik->lock[IK::HR] = glm::clamp(ik->lock[IK::HR] - ik->fade, 0.0f, 1.0f);
			ik->lock[IK::TR] = glm::clamp(ik->lock[IK::TR] - ik->fade, 0.0f, 1.0f);
		}
	}
*/

	// Log first frame data
	FrameCounter++;
	if(FrameCounter == 1)
	{
		Trajectory->LogTrajectoryData(FrameCounter);
		LogNetworkData(FrameCounter);
	}
}

glm::quat FAnimNode_PFNN::QuaternionExpression(const glm::vec3 arg_Vector)
{
	float W = glm::length(arg_Vector);

	const glm::quat Quat = W < 0.01 ?
		glm::quat(1.0f, 0.0f, 0.0f, 0.0f) :
		glm::quat(
			cosf(W),
			arg_Vector.x * (sinf(W) / W),
			arg_Vector.y * (sinf(W) / W),
			arg_Vector.z * (sinf(W) / W)
		);
	auto t = glm::normalize(Quat);
	auto r = Quat / sqrtf(powf(Quat.w, 2) + powf(Quat.x, 2) + powf(Quat.y, 2) + powf(Quat.z, 2));
	assert(t == r);
	return r;
}

UPFNNAnimInstance* FAnimNode_PFNN::GetPFNNInstanceFromContext(const FAnimationInitializeContext& arg_Context)
{
	if(FAnimInstanceProxy* AnimProxy = arg_Context.AnimInstanceProxy)
		return Cast<UPFNNAnimInstance>(AnimProxy->GetAnimInstanceObject());
	return nullptr;
}

UPFNNAnimInstance* FAnimNode_PFNN::GetPFNNInstanceFromContext(const FAnimationUpdateContext& arg_Context)
{
	if(FAnimInstanceProxy* AnimProxy = arg_Context.AnimInstanceProxy)
		return Cast<UPFNNAnimInstance>(AnimProxy->GetAnimInstanceObject());
	return nullptr;
}

void FAnimNode_PFNN::Initialize_AnyThread(const FAnimationInitializeContext& arg_Context)
{
	FAnimNode_Base::Initialize_AnyThread(arg_Context);

	PFNNAnimInstance = GetPFNNInstanceFromContext(arg_Context);
	if(!PFNNAnimInstance)
		UE_LOG(LogTemp, Error, TEXT("PFNN Animation node should only be added to a PFNNAnimInstance child class!"));

	if(!bIsPFNNLoaded)
		LoadData(arg_Context.AnimInstanceProxy);

	if(PFNNAnimInstance)
		Trajectory = PFNNAnimInstance->GetOwningTrajectoryComponent();

	bNeedToReset = true;
}

void FAnimNode_PFNN::Update_AnyThread(const FAnimationUpdateContext& arg_Context)
{
	FAnimNode_Base::Update_AnyThread(arg_Context);

	if(!bIsPFNNLoaded)
		LoadData(arg_Context.AnimInstanceProxy);

	if(PFNNAnimInstance)
		Trajectory = PFNNAnimInstance->GetOwningTrajectoryComponent();


	if(Trajectory != nullptr && bIsPFNNLoaded)
	{
		if(bNeedToReset)
			Reset();

		ensure(PFNN != nullptr);
		ApplyPFNN();
	}
}

void FAnimNode_PFNN::Evaluate_AnyThread(FPoseContext& arg_Output)
{
	if(!bIsPFNNLoaded)
		return;
	
	const FTransform& CharacterTransform = arg_Output.AnimInstanceProxy->GetActorTransform();
	if(FinalBoneLocations.Num() < JOINT_NUM 
	   || FinalBoneRotations.Num() < JOINT_NUM)
	{
		UE_LOG(PFNN_Logging, Error, TEXT("PFNN results were not properly applied!"));
		return;
	}

	const auto Bones = arg_Output.Pose.GetBoneContainer();
	for(int32 i = 0; i < JOINT_NUM; i++)
	{
		const FCompactPoseBoneIndex CurrentBoneIndex(i);
		const FCompactPoseBoneIndex ParentBoneIndex(Bones.GetParentBoneIndex(CurrentBoneIndex));

		if(ParentBoneIndex.GetInt() == -1)
		{
			//Do nothing first UE4 root bone skips
			arg_Output.Pose[CurrentBoneIndex].SetRotation(FQuat::MakeFromEuler(FVector::DegreesToRadians(FVector(90.0f, 0.0f, 0.0f))));
		}
		else if(ParentBoneIndex.GetInt() == 0)
		{
			//Root Bone No conversion needed
			arg_Output.Pose[CurrentBoneIndex].SetRotation(FinalBoneRotations[CurrentBoneIndex.GetInt() - 1]);
			arg_Output.Pose[CurrentBoneIndex].SetLocation(FinalBoneLocations[CurrentBoneIndex.GetInt() - 1]);

		}
		else
		{	//Conversion to LocalSpace (hopefully)
			FTransform CurrentBoneTransform = FTransform(FinalBoneRotations[CurrentBoneIndex.GetInt() - 1], FinalBoneLocations[CurrentBoneIndex.GetInt() - 1], FVector::OneVector);
			FTransform ParentBoneTransform = FTransform(FinalBoneRotations[ParentBoneIndex.GetInt() - 1], FinalBoneLocations[ParentBoneIndex.GetInt() - 1], FVector::OneVector);

			FTransform LocalBoneTransform = CurrentBoneTransform.GetRelativeTransform(ParentBoneTransform);

			arg_Output.Pose[CurrentBoneIndex].SetComponents(LocalBoneTransform.GetRotation(), LocalBoneTransform.GetLocation(), LocalBoneTransform.GetScale3D());
			//arg_Output.Pose[CurrentBoneIndex].SetRotation(LocalBoneTransform.GetRotation());
			LocalBoneTransform.SetLocation(LocalBoneTransform.GetRotation().Inverse() * LocalBoneTransform.GetLocation());
		}
	}
	arg_Output.Pose.NormalizeRotations();
#ifdef WITH_EDITOR
	DrawDebugSkeleton(arg_Output);
	DrawDebugBoneVelocity(arg_Output);
#endif
}

void FAnimNode_PFNN::ForwardKinematics()
{
	for(int32 i = 0; i < JOINT_NUM; i++)
	{
		JointGlobalAnimXform[i] = JointAnimXform[i];
		JointGlobalRestXform[i] = JointRestXform[i];
		int j = JointParents[i];
		while(j != -1)
		{
			JointGlobalAnimXform[i] = JointAnimXform[j] * JointGlobalAnimXform[i];
			JointGlobalRestXform[i] = JointRestXform[j] * JointGlobalRestXform[i];
			j = JointParents[j];
		}
		JointMeshXform[i] = JointGlobalAnimXform[i] * glm::inverse(JointGlobalRestXform[i]);
	}
}

void FAnimNode_PFNN::Reset()
{
	Eigen::ArrayXf Yp = PFNN->Ymean;

	const auto posMesh = PFNNAnimInstance->GetSkelMeshComponent()->GetComponentLocation();
	glm::vec3 root_position = glm::vec3(posMesh.X, posMesh.Y, posMesh.Z);
	glm::mat3 root_rotation = glm::identity<glm::mat3>();

	Trajectory->ResetTrajectory(root_position, root_rotation);
	for(int32 i = 0; i < JOINT_NUM; i++)
	{
		const int32 opos = 8 + (((Trajectory->LENGTH / 2) / 10) * 4) + (JOINT_NUM * 3 * 0);
		const int32 ovel = 8 + (((Trajectory->LENGTH / 2) / 10) * 4) + (JOINT_NUM * 3 * 1);
		const int32 orot = 8 + (((Trajectory->LENGTH / 2) / 10) * 4) + (JOINT_NUM * 3 * 2);

		glm::vec3 pos = root_rotation * glm::vec3(Yp(opos + i * 3 + 0), Yp(opos + i * 3 + 1), Yp(opos + i * 3 + 2)) + root_position;
		glm::vec3 vel = root_rotation * glm::vec3(Yp(ovel + i * 3 + 0), Yp(ovel + i * 3 + 1), Yp(ovel + i * 3 + 2));
		glm::mat3 rot = root_rotation * glm::toMat3(QuaternionExpression(glm::vec3(Yp(orot + i * 3 + 0), Yp(orot + i * 3 + 1), Yp(orot + i * 3 + 2))));

		JointPosition[i] = pos;
		JointVelocitys[i] = vel;
		JointRotations[i] = rot;
	}

	Phase = 0.0;

	/*
	// reset IK position 
	ik->position[IK::HL] = glm::vec3(0, 0, 0); ik->lock[IK::HL] = 0; ik->height[IK::HL] = root_position.y;
	ik->position[IK::HR] = glm::vec3(0, 0, 0); ik->lock[IK::HR] = 0; ik->height[IK::HR] = root_position.y;
	ik->position[IK::TL] = glm::vec3(0, 0, 0); ik->lock[IK::TL] = 0; ik->height[IK::TL] = root_position.y;
	ik->position[IK::TR] = glm::vec3(0, 0, 0); ik->lock[IK::TR] = 0; ik->height[IK::TR] = root_position.y;
	*/
	bNeedToReset = false;
}

void FAnimNode_PFNN::LogNetworkData(int arg_FrameCounter)
{
	try
	{
		FString RelativePath = FPaths::ProjectDir();
		const FString FullPath = RelativePath += "PFNN_Network.log";

		std::fstream fs;
		fs.open(*FullPath, std::ios::out);
		if(!fs.is_open())
			throw std::exception();

		fs << "UE4_Network" << std::endl;
		fs << "Network Frame[" << arg_FrameCounter << "]" << std::endl << std::endl;

		fs << "Current Phase: " << Phase << std::endl << std::endl;
		fs << "Joints" << std::endl;
		for(int32 i = 0; i < JOINT_NUM; i++)
		{
			fs << "Joint[" << i << "]" << std::endl;
			fs << "	JointPosition: " << JointPosition[i].x << "X, " << JointPosition[i].y << "Y, " << JointPosition[i].z << "Z" << std::endl;
			fs << "	JointVelocitys: " << JointVelocitys[i].x << "X, " << JointVelocitys[i].y << "Y, " << JointVelocitys[i].z << "Z" << std::endl;

			for(size_t x = 0; x < 3; x++)
			{
				//fs << "	JointRotations:  " << JointRotations[i][x].x << "X, " << JointRotations[i][x].y << ", " << JointRotations[i][x].z << std::endl;
			}

			for(size_t x = 0; x < 3; x++)
			{
				fs << "	JointAnimXform:  " << JointAnimXform[i][x].x << "X, " << JointAnimXform[i][x].y << ", " << JointAnimXform[i][x].z << std::endl;
			}

			for(size_t x = 0; x < 3; x++)
			{
				fs << "	JointRestXform:  " << JointRestXform[i][x].x << "X, " << JointRestXform[i][x].y << ", " << JointRestXform[i][x].z << std::endl;
			}

			for(size_t x = 0; x < 3; x++)
			{
				fs << "	JointMeshXform:  " << JointMeshXform[i][x].x << "X, " << JointMeshXform[i][x].y << ", " << JointMeshXform[i][x].z << std::endl;
			}

			for(size_t x = 0; x < 3; x++)
			{
				fs << "	JointGlobalRestXform:  " << JointGlobalRestXform[i][x].x << "X, " << JointGlobalRestXform[i][x].y << ", " << JointGlobalRestXform[i][x].z << std::endl;
			}

			for(size_t x = 0; x < 3; x++)
			{
				fs << "	JointGlobalAnimXform:  " << JointGlobalAnimXform[i][x].x << "X, " << JointGlobalAnimXform[i][x].y << ", " << JointGlobalAnimXform[i][x].z << std::endl;
			}

			fs << "JoinParents: " << JointParents[i] << std::endl;
		}
		fs << "End Joints" << std::endl << std::endl;

		fs << "FinalLocations" << std::endl;
		for(size_t i = 0; i < FinalBoneLocations.Num(); i++)
		{
			fs << "Bone[" << i << "]" << std::endl;
			fs << "	FinalBoneLocation: " << FinalBoneLocations[i].X << "X, " << FinalBoneLocations[i].Y << "Y, " << FinalBoneLocations[i].Z << "Z" << std::endl;
			fs << "	FinalBoneRotation: " << FinalBoneRotations[i].X << "X, " << FinalBoneRotations[i].Y << "Y, " << FinalBoneRotations[i].Z << "Z, " << FinalBoneRotations[i].W << "W" << std::endl;
		}
		fs << "End FinalLocations" << std::endl;
	}
	catch(std::exception e)
	{
#ifdef WITH_EDITOR
		UE_LOG(LogTemp, Log, TEXT("Failed to log network data"));
#endif
	}
}

void FAnimNode_PFNN::DrawDebugSkeleton(const FPoseContext& arg_Context)
{
	APFNNCharacter* Character = Cast<APFNNCharacter>(Trajectory->GetOwner());
	if(!Character || !Character->HasDebuggingEnabled())
		return;

	const auto& Mesh = Character->GetMesh();
	auto& AnimInstanceProxy = arg_Context.AnimInstanceProxy;
	for(int32 i = 0; i < JOINT_NUM; i++)
	{
		const FRotator BoneRotator = FRotator(FinalBoneRotations[i]);
		const FCompactPoseBoneIndex CurrentBoneIndex(i);
		const FVector CurrentBoneLocation = Mesh->GetBoneLocation(JointNameByIndex[i], EBoneSpaces::WorldSpace);
		const FVector ParentBoneLocation = !CurrentBoneIndex.IsRootBone() 
			? Mesh->GetBoneLocation(JointNameByIndex[JointParents[i]], EBoneSpaces::WorldSpace) 
			: CurrentBoneLocation;

		// draw pivot
		AnimInstanceProxy->AnimDrawDebugCoordinateSystem(CurrentBoneLocation, BoneRotator, 10.0f, false, -1.0f, 0.2);
		
		// draw line from current bone to parent bone
		AnimInstanceProxy->AnimDrawDebugLine(CurrentBoneLocation, ParentBoneLocation, FColor::White, false, -1.f, 1.0f);

		// draw bone vertex
		AnimInstanceProxy->AnimDrawDebugSphere(CurrentBoneLocation, 2.f, 8, FColor::Green, false, -1.0f);

		// draw bone vertex text
		if(const auto& world = Character->GetWorld())
			DrawDebugString(world, CurrentBoneLocation, FString::FromInt(i), nullptr, FColor::Red, -1.0f, false, 1.0f);
	}
}

void FAnimNode_PFNN::DrawDebugBoneVelocity(const FPoseContext& arg_Context)
{
	if(!GEngine)
		return;

	APFNNCharacter* Character = Cast<APFNNCharacter>(Trajectory->GetOwner());
	if(!Character || !Character->HasDebuggingEnabled())
		return;

	const FTransform& CharacterTransform = arg_Context.AnimInstanceProxy->GetActorTransform();
	const auto charLocation = CharacterTransform.GetLocation();
	for(int32 i = 0; i < JOINT_NUM; i++)
	{
		const auto JointPos = FVector(JointPosition[i].x, JointPosition[i].z, JointPosition[i].y);
		const auto JointVelocity = FVector(JointVelocitys[i].x, JointVelocitys[i].z, JointVelocitys[i].y);
		arg_Context.AnimInstanceProxy->AnimDrawDebugLine(charLocation + JointPos, charLocation + JointPos - 10 * JointVelocity, FColor::Yellow, false, -1, 0.5f);
	}
}
