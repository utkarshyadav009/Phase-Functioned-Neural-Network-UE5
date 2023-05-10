

#include "Animation/AnimNodes/AnimNode_PFNN.h"
#include "Animation/AnimInstances/PFNNAnimInstance.h"
#include "Animation/AnimComponents/TrajectoryComponent.h"
#include "Core/Character/PFNNCharacter.h"
#include "Utilities/PFNNHelperFunctions.h"

#include "Animation/AnimInstanceProxy.h"
#include "Animation/PFNN/PhaseFunctionNeuralNetwork.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "HAL/PlatformFileManager.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <ThirdParty/glm/glm.hpp>
#include <ThirdParty/glm/gtx/transform.hpp>
#include <ThirdParty/glm/gtx/euler_angles.hpp>
#include <ThirdParty/glm/gtx/quaternion.hpp>

#include <fstream>
#define print(msg, ...) GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, FString::Printf(TEXT(msg), __VA_ARGS__))

UPhaseFunctionNeuralNetwork* FAnimNode_PFNN::PFNN = nullptr;

FAnimNode_PFNN::FAnimNode_PFNN(): PFNNAnimInstance(nullptr), Trajectory(nullptr), Phase(0),  FrameCounter(0),
								  bIsPFNNLoaded(false)
{
	Log_LocalBoneTransform.SetNum(JOINT_NUM-2);
}

void FAnimNode_PFNN::LoadData(FAnimInstanceProxy* arg_Context)
{
	LoadXForms(arg_Context);
	LoadPFNN();
}

void FAnimNode_PFNN::LoadXForms(FAnimInstanceProxy* arg_Context)
{
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

	const FString RelativePath = FPaths::ProjectDir();
	const FString FullPathParents = RelativePath + FString::Printf(TEXT("Plugins/PFNN/Content/MachineLearning/PhaseFunctionNeuralNetwork/character_parents.bin"));
	const FString FullPathXforms = RelativePath + FString::Printf(TEXT("Plugins/PFNN/Content/MachineLearning/PhaseFunctionNeuralNetwork/character_xforms.bin"));

	IFileHandle* FileHandle = PlatformFile.OpenRead(*FullPathParents);
	if (FileHandle == nullptr)
	{
		UE_LOG(PFNN_Logging, Error, TEXT("Fatal error, Failed to load charater parents"));
		return;
	}
	float JointParentsFloat[JOINT_NUM];
	FileHandle->Read(reinterpret_cast<uint8*>(JointParentsFloat), sizeof(JointParentsFloat));

	for (int i = 0; i < JOINT_NUM; i++)
	{
		JointParents[i] = static_cast<int>(JointParentsFloat[i]);
	}


	FileHandle = PlatformFile.OpenRead(*FullPathXforms);
	if (FileHandle == nullptr)
	{
		UE_LOG(PFNN_Logging, Fatal, TEXT("Fatal error, Failed to load character xforms"));
		return;
	}
	FileHandle->Read(reinterpret_cast<uint8*>(JointRestXform), sizeof(JointRestXform));

	for (int i = 0; i < JOINT_NUM; i++)
	{
		JointRestXform[i] = glm::transpose(JointRestXform[i]);
	}

	delete FileHandle;

	// collect bone names
	JointNameByIndex.SetNum(JOINT_NUM);
	const auto& Skeleton = arg_Context->GetSkeleton()->GetReferenceSkeleton();
	for (int32 i = 0; i < JOINT_NUM; i++)
		JointNameByIndex[i] = Skeleton.GetBoneName(i);
}

void FAnimNode_PFNN::LoadPFNN()
{
	if(!bIsPFNNLoaded && Trajectory)
	{
		if (PFNN == nullptr)
		{
			PFNN = NewObject<UPhaseFunctionNeuralNetwork>();
		}

		bIsPFNNLoaded = PFNN->LoadNetworkData(Trajectory->GetOwner());
		APFNNCharacter* Character = Cast<APFNNCharacter>(Trajectory->GetOwner());
		if (Character)
		{
			Character->SetPFNNLoaded(bIsPFNNLoaded);
		}
	}
}

inline FQuat CalculateRotation(const glm::mat3& a)
{
	FQuat q;
	float trace = a[0][0] + a[1][1] + a[2][2]; // I removed + 1.0f; see discussion with Ethan
	if (trace > 0)
	{// I changed M_EPSILON to 0
		float s = 0.5f / sqrtf(trace + 1.0f);
		q.W = 0.25f / s;
		q.X = (a[2][1] - a[1][2]) * s;
		q.Y = (a[0][2] - a[2][0]) * s;
		q.Z = (a[1][0] - a[0][1]) * s;
	}
	else
	{
		if (a[0][0] > a[1][1] && a[0][0] > a[2][2])
		{
			float s = 2.0f * sqrtf(1.0f + a[0][0] - a[1][1] - a[2][2]);
			q.W = (a[2][1] - a[1][2]) / s;
			q.X = 0.25f * s;
			q.Y = (a[0][1] + a[1][0]) / s;
			q.Z = (a[0][2] + a[2][0]) / s;
		}
		else if (a[1][1] > a[2][2])
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
	Trajectory->TickTrajectory();

	glm::vec3 RootPosition = Trajectory->GetRootPosition();
	glm::mat3 RootRotation = Trajectory->GetRootRotation();

	//Input trajectiory positions and directions
	const int w = UTrajectoryComponent::LENGTH / 10;
	for (int i = 0; i < UTrajectoryComponent::LENGTH; i += 10)
	{
		const glm::vec3 Position = glm::inverse(RootRotation) * (Trajectory->Positions[i] - RootPosition);
		const glm::vec3 Direction = glm::inverse(RootRotation) * Trajectory->Directions[i];
		PFNN->Xp((w * 0) + i / 10) = Position.x;
		PFNN->Xp((w * 1) + i / 10) = Position.z;
		PFNN->Xp((w * 2) + i / 10) = Direction.x;
		PFNN->Xp((w * 3) + i / 10) = Direction.z;
	}

	// Input trajectory gaits
	for (int i = 0; i < UTrajectoryComponent::LENGTH; i += 10)
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
	for (int32 i = 0; i < JOINT_NUM; i++)
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
	for (int i = 0; i < UTrajectoryComponent::LENGTH; i += 10)
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
	for (int32 i = 0; i < JOINT_NUM; i++)
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
	for (int i = 0; i < JOINT_NUM; i++)
	{
		if (i == 0)
			JointAnimXform[i] = JointGlobalAnimXform[i];
		else
			JointAnimXform[i] = glm::inverse(JointGlobalAnimXform[JointParents[i]]) * JointGlobalAnimXform[i];
	}

	//Forward kinematics
	for (int i = 0; i < JOINT_NUM; i++)
	{
		JointGlobalAnimXform[i] = JointAnimXform[i];
		JointGlobalRestXform[i] = JointRestXform[i];
		int j = JointParents[i];
		while (j != -1)
		{
			JointGlobalAnimXform[i] = JointAnimXform[j] * JointGlobalAnimXform[i];
			JointGlobalRestXform[i] = JointRestXform[j] * JointGlobalRestXform[i];
			j = JointParents[j];
		}
		JointMeshXform[i] = JointGlobalAnimXform[i] * glm::inverse(JointGlobalRestXform[i]);
	}

	Trajectory->UpdatePastTrajectory();

	Trajectory->Positions[UTrajectoryComponent::LENGTH / 2] = RootPosition;

	//Update current trajectory
	float StandAmount = powf(1.0f - Trajectory->GaitStand[UTrajectoryComponent::LENGTH / 2], 0.25f);

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

	//Update future trajectory
	Trajectory->UpdateFutureTrajectory(PFNN->Yp);

	FinalBoneLocations.Empty();
	FinalBoneRotations.Empty();

	FinalBoneLocations.SetNum(JOINT_NUM);
	FinalBoneRotations.SetNum(JOINT_NUM);
	const float FinalScale = 1.0f;

	for (int32 i = 0; i < JOINT_NUM; i++)
	{
		FinalBoneLocations[i] = UPFNNHelperFunctions::XYZTranslationToXZY(JointPosition[i]);
		FQuat UnrealJointRotation = CalculateRotation(JointRotations[i]);
		FinalBoneRotations[i] = FQuat::MakeFromEuler(FVector::RadiansToDegrees(UnrealJointRotation.Vector()));	

		if (FMath::IsNaN(FinalBoneRotations[i].X) || FMath::IsNaN(FinalBoneRotations[i].Y) || FMath::IsNaN(FinalBoneRotations[i].Z) || FMath::IsNaN(FinalBoneRotations[i].W))
		{
			// Add logging statements to help identify the source of the issue
			UE_LOG(PFNN_Logging, Warning, TEXT("Found NaN in FinalBoneRotations at index %d"), i);
		}

		if (FMath::IsNaN(FinalBoneLocations[i].X) || FMath::IsNaN(FinalBoneLocations[i].Y) || FMath::IsNaN(FinalBoneLocations[i].Z))
		{
			// Add logging statements to help identify the source of the issue
			UE_LOG(PFNN_Logging, Warning, TEXT("Found NaN in FinalBoneLocations at index %d"), i);
		}
	}

	//Phase update
	Phase = fmod(Phase + (StandAmount * 0.9f + 0.1f) * 2.0f * PI * PFNN->Yp(3), 2.0f * PI);


	VisualizePhase();


	FrameCounter++;
	if (FrameCounter >= 1)
	{
		Trajectory->LogTrajectoryData(FrameCounter);
		LogNetworkData(FrameCounter);
	}
	//PFNNAnimInstance->LogJointTransform();
}

glm::quat FAnimNode_PFNN::QuaternionExpression(const glm::vec3 arg_Vector)
{
	float W = glm::length(arg_Vector);

	const glm::quat Quat = W < 0.01 ? glm::quat(1.0f, 0.0f, 0.0f, 0.0f) : glm::quat(
		cosf(W),
		arg_Vector.x * (sinf(W) / W),
		arg_Vector.y * (sinf(W) / W),
		arg_Vector.z * (sinf(W) / W));

	return Quat / sqrtf(Quat.w*Quat.w + Quat.x*Quat.x + Quat.y*Quat.y + Quat.z*Quat.z);
}

UPFNNAnimInstance * FAnimNode_PFNN::GetPFNNInstanceFromContext(const FAnimationInitializeContext& arg_Context)
{
	FAnimInstanceProxy* AnimProxy = arg_Context.AnimInstanceProxy;
	if (AnimProxy)
	{
		return Cast<UPFNNAnimInstance>(AnimProxy->GetAnimInstanceObject());
	}
	return nullptr;
}

UPFNNAnimInstance * FAnimNode_PFNN::GetPFNNInstanceFromContext(const FAnimationUpdateContext & arg_Context)
{
	FAnimInstanceProxy* AnimProxy = arg_Context.AnimInstanceProxy;
	if (AnimProxy)
	{
		return Cast<UPFNNAnimInstance>(AnimProxy->GetAnimInstanceObject());
	}
	return nullptr;
}

void FAnimNode_PFNN::Initialize_AnyThread(const FAnimationInitializeContext& arg_Context)
{
	FAnimNode_Base::Initialize_AnyThread(arg_Context);
	
	PFNNAnimInstance = GetPFNNInstanceFromContext(arg_Context);
if (!PFNNAnimInstance)
{
	UE_LOG(LogTemp, Error, TEXT("PFNN Animation node should only be added to a PFNNAnimInstance child class!"));
}
}

void FAnimNode_PFNN::Update_AnyThread(const FAnimationUpdateContext& arg_Context)
{
	FAnimNode_Base::Update_AnyThread(arg_Context);

	if (!bIsPFNNLoaded)
	{
		LoadData(arg_Context.AnimInstanceProxy);
	}

	if (PFNNAnimInstance)
	{
		Trajectory = PFNNAnimInstance->GetOwningTrajectoryComponent();
	}

	if (Trajectory != nullptr && bIsPFNNLoaded)

	{
		ApplyPFNN();
		//SendDatatoContrlRig();
	}

}
void FAnimNode_PFNN::Evaluate_AnyThread(FPoseContext& arg_Output)
{
	if (!bIsPFNNLoaded)
		return;

	const FTransform& CharacterTransform = arg_Output.AnimInstanceProxy->GetActorTransform();
	FVector RootLocation = PFNNAnimInstance->GetRootLocation();
	
	//print("Root Location: %.2f , %.2f, %.2f", RootLocation.X, RootLocation.Y, RootLocation.Z);
	if (FinalBoneLocations.Num() < JOINT_NUM
		|| FinalBoneRotations.Num() < JOINT_NUM)
	{
		UE_LOG(PFNN_Logging, Error, TEXT("PFNN results were not properly applied!"));
		return;
	}

	const auto &Bones = arg_Output.Pose.GetBoneContainer();
	for (int32 i = 0; i < JOINT_NUM; i++)
	{
		const FCompactPoseBoneIndex CurrentBoneIndex(i);
		const FCompactPoseBoneIndex ParentBoneIndex(Bones.GetParentBoneIndex(CurrentBoneIndex));

		if (ParentBoneIndex.GetInt() == -1)
		{
			//Do nothing first UE4 root bone skips
			FVector RootOffSet = FVector(0,0,0);
			int scale = 2;
			//RootOffSet.Normalize(90);
			arg_Output.Pose[CurrentBoneIndex].SetRotation(FQuat::MakeFromEuler(FVector::DegreesToRadians(FVector(90.0f, 0.0f, 0.0f))));
			arg_Output.Pose[CurrentBoneIndex].SetLocation(RootOffSet);
			arg_Output.Pose[CurrentBoneIndex].SetScale3D(FVector(scale, scale, scale));

		}
		else if (ParentBoneIndex.GetInt() == 0)
		{
			//Root Bone No conversion needed
			//print("Index:%d", CurrentBoneIndex);
			//FinalBoneLocations[0].Normalize(90);
			//FinalBoneLocations[0].Z += 10;
			//FinalBoneRotations[0].X -= 0.4;
			int scale = 2;

			arg_Output.Pose[CurrentBoneIndex].SetRotation(FinalBoneRotations[0]);
			arg_Output.Pose[CurrentBoneIndex].SetLocation(FinalBoneLocations[0]);
		}
		else
		{	//Conversion to LocalSpace (hopefully)
			int ChildBoneTransformIndex = CurrentBoneIndex.GetInt() - 1;
			int ParentBoneTransformIndex = ParentBoneIndex.GetInt() - 1;

			//FinalBoneLocations[ChildBoneTransformIndex].Normalize(50);
			//FinalBoneLocations[ParentBoneTransformIndex].Normalize(50);
			//FVector ScaleVector = arg_Output.Pose[CurrentBoneIndex].GetScale3D();
			int scale = 1;
			FVector ScaleVector = FVector(scale, scale, scale);

			//FVector ControlRigPositions = FinalBoneLocations[i];
			//FQuat ControlRigRotations = FinalBoneRotations[i];
			//
			//FTransform ControlRigTransform = FTransform(ControlRigRotations, ControlRigPositions, ScaleVector);

			//Child Bone Transform Variables 
			FQuat ChildBoneRotation = FinalBoneRotations[ChildBoneTransformIndex];
			FVector ChildBoneLocation = FinalBoneLocations[ChildBoneTransformIndex];

			//Parent Bone Transform Variables
			FQuat ParentBoneRotation = FinalBoneRotations[ParentBoneTransformIndex];
			FVector ParentBoneLocation = FinalBoneLocations[ParentBoneTransformIndex];


			FTransform ChildBoneTransform = FTransform(ChildBoneRotation, ChildBoneLocation, ScaleVector);
			FTransform ParentBoneTransform = FTransform(ParentBoneRotation, ParentBoneLocation, ScaleVector);


			FTransform LocalBoneTransform = ChildBoneTransform.GetRelativeTransform(ParentBoneTransform);
			
			arg_Output.Pose[CurrentBoneIndex].SetComponents(LocalBoneTransform.GetRotation(), LocalBoneTransform.GetTranslation(), LocalBoneTransform.GetScale3D());
			
			if (LocalBoneTransform.GetRotation().ContainsNaN())
			{
				UE_LOG(PFNN_Logging, Warning, TEXT("Found NaN in rotation at index %d"), i);
			}

			if (LocalBoneTransform.GetLocation().ContainsNaN())
			{
				UE_LOG(PFNN_Logging, Warning, TEXT("Found NaN in location at index %d"), i);
			}
			
		/*	LocalBoneTransform.NormalizeRotation();
			APFNNCharacter* Character = Cast<APFNNCharacter>(Trajectory->GetOwner());
			if (Character)
			{
				Character->SetJointTransformForControlRig(LocalBoneTransform, i, JointNameByIndex[i]);
			}*/
			//print("CurrentIndex: %d", CurrentBoneIndex.GetInt());
		}
	}
	arg_Output.Pose.NormalizeRotations();
	//LogLocalTransformData(LocalBoneTransform, JointNameByIndex[0]);
}


void FAnimNode_PFNN::LogNetworkData(int arg_FrameCounter)
{
	try
	{
		FString RelativePath = FPaths::ProjectDir();
		const FString FullPath = RelativePath += "PFNN_Network.log";

		std::fstream fs;
		fs.open(*FullPath, std::ios::out);

		if (fs.is_open())
		{
			fs << "UE5_Network" << std::endl;
			fs << "Network Frame[" << arg_FrameCounter << "]" << std::endl << std::endl;

			fs << "Current Phase: " << Phase << std::endl << std::endl;

			fs << "Joints" << std::endl;

			for (size_t i = 0; i < JOINT_NUM; i++)
			{

				FString JointNameStr = JointNameByIndex[i].ToString(); // Convert FName to FString
				JointNameStr.AppendInt(int(i));

				
			
				fs << "Joint[" << i << "]" << TCHAR_TO_UTF8(*JointNameStr) <<std::endl;
				fs << "	JointPosition: " << JointPosition[i].x << "X, " << JointPosition[i].y << "Y, " << JointPosition[i].z << "Z" << std::endl;
				fs << "	JointVelocitys: " << JointVelocitys[i].x << "X, " << JointVelocitys[i].y << "Y, " << JointVelocitys[i].z << "Z" << std::endl;

				for (size_t x = 0; x < 3; x++)
				{
				fs << "	JointRotations:  " << JointRotations[i][x].x << "X, " << JointRotations[i][x].y << ", " << JointRotations[i][x].z << std::endl;
				}

				for (size_t x = 0; x < 3; x++)
				{
					fs << "	JointAnimXform:  " << JointAnimXform[i][x].x << "X, " << JointAnimXform[i][x].y << ", " << JointAnimXform[i][x].z << std::endl;
				}

				for (size_t x = 0; x < 3; x++)
				{
					fs << "	JointRestXform:  " << JointRestXform[i][x].x << "X, " << JointRestXform[i][x].y << ", " << JointRestXform[i][x].z << std::endl;
				}

				for (size_t x = 0; x < 3; x++)
				{
					fs << "	JointMeshXform:  " << JointMeshXform[i][x].x << "X, " << JointMeshXform[i][x].y << ", " << JointMeshXform[i][x].z << std::endl;
				}

				for (size_t x = 0; x < 3; x++)
				{
					fs << "	JointGlobalRestXform:  " << JointGlobalRestXform[i][x].x << "X, " << JointGlobalRestXform[i][x].y << ", " << JointGlobalRestXform[i][x].z << std::endl;
				}

				for (size_t x = 0; x < 3; x++)
				{
					fs << "	JointGlobalAnimXform:  " << JointGlobalAnimXform[i][x].x << "X, " << JointGlobalAnimXform[i][x].y << ", " << JointGlobalAnimXform[i][x].z << std::endl;
				}

				fs << "JoinParents: " << JointParents[i] << std::endl;
			}
			fs << "End Joints" << std::endl << std::endl;

			fs << "FinalLocations" << std::endl;
			for (size_t i = 0; i < FinalBoneLocations.Num(); i++)
			{
				FString JointNameStr = JointNameByIndex[i].ToString(); // Convert FName to FString
				fs << "Joint[" << i << "]" << TCHAR_TO_UTF8(*JointNameStr) << std::endl;
				fs << "	FinalBoneLocation: " << FinalBoneLocations[i].X << "X, " << FinalBoneLocations[i].Y << "Y, " << FinalBoneLocations[i].Z << "Z" << std::endl;
				fs << "	FinalBoneRotation: " << FinalBoneRotations[i].X << "X, " << FinalBoneRotations[i].Y << "Y, " << FinalBoneRotations[i].Z << "Z, " << FinalBoneRotations[i].W << "W"<< std::endl;
			}
			fs << "End FinalLocations" << std::endl;

			//fs << "LocalBoneTransform" << std::endl;
			//for (int32 i = 0; i < JOINT_NUM-2; i++)
			//{
			//	FString JointNameStr = JointNameByIndex[i+2].ToString(); // Convert FName to FString
			//	fs << "Joint[" << i+2 << "]" << TCHAR_TO_UTF8(*JointNameStr) << std::endl;
			//	fs << "	LocalTransformLocation: " << Log_LocalBoneTransform[i].GetLocation().X << "X, " << Log_LocalBoneTransform[i].GetLocation().Y << "Y, " << Log_LocalBoneTransform[i].GetLocation().Z << "Z" << std::endl;
			//	fs << "	LocalTransformRotation: " << Log_LocalBoneTransform[i].GetRotation().X << "X, " << Log_LocalBoneTransform[i].GetRotation().Y << "Y, " << Log_LocalBoneTransform[i].GetRotation().Z << "Z, " << Log_LocalBoneTransform[i].GetRotation().W << "W" << std::endl;
			//	fs << "	LocalTransformScale: " << Log_LocalBoneTransform[i].GetScale3D().X << "X, " << Log_LocalBoneTransform[i].GetScale3D().Y << "Y, " << Log_LocalBoneTransform[i].GetScale3D().Z << "Z " << std::endl;
			//}
			//fs << "End LocalBoneTransform" << std::endl;


		}
		else 
		{
			throw std::exception();
		}
	}
	catch (std::exception e) 
	{
		UE_LOG(LogTemp, Log, TEXT("Failed to log network data"));
	}
	
}



void FAnimNode_PFNN::DrawDebugSkeleton(const FPoseContext& arg_Context) 
{
	APFNNCharacter* Character = Cast<APFNNCharacter>(Trajectory->GetOwner());
	if (!Character || !Character->HasDebuggingEnabled())
		return;

	const auto& Mesh = Character->GetMesh();
	auto& AnimInstanceProxy = arg_Context.AnimInstanceProxy;
	for (int32 i = 0; i < JOINT_NUM; i++)
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
		if (const auto& world = Character->GetWorld())
			DrawDebugString(world, CurrentBoneLocation, FString::FromInt(i), nullptr, FColor::Red, -1.0f, false, 1.0f);
	}
}

void FAnimNode_PFNN::DrawDebugBoneVelocity(const FPoseContext& arg_Context)
{
	if (!GEngine)
		return;

	APFNNCharacter* Character = Cast<APFNNCharacter>(Trajectory->GetOwner());
	if (!Character || !Character->HasDebuggingEnabled())
		return;

	const FTransform& CharacterTransform = arg_Context.AnimInstanceProxy->GetActorTransform();
	const auto charLocation = CharacterTransform.GetLocation();
	for (int32 i = 0; i < JOINT_NUM; i++)
	{
		const auto JointPos = FVector(JointPosition[i].x, JointPosition[i].z, JointPosition[i].y);
		const auto JointVelocity = FVector(JointVelocitys[i].x, JointVelocitys[i].z, JointVelocitys[i].y);
		arg_Context.AnimInstanceProxy->AnimDrawDebugLine(charLocation + JointPos, charLocation + JointPos - 10 * JointVelocity, FColor::Yellow, false, -1, 0.5f);
	}
}

void FAnimNode_PFNN::VisualizePhase()
{
	if(GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Yellow, FString::Printf(TEXT("Phase is %f"), Phase));
}
